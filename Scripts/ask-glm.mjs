#!/usr/bin/env node
/**
 * Ask GLM 5.1 via Zhipu AI API (OpenAI-compatible endpoint).
 * Uses GLM_API_KEY from scripts/.env or scripts/.env.local.
 * Coding plan endpoint: https://api.z.ai/api/coding/paas/v4
 *
 * Usage:
 *   node scripts/ask-glm.mjs "What is 2+2?"
 *   node scripts/ask-glm.mjs --file prompt.txt
 */
import { readFileSync, appendFileSync, mkdirSync, existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

// Load env vars from scripts/.env and scripts/.env.local
import './lib/load-env.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const LOG_PATH = resolve(ROOT, 'data', 'multi-model-log.jsonl');
const MODEL = process.env.GLM_MODEL || 'glm-5.1';
const TIMEOUT_MS = 240_000;

// GLM/Zhipu AI config
const apiKey = process.env.GLM_API_KEY?.trim();
const baseUrl = (process.env.GLM_BASE_URL?.trim() || 'https://api.z.ai/api/coding/paas/v4').replace(/\/$/, '');
if (!apiKey) {
  console.error('Error: GLM_API_KEY not found in env or scripts/.env.local');
  console.error('Add to scripts/.env.local: GLM_API_KEY=your-key');
  process.exit(1);
}

// Parse args
const args = process.argv.slice(2);
let question = args.filter(a => !a.startsWith('--')).join(' ').trim();
const fileIdx = args.indexOf('--file');
if (fileIdx !== -1 && args[fileIdx + 1]) {
  try {
    question = readFileSync(resolve(args[fileIdx + 1]), 'utf8').trim();
  } catch (e) {
    console.error(`Error reading prompt file: ${e.message}`);
    process.exit(1);
  }
}
if (!question) {
  console.error('Usage: node scripts/ask-glm.mjs "your question"');
  console.error('       node scripts/ask-glm.mjs --file prompt.txt');
  process.exit(1);
}

// Ensure data dir
if (!existsSync(resolve(ROOT, 'data'))) mkdirSync(resolve(ROOT, 'data'), { recursive: true });

const url = `${baseUrl}/chat/completions`;
const start = Date.now();

try {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), TIMEOUT_MS);

  const res = await fetch(url, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      Authorization: `Bearer ${apiKey}`,
    },
    body: JSON.stringify({
      model: MODEL,
      messages: [{ role: 'user', content: question }],
      temperature: 0.3,
      max_tokens: 4096,
    }),
    signal: controller.signal,
  });
  clearTimeout(timer);

  if (!res.ok) {
    const text = await res.text();
    throw new Error(`GLM ${res.status}: ${text.slice(0, 300)}`);
  }

  const data = await res.json();
  const answer = data.choices?.[0]?.message?.content || '(no response)';
  const duration_s = ((Date.now() - start) / 1000).toFixed(1);

  console.log(`[GLM ${MODEL}] ${duration_s}s`);
  console.log(answer);

  // Log
  appendFileSync(LOG_PATH, JSON.stringify({
    ts: new Date().toISOString(),
    model: `glm/${MODEL}`,
    question,
    answer,
    duration_s: parseFloat(duration_s),
  }) + '\n');

} catch (err) {
  if (err.name === 'AbortError') {
    console.error(`[GLM] Timed out after ${TIMEOUT_MS / 1000}s`);
  } else {
    console.error(`[GLM] Error: ${err.message}`);
  }
  process.exit(1);
}
