/**
 * Council of Minds v2 — Prompt Builder (SoccerSim Edition)
 *
 * Game-dev focused roles for 4 models:
 *   lead          — GLM 5.1: Lead Game Designer (deepest analysis, UE5.7 C++ expertise)
 *   scout         — Gemini 2.5 Flash: Physics Engine Scout (quick fact-check on physics/gameplay)
 *   devil_advocate — ChatGPT 5.4 Thinking: find flaws in game design reasoning
 *   cross_domain  — Qwen 3.5-Omni-Plus: lessons from FIFA/EA FC, PES, Rocket League, other games
 */

const ROLE_FRAMES = {
  lead: {
    label: 'Lead Game Designer',
    instruction: `You are the Lead Game Designer in a multi-AI deliberation about a soccer game built in Unreal Engine 5.7 (C++). Your job is to produce the primary analysis — the deepest, most thorough answer about game design, gameplay mechanics, physics, AI behavior, or any game development question. You will later receive feedback from three reviewers and produce a final synthesis.`,
    closing: 'Finally, state what evidence would change your mind and identify the single biggest risk to your design recommendation.',
  },
  scout: {
    label: 'Physics Engine Scout',
    instruction: `You are the Physics Engine Scout — a fast, broad reviewer with expertise in game physics and sports simulation. Your job is to fact-check physics claims, validate gameplay mechanics against real-world soccer, flag any unrealistic behaviors or missed edge cases. Be FAST and BROAD — quick validation.`,
    closing: 'Finally, rate the lead analysis overall: STRONG / MODERATE / WEAK for gameplay realism.',
  },
  devil_advocate: {
    label: "Devil's Advocate",
    instruction: `You are the Devil's Advocate — find flaws in game design reasoning. Challenge assumptions about fun vs realism, identify logical gaps in AI behavior proposals, surface edge cases that would break gameplay, question whether proposed features are achievable in UE5.7 C++. Be rigorous but fair.`,
    closing: "Finally, rate: CONVINCING / PARTIALLY CONVINCING / UNCONVINCING as a game design, and explain why.",
  },
  cross_domain: {
    label: 'Cross-Domain Reviewer',
    instruction: `You are the Cross-Domain Reviewer — examine from a different perspective. What lessons from FIFA/EA FC, PES/eFootball, Football Manager, Rocket League, or other sports games apply? Are there analogies from fighting games, RTS, or other genres that could improve this soccer game? What blind spots might the lead have about player experience?`,
    closing: 'Finally, identify the single most important design perspective the lead analysis is missing.',
  },
};

// ── Round 1: Lead thesis ─────────────────────────────────────────────────────

export function buildLeadPrompt(question) {
  const frame = ROLE_FRAMES.lead;
  return [
    frame.instruction,
    '',
    `QUESTION: ${question}`,
    '',
    'Provide your analysis. Structure it clearly with sections.',
    'End with 3-5 specific testable claims in this EXACT format:',
    'CLAIM: <specific statement> | CONFIDENCE: <0.0-1.0>',
    '',
    frame.closing,
  ].join('\n');
}

// ── Round 2: Reviewer evaluates lead thesis ──────────────────────────────────

export function buildReviewerPrompt(question, leadAnswer, role) {
  const frame = ROLE_FRAMES[role] || ROLE_FRAMES.scout;
  return [
    frame.instruction,
    '',
    `ORIGINAL QUESTION: ${question}`,
    '',
    'LEAD GAME DESIGNER\'S THESIS:',
    leadAnswer || '(no thesis)',
    '',
    'Review the lead\'s analysis from your role\'s perspective. Be specific — cite particular claims they made.',
    'End with 2-4 claims in this EXACT format:',
    'CLAIM: <your assessment> | CONFIDENCE: <0.0-1.0>',
    '',
    frame.closing,
  ].join('\n');
}

// ── Round 2 (standard mode): All models see all peer answers ──────────────

export function buildCritiquePrompt(question, selfAnswer, allAnswers, role, roles) {
  const frame = ROLE_FRAMES[role] || ROLE_FRAMES.scout;
  // allAnswers is an array of {model, role, answer} — not a keyed object
  const peerEntries = (Array.isArray(allAnswers) ? allAnswers : Object.values(allAnswers))
    .filter(r => r?.answer && r.answer !== '(no answer)')
    .map(r => {
      const r2 = r.role || roles?.[r.model] || 'reviewer';
      return `--- ${r.model} (${r2}) ---\n${r.answer}`;
    })
    .join('\n\n');

  return [
    `You are the ${frame.label} in a multi-model deliberation.`,
    frame.instruction,
    '',
    `ORIGINAL QUESTION: ${question}`,
    '',
    'YOUR FIRST ANSWER:',
    selfAnswer || '(no answer)',
    '',
    'PEERS\' ANSWERS:',
    peerEntries || '(no peer answers yet)',
    '',
    'From your role\'s perspective, critique and refine your position.',
    'End with 2-4 UPDATED claims:',
    'CLAIM: <statement> | CONFIDENCE: <0.0-1.0> | STANCE: <defend/revise/concede>',
  ].join('\n');
}

// ── Round 3: GLM synthesizes ──────────────────────────────────────────────────

