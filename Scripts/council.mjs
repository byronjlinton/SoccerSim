#!/usr/bin/env node
/**
 * Council of Minds v2 — SoccerSim Edition
 *
 * 4 fixed models with game-dev focused roles:
 *   GLM 5.1 (Lead Game Designer) → Gemini 2.5 Flash (Physics Engine Scout) + ChatGPT 5.4 Thinking (Devil's Advocate) + Qwen 3.5-Omni-Plus (Cross-Domain Reviewer)
 *
 * Modes (--mode flag):
 *   --mode quick     (1 round)  All 4 answer. Compute consensus. ~30-45s
 *   --mode standard  (2 rounds, DEFAULT)
 *     Round 1: All 4 answer blind with role-specific framing
 *     Round 2: Each sees peers' answers, critiques from their role's perspective
 *   --mode deep     (3 rounds)
 *     Round 1: GLM produces deep thesis
 *     Round 2: 3 reviewers evaluate thesis
 *     Round 3: All see critiques + refine position, GLM synthesizes
 *   --mode full     (3-5 rounds)
 *     Round 1: GLM deep thesis
 *     Round 2: Reviewers evaluate
 *     Round 3: GLM sees critiques, forms sub-questions for gaps
 *     Round 4: All answer expanded sub-questions
 *     Round 5: GLM final synthesis with convergence check
 *
 * Usage:
 *   node scripts/council.mjs "Should we use behavior trees or utility AI?"
 *   node scripts/council.mjs "Question" --mode deep
 *   node scripts/council.mjs "Question" --mode full --max-rounds 5
 *   node scripts/council.mjs "Question" --json
 */
