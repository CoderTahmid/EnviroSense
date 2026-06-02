#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <SPI.h>
#include <LoRa.h>
#define SS    18
#define RST   14
#define DIO0  26
#define SCK   5
#define MISO  19
#define MOSI  27
#define BUZZER_PIN 25
const char* WIFI_SSID = "Arghya";
const char* WIFI_PASS = "1234567778";
WebServer server(80);
Preferences prefs;
struct TreeRow {
  const char* serial;
  const char* treeNo;
  const char* label;
  float lat;
  float lng;
  bool active;
  // live sensor inputs
  bool flame;
  bool vibrate;
  bool fall;
  // dashboard state
  bool visibleAlert;
  bool eventLatched;
  bool actionTaken;
  bool suppressUntilNormal;
};
TreeRow rows[5] = {
  {"01", "Tree 01", "Demo Tree", 23.810300, 90.412500, true,  false, false, false, false, false, false, false},
  {"02", "Tree 02", "Dummy",     0.000000,  0.000000,  false, false, false, false, false, false, false, false},
  {"03", "Tree 03", "Dummy",     0.000000,  0.000000,  false, false, false, false, false, false, false, false},
  {"04", "Tree 04", "Dummy",     0.000000,  0.000000,  false, false, false, false, false, false, false, false},
  {"05", "Tree 05", "Dummy",     0.000000,  0.000000,  false, false, false, false, false, false, false, false}
};
uint32_t totalCut  = 0;
uint32_t totalFall = 0;
uint32_t totalFire = 0;
uint32_t logSeq = 0;
bool buzzerActive = false;
bool buzzState = false;
unsigned long lastBuzzMs = 0;
const uint16_t BUZZ_ON = 300;
const uint16_t BUZZ_OFF = 300;
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>EnviroSense Dashboard</title>
  <style>
    :root{
      --bg:#eef3ef; --card:#ffffff; --text:#17311f; --muted:#5c6b61;
      --green:#1f5d3a; --green2:#2f7d4c; --red:#d63b3b; --border:#d9e3dc;
    }
    *{box-sizing:border-box;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif}
    body{margin:0;background:var(--bg);color:var(--text)}
    .wrap{max-width:1400px;margin:0 auto;padding:16px}
    .topbar{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:14px}
    .title{font-size:22px;font-weight:800}
    .sub{font-size:13px;color:var(--muted)}
    .grid{display:grid;grid-template-columns:320px 1fr;gap:14px}
    .card{background:var(--card);border:1px solid var(--border);border-radius:18px;box-shadow:0 8px 24px rgba(20,40,25,.06);padding:16px}
    .metric{padding:14px;border-radius:16px;background:#f7faf8;border:1px solid var(--border);margin-bottom:12px}
    .metric .label{font-size:13px;color:var(--muted)}
    .metric .value{font-size:30px;font-weight:800;margin-top:4px}
    .btn{
      display:inline-flex;align-items:center;justify-content:center;
      border:none;border-radius:12px;padding:10px 14px;cursor:pointer;
      background:var(--green);color:#fff;font-weight:700;text-decoration:none;
    }
    .btn:disabled{opacity:.45;cursor:not-allowed}
    .btn.secondary{background:#edf3ef;color:var(--green);border:1px solid var(--border)}
    table{width:100%;border-collapse:separate;border-spacing:0 10px}
    th{text-align:left;font-size:12px;color:var(--muted);padding:0 10px}
    td{
      background:#fff;border-top:1px solid var(--border);border-bottom:1px solid var(--border);
      padding:14px 10px;font-size:14px
    }
    td:first-child{border-left:1px solid var(--border);border-top-left-radius:14px;border-bottom-left-radius:14px}
    td:last-child{border-right:1px solid var(--border);border-top-right-radius:14px;border-bottom-right-radius:14px}
    .badge{display:inline-block;padding:7px 10px;border-radius:999px;font-size:12px;font-weight:700}
    .healthy{background:#eaf6ef;color:#1f6b3d}
    .fire{background:#fdeaea;color:#a72222}
    .cut{background:#fff0e6;color:#b65b00}
    .fall{background:#edf3ff;color:#3058a8}
    .blink{
      animation:blink 0.9s linear infinite;
    }
    @keyframes blink{
      0%,100%{background:#ffe1e1}
      50%{background:#ff6b6b;color:#fff}
    }
    .link{color:var(--green2);font-weight:700;text-decoration:none}
    .rowactive{border-left:4px solid var(--green2)}
    .dummy{opacity:.55}
    .actions{display:flex;gap:10px;flex-wrap:wrap}
    @media (max-width: 980px){
      .grid{grid-template-columns:1fr}
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="topbar">
      <div>
        <div class="title">EnviroSense Dashboard</div>
        <div class="sub">Local ESP32 dashboard • static IP • no internet required</div>
      </div>
      <div class="actions">
        <a class="btn secondary" href="/download.csv">Download CSV</a>
      </div>
    </div>
    <div class="grid">
      <div class="card">
        <div class="metric">
          <div class="label">Total damaged trees (fallen + cut)</div>
          <div class="value" id="totalDamaged">0</div>
        </div>
        <div class="metric">
          <div class="label">Trees fallen naturally / storm</div>
          <div class="value" id="totalFall">0</div>
        </div>
        <div class="metric">
          <div class="label">Trees cut down</div>
          <div class="value" id="totalCutted">0</div>
        </div>
        <div class="metric">
          <div class="label">Fire incidents</div>
          <div class="value" id="totalFire">0</div>
        </div>
        <div class="sub">Tip: the action button clears the visible alert, but counts stay saved.</div>
      </div>
      <div class="card">
        <table>
          <thead>
            <tr>
              <th>Serial</th>
              <th>Tree Number</th>
              <th>Tree Location</th>
              <th>Fire / No Fire</th>
              <th>Healthy / Cutted / Fall in storm</th>
              <th>Action</th>
            </tr>
          </thead>
          <tbody id="rows"></tbody>
        </table>
      </div>
    </div>
  </div>
<script>
async function actionTaken(id){
  await fetch('/action?id=' + id);
  await refresh();
}
function badgeClass(status){
  if(status === 'Fire') return 'badge fire';
  if(status === 'Cutted') return 'badge cut';
  if(status === 'Fall in storm') return 'badge fall';
  return 'badge healthy';
}
async function refresh(){
  const res = await fetch('/api/status');
  const data = await res.json();
  document.getElementById('totalDamaged').textContent = data.total_damaged;
  document.getElementById('totalFall').textContent    = data.total_fall;
  document.getElementById('totalCutted').textContent  = data.total_cutted;
  document.getElementById('totalFire').textContent    = data.total_fire;
  const tbody = document.getElementById('rows');
  tbody.innerHTML = '';
  data.rows.forEach(r => {
    const tr = document.createElement('tr');
    tr.className = r.active ? 'rowactive' : 'dummy';
    const mapUrl = 'https://www.google.com/maps?q=' + r.lat + ',' + r.lng;
    // Use display_fire and display_status from server — these already
    // account for suppressUntilNormal (action taken state).
    // const fireBadge   = r.display_fire
    //   ? '<span class="badge fire">Fire</span>'
    //   : '<span class="badge healthy">No Fire</span>';
    const fireBadge   = r.display_fire
      ? '<span class="badge fire' + (r.alert ? ' blink' : '') + '">Fire</span>'
      : '<span class="badge healthy">No Fire</span>';
    // const statusBadge = '<span class="'
    //   + badgeClass(r.display_status)
    //   + (r.alert ? ' blink' : '')
    //   + '">' + r.display_status + '</span>';
    const statusBadge = '<span class="'
      + badgeClass(r.display_status)
      + (r.alert && !r.display_fire ? ' blink' : '')
      + '">' + r.display_status + '</span>';
    tr.innerHTML = `
      <td>${r.serial}</td>
      <td>${r.tree_no}</td>
      <td><a class="link" href="${mapUrl}" target="_blank">Open map</a></td>
      <td>${fireBadge}</td>
      <td>${statusBadge}</td>
      <td>
        <button class="btn" ${(!r.active || !r.alert) ? 'disabled' : ''} onclick="actionTaken(${r.id})">
          Action taken
        </button>
      </td>
    `;
    tbody.appendChild(tr);
  });
}
refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)rawliteral";
String csvEscape(const String& s) {
  String out = s;
  out.replace("\"", "\"\"");
  return "\"" + out + "\"";
}
// Raw sensor status — used for CSV logging only
// String currentStatus(const TreeRow& r) {
//   if (r.flame)             return "Fire";
//   if (r.fall && r.vibrate) return "Cutted";
//   if (r.fall)              return "Fall in storm";
//   return "Healthy";
// }
String currentStatus(const TreeRow& r) {
  if (r.fall && r.vibrate) return "Cutted";
  if (r.fall)              return "Fall in storm";
  return "Healthy";
}
// Display status — respects suppressUntilNormal (action taken)
// Once action is taken, show Healthy until sensors physically go back to normal.
String displayStatus(const TreeRow& r) {
  if (r.suppressUntilNormal) return "Healthy";
  return currentStatus(r);
}

void sendAck() {
  int retries = 5, gapMs = 80;
  for (int i = 0; i < retries; i++) {
    delay(gapMs);               // wait for sender to finish its TX burst
    LoRa.beginPacket();
    LoRa.print("1");
    LoRa.endPacket();           // non-blocking, ~20ms TX time on LoRa
  }
}


void saveCounters() {
  prefs.putUInt("totCut",  totalCut);
  prefs.putUInt("totFall", totalFall);
  prefs.putUInt("totFire", totalFire);
  prefs.putUInt("logSeq",  logSeq);
}
void saveRow(uint8_t i) {
  String base = "r" + String(i);
  prefs.putBool((base + "flame").c_str(),    rows[i].flame);
  prefs.putBool((base + "vibr").c_str(),     rows[i].vibrate);
  prefs.putBool((base + "fall").c_str(),     rows[i].fall);
  prefs.putBool((base + "visible").c_str(),  rows[i].visibleAlert);
  prefs.putBool((base + "latched").c_str(),  rows[i].eventLatched);
  prefs.putBool((base + "taken").c_str(),    rows[i].actionTaken);
  prefs.putBool((base + "suppress").c_str(), rows[i].suppressUntilNormal);
}
void loadState() {
  totalCut  = prefs.getUInt("totCut",  0);
  totalFall = prefs.getUInt("totFall", 0);
  totalFire = prefs.getUInt("totFire", 0);
  logSeq    = prefs.getUInt("logSeq",  0);
  for (uint8_t i = 0; i < 5; i++) {
    String base = "r" + String(i);
    rows[i].flame               = prefs.getBool((base + "flame").c_str(),    false);
    rows[i].vibrate             = prefs.getBool((base + "vibr").c_str(),     false);
    rows[i].fall                = prefs.getBool((base + "fall").c_str(),     false);
    rows[i].visibleAlert        = prefs.getBool((base + "visible").c_str(),  false);
    rows[i].eventLatched        = prefs.getBool((base + "latched").c_str(),  false);
    rows[i].actionTaken         = prefs.getBool((base + "taken").c_str(),    false);
    rows[i].suppressUntilNormal = prefs.getBool((base + "suppress").c_str(), false);
  }
}
void appendCsvLog(uint8_t idx, const char* eventType) {
  File f = LittleFS.open("/events.csv", FILE_APPEND);
  if (!f) return;
  f.print(logSeq++);                       f.print(",");
  f.print(rows[idx].serial);               f.print(",");
  f.print(rows[idx].treeNo);               f.print(",");
  f.print(eventType);                      f.print(",");
  f.print(currentStatus(rows[idx]));       f.print(",");
  f.print(rows[idx].flame   ? "1" : "0"); f.print(",");
  f.print(rows[idx].vibrate ? "1" : "0"); f.print(",");
  f.print(rows[idx].fall    ? "1" : "0"); f.print(",");
  f.print(rows[idx].lat, 6);              f.print(",");
  f.print(rows[idx].lng, 6);              f.print(",");
  f.println(csvEscape(rows[idx].label));
  f.close();
  saveCounters();
}
void updateTreeState(uint8_t idx, bool flame, bool vibrate, bool fall) {
  if (idx >= 5) return;
  TreeRow &r = rows[idx];
  bool rawAlert = flame || fall;
  // Always store live sensor values for logging purposes
  r.flame   = flame;
  r.vibrate = vibrate;
  r.fall    = fall;
  if (rawAlert && !r.eventLatched) {
    // First time this event fires — increment counter and log it
    if (flame) {
      totalFire++;
      appendCsvLog(idx, "FIRE");
      buzzerActive = true;
    } else if (fall && vibrate) {
      totalCut++;
      appendCsvLog(idx, "CUT");
      buzzerActive = true;
    } else if (fall) {
      totalFall++;
      appendCsvLog(idx, "FALL");
      buzzerActive = true;
    }
    r.eventLatched        = true;
    r.visibleAlert        = true;
    r.actionTaken         = false;
    r.suppressUntilNormal = false;
    saveCounters();
  }
  // Keep the alert badge blinking while sensors are active
  // and the operator hasn't pressed "Action taken" yet
  if (rawAlert && !r.suppressUntilNormal) {
    r.visibleAlert = true;
  }
  // Sensors physically returned to normal — fully re-arm for the next event
  if (!rawAlert) {
    // r.eventLatched        = false;
    // r.visibleAlert        = false;
    // r.actionTaken         = false;
    // r.suppressUntilNormal = false;
    r.eventLatched        = false;
    r.visibleAlert        = false;
    r.actionTaken         = false;
    r.suppressUntilNormal = false;
    buzzerActive          = false;
    digitalWrite(BUZZER_PIN, LOW);
    // // Notify the sensor node via LoRa
    // LoRa.beginPacket();
    // LoRa.print("1");
    // LoRa.endPacket();
    sendAck();          // ← replaces the 3 LoRa lines
  }
  saveRow(idx);
}
// Replace every single  LoRa.beginPacket(); LoRa.print("1"); LoRa.endPacket();
// with this helper call:



void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}
void handleStatus() {
  String json = "{";
  // total_damaged = cut + storm fall combined
  json += "\"total_damaged\":" + String(totalCut + totalFall) + ",";
  json += "\"total_fall\":"    + String(totalFall)            + ",";
  json += "\"total_cutted\":"  + String(totalCut)             + ",";
  json += "\"total_fire\":"    + String(totalFire)            + ",";
  json += "\"rows\":[";
  for (uint8_t i = 0; i < 5; i++) {
    if (i) json += ",";
    TreeRow &r = rows[i];
    // ── KEY FIX ────────────────────────────────────────────────────────────
    // displayStatus and displayFire both respect suppressUntilNormal.
    // When the operator presses "Action taken", suppress=true so the badge
    // and fire column immediately revert to Healthy / No Fire even while the
    // physical sensors are still active. They snap back automatically to the
    // real sensor state once the physical sensors return to normal (which
    // clears suppressUntilNormal inside updateTreeState).
    String dStatus = displayStatus(r);
    bool   dFire   = r.flame && !r.suppressUntilNormal;
    // ───────────────────────────────────────────────────────────────────────
    json += "{";
    json += "\"id\":"             + String(i + 1)                           + ",";
    json += "\"serial\":\""       + String(r.serial)                        + "\",";
    json += "\"tree_no\":\""      + String(r.treeNo)                        + "\",";
    json += "\"label\":\""        + String(r.label)                         + "\",";
    json += "\"lat\":"            + String(r.lat, 6)                        + ",";
    json += "\"lng\":"            + String(r.lng, 6)                        + ",";
    json += "\"active\":"         + String(r.active       ? "true":"false") + ",";
    json += "\"display_fire\":"   + String(dFire          ? "true":"false") + ",";
    json += "\"display_status\":\"" + dStatus                               + "\",";
    json += "\"alert\":"          + String(r.visibleAlert ? "true":"false") + ",";
    json += "\"action_taken\":"   + String(r.actionTaken  ? "true":"false") + "";
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}
void handleAction() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing id\"}");
    return;
  }
  int id = server.arg("id").toInt() - 1;
  if (id < 0 || id >= 5) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad id\"}");
    return;
  }
  TreeRow &r = rows[id];
  if (r.active) {
    // r.visibleAlert        = false;
    // r.actionTaken         = true;
    // r.suppressUntilNormal = true;
    // saveRow(id);
    r.visibleAlert        = false;
    r.actionTaken         = true;
    r.suppressUntilNormal = true;
    saveRow(id);
    buzzerActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    // // Notify the sensor node via LoRa
    // LoRa.beginPacket();
    // LoRa.print("1");
    // LoRa.endPacket();
    sendAck();          // ← replaces the 3 LoRa lines
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
void handleDownload() {
  if (!LittleFS.exists("/events.csv")) {
    File f = LittleFS.open("/events.csv", FILE_WRITE);
    if (f) {
      f.println("seq,serial,tree_no,event,status,flame,vibrate,fall,lat,lng,label");
      f.close();
    }
  }
  File f = LittleFS.open("/events.csv", FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "CSV not available");
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=events.csv");
  server.streamFile(f, "text/csv");
  f.close();
}
void ensureCsvHeader() {
  if (!LittleFS.exists("/events.csv")) {
    File f = LittleFS.open("/events.csv", FILE_WRITE);
    if (f) {
      f.println("seq,serial,tree_no,event,status,flame,vibrate,fall,lat,lng,label");
      f.close();
    }
  }
}
void readSensorsAndUpdate() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String pac = "";
    while (LoRa.available()) {
      pac += (char)LoRa.read();
    }
    Serial.print("Received packet: ");
    Serial.print(pac);
    Serial.print("  | RSSI: ");
    Serial.println(LoRa.packetRssi());
    if ((pac[0] == '0' || pac[0] == '1') &&
        (pac[1] == '0' || pac[1] == '1') &&
        (pac[2] == '0' || pac[2] == '1'))
    {
      bool is_flame    = (pac[0] == '1');
      bool is_vibrate  = (pac[1] == '1');
      bool is_fall     = (pac[2] == '1');
      // updateTreeState(0, is_flame, is_vibrate, is_fall);
      rows[0].eventLatched = false;   // re-arm before each packet
      updateTreeState(0, is_flame, is_vibrate, is_fall);
    }
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  Serial.println("LoRa Receiver Initializing...");
  if 4(!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa Receiver Ready");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }
  ensureCsvHeader();
  prefs.begin("envirosense", false);
  loadState();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  server.on("/",             HTTP_GET, handleRoot);
  server.on("/api/status",   HTTP_GET, handleStatus);
  server.on("/action",       HTTP_GET, handleAction);
  server.on("/download.csv", HTTP_GET, handleDownload);
  server.begin();
}

void updateBuzzer() {
  if (!buzzerActive) return;
  unsigned long now = millis();
  uint16_t interval = buzzState ? BUZZ_ON : BUZZ_OFF;
  if (now - lastBuzzMs >= interval) {
    buzzState = !buzzState;
    digitalWrite(BUZZER_PIN, buzzState ? HIGH : LOW);
    lastBuzzMs = now;
  }
}

// void loop() {
//   server.handleClient();
//   readSensorsAndUpdate();
// }

void loop() {
  server.handleClient();
  readSensorsAndUpdate();
  updateBuzzer();
}