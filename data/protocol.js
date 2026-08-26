export const TERMINAL_PHASES = new Set(['COMPLETED', 'STOPPED', 'REJECTED', 'FAULT']);

export function makeRequestId(cryptoProvider = globalThis.crypto) {
  if (cryptoProvider?.getRandomValues) {
    const value = new Uint32Array(1);
    cryptoProvider.getRandomValues(value);
    return value[0] || 1;
  }
  const fallback = (Date.now() ^ Math.floor(Math.random() * 0xffffffff)) >>> 0;
  return fallback || 1;
}

export function classifyAcknowledgement(pending, status) {
  if (!pending) return { state: 'idle', terminal: false };
  if (!status || Number(status.lastRequestId) !== Number(pending.requestId)) {
    return { state: 'waiting', terminal: false, reason: 'mismatched-request' };
  }
  const phase = String(status.commandPhase || 'NONE');
  const result = String(status.lastCommandResult || 'IDLE');
  if (phase === 'REJECTED') return { state: 'rejected', terminal: true, phase, result };
  if (phase === 'FAULT') return { state: 'fault', terminal: true, phase, result };
  if (phase === 'STOPPED') return { state: 'stopped', terminal: true, phase, result };
  if (phase === 'COMPLETED') return { state: 'completed', terminal: true, phase, result };
  if (phase === 'STARTED') return { state: 'started', terminal: false, phase, result };
  return { state: 'accepted', terminal: false, phase, result };
}

export function isStatusStale(lastSuccessMs, nowMs = Date.now(), thresholdMs = 5000) {
  return !lastSuccessMs || nowMs - lastSuccessMs > thresholdMs;
}

export function formatDuration(milliseconds) {
  const totalSeconds = Math.max(0, Math.floor(Number(milliseconds || 0) / 1000));
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return hours ? `${hours}g ${minutes}p` : `${minutes}p ${seconds}s`;
}

export function isValidStatus(status) {
  return Boolean(status && status.protocolVersion === 2 && status.platformioBoard === 'nodemcuv2' &&
    typeof status.mode === 'string' && typeof status.motionState === 'string' && typeof status.fault === 'string');
}
