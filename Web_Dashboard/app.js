/* ═══════════════════════════════════════════════
   Rail-Bot Monitor — app.js
   MQTT + Chart.js + Leaflet + Control Logic
   ═══════════════════════════════════════════════ */

'use strict';

// ─── STATE ───────────────────────────────────────
const state = {
  mqtt: null,
  connected: false,
  mockTimer: null,
  mockActive: false,
  packetCount: 0,
  defectCount: 0,
  historyRows: [],
  uptimeSeconds: 0,
  injectDefect: false,

  topics: {
    telemetry: APP_CONFIG.MQTT.TOPIC_TELEMETRY,
    cmd: APP_CONFIG.MQTT.TOPIC_COMMAND,
  },

  thresh: {
    tof: APP_CONFIG.THRESHOLDS.TOF,
    imu: APP_CONFIG.THRESHOLDS.IMU,
    batLow: APP_CONFIG.THRESHOLDS.BAT_LOW
  },

  data: {
    tof1: null, tof2: null, imu: null,
    lat: null, lon: null, spd: null, sat: null,
    rssi: null, bat: null, voltage: null,
    loopHz: null, rpm: null,
  },

  chartLen: 80,
  chartData: { s1: [], s2: [], s3: [], s4: [] },

  map: null,
  mapMarker: null,
  mapTrail: null,
  mapTrailCoords: [],
};

// ─── INIT ────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  // TỰ ĐỘNG ĐIỀN CẤU HÌNH TỪ FILE CONFIG.JS
  document.getElementById('cfg-broker').value = APP_CONFIG.MQTT.BROKER_URL;
  document.getElementById('cfg-user').value = APP_CONFIG.MQTT.USERNAME;
  document.getElementById('cfg-pass').value = APP_CONFIG.MQTT.PASSWORD;
  
  // Tự sinh ClientID để mở nhiều tab web không bị đá nhau
  document.getElementById('cfg-clientid').value = 'Web_UI_' + Math.random().toString(16).substring(2, 8);

  initClock();
  initUptimeCounter();
  initChart();
  initMap();
  addLog('sys', 'Hệ thống khởi động. Sẵn sàng kết nối MQTT.');
});

// ─── CLOCK ───────────────────────────────────────
function initClock() {
  const tick = () => {
    const now = new Date();
    document.getElementById('sys-clock').textContent = now.toTimeString().slice(0, 8);
  };
  tick();
  setInterval(tick, 1000);
}

function initUptimeCounter() {
  setInterval(() => {
    state.uptimeSeconds++;
    const h = String(Math.floor(state.uptimeSeconds / 3600)).padStart(2, '0');
    const m = String(Math.floor((state.uptimeSeconds % 3600) / 60)).padStart(2, '0');
    const s = String(state.uptimeSeconds % 60).padStart(2, '0');
    const el = document.getElementById('uptime-display');
    if (el) el.textContent = `${h}:${m}:${s}`;
  }, 1000);
}

// ─── CHART ───────────────────────────────────────
let tofChart = null;

// Đổi lại object lưu data ở đầu file (xóa s4 đi vì đã gộp ToF)
state.chartData = { s1: [], s2: [], s3: [] };

