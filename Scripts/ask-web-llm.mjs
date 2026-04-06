#!/usr/bin/env node
/**
 * Ask web-based LLMs via Playwright browser automation.
 * Connects to your existing Chrome (must be running with --remote-debugging-port=9222).
 *
 * Models: chatgpt (free tier), qwen (free)
 *
 * Setup: First run will open Chromium. Log into ChatGPT/Qwen once.
 * Sessions are saved in data/chrome-llm-profile/ for reuse.
 *
 * Usage:
 *   node scripts/ask-web-llm.mjs chatgpt "What is 2+2?"
 *   node scripts/ask-web-llm.mjs qwen "Best AI for soccer game physics?"
 */
import { chromium } from 'playwright';
import { appendFileSync, mkdirSync, existsSync, readdirSync, cpSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const LOG_PATH = resolve(ROOT, 'data', 'multi-model-log.jsonl');
const TIMEOUT_MS = 60_000;

// Parse args: node script.mjs <model> "question"
const args = process.argv.slice(2);
let model = 'chatgpt';
let question = '';

// Extract model from first positional arg before processing flags
if (args.length > 0 && ['chatgpt', 'qwen'].includes(args[0])) {
  model = args.shift();
}

for (let i = 0; i < args.length; i++) {
  if (args[i] === '--model' && args[i + 1]) { model = args[++i]; continue; }
  if (args[i] === '--question' && args[i + 1]) { question = args[++i]; continue; }
}
// Support --file flag for reading question from file (avoids shell escaping issues)
const fileIdx = args.indexOf('--file');
if (fileIdx !== -1 && args[fileIdx + 1]) {
  try {
    const { readFileSync: rf } = await import('node:fs');
    question = rf(resolve(args[fileIdx + 1]), 'utf8').trim();
  } catch (e) {
    console.error(`Error reading prompt file: ${e.message}`);
    process.exit(1);
  }
}
if (!question && args.length > 0) {
  question = args.filter(a => !a.startsWith('--')).join(' ');
}
if (!question) {
  console.error('Usage: node scripts/ask-web-llm.mjs chatgpt "your question"');
  console.error('       node scripts/ask-web-llm.mjs qwen "your question"');
  console.error('       node scripts/ask-web-llm.mjs --login chatgpt   (opens browser for login)');
  process.exit(1);
}
model = model.toLowerCase();

// Ensure data dir
const dataDir = resolve(ROOT, 'data');
if (!existsSync(dataDir)) mkdirSync(dataDir, { recursive: true });

// ── Model configs ──────────────────────────────────────────────────────────

const MODELS = {
  chatgpt: {
    url: 'https://chatgpt.com',
    name: 'ChatGPT',
    targetModel: '5.4 Thinking',  // text to look for in model dropdown
    async selectModel(page) {
      try {
        // Look for model selector button/toggle
        const modelBtn = await page.$('[data-testid="model-selector-button"], button[aria-label*="Model"], button[aria-label*="model"]');
        if (modelBtn) {
          await modelBtn.click();
          await page.waitForTimeout(500);
          // Look for "5.4 Thinking" or "Thinking" option
          const option = await page.evaluateHandle(target => {
            const items = document.querySelectorAll('[role="menuitem"], [role="option"], li, button');
            for (const item of items) {
              if (item.textContent?.includes(target)) return item;
            }
            return null;
          }, this.targetModel);
          if (option && await option.asElement()) {
            await option.asElement().click();
            await page.waitForTimeout(500);
            console.error(`[web-llm] Selected model: ${this.targetModel}`);
          } else {
            console.error(`[web-llm] Warning: Could not find "${this.targetModel}" in model selector`);
          }
        }
      } catch (e) {
        console.error(`[web-llm] Model selection warning: ${e.message}`);
      }
    },
    async typeAndSubmit(page, q) {
      // Wait for page to fully load (ChatGPT may redirect)
      await page.waitForTimeout(2000);
      // ChatGPT uses a contenteditable div with id="prompt-textarea"
      const input = await page.$('#prompt-textarea');
      if (!input) throw new Error('No ChatGPT input found');
      await input.scrollIntoViewIfNeeded();
      await input.click();
      await page.waitForTimeout(500);
      // Use fill() which triggers React events properly
      await input.fill(q);
      await page.waitForTimeout(300);
      // Click send button or press Enter
      const sendBtn = await page.$('button[data-testid="send-button"], button[aria-label="Send prompt"]');
      if (sendBtn) { await sendBtn.click(); }
      else { await page.keyboard.press('Enter'); }
    },
    async getResponse(page) {
      // ChatGPT 5.4 Thinking: extended wait for reasoning + streaming
      let lastText = '';
      let stableCount = 0;
      const STABLE_THRESHOLD = 3;  // 6s stability for thinking models
      const MIN_LENGTH = 100;
      for (let attempt = 0; attempt < 30; attempt++) {
        await page.waitForTimeout(2000);
        // Check if still generating (Stop button visible = still going)
        const stillGenerating = await page.evaluate(() => {
          const stopBtn = document.querySelector('button[aria-label="Stop"], button[data-testid="stop-button"]');
          if (stopBtn && stopBtn.offsetParent !== null) return true;
          // Thinking spinner
          const spinner = document.querySelector('[class*="animate-spin"], [class*="thinking"]');
          if (spinner) return true;
          return false;
        });
        if (stillGenerating) {
          stableCount = 0;
          continue;
        }
        let text = null;
        try {
          text = await page.evaluate(() => {
            for (const sel of [
              '[data-message-author-role="assistant"]',
              '.markdown.prose',
              '.agent-turn',
              '[data-testid="conversation-turn-2"]',
              '.text-base .markdown',
            ]) {
              const els = document.querySelectorAll(sel);
              for (let i = els.length - 1; i >= 0; i--) {
                const t = els[i].innerText?.trim();
                if (t && t.length > 0) return t;
              }
            }
            return null;
          });
        } catch {
          await page.waitForTimeout(3000);
          continue;
        }
        if (text && text.length >= MIN_LENGTH) {
          if (text === lastText) {
            stableCount++;
            if (stableCount >= STABLE_THRESHOLD) return text;
          } else {
            lastText = text;
            stableCount = 0;
          }
        }
      }
      return lastText || null;
    },
  },

  qwen: {
    url: 'https://chat.qwen.ai',
    name: 'Qwen',
    targetModel: 'Qwen3.5-Omni-Plus',  // text to look for in model dropdown
    async selectModel(page) {
      try {
        // Qwen has a model selector — typically a dropdown/button in the header or sidebar
        const selectors = [
          '.model-select', '[class*="model-select"]', '[class*="ModelSelect"]',
          'button[aria-label*="Model"]', 'select[class*="model"]',
          '.chat-header button', '.header button[class*="model"]',
        ];
        for (const sel of selectors) {
          const btn = await page.$(sel);
          if (btn) {
            await btn.click();
            await page.waitForTimeout(500);
            const option = await page.evaluateHandle(target => {
              const items = document.querySelectorAll('[role="option"], li, button, [class*="option"], [class*="item"]');
              for (const item of items) {
                if (item.textContent?.includes(target)) return item;
              }
              return null;
            }, this.targetModel);
            if (option && await option.asElement()) {
              await option.asElement().click();
              await page.waitForTimeout(500);
              console.error(`[web-llm] Selected model: ${this.targetModel}`);
              return;
            }
            // Close dropdown if no match
            await page.keyboard.press('Escape');
            break;
          }
        }
        console.error(`[web-llm] Warning: Could not find model selector for "${this.targetModel}"`);
      } catch (e) {
        console.error(`[web-llm] Model selection warning: ${e.message}`);
      }
    },
    async typeAndSubmit(page, q) {
      // Try textarea first (Qwen's primary input)
      const ta = await page.$('textarea');
      if (ta) {
        await ta.scrollIntoViewIfNeeded();
        await ta.click();
        await page.waitForTimeout(300);
        await ta.fill(q);
      } else {
        const ce = await page.$('[contenteditable="true"]');
        if (!ce) throw new Error('No Qwen input found');
        await ce.scrollIntoViewIfNeeded();
        await ce.click();
        await page.waitForTimeout(300);
        await page.evaluate(text => navigator.clipboard.writeText(text), q);
        await page.keyboard.press('Control+v');
      }
      await page.waitForTimeout(500);
      const sendBtn = await page.$('button[aria-label="Send"], button[class*="send"], button[data-testid="send-button"]');
      if (sendBtn) { await sendBtn.click(); }
      else { await page.keyboard.press('Enter'); }
      // Wait for page to navigate from homepage to conversation (/c/...)
      await page.waitForTimeout(2000);
      for (let i = 0; i < 10; i++) {
        const url = page.url();
        if (url.includes('/c/') || url.includes('/chat/')) break;
        await page.waitForTimeout(1000);
      }
    },
    async getResponse(page, question) {
      // Qwen: extended wait for thinking + web search + streaming
      let lastText = '';
      let stableCount = 0;
      const STABLE_THRESHOLD = 3;  // 6s stability
      const MIN_LENGTH = 5;  // short answers are valid
      // Skip only if the ENTIRE text is a short status indicator, not if it's a full response
      const statusPhrases = ['Searching the web', 'Thinking completed', 'Log in', 'Sign up', 'Skip'];
      for (let attempt = 0; attempt < 30; attempt++) {
        await page.waitForTimeout(2000);
        // Check if still generating
        const stillGenerating = await page.evaluate(() => {
          // Stop/Cancel button visible = still generating
          const stopBtn = document.querySelector('button[aria-label*="Stop"], button[aria-label*="Cancel"], button[class*="stop"]');
          if (stopBtn && stopBtn.offsetParent !== null) return true;
          // Typing indicator
          const typing = document.querySelector('[class*="typing"], [class*="generating"], [class*="loading"]');
          if (typing) return true;
          return false;
        });
        if (stillGenerating) {
          stableCount = 0;
          continue;
        }
        const text = await page.evaluate((statusPhrases, questionPrefix) => {
          // Strategy: use body text to find the response.
          // Qwen's DOM uses obfuscated class names that change frequently.
          const bodyText = document.body?.innerText || '';
          const lines = bodyText.split('\n').map(l => l.trim()).filter(l => l.length > 0);

          // Find the user's question by matching first ~30 chars, then take everything after
          const qSnippet = questionPrefix.substring(0, 30);
          let questionIdx = -1;
          for (let i = lines.length - 1; i >= 0; i--) {
            if (lines[i].includes(qSnippet)) { questionIdx = i; break; }
          }

          if (questionIdx === -1) {
            // Fallback: try to find response by structural position
            // Skip known non-response lines
            const skipExact = ['New Chat', 'Community', 'Coder', 'Projects', 'All chats', 'Auto', '?',
              'Previous 7 days', 'Today'];
            let responseLines = [];
            let pastSidebar = false;
            for (const line of lines) {
              if (skipExact.includes(line)) continue;
              if (line.match(/^Qwen/i)) { pastSidebar = true; continue; }
              if (line.startsWith('Byron') || line.includes('Download App') || line.includes('QR code')) continue;
              if (line.includes('AI-generated content') || line.includes('What do you want to know')) continue;
              if (!pastSidebar) continue;
              if (statusPhrases.some(s => line === s)) continue;
              responseLines.push(line);
            }
            return responseLines.length > 0 ? responseLines.join('\n') : null;
          }

          // Everything after the question line (skip status indicators)
          let responseLines = [];
          for (let i = questionIdx + 1; i < lines.length; i++) {
            const line = lines[i];
            if (statusPhrases.some(s => line === s || line.includes(s))) continue;
            if (line === 'Auto' || line.includes('AI-generated content')) continue;
            responseLines.push(line);
          }
          return responseLines.length > 0 ? responseLines.join('\n') : null;
        }, statusPhrases, question.substring(0, 30));
        if (text && text.length >= MIN_LENGTH) {
          if (text === lastText) {
            stableCount++;
            if (stableCount >= STABLE_THRESHOLD) return text;
          } else {
            lastText = text;
            stableCount = 0;
          }
        }
      }
      return lastText || null;
    },
  },
};

const config = MODELS[model];
if (!config) {
  console.error(`Unknown model: ${model}. Supported: chatgpt, qwen`);
  process.exit(1);
}

// ── Helpers ─────────────────────────────────────────────────────────────────

async function isLoggedIn(page, url) {
  // Platform-specific login checks
  if (url.includes('chatgpt.com')) {
    const input = await page.$('#prompt-textarea, textarea');
    return !!input;
  }
  if (url.includes('chat.qwen.ai')) {
    const bodyText = await page.evaluate(() => document.body?.innerText?.substring(0, 2000) || '');
    if (bodyText.includes('Log in') || bodyText.includes('Sign up')) return false;
    const input = await page.$('textarea, [contenteditable="true"]');
    return !!input;
  }
  return true;
}

// ── Main ───────────────────────────────────────────────────────────────────

async function run() {
  const start = Date.now();
  let browser, page;

  // Retry browser launch — profile directory lock can persist briefly after previous close
  let context;
  for (let attempt = 1; attempt <= 3; attempt++) {
    try {
      console.error(`[web-llm] Launching Chromium with saved profile (attempt ${attempt})...`);
      // Each model gets its own profile dir to avoid Chromium lock conflicts
      const profileDir = resolve(ROOT, 'data', `chrome-profile-${model}`);

      // Seed cookies from Polymarket's shared profile if this profile doesn't exist yet
      if (!existsSync(profileDir)) {
        const polyProfile = resolve(ROOT, '..', 'Polymarket', 'data', 'chrome-llm-profile');
        if (existsSync(polyProfile)) {
          mkdirSync(profileDir, { recursive: true });
          cpSync(polyProfile, profileDir, {
            filter: (src) => !src.endsWith('.lock') && !src.endsWith('-journal') && src !== 'LOCK',
            dereference: true,
          });
          console.error(`[web-llm] Seeded profile from Polymarket for ${model}`);
        }
      }

      context = await chromium.launchPersistentContext(
        profileDir,
        {
          headless: false,  // must be visible — sites need interactive login
          args: ['--no-first-run', '--disable-blink-features=AutomationControlled'],
        }
      );
      break;
    } catch (e) {
      if (attempt < 3 && (e.message.includes('closed') || e.message.includes('lock') || e.message.includes('profile'))) {
        console.error(`[web-llm] Profile lock detected, waiting 5s before retry...`);
        await new Promise(r => setTimeout(r, 5000));
        continue;
      }
      throw e;
    }
  }
  browser = context;
  // Reuse default page instead of creating new ones each run
  page = context.pages()[0] || await context.newPage();

  page.setDefaultTimeout(TIMEOUT_MS);

  console.error(`[web-llm] Opening ${config.name}...`);
  await page.goto(config.url, { waitUntil: 'domcontentloaded', timeout: 30000 });
  await page.waitForTimeout(3000);

  try {
    // Check login status — if not logged in, wait for user
    const loggedIn = await isLoggedIn(page, config.url);
    if (!loggedIn) {
      console.error(`\n[web-llm] *** NOT LOGGED IN to ${config.name} ***`);
      console.error(`[web-llm] Please log in using the browser window that opened.`);
      console.error(`[web-llm] Waiting up to 120s for login...\n`);
      try {
        await page.waitForFunction(() => {
          const bodyText = document.body?.innerText?.substring(0, 3000) || '';
          if (bodyText.includes('Sign in')) return false;
          if (document.querySelector('img[data-ogsr-up], img[alt*="Account"], .user-avatar, [aria-label*="Account"]')) return true;
          const input = document.querySelector('.ql-editor[contenteditable="true"], textarea, [contenteditable="true"]');
          return !!input;
        }, { timeout: 120_000 });
        console.error(`[web-llm] Login detected! Continuing...`);
        await page.waitForTimeout(2000);
      } catch {
        console.error(`[web-llm] Login timeout. Screenshot saved.`);
        await page.screenshot({ path: resolve(ROOT, 'data', `debug-${model}-login.png`) });
        process.exit(1);
      }
    }

    // Start a new chat to avoid picking up old responses
    try {
      if (config.url.includes('chat.qwen.ai')) {
        const newChatBtn = await page.$('a[href="/"], button[aria-label*="New"], button[class*="new-chat"], a[class*="new-chat"]');
        if (newChatBtn) { await newChatBtn.click(); await page.waitForTimeout(1000); }
        else { await page.goto(config.url, { waitUntil: 'domcontentloaded', timeout: 15000 }); await page.waitForTimeout(2000); }
      } else if (config.url.includes('chatgpt.com')) {
        const newChatLink = await page.$('a[href="/"], a[aria-label*="New chat"], a[data-testid*="new"]');
        if (newChatLink) { await newChatLink.click(); await page.waitForTimeout(1000); }
      }
    } catch (e) {
      console.error(`[web-llm] New-chat nav warning: ${e.message}`);
    }

    // Auto-select target model in web UI (e.g., "5.4 Thinking" for ChatGPT)
    if (config.selectModel) {
      await config.selectModel(page);
    }

    console.error(`[web-llm] Sending question...`);
    await config.typeAndSubmit(page, question);

    console.error(`[web-llm] Waiting for response...`);
    const response = await config.getResponse(page, question);

    if (!response) {
      const ssPath = resolve(ROOT, 'data', `debug-${model}.png`);
      await page.screenshot({ path: ssPath });
      console.error(`[web-llm] No response found. Screenshot: ${ssPath}`);
    }

    const duration_s = ((Date.now() - start) / 1000).toFixed(1);
    const answer = response || '(no response)';

    // Output to stdout
    console.log(`[${config.name} Web] ${duration_s}s`);
    console.log(answer);

    // Log
    appendFileSync(LOG_PATH, JSON.stringify({
      ts: new Date().toISOString(),
      model: `web/${model}`,
      question,
      answer,
      duration_s: parseFloat(duration_s),
    }) + '\n');

  } catch (err) {
    console.error(`[web-llm] Error: ${err.message}`);
    process.exitCode = 1;
  } finally {
    // Close the whole browser (persistent context = our standalone Chromium)
    try { if (browser) await browser.close(); } catch {}
  }
}

run();
