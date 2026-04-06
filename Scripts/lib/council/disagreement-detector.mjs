/**
 * Council of Minds — Disagreement Detector
 * Extracts structured claims from model answers, groups similar claims,
 * and computes agreement/disagreement metrics.
 */

// ── Claim Parsing ─────────────────────────────────────────────────────────

const CLAIM_RE = /CLAIM:\s*(.+?)\s*\|\s*CONFIDENCE:\s*([0-9.]+)\s*(?:\|\s*STANCE:\s*(\w+))?/gi;

/**
 * Parse CLAIM lines from a model's answer text.
 * Returns [{ text, confidence, stance }]
 */
export function parseClaims(answerText) {
  if (!answerText) return [];
  const claims = [];
  let m;
  while ((m = CLAIM_RE.exec(answerText)) !== null) {
    claims.push({
      text: m[1].trim(),
      confidence: parseFloat(m[2]) || 0.5,
      stance: m[3]?.toLowerCase() || null,
    });
  }
  return claims;
}

// ── Token Overlap (for grouping similar claims) ────────────────────────────

const STOP_WORDS = new Set([
  'a', 'an', 'the', 'is', 'are', 'was', 'were', 'be', 'been', 'being',
  'have', 'has', 'had', 'do', 'does', 'did', 'will', 'would', 'could',
  'should', 'may', 'might', 'shall', 'can', 'need', 'dare', 'ought',
  'used', 'to', 'of', 'in', 'for', 'on', 'with', 'at', 'by', 'from',
  'as', 'into', 'through', 'during', 'before', 'after', 'above', 'below',
  'between', 'out', 'off', 'over', 'under', 'again', 'further', 'then',
  'once', 'here', 'there', 'when', 'where', 'why', 'how', 'all', 'each',
  'every', 'both', 'few', 'more', 'most', 'other', 'some', 'such', 'no',
  'not', 'only', 'own', 'same', 'so', 'than', 'too', 'very', 'just',
  'because', 'but', 'and', 'or', 'if', 'while', 'about', 'up', 'that',
  'this', 'these', 'those', 'it', 'its', 'i', 'me', 'my', 'we', 'our',
  'they', 'them', 'their', 'what', 'which', 'who', 'whom',
]);

function tokenize(text) {
  return (text || '').toLowerCase().replace(/[^a-z0-9\s]/g, '').split(/\s+/)
    .filter(t => t.length > 1 && !STOP_WORDS.has(t));
}

function tokenOverlap(a, b) {
  const ta = new Set(tokenize(a));
  const tb = new Set(tokenize(b));
  if (ta.size === 0 || tb.size === 0) return 0;
  let shared = 0;
  for (const t of ta) { if (tb.has(t)) shared++; }
  return shared / Math.max(ta.size, tb.size);
}

// ── Claim Grouping & Consensus ─────────────────────────────────────────────

/**
 * Group claims from different models that refer to the same thing.
 * Input: { modelName: [{ text, confidence, stance }] }
 * Output: [{ claim: "description", models: { modelName: confidence }, spread: number }]
 */
export function groupSimilarClaims(claimsByModel) {
  const models = Object.keys(claimsByModel);
  const allClaims = [];
  for (const model of models) {
    for (const claim of claimsByModel[model]) {
      allClaims.push({ ...claim, model });
    }
  }

  // Greedy grouping: first claim starts a group, subsequent claims join if overlap > 0.40
  const groups = [];
  for (const claim of allClaims) {
    let matched = false;
    for (const group of groups) {
      if (tokenOverlap(claim.text, group.representativeText) > 0.40) {
        group.claims.push(claim);
        group.models[claim.model] = claim.confidence;
        matched = true;
        break;
      }
    }
    if (!matched) {
      groups.push({
        representativeText: claim.text,
        claims: [claim],
        models: { [claim.model]: claim.confidence },
      });
    }
  }

  // Compute spread for each group
  for (const g of groups) {
    const vals = Object.values(g.models);
    const mean = vals.reduce((a, b) => a + b, 0) / vals.length;
    const variance = vals.reduce((a, b) => a + (b - mean) ** 2, 0) / vals.length;
    g.meanConfidence = mean;
    g.spread = Math.max(...vals) - Math.min(...vals);
    g.variance = variance;
    g.modelCount = vals.length;
    g.agreed = g.spread < 0.20;       // within 0.20 = agreed
    g.dissent = g.spread > 0.30;      // spread > 0.30 = key disagreement
  }

  // Sort: disagreements first, then by mean confidence
  groups.sort((a, b) => {
    if (a.dissent !== b.dissent) return b.dissent - a.dissent;
    return b.meanConfidence - a.meanConfidence;
  });

  return groups;
}

/**
 * Compute full consensus from grouped claims.
 * Returns { agreed, disagreed, overallScore, keyUnknowns }
 */
export function computeConsensus(groups, rawAnswers) {
  const agreed = groups.filter(g => g.agreed && g.modelCount >= 2);
  const disagreed = groups.filter(g => g.dissent);

  // Overall score: weighted average of agreed claims (higher agreement = higher score)
  const allMeans = groups.map(g => g.meanConfidence);
  const overallScore = allMeans.length > 0
    ? allMeans.reduce((a, b) => a + b, 0) / allMeans.length
    : 0;

  // Extract "unknowns" from model answers (look for "unknown", "unclear", "investigate")
  const keyUnknowns = [];
  const unknownRe = /(?:most important unknown|unclear|investigate further|need to know|remains? unclear)\s*:?\s*(.+)/gi;
  for (const ans of Object.values(rawAnswers)) {
    if (!ans) continue;
    let m;
    while ((m = unknownRe.exec(ans)) !== null) {
      const t = m[1].trim().replace(/[.]+$/, '');
      if (t.length > 10 && !keyUnknowns.includes(t)) keyUnknowns.push(t);
    }
  }

  return { agreed, disagreed, overallScore, keyUnknowns: keyUnknowns.slice(0, 3) };
}