function initChart() {
  const ctx = document.getElementById('tofChart').getContext('2d');

  for (let i = 0; i < state.chartLen; i++) {
    state.chartData.s1.push(null); // ToF Tổng
    state.chartData.s2.push(null); // Roll
    state.chartData.s3.push(null); // Pitch
  }

  tofChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: Array.from({ length: state.chartLen }, (_, i) => i),
      datasets: [
        {
          label: 'Khẩu độ', data: [...state.chartData.s1],
          borderColor: '#4f9fff', backgroundColor: 'rgba(79,159,255,0.1)',
          borderWidth: 2, pointRadius: 0, tension: 0.3, fill: true, yAxisID: 'y'
        },
        {
          label: 'Roll', data: [...state.chartData.s2],
          borderColor: '#c670ff',
          borderWidth: 1.5, pointRadius: 0, tension: 0.3, fill: false, yAxisID: 'y1'
        },
        {
          label: 'Pitch', data: [...state.chartData.s3],
          borderColor: '#ffb84d',
          borderWidth: 1.5, pointRadius: 0, tension: 0.3, fill: false, yAxisID: 'y1'
        },
        {
          label: 'Ngưỡng ToF', data: Array(state.chartLen).fill(state.thresh.tof),
          borderColor: 'rgba(255,85,85,0.6)', borderDash: [6, 4],
          borderWidth: 1.5, pointRadius: 0, fill: false, yAxisID: 'y'
        }
      ]
    },
    options: {
      responsive: true, maintainAspectRatio: false, animation: { duration: 0 },
      interaction: { mode: 'index', intersect: false },
      scales: {
        x: { display: false },
        y: {
          type: 'linear', display: true, position: 'left', min: 0, max: 2000,
          grid: { color: 'rgba(79,159,255,0.15)', lineWidth: 1 },
          ticks: { color: '#7cbfff', font: { family: "'DM Mono'", size: 10 }, callback: v => v + ' mm' },
        },
        y1: {
          type: 'linear', display: true, position: 'right', min: -40, max: 40,
          grid: { drawOnChartArea: false },
          ticks: { color: '#ff7fff', font: { family: "'DM Mono'", size: 10 }, callback: v => v + ' °' },
        }
      },
      plugins: { legend: { display: false } }
    }
  });
}

function pushChartData(tof, roll, pitch) {
  state.chartData.s1.push(tof);
  state.chartData.s2.push(roll);
  state.chartData.s3.push(pitch);

  if (state.chartData.s1.length > state.chartLen) {
    state.chartData.s1.shift(); state.chartData.s2.shift(); state.chartData.s3.shift();
  }

  const alarm = tof > state.thresh.tof;
  tofChart.data.datasets[0].data = [...state.chartData.s1];
  tofChart.data.datasets[1].data = [...state.chartData.s2];
  tofChart.data.datasets[2].data = [...state.chartData.s3];
  tofChart.data.datasets[3].data = Array(state.chartLen).fill(state.thresh.tof);

  tofChart.data.datasets[0].borderColor = alarm ? '#ff5555' : '#4f9fff';
  tofChart.update('none');
}

// ─── MAP ─────────────────────────────────────────
function initMap() {
  const map = L.map('map', { zoomControl: false }).setView([21.028, 105.834], 14);

  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '© OpenStreetMap',
    maxZoom: 19,
  }).addTo(map);

  L.control.zoom({ position: 'topright' }).addTo(map);

  const robotIcon = L.divIcon({
    html: `<div style="width:12px;height:12px;border-radius:50%;background:#3b82f6;border:2px solid #fff;box-shadow:0 0 0 3px rgba(59,130,246,0.3);"></div>`,
    className: '',
    iconSize: [12, 12],
    iconAnchor: [6, 6],
  });

  state.mapMarker = L.marker([21.028, 105.834], { icon: robotIcon }).addTo(map);
  state.mapTrail = L.polyline([], { color: '#3b82f6', weight: 2.5, opacity: 0.8 }).addTo(map);
  state.map = map;
}

function updateMapPosition(lat, lon) {
  if (!state.map) return;
  const ll = [lat, lon];
  state.mapMarker.setLatLng(ll);
  state.mapTrailCoords.push(ll);
  if (state.mapTrailCoords.length > 500) state.mapTrailCoords.shift();
  state.mapTrail.setLatLngs(state.mapTrailCoords);
  state.map.panTo(ll);
}

function addDefectMarker(lat, lon, tof1, tof2) {
  if (!state.map) return;
  const icon = L.divIcon({
    html: `<div style="width:10px;height:10px;border-radius:50%;background:#ef4444;border:2px solid #fff;box-shadow:0 0 0 3px rgba(239,68,68,0.3);"></div>`,
    className: '',
    iconSize: [10, 10],
    iconAnchor: [5, 5],
  });
  L.marker([lat, lon], { icon })
    .bindPopup(`<b>Khuyết tật phát hiện</b><br>ToF Trái: ${tof1} mm<br>ToF Phải: ${tof2} mm`)
    .addTo(state.map);
}

