import { classifyAcknowledgement, formatDuration, isStatusStale, isValidStatus, makeRequestId } from './protocol.js';

const byId = (id) => document.getElementById(id);
const commandButtons = [...document.querySelectorAll('[data-command]')];
const state = { status: null, pending: null, lastSuccessMs: 0, polling: false };

const labels = {
  mode: { AUTO: 'Chế độ tự động', MANUAL: 'Chế độ thủ công' },
  rain: { UNKNOWN: 'Chưa xác định', DRY: 'Đang khô', WET: 'Đang mưa' },
  shield: { UNKNOWN: 'Không rõ vị trí', RETRACTED: 'Mái che đã thu', DEPLOYED: 'Cây đang được che', STOPPED_PARTIAL: 'Mái che đang ở giữa' },
  confidence: { UNKNOWN: 'Chưa xác định', ESTIMATED: 'Ước lượng theo thời gian', USER_CALIBRATED: 'Đã đánh dấu thủ công', LIMIT_CONFIRMED: 'Công tắc hành trình xác nhận' },
  motion: { STOPPED: 'Đã dừng', DEPLOYING: 'Đang che cây', RETRACTING: 'Đang thu mái che', REVERSAL_DEAD_TIME: 'Đang chờ đảo chiều' },
};

function token() { return sessionStorage.getItem('greenguardControlToken') || ''; }

function setFeedback(kind, text) {
  const target = byId('commandFeedback');
  target.className = `feedback ${kind}`;
  target.textContent = text;
}

function setConnection(connected, text) {
  byId('connectionPill').className = `pill ${connected ? 'online' : 'offline'}`;
  byId('connectionText').textContent = text;
}

function renderPending(status) {
  const acknowledgement = classifyAcknowledgement(state.pending, status);
  if (!state.pending) return;
  if (acknowledgement.state === 'waiting') {
    setFeedback('pending', `Đã gửi ${state.pending.command}; đang chờ đúng request #${state.pending.requestId}.`);
  } else if (acknowledgement.state === 'accepted') {
    setFeedback('pending', `Thiết bị đã nhận request #${state.pending.requestId}; chưa có xác nhận chuyển động.`);
  } else if (acknowledgement.state === 'started') {
    setFeedback('pending', `Thiết bị xác nhận đã bắt đầu ${state.pending.command}.`);
  } else if (acknowledgement.state === 'completed') {
    setFeedback('success', `Request #${state.pending.requestId} đã hoàn tất trên thiết bị.`);
    state.pending = null;
  } else if (acknowledgement.state === 'stopped') {
    setFeedback('success', `Thiết bị xác nhận motor đã dừng cho request #${state.pending.requestId}.`);
    state.pending = null;
  } else if (acknowledgement.state === 'fault') {
    setFeedback('error', `Request #${state.pending.requestId} kết thúc bằng lỗi ${status.fault}.`);
    state.pending = null;
  } else if (acknowledgement.state === 'rejected') {
    setFeedback('error', `Thiết bị từ chối request #${state.pending.requestId}: ${acknowledgement.result}.`);
    state.pending = null;
  }
}

function updateControls(status) {
  const offline = isStatusStale(state.lastSuccessMs);
  const pending = Boolean(state.pending);
  for (const button of commandButtons) {
    const command = button.dataset.command;
    let disabled = offline || (pending && command !== 'STOP');
    if (command === 'SET_AUTO') disabled ||= status.mode === 'AUTO';
    if (command === 'SET_MANUAL') disabled ||= status.mode === 'MANUAL';
    if (command === 'DEPLOY' || command === 'RETRACT') disabled ||= status.mode !== 'MANUAL' || status.fault !== 'NONE';
    if (command === 'RETRACT') disabled ||= status.rainState !== 'DRY';
    if (command.startsWith('CALIBRATE_')) disabled ||= status.mode !== 'MANUAL' || status.motionState !== 'STOPPED' || status.fault !== 'NONE';
    if (command === 'RESET_FAULT') disabled ||= status.fault === 'NONE';
    button.disabled = disabled;
    button.setAttribute('aria-disabled', String(disabled));
  }
}

