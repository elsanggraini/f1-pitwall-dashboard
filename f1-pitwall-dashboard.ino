#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "NAMA_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";

WebServer server(80);

String getTrackCondition(float temperature, float humidity) {
  if (temperature > 34 && humidity > 75) return "HOT & HUMID";
  if (temperature > 34) return "HOT & DRY";
  if (temperature < 28 && humidity > 80) return "POSSIBLE RAIN";
  if (temperature < 25) return "COLD TRACK";
  return "NORMAL CONDITIONS";
}

String getDriverRisk(float temperature) {
  if (temperature < 30) return "LOW RISK";
  if (temperature <= 35) return "MEDIUM RISK";
  return "HIGH FATIGUE";
}

String getTyreStrategy(const String& trackCondition) {
  if (trackCondition == "HOT & DRY") return "SOFT";
  if (trackCondition == "HOT & HUMID") return "MEDIUM";
  if (trackCondition == "POSSIBLE RAIN") return "WET";
  if (trackCondition == "COLD TRACK") return "HARD";
  return "MEDIUM";
}

String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>F1 Pit Wall - Telemetry System</title>

  <script src="https://cdn.tailwindcss.com"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://unpkg.com/lucide@latest"></script>
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Share+Tech+Mono&display=swap" rel="stylesheet">

  <style>
    body{
      font-family:'Share Tech Mono', monospace;
      background:
        radial-gradient(at 0% 0%, rgba(220,38,38,0.08) 0px, transparent 45%),
        radial-gradient(at 100% 0%, rgba(220,38,38,0.05) 0px, transparent 45%),
        #060608;
      color:#fff;
    }
    .font-racing{ font-family:'Orbitron', sans-serif; }
    .card{
      background:#101114;
      border:1px solid rgba(220,38,38,0.35);
      box-shadow:0 0 15px rgba(220,38,38,0.12);
      border-radius:20px;
      transition:all .25s ease;
    }
    .card:hover{
      transform:translateY(-2px);
      box-shadow:0 0 25px rgba(220,38,38,0.22);
      border-color:rgba(220,38,38,0.6);
    }
    .mini-label{
      color:#9ca3af;
      letter-spacing:.12em;
      font-size:.72rem;
    }
    .soft{ color:#ef4444; border-color:#ef4444; }
    .medium{ color:#eab308; border-color:#eab308; }
    .hard{ color:#ffffff; border-color:#ffffff; }
    .wet{ color:#3b82f6; border-color:#3b82f6; }
    ::-webkit-scrollbar{ width:6px; }
    ::-webkit-scrollbar-thumb{ background:#dc2626; border-radius:999px; }
    ::-webkit-scrollbar-track{ background:#0f0f12; }
  </style>
</head>
<body class="min-h-screen overflow-x-hidden">

  <div class="h-1.5 w-full bg-gradient-to-r from-red-600 via-red-500 to-black"></div>

  <header class="border-b border-gray-800 bg-[#0a0a0d] px-6 md:px-12 py-4 flex flex-col md:flex-row justify-between items-center gap-4">
    <div class="flex items-center gap-4">
      <div class="bg-red-600 text-white px-3 py-2 rounded-lg font-racing font-black text-lg tracking-wider skew-x-[-12deg]">
        F1
      </div>
      <div>
        <h1 class="text-lg md:text-xl font-racing font-bold tracking-widest text-white flex items-center gap-2">
          SMART PIT WALL
          <span class="text-xs bg-red-600/20 text-red-500 px-2 py-0.5 rounded border border-red-500/30">SIMULATOR</span>
        </h1>
        <p class="text-xs text-gray-400 tracking-wider">ESP32 & DHT11 REAL-TIME TELEMETRY SYSTEM</p>
      </div>
    </div>

    <div class="flex items-center gap-3 text-xs">
      <div class="flex items-center gap-2 bg-gray-900/80 px-3 py-1.5 rounded-md border border-gray-800">
        <span class="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
        <span class="text-gray-300">LIVE</span>
      </div>
      <div class="flex items-center gap-2 bg-gray-900/80 px-3 py-1.5 rounded-md border border-gray-800">
        <i data-lucide="wifi" class="w-4 h-4 text-red-500"></i>
        <span class="text-gray-300">HOST: <span class="text-red-500" id="esp-ip">ESP32</span></span>
      </div>
    </div>
  </header>

  <main class="max-w-[1600px] mx-auto p-4 md:p-8">
    <section class="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-6">

      <div class="card p-5">
        <div class="flex justify-between items-start">
          <div>
            <p class="mini-label">TRACK TEMP</p>
            <h3 class="text-4xl md:text-5xl font-racing font-black text-red-500 mt-2" id="val-temp">--<span class="text-xl">°C</span></h3>
          </div>
          <div class="bg-red-600/10 p-2.5 rounded-lg border border-red-500/20">
            <i data-lucide="thermometer" class="w-6 h-6 text-red-500"></i>
          </div>
        </div>
      </div>

      <div class="card p-5">
        <div class="flex justify-between items-start">
          <div>
            <p class="mini-label">HUMIDITY</p>
            <h3 class="text-4xl md:text-5xl font-racing font-black text-sky-400 mt-2" id="val-humid">--<span class="text-xl">%</span></h3>
          </div>
          <div class="bg-sky-500/10 p-2.5 rounded-lg border border-sky-500/20">
            <i data-lucide="droplets" class="w-6 h-6 text-sky-400"></i>
          </div>
        </div>
      </div>

      <div class="card p-5">
        <div class="flex justify-between items-start">
          <div>
            <p class="mini-label">TRACK CONDITION</p>
            <h3 class="text-2xl md:text-3xl font-racing font-black text-white mt-3 leading-tight" id="val-condition">--</h3>
          </div>
          <div class="bg-yellow-500/10 p-2.5 rounded-lg border border-yellow-500/20">
            <i data-lucide="activity" class="w-6 h-6 text-yellow-400"></i>
          </div>
        </div>
      </div>

      <div class="card p-5">
        <div class="flex justify-between items-start">
          <div>
            <p class="mini-label">DRIVER RISK</p>
            <h3 class="text-2xl md:text-3xl font-racing font-black text-yellow-400 mt-3" id="val-risk">--</h3>
          </div>
          <div class="bg-yellow-500/10 p-2.5 rounded-lg border border-yellow-500/20">
            <i data-lucide="heart-pulse" class="w-6 h-6 text-yellow-400"></i>
          </div>
        </div>
      </div>

      <div class="card p-5">
        <div class="flex justify-between items-start">
          <div>
            <p class="mini-label">TIRE STRATEGY</p>
            <h3 class="text-2xl md:text-3xl font-racing font-black mt-3" id="val-tyre">--</h3>
          </div>
          <div class="bg-emerald-500/10 p-2.5 rounded-lg border border-emerald-500/20">
            <i data-lucide="disc-3" class="w-6 h-6 text-emerald-400"></i>
          </div>
        </div>
      </div>

    </section>

    <section class="grid grid-cols-1 xl:grid-cols-2 gap-6 mt-6">

      <div class="card p-5">
        <div class="flex items-center justify-between mb-4">
          <div class="flex items-center gap-3">
            <span class="w-2.5 h-2.5 rounded-full bg-red-500 animate-pulse"></span>
            <h2 class="font-racing font-bold tracking-widest text-sm text-white">TRACK TEMPERATURE GRAPH</h2>
          </div>
          <span class="text-xs text-gray-400">0 - 40°C</span>
        </div>
        <div class="h-[300px]">
          <canvas id="tempChart"></canvas>
        </div>
      </div>

      <div class="card p-5">
        <div class="flex items-center justify-between mb-4">
          <div class="flex items-center gap-3">
            <span class="w-2.5 h-2.5 rounded-full bg-sky-500 animate-pulse"></span>
            <h2 class="font-racing font-bold tracking-widest text-sm text-white">HUMIDITY GRAPH</h2>
          </div>
          <span class="text-xs text-gray-400">0 - 100%</span>
        </div>
        <div class="h-[300px]">
          <canvas id="humidChart"></canvas>
        </div>
      </div>

    </section>
  </main>

  <footer class="mt-8 border-t border-gray-900 py-6 text-center text-xs text-gray-500">
    F1 PIT WALL SYSTEM V2.6 — REALTIME TELEMETRY
  </footer>

<script>
  lucide.createIcons();

  document.getElementById('esp-ip').textContent = location.host || 'ESP32';

  const tempLabel = document.getElementById('val-temp');
  const humidLabel = document.getElementById('val-humid');
  const conditionLabel = document.getElementById('val-condition');
  const riskLabel = document.getElementById('val-risk');
  const tyreLabel = document.getElementById('val-tyre');

  const tempCtx = document.getElementById('tempChart').getContext('2d');
  const humidCtx = document.getElementById('humidChart').getContext('2d');

  const commonOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: {
      duration: 800,
      easing: 'easeInOutQuart'
    },
    interaction: {
      mode: 'index',
      intersect: false
    },
    plugins: {
      legend: {
        labels: {
          color: '#e5e7eb',
          font: {
            family: 'Orbitron'
          }
        }
      },
      tooltip: {
        backgroundColor: '#0a0a0d',
        borderColor: '#dc2626',
        borderWidth: 1,
        titleColor: '#fff',
        bodyColor: '#fff'
      }
    }
  };

  const labels = [];
  const tempData = [];
  const humidData = [];
  const MAX_POINTS = 18;

  const tempChart = new Chart(tempCtx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Temp (°C)',
        data: tempData,
        borderColor: '#ef4444',
        backgroundColor: 'rgba(239,68,68,0.10)',
        fill: true,
        tension: 0.5,
        cubicInterpolationMode: 'monotone',
        pointRadius: 2,
        pointHoverRadius: 5,
        borderWidth: 3
      }]
    },
    options: {
      ...commonOptions,
      scales: {
        x: {
          ticks: { color: '#9ca3af' },
          grid: { color: 'rgba(255,255,255,0.05)' }
        },
        y: {
          min: 25,
          max: 35,
          ticks: {
            color: '#9ca3af',
            stepSize: 5
          },
          grid: { color: 'rgba(255,255,255,0.05)' }
        }
      }
    }
  });

  const humidChart = new Chart(humidCtx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Humidity (%)',
        data: humidData,
        borderColor: '#38bdf8',
        backgroundColor: 'rgba(56,189,248,0.08)',
        fill: true,
        tension: 0.5,
        cubicInterpolationMode: 'monotone',
        pointRadius: 2,
        pointHoverRadius: 5,
        borderWidth: 3
      }]
    },
    options: {
      ...commonOptions,
      scales: {
        x: {
          ticks: { color: '#9ca3af' },
          grid: { color: 'rgba(255,255,255,0.05)' }
        },
        y: {
          min: 50,
          max: 100,
          ticks: {
            color: '#9ca3af',
            stepSize: 20
          },
          grid: { color: 'rgba(255,255,255,0.05)' }
        }
      }
    }
  });

  function tyreStyleClass(tyre) {
    if (tyre === 'SOFT') return 'soft';
    if (tyre === 'MEDIUM') return 'medium';
    if (tyre === 'HARD') return 'hard';
    if (tyre === 'WET') return 'wet';
    return 'medium';
  }

  function updateDashboard(data) {
    const temp = Number(data.temperature).toFixed(1);
    const humid = Number(data.humidity).toFixed(0);

    tempLabel.innerHTML = `${temp}<span class="text-xl">°C</span>`;
    humidLabel.innerHTML = `${humid}<span class="text-xl">%</span>`;
    conditionLabel.textContent = data.condition;
    riskLabel.textContent = data.risk;

    tyreLabel.textContent = data.tyre;
    tyreLabel.className = `text-2xl md:text-3xl font-racing font-black mt-3 ${tyreStyleClass(data.tyre)}`;

    const now = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    labels.push(now);
    tempData.push(Number(data.temperature));
    humidData.push(Number(data.humidity));

    if (labels.length > MAX_POINTS) {
      labels.shift();
      tempData.shift();
      humidData.shift();
    }

    tempChart.update();
    humidChart.update();
  }

  async function fetchTelemetry() {
    try {
      const res = await fetch('/data', { cache: 'no-store' });
      const data = await res.json();
      updateDashboard(data);
    } catch (err) {
      console.log('Failed to fetch telemetry');
    }
  }

  fetchTelemetry();
  setInterval(fetchTelemetry, 2000);
</script>

</body>
</html>
)rawliteral";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    temperature = 28.0;
    humidity = 50.0;
  }

  String condition = getTrackCondition(temperature, humidity);
  String risk = getDriverRisk(temperature);
  String tyre = getTyreStrategy(condition);

  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 0) + ",";
  json += "\"condition\":\"" + condition + "\",";
  json += "\"risk\":\"" + risk + "\",";
  json += "\"tyre\":\"" + tyre + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}