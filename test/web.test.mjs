import { after, before, test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { classifyAcknowledgement, formatDuration, isStatusStale, isValidStatus, makeRequestId } from '../data/protocol.js';
import { createMockServer } from '../scripts/mock-dashboard-server.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
let assertionCount = 0;
const equal = (...args) => { ++assertionCount; assert.equal(...args); };
const ok = (...args) => { ++assertionCount; assert.ok(...args); };
const match = (...args) => { ++assertionCount; assert.match(...args); };
let server;
let base;

before(async () => {
  server = createMockServer();
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  base = `http://127.0.0.1:${server.address().port}`;
});

after(async () => {
  await new Promise((resolve) => server.close(resolve));
  fs.mkdirSync(path.join(root, '.pio'), { recursive: true });
  fs.writeFileSync(path.join(root, '.pio', 'web-assertions.txt'), `${assertionCount}\n`);
});

test('request IDs are nonzero uint32 values', () => {
  const value = makeRequestId({ getRandomValues: (array) => { array[0] = 0; return array; } });
  equal(value, 1);
  const normal = makeRequestId({ getRandomValues: (array) => { array[0] = 4294967295; return array; } });
  equal(normal, 4294967295);
});

test('mismatched and delayed acknowledgements cannot complete another request', () => {
  const pending = { requestId: 77, command: 'DEPLOY' };
  equal(classifyAcknowledgement(pending, { lastRequestId: 76, commandPhase: 'COMPLETED' }).state, 'waiting');
  equal(classifyAcknowledgement(pending, { lastRequestId: 77, commandPhase: 'ACCEPTED' }).state, 'accepted');
  equal(classifyAcknowledgement(pending, { lastRequestId: 77, commandPhase: 'STARTED' }).state, 'started');
  equal(classifyAcknowledgement(pending, { lastRequestId: 77, commandPhase: 'COMPLETED' }).state, 'completed');
});

test('terminal stop, rejection, and fault phases stay distinct', () => {
  const pending = { requestId: 8 };
  equal(classifyAcknowledgement(pending, { lastRequestId: 8, commandPhase: 'STOPPED' }).state, 'stopped');
  equal(classifyAcknowledgement(pending, { lastRequestId: 8, commandPhase: 'REJECTED', lastCommandResult: 'REJECTED_RAIN' }).state, 'rejected');
  equal(classifyAcknowledgement(pending, { lastRequestId: 8, commandPhase: 'FAULT' }).state, 'fault');
});

test('status validation and stale detection reject malformed device state', () => {
  ok(isValidStatus({ protocolVersion: 2, platformioBoard: 'nodemcuv2', mode: 'AUTO', motionState: 'STOPPED', fault: 'NONE' }));
  equal(isValidStatus({ protocolVersion: 2, platformioBoard: 'esp32' }), false);
  equal(isStatusStale(1000, 5999, 5000), false);
  equal(isStatusStale(1000, 6001, 5000), true);
  equal(formatDuration(3723000), '1g 2p');
});

test('dashboard contains precise controls and no vague ON/OFF switch', () => {
  const html = fs.readFileSync(path.join(root, 'data', 'index.html'), 'utf8');
  for (const command of ['SET_AUTO', 'SET_MANUAL', 'DEPLOY', 'RETRACT', 'STOP', 'RESET_FAULT']) match(html, new RegExp(`data-command="${command}"`));
  match(html, /Che cây/);
  match(html, /Thu mái che/);
  equal(/>\s*(ON|OFF)\s*</i.test(html), false);
});

test('dashboard code sends token and request ID and treats timeout honestly', () => {
  const script = fs.readFileSync(path.join(root, 'data', 'app.js'), 'utf8');
  match(script, /X-GreenGuard-Token/);
  match(script, /requestId/);
  match(script, /chưa có kết quả sau 90 giây; không xem đây là thành công/);
  match(script, /classifyAcknowledgement/);
});

test('responsive and reduced-motion CSS contracts exist', () => {
  const css = fs.readFileSync(path.join(root, 'data', 'style.css'), 'utf8');
  match(css, /@media \(max-width: 620px\)/);
  match(css, /prefers-reduced-motion/);
  match(css, /button:focus-visible/);
});

test('mock device serves a valid nodemcuv2 status and assets', async () => {
  const status = await (await fetch(`${base}/api/status`)).json();
  ok(isValidStatus(status));
  equal(status.actuatorDryRun, true);
  equal(status.confirmedController, 'NodeMCU 1.0 (ESP-12E Module)');
  const page = await fetch(`${base}/`);
  equal(page.status, 200);
  match(await page.text(), /GreenGuard/);
});

test('control API rejects missing auth and malformed messages', async () => {
  const unauthorized = await fetch(`${base}/api/command`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ requestId: 1, command: 'STOP' }) });
  equal(unauthorized.status, 401);
  const malformed = await fetch(`${base}/api/command`, { method: 'POST', headers: { 'X-GreenGuard-Token': 'test-token-1234' }, body: '{oops' });
  equal(malformed.status, 400);
  const wrongVersion = await fetch(`${base}/api/command`, { method: 'POST', headers: { 'Content-Type': 'application/json', 'X-GreenGuard-Token': 'test-token-1234' }, body: JSON.stringify({ protocolVersion: 1, requestId: 1, command: 'STOP' }) });
  equal(wrongVersion.status, 400);
});

test('login token, command, delayed acknowledgement, and duplicate flow integrates', async () => {
  const headers = { 'Content-Type': 'application/json', 'X-GreenGuard-Token': 'test-token-1234' };
  let response = await fetch(`${base}/api/command`, { method: 'POST', headers, body: JSON.stringify({ protocolVersion: 2, requestId: 100, command: 'SET_MANUAL' }) });
  equal(response.status, 200);
  equal((await response.json()).phase, 'COMPLETED');

  response = await fetch(`${base}/api/command`, { method: 'POST', headers, body: JSON.stringify({ protocolVersion: 2, requestId: 101, command: 'DEPLOY' }) });
  const accepted = await response.json();
  equal(accepted.accepted, true);
  equal(accepted.phase, 'STARTED');
  let status = await (await fetch(`${base}/api/status`)).json();
  equal(status.lastRequestId, 101);
  equal(status.commandPhase, 'STARTED');
  await new Promise((resolve) => setTimeout(resolve, 300));
  status = await (await fetch(`${base}/api/status`)).json();
  equal(status.commandPhase, 'COMPLETED');
  equal(status.shieldState, 'DEPLOYED');

  response = await fetch(`${base}/api/command`, { method: 'POST', headers, body: JSON.stringify({ protocolVersion: 2, requestId: 101, command: 'RETRACT' }) });
  const duplicate = await response.json();
  equal(duplicate.duplicate, true);
  equal(duplicate.accepted, false);
  status = await (await fetch(`${base}/api/status`)).json();
  equal(status.shieldState, 'DEPLOYED');
});

test('wet state blocks retract with request-specific rejection', async () => {
  await fetch(`${base}/mock/rain?state=wet`, { method: 'POST' });
  const response = await fetch(`${base}/api/command`, {
    method: 'POST', headers: { 'Content-Type': 'application/json', 'X-GreenGuard-Token': 'test-token-1234' },
    body: JSON.stringify({ protocolVersion: 2, requestId: 102, command: 'RETRACT' }),
  });
  equal(response.status, 409);
  const payload = await response.json();
  equal(payload.result, 'REJECTED_RAIN');
  equal(payload.requestId, 102);
});