// ─── MQTT KẾT NỐI ─────────────────────────────────
function connectMQTT() {
  const url = document.getElementById('cfg-broker').value.trim();
  const clientId = document.getElementById('cfg-clientid').value.trim();
  const username = document.getElementById('cfg-user').value.trim();
  const password = document.getElementById('cfg-pass').value.trim();

  if (state.mqtt) state.mqtt.end(true);

  const opts = { clientId, clean: true, reconnectPeriod: 4000 };
  if (username) { opts.username = username; opts.password = password; }

  addLog('sys', `Đang kết nối tới: ${url}`);

  try { state.mqtt = mqtt.connect(url, opts); }
  catch (e) { showToast('Lỗi kết nối: ' + e.message, 'err'); return; }

  state.mqtt.on('connect', () => {
    state.connected = true;
    setConnStatus(true);
    const short = url.replace('ws://', '').replace('wss://', '').split('/')[0];
    document.getElementById('broker-short').textContent = short.slice(0, 20);

    // CHỈ SUBSCRIBE ĐÚNG TOPIC TELEMETRY CỦA XE
    state.mqtt.subscribe(state.topics.telemetry, { qos: 0 }, err => {
      if (err) addLog('err', 'Lỗi subscribe: ' + err.message);
      else addLog('sys', `Đã đăng ký: ${state.topics.telemetry}`);
    });

    showToast('Kết nối MQTT thành công', 'ok');
  });

  state.mqtt.on('message', (topic, payload) => handleMessage(topic, payload.toString()));

  state.mqtt.on('error', err => {
    addLog('err', 'MQTT Error: ' + err.message);
    showToast('Lỗi MQTT: ' + err.message, 'err');
  });

  state.mqtt.on('offline', () => {
    state.connected = false;
    setConnStatus(false);
    addLog('sys', 'Mất kết nối — đang thử lại...');
  });

  state.mqtt.on('reconnect', () => addLog('sys', 'Đang kết nối lại...'));
}

function disconnectMQTT() {
  if (state.mqtt) { state.mqtt.end(true); state.mqtt = null; }
  state.connected = false;
  setConnStatus(false);
  showToast('Đã ngắt kết nối', 'warn');
  addLog('sys', 'Ngắt kết nối MQTT.');
}

function setConnStatus(online) {
  const dot = document.getElementById('conn-dot');
  const label = document.getElementById('conn-label');
  dot.classList.toggle('online', online);
  label.textContent = online ? 'Đã kết nối' : 'Chưa kết nối';
}

// ─── PARSE DỮ LIỆU TỪ STM32 ───────────────────────

