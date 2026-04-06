#!/usr/bin/env node
/**
 * Ask Gemini API a question — deeper reasoning, cloud-based.
 * Usage: node scripts/ask-gemini.mjs "Should we use behavior trees or utility AI?"
 * Uses GEMINI_API_KEY from scripts/.env or scripts/.env.local.
 * Logs to data/multi-model-log.jsonl
 */
import { readFileSync, appendFileSync, mkdirSync, existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

// Load env vars from scripts/.env and scripts/.env.local
import './lib/load-env.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const LOG_PATH = resolve(ROOT, 'data', 'multi-model-log.jsonl');
const MODEL = process.env.GEMINI_MODEL || 'gemini-2.5-flash';
const TIMEOUT_MS = 30_000;

const apiKey = process.env.GEMINI_API_KEY?.trim();
if (!apiKey) {
  console.error('Error: GEMINI_API_KEY not found in env or scripts/.env.local');
  console.error('Add to scripts/.env.local: GEMINI_API_KEY=your-key');
  process.exit(1);
}

const question = process.argv.slice(2).join(' ').trim();
// Support --file flag for reading question from file (avoids shell escaping issues)
let finalQuestion = question;
const fileIdx = process.argv.indexOf('--file');
if (fileIdx !== -1 && process.argv[fileIdx + 1]) {
  try {
    finalQuestion = readFileSync(resolve(process.argv[fileIdx + 1]), 'utf8').trim();
  } catch (e) {
    console.error(`Error reading prompt file: ${e.message}`);
    process.exit(1);
  }
}
if (!finalQuestion) {
  console.error('Usage: node scripts/ask-gemini.mjs "your question"');
  console.error('       node scripts/ask-gemini.mjs --file prompt.txt');
  process.exit(1);
}

// ensure data dir exists
if (!existsSync(resolve(ROOT, 'data'))) mkdirSync(resolve(ROOT, 'data'), { recursive: true });

const url = `https://generativelanguage.googleapis.com/v1beta/models/${encodeURIComponent(MODEL)}:generateContent?key=${encodeURIComponent(apiKey)}`;

const start = Date.now();

try {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), TIMEOUT_MS);

  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      contents: [{ parts: [{ text: finalQuestion }] }],
      generationConfig: { maxOutputTokens: 2048 },
    }),
    signal: controller.signal,
  });
  clearTimeout(timer);

  if (!res.ok) {
    const text = await res.text();
    throw new Error(`Gemini ${res.status}: ${text}`);
  }

  const data = await res.json();
  const answer = data.candidates?.[0]?.content?.parts?.[0]?.text || '(no response)';
  const duration_s = ((Date.now() - start) / 1000).toFixed(1);

  console.log(`[Gemini ${MODEL}] ${duration_s}s`);
  console.log(answer);

  // log it
  const entry = {
    ts: new Date().toISOString(),
    model: `gemini/${MODEL}`,
    question: finalQuestion,
    answer,
    duration_s: parseFloat(duration_s),
  };
  appendFileSync(LOG_PATH, JSON.stringify(entry) + '\n');
} catch (err) {
  if (err.name === 'AbortError') {
    console.error(`[Gemini] Timed out after ${TIMEOUT_MS / 1000}s`);
  } else {
    console.error(`[Gemini] Error: ${err.message}`);
  }
  process.exit(1);
}
