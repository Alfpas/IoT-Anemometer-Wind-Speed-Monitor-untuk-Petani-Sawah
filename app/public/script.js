const socket = io();

const speedEl = document.getElementById("speed");
const rpmEl = document.getElementById("rpm");
const pulseEl = document.getElementById("pulse");
const signalEl = document.getElementById("signal");
const statusWindEl = document.getElementById("status-wind");
const statusRpmEl = document.getElementById("status-rpm");
const statusPulseEl = document.getElementById("status-pulse");
const statusSignalEl = document.getElementById("status-signal");
const insightEl = document.getElementById("insight");

// Performance tracking elements
const avgWindEl = document.querySelector(".report-grid .report-item:nth-child(1) strong");
const peakGustEl = document.querySelector(".report-grid .report-item:nth-child(2) strong");
const uptimeEl = document.querySelector(".report-grid .report-item:nth-child(3) strong");
const chartLineEl = document.querySelector(".chart-line");

// Data storage for 7 days
const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];
let dailyData = {
  Mon: [], Tue: [], Wed: [], Thu: [], Fri: [], Sat: [], Sun: []
};
let sessionStart = Date.now();
let lastReportTime = Date.now();

// Load saved data from localStorage
function loadData() {
  const saved = localStorage.getItem("windSensorData");
  if (saved) {
    dailyData = JSON.parse(saved);
  }
}

// Save data to localStorage
function saveData() {
  localStorage.setItem("windSensorData", JSON.stringify(dailyData));
}

// Get today's day name
function getTodayName() {
  const dayIndex = new Date().getDay();
  const dayNames = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
  return dayNames[dayIndex];
}

// Update performance stats
function updatePerformanceStats() {
  let allSpeeds = [];
  let maxSpeed = 0;
  
  days.forEach(day => {
    if (dailyData[day].length > 0) {
      allSpeeds.push(...dailyData[day]);
      maxSpeed = Math.max(maxSpeed, ...dailyData[day]);
    }
  });

  const avgWind = allSpeeds.length > 0 ? (allSpeeds.reduce((a, b) => a + b, 0) / allSpeeds.length) : 0;
  const uptime = 98; // Can be calculated from connected time

  if (avgWindEl) avgWindEl.textContent = `${avgWind.toFixed(1)} m/s`;
  if (peakGustEl) peakGustEl.textContent = `${maxSpeed.toFixed(1)} m/s`;
  if (uptimeEl) uptimeEl.textContent = `${uptime}%`;
}

// Update chart visualization
function updateChart() {
  let chartSVG = '<svg viewBox="0 0 700 300" style="width: 100%; height: 100%;">';
  
  const padding = 40;
  const graphWidth = 700 - padding * 2;
  const graphHeight = 250;
  const dayWidth = graphWidth / 7;
  const barWidth = dayWidth * 0.6;
  let maxValue = 0;

  days.forEach(day => {
    if (dailyData[day].length > 0) {
      const dayMax = Math.max(...dailyData[day]);
      maxValue = Math.max(maxValue, dayMax);
    }
  });

  maxValue = Math.max(maxValue, 15); // Minimum scale

  // Draw grid lines
  for (let i = 0; i <= 5; i++) {
    const y = padding + (i * (graphHeight / 5));
    chartSVG += `<line x1="${padding}" y1="${y}" x2="${padding + graphWidth}" y2="${y}" stroke="#e2e8f0" stroke-width="1"/>`;
    const value = Math.round((maxValue - (i * (maxValue / 5))) * 10) / 10;
    chartSVG += `<text x="${padding - 10}" y="${y + 4}" text-anchor="end" font-size="11" fill="#94a3b8">${value}</text>`;
  }

  // Draw bars for each day
  days.forEach((day, idx) => {
    const dayAvg = dailyData[day].length > 0 
      ? dailyData[day].reduce((a, b) => a + b, 0) / dailyData[day].length 
      : 0;
    
    const x = padding + idx * dayWidth + (dayWidth - barWidth) / 2;
    const barHeight = (dayAvg / maxValue) * graphHeight;
    const y = padding + graphHeight - barHeight;
    
    // Draw bar
    chartSVG += `<rect x="${x}" y="${y}" width="${barWidth}" height="${barHeight}" fill="#06b6d4" opacity="0.8" rx="4"/>`;
    
    // Draw day label
    const labelX = padding + idx * dayWidth + dayWidth / 2;
    chartSVG += `<text x="${labelX}" y="${padding + graphHeight + 25}" text-anchor="middle" font-size="12" fill="#94a3b8" font-weight="500">${day}</text>`;
    
    // Draw value on top of bar
    if (dayAvg > 0) {
      chartSVG += `<text x="${labelX}" y="${y - 5}" text-anchor="middle" font-size="11" fill="#0369a1" font-weight="600">${dayAvg.toFixed(1)}</text>`;
    }
  });

  chartSVG += '</svg>';
  if (chartLineEl) {
    chartLineEl.innerHTML = chartSVG;
  }
}