function handleMessage(topic, raw) {
  state.packetCount++;
  document.getElementById('packet-count').textContent = state.packetCount;
  addLog('rx', `[${topic}] ${raw.slice(0, 100)}`);

  if (topic === state.topics.telemetry) {
    if (raw.includes("ECHO:")) {
      document.getElementById('cmd-echo').textContent = raw.replace("ECHO:", "").trim();
      return;
    }

    try {
      // 1. Dọn dẹp chuỗi: Xóa dấu ! và tách các thành phần bằng khoảng trắng
      const cleanRaw = raw.replace('!', '').trim();
      const parts = cleanRaw.split(/\s+/);

      // 2. Khởi tạo biến mặc định
      let distL = 0, distR = 0, rpm = 0, roll = 0, pitch = 0, batV = 0;
      let piStatus = "NONE", lat = null, lon = null;

      // 3. Quét mảng linh hoạt (Lưu ý: PI: phải đứng trước P)
      parts.forEach(p => {
        // Dùng Regex để trích xuất từng thông số một cách chính xác nhất
        const matchL = raw.match(/L([0-9.-]+)/);
        if (matchL) distL = parseFloat(matchL[1]);

        const matchR = raw.match(/R([0-9.-]+)/);
        if (matchR) distR = parseFloat(matchR[1]);

        const matchE = raw.match(/E([0-9.-]+)/);
        if (matchE) rpm = parseFloat(matchE[1]);

        // Bắt MPU6050: Pitch và Roll
        const matchP = raw.match(/P([0-9.-]+)/);
        if (matchP) pitch = parseFloat(matchP[1]);

        const matchRoll = raw.match(/Ro([0-9.-]+)/); // Thêm dấu cách để phân biệt với Laser R
        if (matchRoll) roll = parseFloat(matchRoll[1]);

        // Bắt Pin V
        const matchV = raw.match(/V([0-9.-]+)/);
        if (matchV) batV = parseFloat(matchV[1]);

        // BẮT CHUỖI PI (Bao gồm cả chữ cái và phần trăm nếu có)
        // Ví dụ: Bắt "NORMAL 95.9%" từ "PI:NORMAL 95.9% debug0,0"
        const matchPI = raw.match(/PI:([A-Za-z_]+\s?[0-9.]*%?)/);
        if (matchPI) piStatus = matchPI[1].trim();

        // Bắt GPS
        const matchGPS = raw.match(/GPS:([0-9.-]+),([0-9.-]+)/);
        if (matchGPS) {
          // lat = parseFloat(matchGPS[1]);
          // lon = parseFloat(matchGPS[2]);21.028579711623063, 105.80285017107042
          lat = 21.028579711623063;
          lon = 105.80285017107042;
        }
      });

      console.log('Parsed lat:', lat, 'lon:', lon);

      // 4. Bảo vệ Web không sập nếu có cảm biến nào đó bị lỗi gửi về NaN
      if (isNaN(pitch)) pitch = 0;
      if (isNaN(roll)) roll = 0;
      if (isNaN(distL)) distL = 0;
      if (isNaN(distR)) distR = 0;

      // 5. Tính toán Khẩu độ Ray (Track Gauge) = Trái + Phải + 150mm
      const trackGauge = distL + distR + 200; // 150mm là khoảng cách từ cảm biến đến tâm ray

      // 6. Tính % Pin
      let batPct = (batV / 12.6) * 100;
      batPct = Math.max(0, Math.min(100, Math.round(batPct)));

      // 7. Đẩy dữ liệu lên các ô Trạng thái & Biểu đồ
      processSensorData({
        tof: trackGauge,
        roll: roll,
        pitch: pitch,
        rpm: rpm,
        pi: piStatus
      });

      processStatusData({
        bat_v: batV,
        bat_pct: batPct,
        rssi: -50 // Giả lập mức sóng
      });

      // 8. Đẩy dữ liệu lên Bản đồ (Chỉ thực hiện nếu bắt được tọa độ GPS)
      console.log('About to check GPS condition: lat=' + lat + ', lon=' + lon + ', isNaN(lat)=' + isNaN(lat) + ', isNaN(lon)=' + isNaN(lon));
      if (lat !== null && lon !== null && !isNaN(lat) && !isNaN(lon)) {
        processGpsData({
          lat: lat,
          lon: lon,
          speed: Math.abs(rpm) / 100, // Ước lượng tốc độ km/h từ RPM
          sats: 8,
          fix: 1
        });
      }

    } catch (e) {
      console.error("Lỗi parse chuỗi từ Robot:", e);
    }
  }
}

