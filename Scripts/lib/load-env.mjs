import { existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { config } from 'dotenv';

const __dirname = dirname(fileURLToPath(import.meta.url));
const SCRIPTS_ROOT = join(__dirname, '..');

const envPath = join(SCRIPTS_ROOT, '.env');
const localPath = join(SCRIPTS_ROOT, '.env.local');

if (existsSync(envPath)) {
  config({ path: envPath, quiet: true });
}
if (existsSync(localPath)) {
  config({ path: localPath, override: true, quiet: true });
}
