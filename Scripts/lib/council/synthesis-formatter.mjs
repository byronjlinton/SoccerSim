/**
 * Council of Minds v2 — Synthesis Formatter (SoccerSim Edition)
 * Produces terminal-formatted consensus report from deliberation session.
 */

const W = 78;

function divider(char = '─') { return char.repeat(W); }
function header(text) { return divider('═') + '\n' + padCenter(text, W) + '\n' + divider('═'); }
function padCenter(text, width) { const pad = Math.max(0, width - text.length); return ' '.repeat(Math.floor(pad / 2)) + text; }
function trunc(text, maxLen = 120) { if (!text) return '(no answer)'; const oneLine = text.replace(/\n/g, ' ').replace(/\s+/g, ' ').trim(); return oneLine.length > maxLen ? oneLine.substring(0, maxLen) + '...' : oneLine; }
function fmtConf(c) { return c.toFixed(2); }

const PHASE_LABELS = {
  lead_thesis: 'LEAD DESIGN THESIS — GLM 5.1',
  peer_review: 'PEER REVIEW',
  synthesis: 'FINAL SYNTHESIS — GLM 5.1',
  blind: 'BLIND ANSWERS',
  critique: 'DESIGN CRITIQUE & REFINEMENT',
  refinement: 'REFINEMENT',
};

const ROLE_LABELS = {
  lead: 'Lead Game Designer',
  scout: 'Physics Scout',
  devil_advocate: "Devil's Advocate",
  cross_domain: 'Cross-Domain',
  analyst: 'Analyst',
  skeptic: 'Skeptic',
  synthesist: 'Synthesist',
};

function roleLabel(role) { return ROLE_LABELS[role] || role; }

export function formatReport(session) {
  const { question, rounds_data, roles, consensus, totalDuration_s, version } = session;
  const lines = [];
  const isV2 = version === 2;

  // Header
  const modelNames = Object.keys(roles);
  const modelCount = modelNames.length;
  lines.push(header(
    isV2
      ? `COUNCIL OF MINDS v2  |  ${modelCount} models  |  ${rounds_data.length} rounds  |  ${totalDuration_s.toFixed(1)}s`
      : `COUNCIL OF MINDS  |  ${modelCount} models  |  ${rounds_data.length} rounds  |  ${totalDuration_s.toFixed(1)}s total`
  ));
  lines.push('');
  lines.push(`QUESTION: "${question}"`);
  if (isV2) {
    lines.push(`MODELS: ${modelNames.map(m => `${m}=${roleLabel(roles[m])}`).join(' | ')}`);
  }
  lines.push('');

  // Rounds
  for (const round of rounds_data) {
    const phaseLabel = PHASE_LABELS[round.phase] || `ROUND ${round.round}`;
    lines.push(divider());
    lines.push(`  ${phaseLabel}`);
    lines.push(divider());

    for (const resp of round.responses) {
      const role = roles[resp.model] || 'analyst';
      const tag = resp.error ? ' \u2717 FAILED' : '';
      lines.push('');
      lines.push(`  ${resp.model} (${roleLabel(role)}) [${resp.duration_s.toFixed(1)}s]${tag}`);

      if (resp.error) {
        lines.push(`    Error: ${resp.error}`);
      } else {
        const answerLines = (resp.answer || '').split('\n');
        const displayLines = answerLines.slice(0, 10);
        for (const al of displayLines) lines.push(`    ${al}`);
        if (answerLines.length > 10) lines.push(`    ... (${answerLines.length - 10} more lines)`);

        if (resp.claims && resp.claims.length > 0) {
          lines.push('');
          for (const c of resp.claims) {
            const stance = c.stance ? ` | ${c.stance.toUpperCase()}` : '';
            lines.push(`    CLAIM: ${trunc(c.text, 55)} | ${fmtConf(c.confidence)}${stance}`);
          }
        }
      }
    }
    lines.push('');
  }

  // Consensus
  if (consensus) {
    lines.push(divider());
    lines.push('  CONSENSUS');
    lines.push(divider());

    if (consensus.agreed && consensus.agreed.length > 0) {
      lines.push('');
      lines.push('  AGREED:');
      for (const g of consensus.agreed) {
        const modelsList = Object.entries(g.models).map(([m, c]) => `${m}: ${fmtConf(c)}`).join(' | ');
        lines.push(`    [${fmtConf(g.meanConfidence)}] ${trunc(g.representativeText, 50)}`);
        lines.push(`           ${modelsList}`);
      }
    }

    if (consensus.disagreed && consensus.disagreed.length > 0) {
      lines.push('');
      lines.push('  DISAGREED:');
      for (const g of consensus.disagreed) {
        lines.push(`    ${trunc(g.representativeText, 55)}`);
        const modelsList = Object.entries(g.models).map(([m, c]) => `${m}: ${fmtConf(c)}`).join('  |  ');
        lines.push(`      ${modelsList}`);
        lines.push(`      SPREAD: ${fmtConf(g.spread)}`);
      }
    }

    lines.push('');
    lines.push(`  CONSENSUS SCORE: ${fmtConf(consensus.overallScore)}/1.00`);

    if (consensus.keyUnknowns && consensus.keyUnknowns.length > 0) {
      lines.push('  KEY UNKNOWNS:');
      for (const u of consensus.keyUnknowns) lines.push(`    - ${trunc(u, 60)}`);
    }
  }

  lines.push('');
  lines.push(divider('═'));
  lines.push(`  Logged to data/council-log.jsonl`);
  lines.push(divider('═'));

  return lines.join('\n');
}