function processSensorData(d) {
  const tof = d.tof ?? null;
  const roll = d.roll ?? null;
  const pitch = d.pitch ?? null;
  const rpm = d.rpm ?? null;
  const pi = d.pi ?? "NORMAL";

  // Cập nhật Cảm biến Laser (Khẩu độ)
  if (tof !== null) {
    document.getElementById('tof-val').textContent = tof.toFixed(1);
    setSensorStatus('dot-tof', 'sc-tof', tof > state.thresh.tof ? 'err' : 'ok', tof > state.thresh.tof ? 'CẢNH BÁO' : 'OK');
  }

  // Cập nhật Raspberry Pi
  document.getElementById('pi-val').textContent = pi;
  // DÒNG MỚI:
  const piErr = (!pi.includes("NORMAL") && !pi.includes("NONE"));
  setSensorStatus('dot-pi', 'sc-pi', piErr ? 'err' : 'ok', piErr ? 'PHÁT HIỆN LỖI' : 'OK');

  if (tof !== null && roll !== null && pitch !== null) {
    pushChartData(tof, roll, pitch);

    const alarmToF = tof > state.thresh.tof; // Cảnh báo vượt khẩu độ
    const alarm = alarmToF || piErr;
    document.getElementById('alarm-lamp').classList.toggle('active', alarm);

    let reason = [];
    if (alarmToF) reason.push("Lệch khẩu độ (ToF)");
    if (piErr) reason.push(`Camera (${pi})`);

    if (alarm) {
      state.defectCount++;
      document.getElementById('defect-count').textContent = state.defectCount;
      if (state.data.lat !== null) addDefectMarker(state.data.lat, state.data.lon, tof.toFixed(1), "");
      addHistoryRow({ time: new Date().toLocaleString('vi-VN'), lat: state.data.lat, lon: state.data.lon, tof, roll, pitch, pi, defect: true, reason: reason.join(" + ") });
    } else {
      addHistoryRow({ time: new Date().toLocaleString('vi-VN'), lat: state.data.lat, lon: state.data.lon, tof, roll, pitch, pi, defect: false, reason: "Bình thường" });
    }
  }

  if (roll !== null) {
    document.getElementById('imu-val').textContent = roll.toFixed(1);
    setSensorStatus('dot-imu', 'sc-imu', Math.abs(roll) > state.thresh.imu ? 'warn' : 'ok', Math.abs(roll) > state.thresh.imu ? 'NGHIÊNG' : 'OK');
  }
  if (pitch !== null) {
    document.getElementById('pitch-val').textContent = pitch.toFixed(1);
    setSensorStatus('dot-pitch', 'sc-pitch', Math.abs(pitch) > state.thresh.imu ? 'warn' : 'ok', Math.abs(pitch) > state.thresh.imu ? 'NGHIÊNG' : 'OK');
  }
  if (rpm !== null) {
    document.getElementById('rpm-val').textContent = Math.abs(rpm).toFixed(1);
    setSensorStatus('dot-rpm', 'sc-rpm', 'ok', 'OK');
  }
}

// ─── CẬP NHẬT BẢN ĐỒ VÀ THÔNG SỐ GPS ───────────────────────
function processGpsData(d) {
  console.log('processGpsData called with:', d);
  const lat = d.lat ?? null;
  const lon = d.lon ?? null;
  const spd = d.speed ?? null;
  const sat = d.sats ?? null;
  const fix = d.fix ?? 1;

  if (lat !== null && lon !== null) {
    console.log('Setting GPS lat/lon:', lat, lon);
    state.data.lat = lat;
    state.data.lon = lon;

    // Ghi tọa độ lên giao diện
    const latEl = document.getElementById('gps-lat');
    const lonEl = document.getElementById('gps-lon');
    if (!latEl) console.log('gps-lat element not found');
    if (!lonEl) console.log('gps-lon element not found');
    if (latEl) latEl.textContent = lat.toFixed(6);
    if (lonEl) lonEl.textContent = lon.toFixed(6);
    console.log('GPS elements updated to:', latEl ? latEl.textContent : 'N/A', lonEl ? lonEl.textContent : 'N/A');

    // Vẽ lên bản đồ Leaflet
    updateMapPosition(lat, lon);
    setSensorStatus('dot-gps', 'sc-gps', 'ok', 'OK');
  }

  const spdEl = document.getElementById('gps-spd');
  if (spd !== null && spdEl) spdEl.innerHTML = spd.toFixed(1) + '<small> km/h</small>';

  const satEl = document.getElementById('gps-sat');
  if (sat !== null && satEl) satEl.textContent = sat;

  const badge = document.getElementById('gps-fix-badge');
  if (badge) {
    badge.textContent = fix ? 'GPS: Đã bắt được' : 'GPS: Chưa có tín hiệu';
    badge.classList.toggle('ok', !!fix);
  }
}

function processStatusData(d) {
  if (d.bat_pct !== undefined) {
    // Tìm thẻ HTML
    const batPctEl = document.getElementById('bat-pct');
    const batBarEl = document.getElementById('bat-bar');

    // CHỈ cập nhật dữ liệu NẾU thẻ HTML đó có tồn tại trên giao diện
    if (batPctEl) {
      batPctEl.textContent = d.bat_pct.toFixed(0) + '%';
    }
    if (batBarEl) {
      batBarEl.style.width = d.bat_pct + '%';
    }
  }

  if (d.rssi !== undefined) {
    state.data.rssi = d.rssi;
    const rssiEl = document.getElementById('lora-rssi');
    if (rssiEl) {
      rssiEl.textContent = `${d.rssi.toFixed(0)} dBm`;
    }
    updateSignalBars(d.rssi);
  }
}

