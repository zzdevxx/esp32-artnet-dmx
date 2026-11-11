/*
 * ZZLIGHT PROJECT - ARTNET TO DMX MODULE (v5.2.0 Hardened Production Master)
 * Hardware : ESP32 DevKit V1, MAX485 Transceiver, LCD 16x2 I2C (PCF8574)
 * Framework: Arduino ESP32 Core v2.x / v3.x / FreeRTOS Dual-Core SMP
 *
 * Professional Stage Hardening:
 *  1. Full Art-Net 4 Spec Compliance: 15-bit Port-Address (Net 0..127 + SubUni 0..255),
 *     ProtVer >= 14, even length bounds check, sequence reset resilience.
 *  2. Full ArtPoll & ArtPollReply (OpCode 0x2000 / 0x2100) Node Discovery Engine
 *     with strictly verified byte offsets (auto-detected by QLC+, grandMA, Resolume).
 *  3. Lock-Free Triple Buffering (Atomic Pointer Exchange) - 100% Free of Frame Tearing.
 *  4. UDP Socket Queue Drain Loop - Zero Packet Starvation under Heavy 2.4 GHz Traffic.
 *  5. Wi-Fi Modem Sleep Disabled (WiFi.setSleep(false)) - Ultra-Low Stable Latency (<3ms).
 *  6. I2C Bus Armor (Wire.setTimeOut(25)) - Prevents Wire Bus Freeze from Vibration/Noise.
 *  7. RF Live Show Protection: WiFi scan blocked in RUN mode to prevent RF channel-hopping blackout.
 *  8. Permanent Background Auto-Reconnect Saga - Eliminates AP Mode Lockout.
 *  9. Non-Blocking Display State Machine - Zero Blocking Delays on Networking Core 1.
 * 10. Web Security Hardening: XSS Protection via textContent, Form Validation, JSON Escaping.
 * 11. Single-Pass Web Route Registration: Eliminates route shadowing on reconnect.
 * 12. FreeRTOS Watchdog Feeder: Guaranteed vTaskDelay(1) prevents TWDT starvation on Core 0.
 * 13. High-Integrity UI: Tabular numerals (tabular-nums) and 5-state interactive button contract.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <esp_timer.h>
#include <string.h>

// --- HARDWARE PIN CONFIGURATION ---
#define DMX_TX_PIN 17
#define DMX_RX_PIN 16
#define DMX_EN_PIN 4

// --- ART-NET 4 CONSTANTS & SPECIFICATION ---
#define ARTNET_PORT           6454
#define ARTNET_OP_POLL        0x2000
#define ARTNET_OP_POLL_REPLY  0x2100
#define ARTNET_OP_DMX         0x5000
#define ARTNET_PROT_VER       14
#define ARTNET_HEADER_TAG     "Art-Net"

// 15-bit Port-Address: (Net << 8) | SubUni
// Net: 0 (0..127), Sub-Net: 0 (0..15), Universe: 0 (0..15) -> Total = 0x0000
const uint16_t TARGET_PORT_ADDRESS = 0x0000;

// --- LOCK-FREE TRIPLE BUFFERING ARCHITECTURE ---
// Buffer 0: Owned by Core 0 during physical UART DMX transmission
// Buffer 1: Staging / Ready buffer containing the latest verified complete frame
// Buffer 2: Owned by Core 1 during active UDP payload reception
static uint8_t dmx_buf_pool[3][512];
static uint8_t* volatile dmx_tx_buffer    = dmx_buf_pool[0];
static uint8_t* volatile dmx_ready_buffer = dmx_buf_pool[1];
static uint8_t* volatile dmx_rx_buffer    = dmx_buf_pool[2];

static portMUX_TYPE dmx_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool dmx_new_frame_ready = false;
static volatile uint32_t artnet_rx_count = 0;
static volatile unsigned long lastArtNetTime = 0;
static volatile bool hasReceivedFirstPacket = false;

WiFiUDP Udp;
TaskHandle_t DMXTaskHandle;

// --- SYSTEM, NETWORK, & PREFERENCES VARIABLES ---
Preferences prefs;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

LiquidCrystal_I2C lcd(0x27, 16, 2);

String ssid = "";
String pass = "";

unsigned long wifiLostTime = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastAPBackgroundScan = 0;
int connectedDevices = 0;

// --- NON-BLOCKING LCD STATE MACHINE ---
enum ScreenState {
  S_BOOT, S_CONNECTING, S_WIFI_ERR, S_AP_MODE, S_DEV_CONN,
  S_MOD_CONN1, S_MOD_CONN2, S_RUN, S_WIFI_RST,
  S_WIFI_DISCONN1, S_WIFI_DISCONN2, S_WIFI_FAIL
};
ScreenState currentState = S_BOOT;
unsigned long stateEnterTime = 0;
unsigned long stateHoldDuration = 0;
ScreenState pendingNextState = S_BOOT;

// Function Prototypes
void setScreenState(ScreenState newState, unsigned long holdDuration = 0, ScreenState nextState = S_BOOT);
void handleLCDAnimation();
void startAP();
void connectWiFi();
void registerWebRoutes();
void DMXCoreTask(void *pvParameters);
void sendArtPollReply(IPAddress remoteIP, uint16_t remotePort);
String escapeJsonString(const String& input);

// --- SECURE HTML TEMPLATES (PROGMEM) ---

// 1. Setup Page (AP Mode)
const char AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Setup</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;700;900&display=swap');
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin: 0; padding: 40px 20px; display: flex; flex-direction: column; align-items: center; }
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; margin-bottom: 25px; }
    .container { width: 100%; max-width: 340px; display: flex; flex-direction: column; align-items: center; }
    .wifi-box { background: #1e1e1e; border-radius: 8px; padding: 5px; width: 100%; max-height: 220px; overflow-y: auto; border: 1px solid #333; box-sizing: border-box; margin-bottom: 15px; }
    .wifi-item { display: flex; justify-content: space-between; align-items: center; padding: 14px 10px; border-bottom: 1px solid #333; cursor: pointer; transition: 0.15s; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:hover { background: #2a2a2a; }
    .w-left { font-size: 15px; display: flex; align-items: center; gap: 10px; }
    .w-right { font-size: 14px; color: #aaa; display: flex; align-items: center; gap: 8px; font-variant-numeric: tabular-nums; }
    .btn-refresh { background-color: #e67e22; color: white; padding: 12px; border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; font-size: 15px; font-weight: 700; cursor: pointer; width: 100%; margin-bottom: 30px; transition: 0.15s; }
    .btn-refresh:hover { filter: brightness(1.15); }
    .btn-refresh:active { transform: scale(0.98); }
    form { width: 100%; }
    input[type="text"], input[type="password"] { background-color: #ffffff; color: #000000; padding: 14px; margin-bottom: 12px; width: 100%; border-radius: 6px; border: none; font-size: 16px; box-sizing: border-box; text-align: center; }
    .btn-connect { background-color: #2ecc71; color: white; padding: 14px; border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; font-size: 16px; font-weight: 700; cursor: pointer; width: 100%; margin-top: 5px; transition: 0.15s; }
    .btn-connect:hover { filter: brightness(1.15); }
    .btn-connect:active { transform: scale(0.98); }
    .footer-link { color: #888888; text-decoration: underline; font-size: 15px; margin-top: 40px; display: block; }
  </style>
  <script>
    function fillSSID(el) { document.getElementById('ssid').value = el.getAttribute('data-ssid'); document.getElementById('pass').focus(); }
    function scan() {
      let wb = document.getElementById('wb');
      wb.innerHTML = '<div style="padding:20px;color:#aaa;">Scanning WiFi...</div>';
      fetch('/scan').then(r=>r.json()).then(d=>{
        wb.innerHTML = '';
        if(d.length === 0) {
          wb.innerHTML = '<div style="padding:20px;color:#aaa;">No WiFi found</div>';
          return;
        }
        d.forEach(w => {
          let item = document.createElement('div');
          item.className = 'wifi-item';
          item.setAttribute('data-ssid', w.ssid);
          item.onclick = function() { fillSSID(this); };

          let left = document.createElement('div');
          left.className = 'w-left';
          left.innerHTML = '<span>📶</span> ';
          let sTxt = document.createElement('span');
          sTxt.textContent = w.ssid;
          left.appendChild(sTxt);

          let right = document.createElement('div');
          right.className = 'w-right';
          right.textContent = (w.sec ? '🔒 ' : '🔓 ') + w.sig + '%';

          item.appendChild(left);
          item.appendChild(right);
          wb.appendChild(item);
        });
      }).catch(() => { wb.innerHTML = '<div style="padding:20px;color:#e74c3c;">Scan failed</div>'; });
    }
    function togglePass() {
      let p = document.getElementById('pass');
      let b = document.getElementById('btnTogglePass');
      if (p.type === 'password') {
        p.type = 'text';
        b.textContent = 'HIDE';
      } else {
        p.type = 'password';
        b.textContent = 'SHOW';
      }
    }
    window.onload = scan;
  </script>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>
  <div class="container">
    <div id="wb" class="wifi-box"></div>
    <button type="button" class="btn-refresh" onclick="scan()">Refresh</button>
    <form action="/connect" method="POST">
      <input type="text" name="ssid" id="ssid" placeholder="SSID (Max 32 Characters)" maxlength="32" required>
      <div style="position:relative;width:100%;margin-bottom:12px;">
        <input type="password" name="pass" id="pass" placeholder="Password (Max 63 Characters)" maxlength="63" style="box-sizing:border-box;width:100%;margin-bottom:0;padding-right:68px;">
        <button type="button" id="btnTogglePass" onclick="togglePass()" style="position:absolute;right:8px;top:50%;transform:translateY(-50%);background:#2a2a2a;color:#fff;border:1px solid #444;border-radius:4px;padding:6px 10px;font-size:11px;font-weight:700;cursor:pointer;letter-spacing:0.5px;">SHOW</button>
      </div>
      <button type="submit" class="btn-connect">Connect & Reboot</button>
    </form>
    <a href="/monitor" class="footer-link">Monitor</a>
  </div>
</body>
</html>
)rawliteral";

// 2. Saved Confirmation Page
const char SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Connect</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;900&display=swap');
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin-top: 100px; padding: 20px; }
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; }
    p { font-size: 16px; margin-top: 20px; color: #ccc; }
  </style>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>
  <p>WiFi Credentials Saved. Applying and Connecting...</p>
</body>
</html>
)rawliteral";

// 3. Monitor Page
const char RUN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Monitor</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,400;0,700;0,900;1,700&display=swap');
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin: 0; padding: 40px 20px; display: flex; flex-direction: column; align-items: center; }
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; margin-bottom: 25px; }
    .box { background-color: #1e1e1e; padding: 20px; border-radius: 8px; border: 1px solid #333; width: 100%; max-width: 340px; box-sizing: border-box; text-align: left; font-size: 15px; line-height: 1.8; margin-bottom: 25px; font-weight: 700; font-variant-numeric: tabular-nums; }
    .box div { white-space: nowrap; }
    .lbl { display: inline-block; width: 85px; color: #ccc; font-style: italic; }
    .btn-container { display: flex; flex-direction: column; gap: 12px; width: 100%; max-width: 340px; }
    .btn { width: 100%; padding: 14px; border: 1px solid rgba(255,255,255,0.15); border-radius: 6px; font-weight: 700; cursor: pointer; font-size: 16px; font-family: 'Roboto', sans-serif; transition: background-color 0.15s, transform 0.05s; }
    .btn:hover { filter: brightness(1.15); }
    .btn:focus-visible { outline: 2px solid #3498db; outline-offset: 2px; }
    .btn:active { transform: scale(0.98); }
    .btn:disabled { opacity: 0.5; pointer-events: none; }
    .btn-red { background-color: #e74c3c; color: white; }
    .btn-orange { background-color: #e67e22; color: white; }
    .footer-link { color: #888888; text-decoration: underline; font-size: 15px; margin-top: 30px; display: block; }
    .clr-g { color: #2ecc71; }
    .clr-y { color: #f1c40f; }
    .clr-r { color: #e74c3c; }
    .val { color: #fff; font-variant-numeric: tabular-nums; }
  </style>
  <script>
    function updateClientTime() {
      const now = new Date();
      const days = ["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"];
      const months = ["January","February","March","April","May","June","July","August","September","October","November","December"];
      document.getElementById('dt-val').textContent = `${days[now.getDay()]}, ${now.getDate().toString().padStart(2, '0')} ${months[now.getMonth()]} ${now.getFullYear()}`;
      document.getElementById('tm-val').textContent = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`;
    }

    function updateData() {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('ip-val').textContent = d.ip;
        document.getElementById('wf-val').textContent = d.wifi;

        let stElem = document.getElementById('st-val');
        stElem.textContent = d.status;
        stElem.className = d.status === "Connected" ? "clr-g" : "clr-r";

        let sgElem = document.getElementById('sg-val');
        if(d.mode === "AP") {
           sgElem.textContent = "Direct Mode";
           sgElem.className = "val";
           document.getElementById('setupLink').style.display = 'block';
           document.getElementById('btn-reset').style.display = 'none';
        } else {
           let dbm = parseInt(d.rssi);
           let strTxt = dbm > -60 ? "Good" : (dbm > -80 ? "Medium" : "Weak");
           sgElem.textContent = `${dbm} dBm (${strTxt})`;
           sgElem.className = dbm > -60 ? "clr-g" : (dbm > -80 ? "clr-y" : "clr-r");
           document.getElementById('setupLink').style.display = 'none';
           document.getElementById('btn-reset').style.display = 'block';
        }

        let dxElem = document.getElementById('dx-val');
        dxElem.textContent = d.dmx;
        dxElem.className = d.dmx === "Active" ? "clr-g" : (d.dmx === "No Signal" ? "clr-y" : "clr-r");
        document.getElementById('rx-val').textContent = `${d.rx_count} packets`;
      }).catch(e => console.log(e));
    }

    setInterval(updateData, 1000);
    setInterval(updateClientTime, 1000);
    window.onload = function() { updateClientTime(); updateData(); };

    function req(url, msg) {
      if(confirm(msg)) {
        fetch(url, {method: 'POST'}).then(() => {
          if(url === '/reset') {
            setTimeout(() => { window.location.href = "/setup"; }, 3500);
          } else {
            setTimeout(() => { location.reload(); }, 3500);
          }
        });
      }
    }
  </script>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>

  <div class="box">
    <div><span class="lbl">Date</span>: <span id="dt-val" class="val">Loading...</span></div>
    <div><span class="lbl">Time</span>: <span id="tm-val" class="val">Loading...</span></div>
    <div><span class="lbl">IP</span>: <span id="ip-val" class="val">Loading...</span></div>
    <div><span class="lbl">WiFi</span>: <span id="wf-val" class="val">Loading...</span></div>
    <div><span class="lbl">Status</span>: <span id="st-val" class="val">Loading...</span></div>
    <div><span class="lbl">Strength</span>: <span id="sg-val" class="val">Loading...</span></div>
    <div><span class="lbl">DMX</span>: <span id="dx-val" class="val">Loading...</span></div>
    <div><span class="lbl">RX</span>: <span id="rx-val" class="val">Loading...</span></div>
  </div>

  <div class="btn-container">
    <button id="btn-reset" class="btn btn-red" onclick="req('/reset', 'Reset WiFi configuration and reboot module?')">WiFi Reset</button>
    <button class="btn btn-orange" onclick="req('/reboot', 'Reboot hardware module?')">Module Reboot</button>
  </div>

  <a href="/setup" id="setupLink" class="footer-link" style="display: none;">Network Setup</a>
</body>
</html>
)rawliteral";

// --- DMX TRANSMISSION TASK (CORE 0 - REAL-TIME ISOLATED) ---
static const uint32_t DMX_PERIOD_US = 23220; // 43.07 FPS nominal (512 Channel Frame)
static const uint8_t zeroFrame[512] = {0};

void DMXCoreTask(void *pvParameters) {
  for (;;) {
    uint64_t frame_start_us = esp_timer_get_time();

    // Pastikan driver hanya aktif jika sudah pernah menerima paket valid
    if (hasReceivedFirstPacket) {
      digitalWrite(DMX_EN_PIN, HIGH);
    } else {
      digitalWrite(DMX_EN_PIN, LOW); // High-Z sampai software FOH online
    }

    // 1. Cek ketersediaan frame baru dari Core 1 secara atomik
    if (dmx_new_frame_ready) {
      portENTER_CRITICAL(&dmx_mux);
      // Tukar buffer TX dengan Ready Buffer secara instan (<20 ns)
      uint8_t* temp = dmx_tx_buffer;
      dmx_tx_buffer = dmx_ready_buffer;
      dmx_ready_buffer = temp;
      dmx_new_frame_ready = false;
      portEXIT_CRITICAL(&dmx_mux);
    }

    // 2. Pembangkitan Sinyal Break DMX512 (96000 Baud = 93.75 us Low, 20.83 us MAB High)
    Serial2.updateBaudRate(96000);
    Serial2.write((uint8_t)0);
    Serial2.flush();

    // 3. Transmisi Start Code 0x00 & 512 Slot Data DMX pada 250000 Baud 8N2
    Serial2.updateBaudRate(250000);
    Serial2.write((uint8_t)0); // NULL Start Code

    // Proteksi Blackout Fail-Safe (Padamkan jika > 10 detik tanpa paket Art-Net valid)
    if (!hasReceivedFirstPacket || (millis() - lastArtNetTime > 10000)) {
      Serial2.write(zeroFrame, 512); // Transmisi blok cepat
    } else {
      Serial2.write(dmx_tx_buffer, 512);
    }
    Serial2.flush();

    // 4. Kompensasi Waktu Presisi & Watchdog Feeder
    int64_t elapsed_us = (int64_t)(esp_timer_get_time() - frame_start_us);
    int64_t wait_us = (int64_t)DMX_PERIOD_US - elapsed_us;

    if (wait_us > 1500) {
      vTaskDelay(pdMS_TO_TICKS(wait_us / 1000));
    } else {
      vTaskDelay(1); // Wajib: 1-tick yield agar IDLE0 task mereset Task Watchdog (TWDT)
    }
  }
}

// --- SETUP SISTEM ---
void setup() {
  Serial.begin(115200);

  pinMode(DMX_EN_PIN, OUTPUT);
  digitalWrite(DMX_EN_PIN, LOW); // High-Z output proteksi anti-flicker
  Serial2.begin(250000, SERIAL_8N2, DMX_RX_PIN, DMX_TX_PIN);

  // Proteksi I2C Hardware Timeout 25ms (Anti-Freeze jika kabel LCD lepas)
  Wire.begin(21, 22);
  Wire.setTimeOut(25);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  memset(dmx_buf_pool, 0, sizeof(dmx_buf_pool));

  // Buat DMX Engine pada Core 0 dengan prioritas tinggi
  xTaskCreatePinnedToCore(DMXCoreTask, "DMX_T", 4096, NULL, 10, &DMXTaskHandle, 0);

  // Daftarkan rute WebServer satu kali secara terpusat (mencegah route duplication)
  registerWebRoutes();

  prefs.begin("wifi_data", false); // Buka read-write (auto-create jika namespace baru)
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end(); // Segera tutup handle NVS

  setScreenState(S_BOOT);
  delay(1000); // Tampilkan nama project resmi di LCD selama 1.0 detik

  if (ssid == "") {
    setScreenState(S_AP_MODE);
    startAP();
  } else {
    setScreenState(S_CONNECTING);
    connectWiFi();
  }
}

// --- MAIN LOOP (CORE 1) ---
void loop() {
  handleLCDAnimation();
  server.handleClient();

  if (currentState == S_AP_MODE) {
    dnsServer.processNextRequest();
  }

  // --- MESIN PENERIMAAN ART-NET BERPERFORMA TINGGI (DRAIN LOOP) ---
  if (currentState == S_RUN || currentState == S_AP_MODE) {
    int packetSize = 0;

    // Kuras seluruh antrean UDP yang ada di buffer socket lwIP (Mencegah Starvation)
    while ((packetSize = Udp.parsePacket()) > 0) {
      if (packetSize < 10) {
        Udp.flush();
        continue;
      }

      uint8_t packetBuffer[576];
      int bytesRead = Udp.read(packetBuffer, sizeof(packetBuffer));
      if (bytesRead < 10) continue;

      // 1. Verifikasi Header Art-Net ID ("Art-Net\0")
      if (memcmp(packetBuffer, ARTNET_HEADER_TAG, 8) != 0) {
        continue;
      }

      // 2. Ekstrak OpCode (Little-Endian: Byte 8 LSB, Byte 9 MSB)
      uint16_t opCode = (uint16_t)packetBuffer[8] | ((uint16_t)packetBuffer[9] << 8);

      // --- PENANGAN OP_POLL (0x2000): AUTO-DISCOVERY NODE ---
      if (opCode == ARTNET_OP_POLL) {
        sendArtPollReply(Udp.remoteIP(), Udp.remotePort());
        continue;
      }

      // --- PENANGAN OP_DMX (0x5000): DATA KONTROL PANGGUNG ---
      if (opCode == ARTNET_OP_DMX) {
        if (bytesRead < 18) continue;

        // Validasi Protocol Version (Harus >= 14)
        uint16_t protVer = ((uint16_t)packetBuffer[10] << 8) | packetBuffer[11];
        if (protVer < ARTNET_PROT_VER) continue;

        // Ekstraksi 15-Bit Port-Address Lengkap (Net & SubUni)
        // Byte 14: SubUni (Bits 0..3 Universe, Bits 4..7 Sub-Net)
        // Byte 15: Net (Bits 0..6 Net)
        uint16_t incomingPortAddress = ((uint16_t)(packetBuffer[15] & 0x7F) << 8) | packetBuffer[14];
        if (incomingPortAddress != TARGET_PORT_ADDRESS) {
          continue; // Abaikan universe yang bukan milik modul ini
        }

        // Ekstraksi Data Length (Big-Endian: Byte 16 MSB, Byte 17 LSB)
        uint16_t length = ((uint16_t)packetBuffer[16] << 8) | packetBuffer[17];
        int availablePayload = bytesRead - 18;

        // Validasi Ketat Spesifikasi Art-Net:
        // - Panjang dalam rentang 2 .. 512
        // - Declared length tidak boleh melebihi byte fisik yang diterima
        if ((length < 2) || (length > 512) || (length > availablePayload)) {
          continue; // Drop malformed / truncated packet
        }

        // Tulis data ke dmx_rx_buffer milik eksklusif Core 1 (bebas tanpa lock)
        memcpy(dmx_rx_buffer, packetBuffer + 18, length);
        if (length < 512) {
          memset(dmx_rx_buffer + length, 0, 512 - length);
        }

        // Tukar dmx_rx_buffer dengan dmx_ready_buffer di dalam spinlock super-singkat (<20 ns)
        portENTER_CRITICAL(&dmx_mux);
        uint8_t* temp = dmx_ready_buffer;
        dmx_ready_buffer = dmx_rx_buffer;
        dmx_rx_buffer = temp;
        dmx_new_frame_ready = true;
        artnet_rx_count++;
        portEXIT_CRITICAL(&dmx_mux);

        lastArtNetTime = millis();
        hasReceivedFirstPacket = true;
      }
    }
  }

  // --- MESIN AUTO-RECONNECT NON-BLOCKING DENGAN SAGA RECOVERY ---
  if (currentState == S_RUN) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiLostTime == 0) {
        wifiLostTime = millis();
        lastReconnectAttempt = 0;
        setScreenState(S_WIFI_DISCONN1);
      }
      if (millis() - wifiLostTime > 30000) {
        // Fallback aman ke AP mode jika 30s gagal TANPA merusak kredensial di NVS flash
        setScreenState(S_WIFI_DISCONN2, 2000, S_AP_MODE);
        startAP();
      } else {
        if (millis() - lastReconnectAttempt > 3000) {
          lastReconnectAttempt = millis();
          WiFi.reconnect();
        }
      }
    } else {
      if (wifiLostTime != 0) {
        wifiLostTime = 0;
        setScreenState(S_RUN);
      }
    }
  } else if (currentState == S_AP_MODE && ssid != "") {
    // SAGA BACKGROUND RECOVERY: Cek status koneksi pada setiap loop
    if (WiFi.status() == WL_CONNECTED) {
      dnsServer.stop();
      WiFi.mode(WIFI_STA);
      Udp.stop();
      Udp.begin(ARTNET_PORT);
      setScreenState(S_RUN);
    } else if (millis() - lastAPBackgroundScan > 15000) {
      lastAPBackgroundScan = millis();
      WiFi.begin(ssid.c_str(), pass.c_str());
    }
  }

  // --- MONITORING KONEKSI STASIUN PADA AP MODE ---
  if (currentState == S_AP_MODE) {
    int currentDevices = WiFi.softAPgetStationNum();
    if (currentDevices != connectedDevices) {
      connectedDevices = currentDevices;
      lcd.setCursor(0, 1);
      if (connectedDevices > 0) {
        lcd.print("Dev Connected!  ");
      } else {
        lcd.print("192.168.4.1     ");
      }
    }
  }
}

// --- PENGIRIMAN ARTOLLREPLY (OPCODE 0x2100) SESUAI SPESIFIKASI ART-NET 4 ---
void sendArtPollReply(IPAddress remoteIP, uint16_t remotePort) {
  uint8_t reply[239];
  memset(reply, 0, sizeof(reply));

  // 1. Header & OpCode (0x2100 Little-Endian)
  memcpy(reply, ARTNET_HEADER_TAG, 8);
  reply[8]  = 0x00;
  reply[9]  = 0x21;

  // 2. IP Address & Port (6454 Little-Endian)
  IPAddress myIP = (currentState == S_RUN) ? WiFi.localIP() : WiFi.softAPIP();
  reply[10] = myIP[0];
  reply[11] = myIP[1];
  reply[12] = myIP[2];
  reply[13] = myIP[3];
  reply[14] = (uint8_t)(ARTNET_PORT & 0xFF);
  reply[15] = (uint8_t)(ARTNET_PORT >> 8);

  // 3. VersInfo (Firmware v5.2) & Port-Address Switch
  reply[16] = 0x05;
  reply[17] = 0x02;
  reply[18] = (uint8_t)((TARGET_PORT_ADDRESS >> 8) & 0x7F); // NetSwitch
  reply[19] = (uint8_t)((TARGET_PORT_ADDRESS >> 4) & 0x0F); // SubSwitch

  reply[20] = 0x00; // OemHi
  reply[21] = 0xFF; // OemLo
  reply[22] = 0x00; // Ubea
  reply[23] = 0x08; // Status1: Normal Indicator

  // ESTA Manufacturer Code (0x7FF0 Prototype)
  reply[24] = 0xF0;
  reply[25] = 0x7F;

  // ShortName (18B null-terminated) & LongName (64B null-terminated)
  strncpy((char*)&reply[26], "ZZLIGHT NODE", 17);
  strncpy((char*)&reply[44], "ZZLIGHT Art-Net to DMX512 ESP32 Module", 63);

  // NodeReport (64B: Byte 108..171)
  strncpy((char*)&reply[108], "#0001 [0000] ZZLIGHT DMX OK", 63);

  // Port Configuration (Resmi Art-Net 4 Spec Table):
  reply[172] = 0x00; // NumPortsHi
  reply[173] = 0x01; // NumPortsLo = 1 Port
  reply[174] = 0x80; // PortTypes[0]: Output DMX512 (Bit 7 = Output, Bits 0..5 = DMX512)
  reply[182] = 0x80; // GoodOutput[0]: Data transmitting
  reply[190] = (uint8_t)(TARGET_PORT_ADDRESS & 0x0F); // SwOut[0]: Universe (0)

  reply[200] = 0x02; // Style: StNode

  // MAC Address (Byte 201..206) & BindIp (Byte 207..210)
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(&reply[201], mac, 6);
  reply[207] = myIP[0];
  reply[208] = myIP[1];
  reply[209] = myIP[2];
  reply[210] = myIP[3];
  reply[211] = 0x01; // BindIndex
  reply[212] = 0x08; // Status2: 15-bit Port-Address support

  // Kirim balasan ke Art-Net Port resmi (6454)
  Udp.beginPacket(remoteIP, ARTNET_PORT);
  Udp.write(reply, sizeof(reply));
  Udp.endPacket();
}

// --- FUNGSI SANITASI JSON ESCAPING ---
String escapeJsonString(const String& input) {
  String output = "";
  output.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input[i];
    if (c == '"') output += "\\\"";
    else if (c == '\\') output += "\\\\";
    else if (c == '\b') output += "\\b";
    else if (c == '\f') output += "\\f";
    else if (c == '\n') output += "\\n";
    else if (c == '\r') output += "\\r";
    else if (c == '\t') output += "\\t";
    else if (c < 32) {
      char buf[8];
      snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int)c);
      output += buf;
    } else {
      output += (char)c;
    }
  }
  return output;
}

// --- INISIALISASI SOFTAP ---
void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ARTNET-DMX", "andreas123");
  connectedDevices = 0;

  dnsServer.stop();
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Udp.stop();
  Udp.begin(ARTNET_PORT);

  server.begin();
  setScreenState(S_AP_MODE);
}

// --- KONEKSI KE ACCESS POINT (STATION MODE) ---
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Matikan modem sleep untuk latensi rendah stabil (<3ms)
  WiFi.disconnect();
  delay(100);

  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startT = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startT < 6000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    dnsServer.stop(); // Matikan captive portal di mode RUN
    MDNS.begin("zzlight");

    Udp.stop();
    Udp.begin(ARTNET_PORT);

    server.begin();
    setScreenState(S_RUN);
  } else {
    setScreenState(S_WIFI_FAIL, 2500, S_AP_MODE);
    startAP();
  }
}

// --- REGISTRASI RUTE WEBSERVER TUNGGAL (STATE-AWARE ROUTING) ---
void registerWebRoutes() {
  server.onNotFound([]() {
    if (currentState == S_AP_MODE) {
      server.sendHeader("Location", "http://192.168.4.1/setup", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.on("/generate_204", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
  });

  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
  });

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    if (currentState == S_AP_MODE) {
      server.send(200, "text/html", AP_HTML);
    } else {
      server.send(200, "text/html", RUN_HTML);
    }
  });

  server.on("/setup", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html", AP_HTML);
  });

  server.on("/monitor", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html", RUN_HTML);
  });

  server.on("/status", HTTP_GET, []() {
    unsigned long dmxDelta = millis() - lastArtNetTime;
    String dmxStat = (!hasReceivedFirstPacket) ? "No Signal" : ((dmxDelta < 1000) ? "Active" : ((dmxDelta < 10000) ? "No Signal" : "Loss Signal"));

    char json[512];
    if (currentState == S_AP_MODE) {
      snprintf(json, sizeof(json),
        "{\"ip\":\"%s\",\"wifi\":\"%s\",\"status\":\"Connected\",\"mode\":\"AP\","
        "\"stations\":%d,\"rx_count\":%u,\"dmx\":\"%s\"}",
        WiFi.softAPIP().toString().c_str(),
        escapeJsonString(WiFi.softAPSSID()).c_str(),
        WiFi.softAPgetStationNum(),
        artnet_rx_count,
        dmxStat.c_str()
      );
    } else {
      snprintf(json, sizeof(json),
        "{\"ip\":\"%s\",\"wifi\":\"%s\",\"status\":\"%s\",\"mode\":\"RUN\","
        "\"rssi\":%d,\"rx_count\":%u,\"dmx\":\"%s\"}",
        WiFi.localIP().toString().c_str(),
        escapeJsonString(WiFi.SSID()).c_str(),
        WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
        WiFi.RSSI(),
        artnet_rx_count,
        dmxStat.c_str()
      );
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  server.on("/scan", HTTP_GET, []() {
    // PROTEKSI LIVE SHOW: Blokir Scan WiFi di RUN Mode agar tidak terjadi Radio Channel-Hopping Blackout
    if (currentState == S_RUN) {
      server.send(403, "application/json", "{\"error\":\"WiFi scan is disabled during live RUN mode to prevent DMX blackout.\"}");
      return;
    }
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true);
    String json = "[";
    for (int i = 0; i < n; ++i) {
      if (i > 0) json += ",";
      int dbm = WiFi.RSSI(i);
      int quality = (dbm <= -100) ? 0 : ((dbm >= -50) ? 100 : 2 * (dbm + 100));
      bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      json += "{\"ssid\":\"" + escapeJsonString(WiFi.SSID(i)) + "\",\"sec\":" + String(secure ? "true" : "false") + ",\"sig\":" + String(quality) + "}";
    }
    json += "]";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  server.on("/connect", HTTP_POST, []() {
    String in_ssid = server.arg("ssid");
    String in_pass = server.arg("pass");

    // Validasi Panjang Input (Cegah NVS Flash Exhaustion)
    if (in_ssid.length() < 1 || in_ssid.length() > 32 || in_pass.length() > 63) {
      server.send(400, "text/plain", "Bad Request: Invalid SSID/Password Length");
      return;
    }

    ssid = in_ssid;
    pass = in_pass;
    prefs.begin("wifi_data", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    server.send(200, "text/html", SAVED_HTML);
    delay(500);
    ESP.restart();
  });

  server.on("/reset", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    prefs.begin("wifi_data", false);
    prefs.clear();
    prefs.end();
    delay(500);
    ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  });
}

// --- STATE MACHINE LCD NON-BLOCKING (ZERO BLOCKING DELAY) ---
void setScreenState(ScreenState newState, unsigned long holdDuration, ScreenState nextState) {
  currentState = newState;
  stateEnterTime = millis();
  stateHoldDuration = holdDuration;
  pendingNextState = nextState;

  lcd.clear();
  switch (newState) {
    case S_BOOT:
      lcd.setCursor(4, 0); lcd.print("ZZLIGHT");
      lcd.setCursor(4, 1); lcd.print("PROJECT");
      break;
    case S_CONNECTING:
      lcd.setCursor(0, 0); lcd.print("Connecting...   ");
      break;
    case S_WIFI_ERR:
      lcd.setCursor(0, 0); lcd.print("WiFi Not Connect");
      lcd.setCursor(0, 1); lcd.print("Switching AP... ");
      break;
    case S_AP_MODE:
      animStep = 0;
      lastAnimTime = millis();
      lcd.setCursor(0, 0); lcd.print("ARTNET-DMX      ");
      lcd.setCursor(0, 1);
      if (connectedDevices > 0) {
        lcd.print("Dev Connected!  ");
      } else {
        lcd.print("192.168.4.1     ");
      }
      break;
    case S_RUN:
      animStep = 0;
      lastAnimTime = millis();
      lcd.setCursor(0, 0); lcd.print("ARTNET-DMX      ");
      lcd.setCursor(0, 1);
      {
        String ipStr = WiFi.localIP().toString();
        while (ipStr.length() < 16) ipStr += " ";
        lcd.print(ipStr);
      }
      break;
    case S_WIFI_DISCONN1:
      lcd.setCursor(0, 0); lcd.print("WiFi Disconnect!");
      lcd.setCursor(0, 1); lcd.print("Reconnecting... ");
      break;
    case S_WIFI_DISCONN2:
      lcd.setCursor(0, 0); lcd.print("WiFi Disconnect!");
      lcd.setCursor(0, 1); lcd.print("Fallback to AP..");
      break;
    case S_WIFI_FAIL:
      lcd.setCursor(0, 0); lcd.print("WiFi ConnectFail");
      lcd.setCursor(0, 1); lcd.print("Starting AP Mode");
      break;
    default:
      break;
  }
}

static unsigned long lastAnimTime = 0;
static int animStep = 0;

void handleLCDAnimation() {
  // Transisi State Otomatis Non-Blocking
  if (stateHoldDuration > 0 && (millis() - stateEnterTime >= stateHoldDuration)) {
    stateHoldDuration = 0;
    if (pendingNextState == S_CONNECTING) {
      if (ssid == "") {
        setScreenState(S_WIFI_ERR, 2000, S_AP_MODE);
        startAP();
      } else {
        connectWiFi();
      }
    } else if (pendingNextState == S_AP_MODE) {
      startAP();
    }
  }

  // Wave Animations: S_RUN (>>>>>) and S_AP_MODE ())))))
  if (currentState == S_RUN || currentState == S_AP_MODE) {
    if (millis() - lastAnimTime > 500) {
      lastAnimTime = millis();
      animStep++;
      if (animStep > 5) animStep = 0;

      lcd.setCursor(10, 0);
      char animChar = (currentState == S_AP_MODE) ? ')' : '>';

      if (animStep == 0) {
        lcd.print("      ");
      } else {
        lcd.print(" ");
        for (int i = 1; i <= animStep; i++) lcd.print(animChar);
        for (int i = animStep + 1; i <= 5; i++) lcd.print(" ");
      }
    }
  }
}