function render(status) {
  state.status = status;
  setConnection(true, `Đã kết nối · ${status.ipAddress || 'ESP8266'}`);
  byId('safetyBanner').hidden = !status.actuatorDryRun;
  byId('authBanner').hidden = status.controlAuthConfigured;
  byId('faultBanner').hidden = status.fault === 'NONE';
  byId('faultTitle').textContent = `GreenGuard đã dừng: ${status.fault}`;
  byId('shieldLabel').textContent = labels.shield[status.shieldState] || status.shieldState;
  byId('shieldDetail').textContent = `${labels.motion[status.motionState] || status.motionState} · ${labels.confidence[status.positionConfidence] || status.positionConfidence}`;
  byId('positionValue').textContent = status.estimateAvailable ? Number(status.estimatedPositionPct).toFixed(0) : '—';
  byId('positionBar').style.width = status.estimateAvailable ? `${Math.max(0, Math.min(100, Number(status.estimatedPositionPct)))}%` : '0%';
  byId('confidenceLabel').textContent = labels.confidence[status.positionConfidence] || status.positionConfidence;
  byId('rainLabel').textContent = labels.rain[status.rainState] || status.rainState;
  byId('rainStable').textContent = status.rainState;
  byId('rainDigital').textContent = `${status.rainDigitalLevel} · raw ${status.rainRawWet ? 'wet' : 'dry'}`;
  byId('rainEvents').textContent = status.rainEventCount;
  byId('rainIcon').textContent = status.rainState === 'WET' ? '☂' : status.rainState === 'DRY' ? '☀' : '◌';
  byId('modeLabel').textContent = labels.mode[status.mode] || status.mode;
  byId('motionBadge').textContent = labels.motion[status.motionState] || status.motionState;
  byId('motionBadge').className = `state-badge ${status.motionState !== 'STOPPED' ? 'moving' : ''}`;
  byId('motorState').textContent = status.motionState;
  byId('motorTiming').textContent = status.motionState === 'STOPPED' ? 'Không cấp lệnh chạy' : `${Math.round(status.movementElapsedMs / 100) / 10}s / ${Math.round(status.movementDurationMs / 100) / 10}s`;
  byId('wifiState').textContent = status.wifiConnected ? `${status.wifiRssi} dBm` : 'Mất kết nối';
  byId('ipAddress').textContent = status.ipAddress || 'Tự động vẫn chạy offline';
  byId('faultCode').textContent = status.fault;
  byId('stopLatch').textContent = status.stopLatched ? 'STOP đang khóa' : 'Không khóa STOP';
  byId('boardId').textContent = `${status.confirmedController} · ${status.platformioBoard}`;
  byId('uptime').textContent = formatDuration(status.uptimeMs);
  byId('lastUpdate').textContent = `Cập nhật lúc ${new Date().toLocaleTimeString('vi-VN')}`;
  byId('freeHeap').textContent = `${Math.round(status.freeHeapBytes / 1024)} KiB`;
  renderPending(status);
  updateControls(status);
}

async function fetchJson(url, options = {}, timeoutMs = 3500) {
  const abort = new AbortController();
  const timer = setTimeout(() => abort.abort(), timeoutMs);
  try {
    const response = await fetch(url, { cache: 'no-store', ...options, signal: abort.signal });
    let payload = {};
    try { payload = await response.json(); } catch { /* handled by status below */ }
    if (!response.ok) {
      const error = new Error(payload.message || `HTTP ${response.status}`);
      error.status = response.status;
      error.payload = payload;
      throw error;
    }
    return payload;
  } finally {
    clearTimeout(timer);
  }
}