function setSensorStatus(dotId, tileId, status, label) {
  const dot = document.getElementById(dotId);
  const tile = document.getElementById(tileId);
  if (dot) { dot.className = 'st-status ' + status; dot.textContent = label; }
  if (tile) tile.classList.toggle('alert', status === 'err');
}

// ─── ĐIỀU KHIỂN RAIL-BOT (CHỈ TIẾN/LÙI) ───────────
function sendCmd(cmd) {
  if (!state.connected && !state.mockActive) {
    showToast('Chưa kết nối MQTT', 'warn');
    return;
  }

  const speedPct = parseInt(document.getElementById('speed-slider').value);
  const MAX_RPM = 250; // Giới hạn RPM tối đa
  let targetSpeed = Math.floor((speedPct / 100) * MAX_RPM);

  let speedL = 0, speedR = 0;

  // Tránh chuyển hướng rẽ (left/right) vì xe chạy trên ray
  if (cmd === 'fwd') { speedL = -targetSpeed; speedR = -targetSpeed; }
  else if (cmd === 'bwd') { speedL = targetSpeed; speedR = targetSpeed; }
  else if (cmd === 'stop' || cmd === 'estop') { speedL = 0; speedR = 0; }
  else return; // Bỏ qua nếu bấm nút trái/phải trên UI cũ

  const payload = `${speedL},${speedR}`;
  document.getElementById('cmd-echo').textContent = cmd.toUpperCase();

  if (state.mqtt && state.connected) {
    state.mqtt.publish(state.topics.cmd, payload);
    addLog('tx', `[${state.topics.cmd}] Bắn lệnh: ${payload}`);
  }
}

function sendEstop() {
  document.getElementById('speed-slider').value = 0;
  updateSpeed(0);
  sendCmd('estop');
  showToast('Lệnh dừng khẩn cấp đã được gửi!', 'err');
}

function updateSpeed(val) {
  val = parseInt(val);
  document.getElementById('speed-val').textContent = val;
  const track = document.getElementById('speed-slider');
  const pct = val + '%';
  track.style.background = `linear-gradient(to right, var(--blue) ${pct}, var(--border-light) ${pct})`;
}

function setMode(mode, btn) {
  document.querySelectorAll('.mode-pill').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');
  if (mode === 'auto') showToast('Chế độ Tự động: Robot tự quét', 'ok');
}

// ─── CÁC HÀM TIỆN ÍCH (Giữ nguyên) ────────────────
function updateSignalBars(rssi) {
  const el = document.getElementById('signal-bars');
  el.className = 'sig-bars';
  if (rssi > -60) el.classList.add('lv5');
  else if (rssi > -70) el.classList.add('lv4');
  else if (rssi > -80) el.classList.add('lv3');
  else if (rssi > -90) el.classList.add('lv2');
  else el.classList.add('lv1');
}

function addHistoryRow(data) {
  state.historyRows.push(data);
  if (state.historyRows.length > 1000) state.historyRows.shift();
  renderHistory(state.historyRows);
}

// Render History
function renderHistory(rows) {
  const tbody = document.getElementById('history-tbody');
  const nDefect = rows.filter(r => r.defect).length;
  tbody.innerHTML = '';

  if (rows.length === 0) {
    tbody.innerHTML = '<tr><td colspan="8" class="empty-row">Không có dữ liệu</td></tr>';
    document.getElementById('history-summary').textContent = 'Tổng: 0 bản ghi · 0 khuyết tật';
    return;
  }

  const visible = rows.slice(-200).reverse();
  visible.forEach((r, i) => {
    const tr = document.createElement('tr');
    if (r.defect) tr.classList.add('row-defect');
    const gpsTxt = (r.lat !== null && r.lon !== null) ? `${r.lat.toFixed(5)}, ${r.lon.toFixed(5)}` : '—';

    tr.innerHTML = `
      <td>${rows.length - i}</td>
      <td>${r.time}</td>
      <td style="font-family:var(--mono)">${gpsTxt}</td>
      <td style="color:${r.tof > state.thresh.tof ? 'var(--red)' : 'inherit'};font-family:var(--mono);font-weight:600">${r.tof !== null ? r.tof.toFixed(1) : '—'}</td>
      <td style="font-family:var(--mono)">${r.roll !== null ? r.roll.toFixed(1) : '—'}</td>
      <td style="font-family:var(--mono)">${r.pitch !== null ? r.pitch.toFixed(1) : '—'}</td>
      <td style="font-family:var(--mono);color:${(!r.pi.includes('NORMAL') && !r.pi.includes('NONE')) ? 'var(--red)' : 'inherit'}">${r.pi}</td>
      <td>${r.defect ? `<span style="color:var(--red);font-weight:600">⚠ ${r.reason}</span>` : '<span style="color:var(--green)">✓</span>'}</td>
    `;
    tbody.appendChild(tr);
  });
  document.getElementById('history-summary').textContent = `Tổng: ${rows.length} bản ghi · ${nDefect} khuyết tật`;
}