loadData();

socket.on("connect", () => {
  signalEl.textContent = "Live";
  statusSignalEl.textContent = "Connected";
  statusSignalEl.style.color = "#16a34a";
  sessionStart = Date.now();
});

socket.on("disconnect", () => {
  signalEl.textContent = "Offline";
  statusSignalEl.textContent = "Disconnected";
  statusSignalEl.style.color = "#ef4444";
  insightEl.textContent = "Connection lost. Waiting for sensor data...";
});

socket.on("sensor", (data) => {
  const speed = Number(data.speed ?? data.speed_ms ?? 0);
  const rpm = Number(data.rpm || 0);
  const pulse = Number(data.pulse || 0);

  // Update live values
  speedEl.textContent = `${speed.toFixed(2)} m/s`;
  rpmEl.textContent = rpm;
  pulseEl.textContent = pulse;
  signalEl.textContent = "Live";

  // Store data for today
  const todayName = getTodayName();
  dailyData[todayName].push(speed);
  if (dailyData[todayName].length > 1000) {
    dailyData[todayName].shift(); // Keep only last 1000 readings
  }
  saveData();

  // Update performance stats every 10 seconds
  const now = Date.now();
  if (now - lastReportTime >= 10000) {
    updatePerformanceStats();
    updateChart();
    lastReportTime = now;
  }

  // Update status indicators
  statusWindEl.textContent = speed > 0 ? "Active" : "Idle";
  statusRpmEl.textContent = rpm > 0 ? "Running" : "Stopped";
  statusPulseEl.textContent = pulse > 0 ? "Recording" : "Waiting";
  statusSignalEl.textContent = "Connected";

  // Update status colors
  statusWindEl.style.color = speed > 0 ? "#0ea5e9" : "#94a3b8";
  statusRpmEl.style.color = rpm > 0 ? "#fbbf24" : "#94a3b8";
  statusPulseEl.style.color = pulse > 0 ? "#f97316" : "#94a3b8";
  statusSignalEl.style.color = "#16a34a";

  // Generate insight message
  let condition = "Calm conditions";
  let recommendation = "Perfect for most farming activities.";

  if (speed > 12) {
    condition = "⚠️ High wind activity";
    recommendation = "High winds detected. Secure crops and equipment. Consider postponing field work.";
  } else if (speed > 8) {
    condition = "🌬️ Strong wind flow";
    recommendation = "Strong winds present. Monitor crop impact and adjust irrigation schedules.";
  } else if (speed > 3) {
    condition = "💨 Moderate wind flow";
    recommendation = "Good ventilation for crops. Ideal wind conditions for most operations.";
  }

  insightEl.innerHTML = `<strong>${condition}</strong> — ${recommendation}<br><em>Reading: ${rpm} RPM, ${pulse} pulses detected</em>`;
});

// Initial update
updatePerformanceStats();
updateChart();