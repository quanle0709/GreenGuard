import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..');
const required = [
  'platformio.ini', 'README.md', 'WIRING.md', 'include/HardwareConfig.h', 'include/secrets.example.h',
  'lib/GreenGuardCore/src/GreenGuardCore.h', 'lib/GreenGuardCore/src/GreenGuardCore.cpp',
  'src/main.cpp', 'data/index.html', 'data/style.css', 'data/protocol.js', 'data/app.js',
  'docs/HARDWARE_AUDIT.md', 'docs/SYSTEM_DESIGN.md', 'docs/PROTOCOL.md',
  'docs/HARDWARE_TEST_CHECKLIST.md', 'docs/TEST_RESULTS.md',
];

for (const relative of required) {
  if (!fs.existsSync(path.join(root, relative))) throw new Error(`Missing required file: ${relative}`);
}

for (const relative of ['data/protocol.js', 'data/app.js', 'scripts/mock-dashboard-server.mjs', 'test/web.test.mjs']) {
  execFileSync(process.execPath, ['--check', path.join(root, relative)], { stdio: 'inherit' });
}

const platformio = fs.readFileSync(path.join(root, 'platformio.ini'), 'utf8');
const hardware = fs.readFileSync(path.join(root, 'include/HardwareConfig.h'), 'utf8');
const ignored = fs.readFileSync(path.join(root, '.gitignore'), 'utf8');
if (!/^board\s*=\s*nodemcuv2\s*$/m.test(platformio)) throw new Error('platformio.ini must use board = nodemcuv2');
if (!/ACTUATOR_DRY_RUN\s*=\s*true/.test(hardware)) throw new Error('ACTUATOR_DRY_RUN must default to true');
if (!ignored.includes('include/secrets.h') || !ignored.includes('.pio/')) throw new Error('Secrets or PlatformIO output are not ignored');

const sourceFiles = ['src/main.cpp', 'include/HardwareConfig.h', 'lib/GreenGuardCore/src/GreenGuardCore.cpp'];
for (const relative of sourceFiles) {
  if (/ESP32|esp32/.test(fs.readFileSync(path.join(root, relative), 'utf8'))) throw new Error(`Stale ESP32 source reference: ${relative}`);
}

function findMarkdown(directory) {
  const files = [];
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (entry.name === '.git' || entry.name === '.pio' || entry.name === 'node_modules') continue;
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...findMarkdown(full));
    else if (entry.isFile() && entry.name.endsWith('.md')) files.push(full);
  }
  return files;
}
const markdownFiles = findMarkdown(root);
let relativeLinkCount = 0;
for (const filename of markdownFiles) {
  const text = fs.readFileSync(filename, 'utf8');
  for (const match of text.matchAll(/\[[^\]]+\]\(([^)]+)\)/g)) {
    const target = match[1].split('#')[0];
    if (!target || /^(https?:|mailto:)/i.test(target)) continue;
    ++relativeLinkCount;
    if (!fs.existsSync(path.resolve(path.dirname(filename), decodeURIComponent(target)))) {
      throw new Error(`Broken relative link in ${path.relative(root, filename)}: ${match[1]}`);
    }
  }
}

console.log(`Project checks passed: ${required.length} required artifacts, ${relativeLinkCount} relative documentation links, JavaScript syntax, nodemcuv2 target, dry-run default, ignore rules, and controller scan.`);