export function buildSynthesisPrompt(question, leadAnswer, reviews) {
  const reviewSections = reviews.map(r =>
    `--- ${r.model} (${r.role}) ---\n${r.answer || '(no response)'}`
  ).join('\n\n');

  return [
    'You are the Lead Game Designer. You produced an initial thesis and three reviewers have now provided their feedback.',
    'Synthesize everything into your FINAL answer.',
    '',
    `ORIGINAL QUESTION: ${question}`,
    '',
    'YOUR INITIAL THESIS:',
    leadAnswer,
    '',
    'REVIEWER FEEDBACK:',
    reviewSections,
    '',
    'Produce a refined final answer. Address the strongest critiques. Where reviewers changed your mind, say so.',
    'End with your FINAL claims:',
    'CLAIM: <statement> | CONFIDENCE: <0.0-1.0> | STANCE: <defend/revise/concede>',
    '(List 3-5 claims, one per line.)',
  ].join('\n');
}

// ── Round 4 (full mode): GLM identifies gaps and forms sub-questions ─────────

export function buildSubQuestionPrompt(question, sessionRounds) {
  const roundSummaries = sessionRounds.map(r => {
    const responses = r.responses.map(resp =>
      `[${resp.model}]: ${(resp.answer || '(no answer)').substring(0, 500)}`
    ).join('\n');
    return `Round ${r.round} (${r.phase}):\n${responses}`;
  }).join('\n\n');

  return [
    `You are the Lead Game Designer. After ${sessionRounds.length} rounds, gaps remain.`,
    '',
    `ORIGINAL QUESTION: ${question}`,
    '',
    'DELIBERATION SO FAR:',
    roundSummaries,
    '',
    'Identify the 2-3 most critical gaps or unresolved sub-questions.',
    'For each gap, formulate a focused sub-question and provide your best answer.',
    '',
    'End with your UPDATED claims:',
    'CLAIM: <statement> | CONFIDENCE: <0.0-1.0> | STANCE: <defend/revise/concede>',
  ].join('\n');
}

// ── Round 5 (full mode): All models address convergence ──────────────────────

export function buildConvergencePrompt(question, gapAnalysis, sessionRounds, role) {
  const frame = ROLE_FRAMES[role] || ROLE_FRAMES.scout;
  const keyPoints = sessionRounds.slice(-2).map(r =>
    `Round ${r.round}: ${(r.responses[0]?.answer || '(no answer)').substring(0, 300)}`
  ).join('\n');

  return [
    `You are the ${frame.label}. The council has gone through multiple rounds of analysis.`,
    'A gap analysis has been performed. Now address the identified gaps.',
    '',
    `ORIGINAL QUESTION: ${question}`,
    '',
    'GAP ANALYSIS:',
    (gapAnalysis || '(none)').substring(0, 2000),
    '',
    'KEY POINTS FROM EARLIER ROUNDS:',
    keyPoints,
    '',
    'From your role\'s perspective, address the identified gaps.',
    'Do you agree with the gap analysis? What would you add?',
    'End with 2-4 FINAL claims:',
    'CLAIM: <statement> | CONFIDENCE: <0.0-1.0> | STANCE: <defend/revise/concede>',
  ].join('\n');
}

// ── Backwards compat ─────────────────────────────────────────────────────────

export function buildFirstRoundPrompt(question, role) {
  if (role === 'lead') return buildLeadPrompt(question);
  const frame = ROLE_FRAMES[role] || ROLE_FRAMES.scout;
  return [
    `You are the ${frame.label}. ${frame.instruction}`,
    '',
    `QUESTION: ${question}`,
    '',
    'Answer the question directly, then end with 3-5 claims:',
    'CLAIM: <specific statement> | CONFIDENCE: <0.0-1.0>',
    '',
    frame.closing,
  ].join('\n');
}

// ── Audit: Council self-check ────────────────────────────────────────────

export function buildAuditPrompt() {
  return [
    'COUNCIL STRUCTURE AUDIT',
    '',
    'You are part of a Council of Minds — a multi-AI deliberation system with 4 models:',
    '',
    '1. GLM 5.1 (Lead Game Designer) — via Zhipu AI API',
    '2. Gemini 2.5 Flash (Physics Engine Scout) — via Google API',
    '3. ChatGPT 5.4 Thinking (Devil\'s Advocate) — via chatgpt.com web automation',
    '4. Qwen 3.5-Omni-Plus (Cross-Domain Reviewer) — via chat.qwen.ai web automation',
    '',
    'Council Flow:',
    '  - Mode quick: 1 round, all 4 answer independently, consensus (~30s)',
    '  - Mode standard (default): 2 rounds — blind answers then cross-examination',
    '  - Mode deep: 3 rounds — GLM thesis → 3 reviewers → GLM synthesis',
    '  - Mode full: 3-5 rounds — iterative convergence with gap analysis',
    '',
    'Please audit this structure:',
    '1. Are the 4 roles well-assigned for game development deliberation?',
    '2. Is the flow (round structure) optimal? Would you change the order or add/remove rounds?',
    '3. Are there any missing models that should be included?',
    '4. Any concerns about reliability (web models failing, timeouts, selector drift)?',
    '5. Is the consensus scoring (claim overlap + confidence spread) adequate?',
    '',
    'End with CLAIM: <your assessment> | CONFIDENCE: <0.0-1.0> for each point.',
  ].join('\n');
}

export { ROLE_FRAMES };