function filterHistory() {
  const type = document.getElementById('hist-type').value;
  let rows = [...state.historyRows];
  if (type === 'defect') rows = rows.filter(r => r.defect);
  if (type === 'normal') rows = rows.filter(r => !r.defect);
  renderHistory(rows);
}

function clearHistory() {
  state.historyRows = [];
  state.defectCount = 0;
  document.getElementById('defect-count').textContent = '0';
  renderHistory([]);
  showToast('Đã xóa lịch sử quét', 'warn');
}

// ─── XUẤT DỮ LIỆU RA FILE EXCEL (CSV) ────────────────
function exportCSV() {
  // Kiểm tra xem có dữ liệu trong lịch sử không
  if (!state.historyRows || state.historyRows.length === 0) {
    showToast('Không có dữ liệu lịch sử để xuất!', 'warn');
    return;
  }

  // 1. Định nghĩa tiêu đề các cột (Header)
  // Lưu ý: Dùng dấu phẩy để ngăn cách các cột
  const header = [
    'STT',
    'Thời gian',
    'Vĩ độ (Lat)',
    'Kinh độ (Lon)',
    'Khẩu độ Ray (mm)',
    'Góc Roll (độ)',
    'Góc Pitch (độ)',
    'Trạng thái Pi (AI)',
    'Kết quả kiểm tra',
    'Chi tiết lỗi'
  ].join(',');

  // 2. Chuyển đổi từng dòng dữ liệu trong historyRows sang định dạng CSV
  const rows = state.historyRows.map((r, index) => {
    return [
      index + 1,
      `"${r.time}"`,                            // Bao dấu ngoặc kép để tránh lỗi định dạng ngày tháng
      r.lat !== null ? r.lat.toFixed(6) : '',
      r.lon !== null ? r.lon.toFixed(6) : '',
      r.tof !== null ? r.tof.toFixed(1) : '',    // Đây là (Trái + Phải + 150)
      r.roll !== null ? r.roll.toFixed(2) : '',
      r.pitch !== null ? r.pitch.toFixed(2) : '',
      `"${r.pi}"`,                              // Trạng thái từ Raspberry Pi
      r.defect ? 'NGHI VAN KHUYET TAT' : 'BINH THUONG',
      `"${r.reason || ''}"`                     // Chi tiết nguyên nhân (Vật cản hoặc Nứt ray)
    ].join(',');
  }).join('\n');

  // 3. Tạo file Blob và tải xuống
  const csvContent = '\uFEFF' + header + '\n' + rows; // Thêm BOM (\uFEFF) để hiển thị được tiếng Việt có dấu trong Excel
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);

  const link = document.createElement('a');
  const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);

  link.setAttribute('href', url);
  link.setAttribute('download', `nhat_ky_quet_ray_${timestamp}.csv`);
  link.style.visibility = 'hidden';

  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);

  showToast('Đã xuất file báo cáo CSV thành công!', 'ok');
  addLog('sys', `Đã xuất báo cáo: railbot_log_${timestamp}.csv`);
}

function saveThresholds() {
  state.thresh.tof = parseFloat(document.getElementById('cfg-threshold').value) || 400;
  state.thresh.imu = parseFloat(document.getElementById('cfg-imu-thresh').value) || 15;
  state.thresh.batLow = parseFloat(document.getElementById('cfg-bat-thresh').value) || 20;
  document.getElementById('thresh-display').textContent = state.thresh.tof;
  showToast(`Đã lưu ngưỡng!`, 'ok');
}

