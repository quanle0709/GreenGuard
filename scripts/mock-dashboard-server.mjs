import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const assets = new Map([
  ['/', ['data/index.html', 'text/html; charset=utf-8']],
  ['/style.css', ['data/style.css', 'text/css; charset=utf-8']],
  ['/protocol.js', ['data/protocol.js', 'application/javascript; charset=utf-8']],
  ['/app.js', ['data/app.js', 'application/javascript; charset=utf-8']],
]);

export function createMockServer({ token = 'test-token-1234' } = {}) {
  const seen = new Set();
  const status = {
    protocolVersion: 2,
    deviceName: 'GreenGuard Mock',
    confirmedController: 'NodeMCU 1.0 (ESP-12E Module)',
    platformioBoard: 'nodemcuv2',
    actuatorDryRun: true,
    controlAuthConfigured: true,
    filesystemReady: true,
    uptimeMs: 843200,
    mode: 'AUTO',
    rainState: 'DRY',
    rainRawWet: false,
    rainDigitalLevel: 'HIGH',
    shieldState: 'RETRACTED',
    motionState: 'STOPPED',
    positionConfidence: 'ESTIMATED',
    estimateAvailable: true,
    estimatedPositionPct: 0,
    stopLatched: false,
    fault: 'NONE',
    rainEventCount: 4,
    movementElapsedMs: 0,
    movementDurationMs: 30000,
    lastRequestId: 0,
    lastCommand: 'INVALID',
    lastCommandResult: 'IDLE',
    commandPhase: 'NONE',
    wifiConnected: true,
    wifiRssi: -53,
    ipAddress: '192.168.1.88',
    freeHeapBytes: 39120,
    useLimitSwitches: false,
  };
  const config = {
    wetConfirmMs: 3000, dryConfirmMs: 120000, directionDeadTimeMs: 300,
    inferredFullTravelMs: 30000, motorMaxRuntimeMs: 35000, motorPwm: 820,
    rainActiveLow: true, deployUsesRpwm: true, useLimitSwitches: false,
  };

  const server = http.createServer(async (request, response) => {
    const url = new URL(request.url, 'http://localhost');
    if (request.method === 'GET' && assets.has(url.pathname)) {
      const [relative, contentType] = assets.get(url.pathname);
      response.writeHead(200, { 'Content-Type': contentType, 'Cache-Control': url.pathname === '/' ? 'no-store' : 'public, max-age=300' });
      return response.end(await fs.promises.readFile(path.join(root, relative)));
    }
    if (request.method === 'GET' && url.pathname === '/api/status') return json(response, 200, status);
    if (request.method === 'GET' && url.pathname === '/api/config') return json(response, 200, config);
    if (request.method === 'GET' && url.pathname === '/health') return response.writeHead(200).end('OK');
    if (request.method === 'POST' && (url.pathname === '/api/command' || url.pathname === '/api/config')) {
      if (request.headers['x-greenguard-token'] !== token) return json(response, 401, { received: false, accepted: false, message: 'Bad token' });
      let body;
      try { body = await readJson(request); }
      catch { return json(response, 400, { received: false, accepted: false, message: 'Malformed JSON' }); }
      if (url.pathname === '/api/config') {
        Object.assign(config, body);
        return json(response, 200, { success: true, message: 'Đã lưu cấu hình.' });
      }
      return handleCommand(response, body);
    }
    if (request.method === 'POST' && url.pathname === '/mock/rain') {
      status.rainState = url.searchParams.get('state') === 'wet' ? 'WET' : 'DRY';
      status.rainRawWet = status.rainState === 'WET';
      return json(response, 200, { success: true });
    }
    return json(response, 404, { success: false, message: 'Not found' });
  });

  function handleCommand(response, body) {
    const requestId = Number(body?.requestId);
    const command = String(body?.command || '');
    if (body?.protocolVersion !== 2 || !Number.isInteger(requestId) || requestId <= 0 || requestId > 0xffffffff || !['SET_AUTO', 'SET_MANUAL', 'DEPLOY', 'RETRACT', 'STOP', 'RESET_FAULT', 'CALIBRATE_DEPLOYED', 'CALIBRATE_RETRACTED'].includes(command)) {
      return json(response, 400, { received: false, accepted: false, result: 'INVALID', phase: 'REJECTED' });
    }
    if (seen.has(requestId)) return json(response, 200, { received: true, accepted: false, duplicate: true, requestId, command, result: 'DUPLICATE', phase: status.commandPhase });
    seen.add(requestId);
    status.lastRequestId = requestId;
    status.lastCommand = command;
    status.lastCommandResult = 'ACCEPTED';
    if (command === 'RETRACT' && status.rainState !== 'DRY') {
      status.lastCommandResult = 'REJECTED_RAIN';
      status.commandPhase = 'REJECTED';
      return json(response, 409, { received: true, accepted: false, requestId, command, result: 'REJECTED_RAIN', phase: 'REJECTED' });
    }
    if (command === 'STOP') {
      status.mode = 'MANUAL'; status.motionState = 'STOPPED'; status.stopLatched = true; status.commandPhase = 'STOPPED';
      return json(response, 200, { received: true, accepted: true, requestId, command, result: 'ACCEPTED', phase: 'STOPPED' });
    }
    if (command === 'SET_AUTO' || command === 'SET_MANUAL' || command === 'RESET_FAULT' || command.startsWith('CALIBRATE_')) {
      if (command === 'SET_AUTO') status.mode = 'AUTO';
      if (command === 'SET_MANUAL') status.mode = 'MANUAL';
      if (command === 'RESET_FAULT') { status.mode = 'MANUAL'; status.fault = 'NONE'; status.stopLatched = true; }
      if (command === 'CALIBRATE_DEPLOYED') { status.shieldState = 'DEPLOYED'; status.estimatedPositionPct = 100; status.positionConfidence = 'USER_CALIBRATED'; }
      if (command === 'CALIBRATE_RETRACTED') { status.shieldState = 'RETRACTED'; status.estimatedPositionPct = 0; status.positionConfidence = 'USER_CALIBRATED'; }
      status.commandPhase = 'COMPLETED';
      return json(response, 200, { received: true, accepted: true, requestId, command, result: 'ACCEPTED', phase: 'COMPLETED' });
    }
    if (status.mode !== 'MANUAL') {
      status.lastCommandResult = 'REJECTED_MODE'; status.commandPhase = 'REJECTED';
      return json(response, 409, { received: true, accepted: false, requestId, command, result: 'REJECTED_MODE', phase: 'REJECTED' });
    }
    status.commandPhase = 'STARTED';
    status.motionState = command === 'DEPLOY' ? 'DEPLOYING' : 'RETRACTING';
    const targetRequestId = requestId;
    setTimeout(() => {
      if (status.lastRequestId !== targetRequestId || status.commandPhase === 'STOPPED') return;
      status.motionState = 'STOPPED';
      status.shieldState = command === 'DEPLOY' ? 'DEPLOYED' : 'RETRACTED';
      status.estimatedPositionPct = command === 'DEPLOY' ? 100 : 0;
      status.positionConfidence = 'ESTIMATED';
      status.commandPhase = 'COMPLETED';
    }, 250);
    return json(response, 200, { received: true, accepted: true, requestId, command, result: 'ACCEPTED', phase: 'STARTED' });
  }

  return server;
}

function json(response, status, payload) {
  response.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
  response.end(JSON.stringify(payload));
}

function readJson(request) {
  return new Promise((resolve, reject) => {
    let raw = '';
    request.on('data', (chunk) => { raw += chunk; if (raw.length > 10_000) reject(new Error('Too large')); });
    request.on('end', () => { try { resolve(JSON.parse(raw)); } catch (error) { reject(error); } });
    request.on('error', reject);
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const port = Number(process.env.GREENGUARD_PREVIEW_PORT || 4173);
  createMockServer().listen(port, '127.0.0.1', () => console.log(`GreenGuard preview: http://127.0.0.1:${port}`));
}