import { execSync } from 'node:child_process';
import { appendFileSync, mkdirSync, existsSync, writeFileSync, unlinkSync, readFileSync } from 'node:fs';
import { resolve, dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

import { buildLeadPrompt, buildReviewerPrompt, buildSynthesisPrompt, buildSubQuestionPrompt, buildConvergencePrompt, buildFirstRoundPrompt, buildAuditPrompt, buildCritiquePrompt } from './lib/council/prompt-builder.mjs';
import { parseClaims, groupSimilarClaims, computeConsensus } from './lib/council/disagreement-detector.mjs';
import { formatReport } from './lib/council/synthesis-formatter.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const LOG_PATH = resolve(ROOT, 'data', 'council-log.jsonl');

// ── Arg Parsing ────────────────────────────────────────────────────────────

const args = process.argv.slice(2);
let maxRounds = 5;
let mode = 'standard';   // quick | standard | deep | full
let jsonOutput = false;
let audit = false;
let filePath = null;
const parts = [];

for (let i = 0; i < args.length; i++) {
  if (args[i] === '--mode' && args[i + 1]) { mode = args[++i]; continue; }
  if (args[i] === '--max-rounds' && args[i + 1]) { maxRounds = parseInt(args[++i]) || 5; continue; }
  if (args[i] === '--json') { jsonOutput = true; continue; }
  if (args[i] === '--file' && args[i + 1]) { filePath = args[++i]; continue; }
  if (args[i] === '--audit') { audit = true; continue; }
  parts.push(args[i]);
}

let question = parts.join(' ').trim();
if (filePath) {
  try {
    question = readFileSync(resolve(filePath), 'utf8').trim();
  } catch (e) {
    console.error(`Error reading prompt file: ${e.message}`);
    process.exit(1);
  }
}
if (!question) {
  console.error('Usage: node scripts/council.mjs "your question" [--mode standard] [--json]');
  console.error('       node scripts/council.mjs --file prompt.txt [--mode deep]');
  console.error('');
  console.error('  --mode quick     1 round: all 4 answer, consensus (~30s)');
  console.error('  --mode standard  2 rounds: blind answers + critique (default)');
  console.error('  --mode deep     3 rounds: thesis + review + synthesis');
  console.error('  --mode full     3-5 rounds: deep iterative convergence');
  console.error('  --max-rounds N  Cap rounds in full mode (default 5)');
  console.error('  --audit        Run structural self-check before main council');
  console.error('  --file PATH   Read question from file');
  console.error('  --json         Output raw JSON');
  process.exit(1);
}

// Determine rounds from mode
const ROUNDS_BY_MODE = { quick: 1, standard: 2, deep: 3, full: maxRounds };
const rounds = Math.min(ROUNDS_BY_MODE[mode] || ROUNDS_BY_MODE.standard);

// Ensure data dir
const dataDir = resolve(ROOT, 'data');
if (!existsSync(dataDir)) mkdirSync(dataDir, { recursive: true });

// ── Temp File Helper ─────────────────────────────────────────────────────────

const tmpDir = resolve(ROOT, 'data', 'tmp');
if (!existsSync(tmpDir)) mkdirSync(tmpDir, { recursive: true });

function writePromptFile(prompt) {
  const hash = Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
  const path = resolve(tmpDir, `prompt-${hash}.txt`);
  writeFileSync(path, prompt, 'utf8');
  return path;
}

// ── Model Definitions (v2: 4 fixed models, always all present) ──────────────

const ALL_MODELS = {
  'glm-5.1': {
    buildCmd: promptFile => `node "${resolve(ROOT, 'scripts', 'ask-glm.mjs')}" --file "${promptFile}"`,
    type: 'api',
    role: 'lead',
  },
  'gemini-2.5-flash': {
    buildCmd: promptFile => `node "${resolve(ROOT, 'scripts', 'ask-gemini.mjs')}" --file "${promptFile}"`,
    type: 'api',
    role: 'scout',
  },
  'chatgpt-5.4-thinking': {
    buildCmd: promptFile => `node "${resolve(ROOT, 'scripts', 'ask-web-llm.mjs')}" chatgpt --file "${promptFile}"`,
    type: 'web',
    role: 'devil_advocate',
  },
  'qwen-3.5-omni-plus': {
    buildCmd: promptFile => `node "${resolve(ROOT, 'scripts', 'ask-web-llm.mjs')}" qwen --file "${promptFile}"`,
    type: 'web',
    role: 'cross_domain',
  },
};

const modelNames = ['glm-5.1', 'gemini-2.5-flash', 'chatgpt-5.4-thinking', 'qwen-3.5-omni-plus'];

// Fixed role mapping
const roles = {};
for (const name of modelNames) {
  roles[name] = ALL_MODELS[name].role;
}

// ── Query Execution ────────────────────────────────────────────────────────

function queryModel(modelName, prompt) {
  const config = ALL_MODELS[modelName];
  const promptFile = writePromptFile(prompt);
  const start = Date.now();
  try {
    const output = execSync(config.buildCmd(promptFile), {
      encoding: 'utf8',
      timeout: 300_000,
      stdio: ['pipe', 'pipe', 'pipe'],
      cwd: ROOT,
    });
    const duration_s = (Date.now() - start) / 1000;
    const lines = output.trim().split('\n');
    const answer = lines.slice(1).join('\n').trim();
    try { unlinkSync(promptFile); } catch {}
    return { model: modelName, answer, duration_s, error: null };
  } catch (err) {
    const duration_s = (Date.now() - start) / 1000;
    try { unlinkSync(promptFile); } catch {}
    return { model: modelName, answer: null, duration_s, error: err.stderr?.trim() || err.message };
  }
}

/**
 * Query all models — API models in parallel, web models sequentially
 * (Playwright profile locks prevent parallel web sessions).
 */
async function queryAllModels(prompts) {
  const apiModels = modelNames.filter(n => ALL_MODELS[n].type === 'api');
  const webModels = modelNames.filter(n => ALL_MODELS[n].type === 'web');
  const results = {};

  // API models in parallel
  if (apiModels.length > 0) {
    const apiResults = await Promise.all(
      apiModels.map(name => queryModel(name, prompts[name]))
    );
    for (const r of apiResults) results[r.model] = r;
  }

  // Web models sequentially (Playwright profile lock prevents parallel)
  for (const name of webModels) {
    results[name] = await queryModel(name, prompts[name]);
    // Brief pause to let Chromium release the profile directory lock
    await new Promise(r => setTimeout(r, 2000));
  }

  return results;
}

// ── Convergence Check ────────────────────────────────────────────────────

function checkConvergence(sessionRounds) {
  // Get all claims from the last round
  const lastRound = sessionRounds[sessionRounds.length - 1];
  const allClaims = {};
  for (const resp of lastRound.responses) {
    if (resp.claims && resp.claims.length > 0) {
      allClaims[resp.model] = resp.claims;
    }
  }

  if (Object.keys(allClaims).length < 2) return { converged: false, spread: 1.0 };

  const groups = groupSimilarClaims(allClaims);
  const maxSpread = groups.reduce((max, g) => Math.max(max, g.spread || 0), 0);

  // Converged if max confidence spread < 0.20
  return { converged: maxSpread < 0.20, spread: maxSpread };
}

// ── Main Council Flow ──────────────────────────────────────────────────────

async function runCouncil() {
  const councilStart = Date.now();

  console.error(`\nCouncil of Minds v2 — SoccerSim — ${modelNames.length} models | mode: ${mode} | ${rounds} rounds`);
  console.error(`Roles: ${modelNames.map(m => `${m}=${roles[m]}`).join(', ')}`);
  console.error(`Question: "${question}"\n`);

  // ── Audit Mode ─────────────────────────────────────────────────────────────
  if (audit) {
    console.error('\n══════════════════════════════════════════════════════════════');
    console.error('  STRUCTURAL AUDIT — All models Self-Check');
    console.error('══════════════════════════════════════════════════════════════\n');
    const auditPrompt = buildAuditPrompt();
    const auditPrompts = {};
    for (const name of modelNames) {
      auditPrompts[name] = auditPrompt;
    }
    const auditResults = await queryAllModels(auditPrompts);
    const auditResponses = Object.values(auditResults);

    console.error('\n  AUDIT RESULTS:');
    console.error('  ──────────────');
    for (const r of auditResponses) {
      const role = roles[r.model] || 'unknown';
      const answer = r.answer || '(no response)';
      const firstLine = answer.split('\n')[0];
      console.error(`  ${r.model} (${role}): ${firstLine}`);
      if (r.error) console.error(`    Error: ${r.error}`);
    }
    console.error('\n══════════════════════════════════════════════════════════════\n');

    // Log audit
    appendFileSync(LOG_PATH, JSON.stringify({
      ts: new Date().toISOString(),
      type: 'audit',
      models: modelNames,
      results: auditResponses.map(r => ({
        model: r.model,
        role: roles[r.model],
        responded: !!r.answer,
        error: r.error,
      })),
    }) + '\n');

    console.error('  Proceeding to main council...\n');
  }

  const sessionRounds = [];
  let previousRound = null;

  // ═══════════════════════════════════════════════════════════════════════════
  // ROUND 1: All 4 answer blind with role-specific framing
  // ═══════════════════════════════════════════════════════════════════════════
  console.error('Round 1: All models answer blind...');
  const r1Prompts = {};
  for (const name of modelNames) {
    r1Prompts[name] = buildFirstRoundPrompt(question, roles[name]);
  }

  const r1Results = await queryAllModels(r1Prompts);
  const r1Responses = Object.values(r1Results);
  for (const r of r1Responses) {
    if (r.answer) r.claims = parseClaims(r.answer);
  }

  sessionRounds.push({ round: 1, phase: 'blind', responses: r1Responses });
  previousRound = r1Results;
  console.error(`  Round 1 done (${r1Responses.filter(r => r.answer).length}/${modelNames.length} responded)`);

  // Quick mode stops after 1 round
  if (mode === 'quick') {
    return finishCouncil(sessionRounds, councilStart);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // ROUND 2: Each model sees all peer answers, critiques from their role's perspective
  // ═══════════════════════════════════════════════════════════════════════════
  console.error('Round 2: Cross-examination...');
  const r2Prompts = {};
  for (const name of modelNames) {
    const otherAnswers = modelNames
      .filter(n => n !== name)
      .map(n => ({ model: n, role: roles[n], answer: previousRound[n]?.answer || '(no answer)' }));
    r2Prompts[name] = buildCritiquePrompt(question, previousRound[name]?.answer || '(no answer)', otherAnswers, roles[name]);
  }

  const r2Results = await queryAllModels(r2Prompts);
  const r2Responses = Object.values(r2Results);
  for (const r of r2Responses) {
    if (r.answer) r.claims = parseClaims(r.answer);
  }

  sessionRounds.push({ round: 2, phase: 'critique', responses: r2Responses });
  previousRound = r2Results;
  console.error(`  Round 2 done (${r2Responses.filter(r => r.answer).length}/${modelNames.length} responded)`);

  // Standard mode stops after 2 rounds
  if (mode === 'standard') {
    return finishCouncil(sessionRounds, councilStart);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // ROUND 3: GLM synthesizes — sees all critiques, produces refined answer
  // ═══════════════════════════════════════════════════════════════════════════
  console.error('Round 3: Lead synthesis...');
  const reviews = r2Responses.map(r => ({
    model: r.model,
    role: roles[r.model],
    answer: r.answer,
  }));
  const synthPrompt = buildSynthesisPrompt(question, previousRound['glm-5.1']?.answer, reviews);
  const synthResult = queryModel('glm-5.1', synthPrompt);
  if (synthResult.answer) synthResult.claims = parseClaims(synthResult.answer);

  sessionRounds.push({ round: 3, phase: 'synthesis', responses: [{ ...synthResult }] });
  previousRound = { 'glm-5.1': synthResult };
  console.error(`  Synthesis done [${synthResult.duration_s.toFixed(1)}s]`);

  // Deep mode stops after 3 rounds
  if (mode === 'deep') {
    return finishCouncil(sessionRounds, councilStart);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // ROUNDS 4-5 (full mode): Iterative convergence
  // GLM identifies gaps → sub-questions → all answer → GLM synthesizes
  // ═══════════════════════════════════════════════════════════════════════════
  for (let roundNum = 4; roundNum <= maxRounds; roundNum++) {
    const { converged, spread } = checkConvergence(sessionRounds);
    if (converged) {
      console.error(`  Convergence achieved (spread: ${spread.toFixed(2)}) — stopping early`);
      break;
    }
    console.error(`  Not yet converged (spread: ${spread.toFixed(2)})`);

    if (roundNum % 2 === 0) {
      // Even rounds: GLM identifies gaps and forms sub-questions
      console.error(`Round ${roundNum}: Gap analysis & sub-questions...`);
      const gapPrompt = buildSubQuestionPrompt(question, sessionRounds);
      const gapResult = queryModel('glm-5.1', gapPrompt);
      if (gapResult.answer) gapResult.claims = parseClaims(gapResult.answer);

      sessionRounds.push({ round: roundNum, phase: 'gap_analysis', responses: [{ ...gapResult }] });
      previousRound = { 'glm-5.1': gapResult };
      console.error(`  Gap analysis done [${gapResult.duration_s.toFixed(1)}s]`);
    } else {
      // Odd rounds: All models answer expanded sub-questions, then GLM synthesizes
      console.error(`Round ${roundNum}: Expanded analysis + convergence...`);
      const lastGap = sessionRounds[sessionRounds.length - 1];
      const gapAnalysis = lastGap.responses[0]?.answer || '';

      const expandPrompts = {};
      for (const name of modelNames) {
        expandPrompts[name] = buildConvergencePrompt(question, gapAnalysis, sessionRounds, roles[name]);
      }

      const expandResults = await queryAllModels(expandPrompts);
      const expandResponses = Object.values(expandResults);
      for (const r of expandResponses) {
        if (r.answer) r.claims = parseClaims(r.answer);
      }

      sessionRounds.push({ round: roundNum, phase: 'convergence', responses: expandResponses });
      previousRound = expandResults;
      console.error(`  Round ${roundNum} done (${expandResponses.filter(r => r.answer).length}/${modelNames.length} responded)`);
    }
  }

  return finishCouncil(sessionRounds, councilStart);
}

// ── Finish & Output ──────────────────────────────────────────────────────────

function finishCouncil(sessionRounds, councilStart) {
  // Consensus Computation
  const finalClaimsByModel = {};
  for (const round of sessionRounds) {
    for (const resp of round.responses) {
      if (resp.claims && resp.claims.length > 0) {
        finalClaimsByModel[resp.model] = resp.claims;
      }
    }
  }

  const groups = groupSimilarClaims(finalClaimsByModel);
  const rawAnswers = {};
  for (const round of sessionRounds) {
    for (const resp of round.responses) {
      rawAnswers[resp.model] = resp.answer;
    }
  }
  const consensus = computeConsensus(groups, rawAnswers);

  const totalDuration_s = (Date.now() - councilStart) / 1000;

  const session = {
    ts: new Date().toISOString(),
    version: 2,
    mode,
    question,
    rounds: sessionRounds.length,
    models: modelNames,
    roles,
    rounds_data: sessionRounds,
    consensus,
    totalDuration_s,
  };

  // Output
  if (jsonOutput) {
    console.log(JSON.stringify(session, null, 2));
  } else {
    console.log(formatReport(session));
  }

  // Log
  appendFileSync(LOG_PATH, JSON.stringify(session) + '\n');
  console.error(`\nLogged to ${LOG_PATH}`);
}

runCouncil().catch(err => {
  console.error(`Council error: ${err.message}`);
  process.exitCode = 1;
});