function saveTopics() {
  const t_tel = document.getElementById('cfg-topic-telemetry').value.trim();
  const t_cmd = document.getElementById('cfg-topic-cmd').value.trim();

  if (t_tel && t_cmd) {
    // Nếu đang kết nối thì Unsubscribe topic cũ, Subscribe topic mới
    if (state.mqtt && state.connected) {
      state.mqtt.unsubscribe(state.topics.telemetry);
      state.topics.telemetry = t_tel;
      state.topics.cmd = t_cmd;
      state.mqtt.subscribe(state.topics.telemetry, { qos: 0 });
    } else {
      state.topics.telemetry = t_tel;
      state.topics.cmd = t_cmd;
    }
    showToast('Đã lưu và áp dụng Topic MQTT mới!', 'ok');
    addLog('sys', `Topic đổi thành: RX=${t_tel}, TX=${t_cmd}`);
  }
}

function toggleMock(on) {
  state.mockActive = on;
  if (on) {
    const interval = parseInt(document.getElementById('mock-interval').value) || 600;
    state.mockTimer = setInterval(generateMock, interval);
    setConnStatus(true);
    showToast('Chế độ Demo: Bật', 'ok');
  } else {
    clearInterval(state.mockTimer);
    state.mockTimer = null;
    if (!state.connected) setConnStatus(false);
    showToast('Chế độ Demo: Tắt', 'warn');
  }
}

function injectDefect() {
  state.injectDefect = true;
  showToast('Giả lập khuyết tật...', 'warn');
  setTimeout(() => { state.injectDefect = false; }, 3000);
}

let _lat = 21.028011, _lon = 105.834003, _step = 0;
function generateMock() {
  _lat += 0.000015 + (Math.random() - 0.5) * 0.000004;
  _lon += 0.000010 + (Math.random() - 0.5) * 0.000004;
  _step++;

  const base1 = 32 + Math.sin(_step * 0.12) * 6 + (Math.random() - 0.5) * 3;
  const base2 = 33 + Math.cos(_step * 0.10) * 5 + (Math.random() - 0.5) * 3;

  const tof1 = state.injectDefect ? 72 + Math.random() * 30 : base1;
  const tof2 = state.injectDefect ? 68 + Math.random() * 30 : base2;
  const imu = Math.sin(_step * 0.06) * 7 + (Math.random() - 0.5) * 0.5;

  processSensorData({ tof1, tof2, imu, loopHz: 10.0, rpm: Math.floor(90 + Math.random() * 50) });
  processGpsData({ lat: _lat, lon: _lon, speed: 2.8 + Math.random() * 0.8, sats: Math.floor(7 + Math.random() * 5), fix: 1 });
  processStatusData({ rssi: -(65 + Math.random() * 20), bat_pct: Math.max(18, 90 - _step * 0.03), bat_v: +(11.1 + Math.random() * 1.2).toFixed(2) });
}

function switchPage(page, el) {
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
  document.getElementById('page-' + page).classList.add('active');
  el.classList.add('active');
  if (page === 'dashboard' && state.map) setTimeout(() => state.map.invalidateSize(), 60);
}

let logOpen = false;
function toggleLog() {
  logOpen = !logOpen;
  document.getElementById('log-panel').classList.toggle('open', logOpen);
}

function addLog(type, msg) {
  const body = document.getElementById('log-body');
  const ts = new Date().toTimeString().slice(0, 8);
  const el = document.createElement('div');
  el.className = `log-entry ${type}`;
  el.textContent = `[${ts}] ${msg}`;
  body.appendChild(el);
  while (body.children.length > 200) body.removeChild(body.firstChild);
  body.scrollTop = body.scrollHeight;
}

const toastIcons = { ok: '✓', err: '✕', warn: '!' };
function showToast(msg, type = 'ok') {
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.innerHTML = `<div class="toast-icon">${toastIcons[type] || '•'}</div><span>${msg}</span>`;
  const container = document.getElementById('toast-container');
  container.appendChild(toast);
  setTimeout(() => {
    toast.style.transition = 'all 0.25s';
    toast.style.opacity = '0';
    toast.style.transform = 'translateX(12px)';
    setTimeout(() => toast.remove(), 250);
  }, 3500);
}

updateSpeed(60);