async function pollStatus() {
  if (state.polling) return;
  state.polling = true;
  try {
    const status = await fetchJson('/api/status');
    if (!isValidStatus(status)) throw new Error('Phản hồi trạng thái không đúng protocol v2/nodemcuv2.');
    state.lastSuccessMs = Date.now();
    render(status);
  } catch (error) {
    if (isStatusStale(state.lastSuccessMs)) {
      setConnection(false, 'Mất kết nối với thiết bị');
      if (state.status) updateControls(state.status);
    }
  } finally {
    state.polling = false;
  }
}

async function sendCommand(command) {
  if (['CALIBRATE_DEPLOYED', 'CALIBRATE_RETRACTED'].includes(command)) {
    const meaning = command.endsWith('DEPLOYED') ? 'đang che cây hoàn toàn' : 'đã thu hoàn toàn';
    if (!confirm(`Chỉ tiếp tục nếu bạn đã nhìn trực tiếp và chắc chắn mái che ${meaning}.`)) return;
  }
  const requestId = makeRequestId();
  if (command === 'STOP') state.pending = null;
  setFeedback('pending', `Đang gửi ${command} với request #${requestId}…`);
  try {
    const response = await fetchJson('/api/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-GreenGuard-Token': token() },
      body: JSON.stringify({ protocolVersion: 2, requestId, command }),
    });
    if (response.duplicate) {
      setFeedback('pending', `Request #${requestId} đã được thiết bị thấy trước đó; không chạy lặp.`);
      return;
    }
    if (!response.accepted) {
      setFeedback('error', `Thiết bị từ chối lệnh: ${response.result || 'UNKNOWN'}.`);
      return;
    }
    state.pending = { requestId, command, sentAt: Date.now() };
    setFeedback('pending', `Thiết bị đã nhận request #${requestId}; phase ${response.phase}.`);
    await pollStatus();
  } catch (error) {
    setFeedback('error', error.status === 401 ? 'Token điều khiển chưa đúng.' : `Không gửi được lệnh: ${error.message}`);
  } finally {
    if (state.status) updateControls(state.status);
  }
}

async function loadConfig() {
  try {
    const config = await fetchJson('/api/config');
    const form = byId('configForm');
    for (const [key, value] of Object.entries(config)) {
      const input = form.elements[key];
      if (!input) continue;
      if (input.type === 'checkbox') input.checked = Boolean(value);
      else input.value = value;
    }
  } catch (error) {
    setFeedback('error', `Không đọc được cấu hình: ${error.message}`);
  }
}

commandButtons.forEach((button) => button.addEventListener('click', () => sendCommand(button.dataset.command)));

byId('tokenForm').addEventListener('submit', (event) => {
  event.preventDefault();
  sessionStorage.setItem('greenguardControlToken', byId('controlToken').value.trim());
  setFeedback('success', 'Đã lưu token cho phiên trình duyệt này.');
});

byId('configForm').addEventListener('submit', async (event) => {
  event.preventDefault();
  const payload = {};
  for (const input of event.currentTarget.elements) {
    if (!input.name) continue;
    payload[input.name] = input.type === 'checkbox' ? input.checked : Number(input.value);
  }
  try {
    const response = await fetchJson('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-GreenGuard-Token': token() },
      body: JSON.stringify(payload),
    });
    setFeedback('success', response.message || 'Đã lưu cấu hình.');
  } catch (error) {
    setFeedback('error', `Không lưu được cấu hình: ${error.message}`);
  }
});

byId('controlToken').value = token();
loadConfig();
pollStatus();
setInterval(pollStatus, 1500);
setInterval(() => {
  if (state.pending && Date.now() - state.pending.sentAt > 90000) {
    setFeedback('error', `Request #${state.pending.requestId} chưa có kết quả sau 90 giây; không xem đây là thành công.`);
    state.pending = null;
  }
  if (state.status) updateControls(state.status);
}, 1000);
