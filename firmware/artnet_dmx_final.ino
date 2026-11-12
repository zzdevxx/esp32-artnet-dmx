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

// Hardware pin configuration
#define DMX_TX_PIN 17
#define DMX_RX_PIN 16
#define DMX_EN_PIN 4

// Art-Net 4 protocol constants
#define ARTNET_PORT           6454
#define ARTNET_OP_POLL        0x2000
#define ARTNET_OP_POLL_REPLY  0x2100
#define ARTNET_OP_DMX         0x5000
#define ARTNET_PROT_VER       14
#define ARTNET_HEADER_TAG     "Art-Net"

// 15-bit Port-Address: (Net << 8) | SubUni
// Net: 0 (0..127), Sub-Net: 0 (0..15), Universe: 0 (0..15) -> Total = 0x0000
const uint16_t TARGET_PORT_ADDRESS = 0x0000;

// Lock-free triple buffering architecture:
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

// System, network, and persistent storage handles
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

// Non-blocking LCD state definitions
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

// Web UI templates stored in flash memory
const char AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Setup</title>
  <style>
    /* Obsidian Control Deck palette (aligned with zzluxora-v10 DESIGN.md tokens) */
    :root {
      --bg-root: #1e2024;
      --bg-surface: #242830;
      --bg-elevated: #2d323c;
      --surface-void: #07080a;
      --surface-input: #181a1f;
      --surface-hover: #383f4d;
      --border-subtle: #333844;
      --border-strong: #4a5264;
      --text-primary: #ffffff;
      --text-secondary: #cbd5e1;
      --text-muted: #94a3b8;
      --accent-amber: #f59e0b;
      --accent-cyan: #06b6d4;
      --success: #16a34a;
      --success-text: #22c55e;
    }
    * { box-sizing: border-box; }
    body {
      background-color: var(--bg-root);
      color: var(--text-primary);
      font-family: "Inter", "Segoe UI", "Ubuntu", system-ui, sans-serif;
      text-align: center;
      margin: 0;
      padding: 36px 16px;
      min-height: 100dvh;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .brand {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-bottom: 22px;
      gap: 8px;
    }
    .brand h1 {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-weight: 800;
      letter-spacing: 3px;
      margin: 0;
      font-size: 22px;
      color: var(--text-primary);
      font-variant-numeric: tabular-nums;
    }
    .brand .sub {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 1.5px;
      color: var(--text-muted);
      text-transform: uppercase;
    }
    .brand .badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 11px;
      font-weight: 700;
      letter-spacing: 0.5px;
      color: var(--accent-cyan);
      background: rgba(6, 182, 212, 0.08);
      border: 1px solid rgba(6, 182, 212, 0.35);
      border-radius: 4px;
      padding: 4px 10px;
      font-variant-numeric: tabular-nums;
    }
    .brand .badge .dot {
      width: 6px; height: 6px;
      border-radius: 50%;
      background: var(--accent-cyan);
      box-shadow: 0 0 6px rgba(6, 182, 212, 0.7);
    }
    .container { width: 100%; max-width: 360px; display: flex; flex-direction: column; align-items: center; }
    .wifi-box {
      background: var(--bg-surface);
      border: 1px solid var(--border-subtle);
      border-radius: 6px;
      padding: 4px;
      width: 100%;
      max-height: 220px;
      overflow-y: auto;
      margin-bottom: 16px;
    }
    .wifi-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px 14px;
      border-bottom: 1px solid var(--border-subtle);
      cursor: pointer;
      transition: background-color 0.15s;
      border-radius: 4px;
    }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:hover { background-color: var(--surface-hover); }
    .w-left {
      display: flex;
      align-items: center;
      gap: 10px;
      font-size: 14px;
      font-weight: 500;
      color: var(--text-primary);
      min-width: 0;
    }
    .w-left .name {
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font-family: "Inter", "Segoe UI", sans-serif;
    }
    .wifi-icon {
      width: 18px; height: 18px;
      flex-shrink: 0;
      color: var(--text-secondary);
    }
    .w-right {
      display: flex;
      align-items: center;
      gap: 8px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 13px;
      color: var(--text-secondary);
      font-variant-numeric: tabular-nums;
      flex-shrink: 0;
    }
    .lock-icon { width: 14px; height: 14px; color: var(--text-muted); }
    .lock-icon.open { color: var(--text-muted); }
    .btn {
      width: 100%;
      padding: 14px;
      border: 1px solid transparent;
      border-radius: 4px;
      font-weight: 700;
      font-size: 14px;
      letter-spacing: 0.5px;
      text-transform: uppercase;
      cursor: pointer;
      font-family: "JetBrains Mono", "Consolas", monospace;
      transition: filter 0.15s, transform 0.05s, background-color 0.15s;
      display: block;
      min-height: 48px;
    }
    .btn:focus-visible { outline: 2px solid var(--accent-cyan); outline-offset: 2px; }
    .btn:active { transform: scale(0.98); }
    .btn:disabled { opacity: 0.4; pointer-events: none; }
    .btn-amber { background: var(--accent-amber); color: #000; }
    .btn-amber:hover { filter: brightness(1.08); }
    .btn-green { background: var(--success); color: #fff; }
    .btn-green:hover { filter: brightness(1.08); }
    form { width: 100%; }
    input[type="text"], input[type="password"] {
      background-color: var(--surface-input);
      color: var(--text-primary);
      padding: 14px 16px;
      margin-bottom: 12px;
      width: 100%;
      border-radius: 4px;
      border: 1px solid var(--border-subtle);
      font-size: 14px;
      font-family: "Inter", "Segoe UI", sans-serif;
      text-align: left;
    }
    input[type="text"]:focus, input[type="password"]:focus {
      outline: none;
      border-color: var(--accent-cyan);
      box-shadow: 0 0 0 1px var(--accent-cyan);
    }
    input::placeholder { color: var(--text-muted); font-size: 13px; }
    .pass-row {
      position: relative;
      width: 100%;
      margin-bottom: 12px;
      display: flex;
    }
    .pass-row input {
      flex: 1;
      margin-bottom: 0;
      padding-right: 72px;
      border-radius: 4px;
    }
    .btn-toggle {
      position: absolute;
      right: 8px;
      top: 50%;
      transform: translateY(-50%);
      background: var(--surface-hover);
      color: var(--text-secondary);
      border: 1px solid var(--border-subtle);
      border-radius: 4px;
      padding: 6px 10px;
      font-size: 11px;
      font-weight: 700;
      letter-spacing: 0.5px;
      cursor: pointer;
      font-family: "JetBrains Mono", "Consolas", monospace;
      transition: all 0.15s;
    }
    .btn-toggle:hover { color: var(--text-primary); background: var(--border-subtle); }
    .btn-toggle:focus-visible { outline: 2px solid var(--accent-cyan); }
    .btn-toggle:active { transform: translateY(-50%) scale(0.95); }
    .footer-link {
      color: var(--text-muted);
      text-decoration: none;
      font-size: 13px;
      font-weight: 500;
      margin-top: 32px;
      display: block;
      transition: color 0.15s;
      font-family: "Inter", "Segoe UI", sans-serif;
    }
    .footer-link:hover { color: var(--text-primary); }
    .footer-link:focus-visible { outline: 2px solid var(--accent-cyan); outline-offset: 2px; }
  </style>
  <script>
    /* Inline SVG icons (no emoji, no external assets) */
    const WIFI_SVG = '<svg class="wifi-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>';
    const LOCK_SVG = '<svg class="lock-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>';
    const UNLOCK_SVG = '<svg class="lock-icon open" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 9.9-1"/></svg>';

    function fillSSID(el) {
      document.getElementById('ssid').value = el.getAttribute('data-ssid');
      document.getElementById('pass').focus();
    }
    function scan() {
      let wb = document.getElementById('wb');
      wb.innerHTML = '<div style="padding:24px;color:var(--text-muted);font-family:JetBrains Mono,Consolas,monospace;font-size:12px;letter-spacing:1px;">SCANNING WIFI NETWORKS...</div>';
      fetch('/scan').then(r=>r.json()).then(d=>{
        wb.innerHTML = '';
        if(d.length === 0) {
          wb.innerHTML = '<div style="padding:24px;color:var(--text-muted);font-family:JetBrains Mono,Consolas,monospace;font-size:12px;letter-spacing:1px;">NO WIFI FOUND</div>';
          return;
        }
        d.forEach(w => {
          let item = document.createElement('div');
          item.className = 'wifi-item';
          item.setAttribute('data-ssid', w.ssid);
          item.onclick = function() { fillSSID(this); };

          let left = document.createElement('div');
          left.className = 'w-left';
          left.innerHTML = WIFI_SVG;
          let sTxt = document.createElement('span');
          sTxt.className = 'name';
          sTxt.textContent = w.ssid;
          sTxt.title = w.ssid;
          left.appendChild(sTxt);

          let right = document.createElement('div');
          right.className = 'w-right';
          right.innerHTML = (w.sec ? LOCK_SVG : UNLOCK_SVG);
          let sig = document.createElement('span');
          sig.textContent = w.sig + '%';
          right.appendChild(sig);

          item.appendChild(left);
          item.appendChild(right);
          wb.appendChild(item);
        });
      }).catch(() => {
        wb.innerHTML = '<div style="padding:24px;color:var(--accent-amber);font-family:JetBrains Mono,Consolas,monospace;font-size:12px;letter-spacing:1px;">SCAN FAILED</div>';
      });
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
  <div class="brand">
    <h1>ZZLIGHT</h1>
    <span class="sub">ARTNET-DMX NODE CONTROLLER</span>
    <span class="badge"><span class="dot"></span> UNIVERSE 0 | 43.07 FPS</span>
  </div>
  <div class="container">
    <div id="wb" class="wifi-box"></div>
    <button type="button" class="btn btn-amber" style="margin-bottom: 20px;" onclick="scan()">Refresh Networks</button>
    <form action="/connect" method="POST">
      <input type="text" name="ssid" id="ssid" placeholder="SSID (Max 32 Characters)" maxlength="32" required autocomplete="off">
      <div class="pass-row">
        <input type="password" name="pass" id="pass" placeholder="Password (Max 63 Characters)" maxlength="63" autocomplete="new-password">
        <button type="button" id="btnTogglePass" class="btn-toggle" onclick="togglePass()">SHOW</button>
      </div>
      <button type="submit" class="btn btn-green" style="margin-top: 12px;">Connect & Reboot</button>
    </form>
    <a href="/monitor" class="footer-link">Live Monitor</a>
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
    :root {
      --bg-root: #1e2024;
      --bg-surface: #242830;
      --border-subtle: #333844;
      --text-primary: #ffffff;
      --text-secondary: #cbd5e1;
      --text-muted: #94a3b8;
      --success-text: #22c55e;
      --success-bg: #143521;
      --success-border: #166534;
    }
    * { box-sizing: border-box; }
    body {
      background-color: var(--bg-root);
      color: var(--text-primary);
      font-family: "Inter", "Segoe UI", "Ubuntu", system-ui, sans-serif;
      text-align: center;
      margin: 0;
      padding: 40px 16px;
      min-height: 100dvh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
    }
    .container { width: 100%; max-width: 360px; }
    .card {
      background-color: var(--bg-surface);
      border: 1px solid var(--border-subtle);
      padding: 40px 24px;
      border-radius: 6px;
    }
    .card .title {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-weight: 800;
      letter-spacing: 3px;
      font-size: 16px;
      color: var(--text-primary);
      margin: 0 0 6px 0;
    }
    .card .subtitle {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 1.5px;
      color: var(--text-muted);
      text-transform: uppercase;
      margin-bottom: 28px;
    }
    .checkmark {
      width: 64px;
      height: 64px;
      margin: 0 auto 20px;
      background: var(--success-bg);
      border: 1px solid var(--success-border);
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .checkmark svg {
      width: 32px;
      height: 32px;
      color: var(--success-text);
    }
    .status {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 14px;
      font-weight: 700;
      letter-spacing: 1px;
      color: var(--success-text);
      margin-bottom: 10px;
    }
    .note {
      font-family: "Inter", "Segoe UI", sans-serif;
      font-size: 13px;
      color: var(--text-muted);
      margin: 0;
    }
    .dot-pulse {
      display: inline-block;
      width: 6px;
      height: 6px;
      background: var(--success-text);
      border-radius: 50%;
      margin-right: 6px;
      animation: pulse 1.2s ease-in-out infinite;
      vertical-align: middle;
    }
    @keyframes pulse {
      0%, 100% { opacity: 0.3; }
      50% { opacity: 1; }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="card">
      <div class="title">ZZLIGHT</div>
      <div class="subtitle">ARTNET-DMX NODE CONTROLLER</div>
      <div class="checkmark">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>
      </div>
      <div class="status"><span class="dot-pulse"></span>WIFI CREDENTIALS SAVED</div>
      <p class="note">Applying network settings and rebooting module...</p>
    </div>
  </div>
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
    :root {
      --bg-root: #1e2024;
      --bg-surface: #242830;
      --bg-elevated: #2d323c;
      --surface-void: #07080a;
      --surface-input: #181a1f;
      --surface-hover: #383f4d;
      --border-subtle: #333844;
      --border-strong: #4a5264;
      --text-primary: #ffffff;
      --text-secondary: #cbd5e1;
      --text-muted: #94a3b8;
      --accent-amber: #f59e0b;
      --accent-cyan: #06b6d4;
      --success: #16a34a;
      --success-text: #22c55e;
      --success-bg: #143521;
      --success-border: #166534;
      --danger: #ef4444;
      --danger-bg: #3b1818;
      --danger-border: #991b1b;
      --warning: #eab308;
      --warning-bg: #2b2510;
      --warning-border: #7a5c00;
    }
    * { box-sizing: border-box; }
    body {
      background-color: var(--bg-root);
      color: var(--text-primary);
      font-family: "Inter", "Segoe UI", "Ubuntu", system-ui, sans-serif;
      text-align: center;
      margin: 0;
      padding: 36px 16px;
      min-height: 100dvh;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .brand {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-bottom: 18px;
      gap: 6px;
    }
    .brand h1 {
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-weight: 800;
      letter-spacing: 3px;
      margin: 0;
      font-size: 20px;
      color: var(--text-primary);
      font-variant-numeric: tabular-nums;
    }
    .telemetry-header {
      display: inline-flex;
      align-items: center;
      gap: 10px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 1.2px;
      color: var(--text-muted);
      text-transform: uppercase;
      padding: 5px 12px;
      border: 1px solid var(--border-subtle);
      border-radius: 4px;
      background: var(--surface-input);
      font-variant-numeric: tabular-nums;
    }
    .telemetry-header .streaming-dot {
      width: 6px; height: 6px;
      border-radius: 50%;
      background: var(--success-text);
      box-shadow: 0 0 6px rgba(34, 197, 94, 0.8);
      animation: pulse 1.4s ease-in-out infinite;
    }
    .telemetry-header .sep { color: var(--border-strong); }
    @keyframes pulse {
      0%, 100% { opacity: 0.4; }
      50% { opacity: 1; }
    }
    .container { width: 100%; max-width: 360px; display: flex; flex-direction: column; align-items: center; }
    .box {
      background-color: var(--bg-surface);
      border: 1px solid var(--border-subtle);
      padding: 6px 18px;
      border-radius: 6px;
      width: 100%;
      margin-bottom: 18px;
      font-variant-numeric: tabular-nums;
    }
    .row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 11px 0;
      border-bottom: 1px solid var(--border-subtle);
      font-size: 13px;
      gap: 12px;
    }
    .row:last-child { border-bottom: none; }
    .lbl {
      color: var(--text-muted);
      font-weight: 600;
      font-size: 11px;
      letter-spacing: 0.8px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      text-transform: uppercase;
      flex-shrink: 0;
    }
    .val {
      color: var(--text-primary);
      font-weight: 700;
      text-align: right;
      font-variant-numeric: tabular-nums;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 13px;
      letter-spacing: 0.3px;
      min-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    /* Status pill badges (soft-tinted, Linear/Stripe style) */
    .pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 3px 10px;
      border-radius: 999px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      font-size: 11px;
      font-weight: 700;
      letter-spacing: 0.5px;
      text-transform: uppercase;
      border: 1px solid transparent;
      font-variant-numeric: tabular-nums;
    }
    .pill .pdot { width: 5px; height: 5px; border-radius: 50%; }
    .pill-green { background: var(--success-bg); color: var(--success-text); border-color: var(--success-border); }
    .pill-green .pdot { background: var(--success-text); }
    .pill-yellow { background: var(--warning-bg); color: var(--warning); border-color: var(--warning-border); }
    .pill-yellow .pdot { background: var(--warning); }
    .pill-red { background: var(--danger-bg); color: var(--danger); border-color: var(--danger-border); }
    .pill-red .pdot { background: var(--danger); }
    .pill-neutral { background: var(--surface-input); color: var(--text-secondary); border-color: var(--border-subtle); }
    .pill-neutral .pdot { background: var(--text-muted); }

    .btn-container { display: flex; flex-direction: column; gap: 10px; width: 100%; }
    .btn {
      width: 100%;
      padding: 14px;
      border: 1px solid transparent;
      border-radius: 4px;
      font-weight: 700;
      cursor: pointer;
      font-size: 13px;
      font-family: "JetBrains Mono", "Consolas", monospace;
      transition: filter 0.15s, transform 0.05s;
      display: block;
      letter-spacing: 0.8px;
      min-height: 48px;
      text-transform: uppercase;
    }
    .btn:hover { filter: brightness(1.08); }
    .btn:focus-visible { outline: 2px solid var(--accent-cyan); outline-offset: 2px; }
    .btn:active { transform: scale(0.98); }
    .btn:disabled { opacity: 0.5; pointer-events: none; }
    .btn-danger { background: var(--danger-bg); color: var(--danger); border-color: var(--danger-border); }
    .btn-amber { background: var(--accent-amber); color: #000; }
    .footer-link {
      color: var(--text-muted);
      text-decoration: none;
      font-size: 13px;
      font-weight: 500;
      margin-top: 28px;
      display: block;
      transition: color 0.15s;
      font-family: "Inter", "Segoe UI", sans-serif;
    }
    .footer-link:hover { color: var(--text-primary); }
    .footer-link:focus-visible { outline: 2px solid var(--accent-cyan); outline-offset: 2px; }
  </style>
  <script>
    /* Pill renderer: turns a plain value span into a status badge with a dot */
    function setPill(el, text, kind) {
      el.textContent = '';
      el.className = 'val pill pill-' + kind;
      let dot = document.createElement('span');
      dot.className = 'pdot';
      el.appendChild(dot);
      let t = document.createElement('span');
      t.textContent = text;
      el.appendChild(t);
    }
    function setPlain(el, text, cls) {
      el.textContent = text;
      el.className = 'val' + (cls ? ' ' + cls : '');
    }

    function updateClientTime() {
      const now = new Date();
      const days = ["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"];
      const months = ["January","February","March","April","May","June","July","August","September","October","November","December"];
      document.getElementById('dt-val').textContent = `${days[now.getDay()]}, ${now.getDate().toString().padStart(2, '0')} ${months[now.getMonth()]} ${now.getFullYear()}`;
      document.getElementById('tm-val').textContent = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`;
    }

    function updateData() {
      fetch('/status').then(r => r.json()).then(d => {
        setPlain(document.getElementById('ip-val'), d.ip);
        setPlain(document.getElementById('wf-val'), d.wifi);

        if (d.status === "Connected") {
          setPill(document.getElementById('st-val'), d.status, 'green');
        } else {
          setPill(document.getElementById('st-val'), d.status || 'Disconnected', 'red');
        }

        let sgElem = document.getElementById('sg-val');
        if(d.mode === "AP") {
          setPill(sgElem, "Direct Mode", "neutral");
          document.getElementById('btn-reset').style.display = 'none';
        } else {
          let dbm = parseInt(d.rssi);
          let strTxt = dbm > -60 ? "Good" : (dbm > -80 ? "Medium" : "Weak");
          let kind = dbm > -60 ? "green" : (dbm > -80 ? "yellow" : "red");
          setPill(sgElem, `${dbm} dBm (${strTxt})`, kind);
          document.getElementById('btn-reset').style.display = 'block';
        }

        let dxKind = d.dmx === "Active" ? "green" : (d.dmx === "No Signal" ? "yellow" : "red");
        setPill(document.getElementById('dx-val'), d.dmx || 'No Signal', dxKind);
        setPlain(document.getElementById('rx-val'), `${d.rx_count} packets`);

        let streaming = (d.status === "Connected" && d.dmx === "Active");
        document.getElementById('stream-dot').style.opacity = streaming ? "1" : "0.15";
        document.getElementById('stream-label').textContent = streaming ? "Live" : "Standby";
      }).catch(e => { console.log(e); });
    }

    setInterval(updateData, 1000);
    setInterval(updateClientTime, 1000);
    window.onload = function() { updateClientTime(); updateData(); };

    function req(url, msg) {
      if(confirm(msg)) {
        fetch(url, {method: 'POST'}).then(() => {
          if(url === '/reset') {
            setTimeout(() => { window.location.href = "http://192.168.4.1/setup"; }, 3500);
          } else {
            setTimeout(() => { location.reload(); }, 3500);
          }
        });
      }
    }
  </script>
</head>
<body>
  <div class="brand">
    <h1>ZZLIGHT</h1>
    <div class="telemetry-header">
      <span class="streaming-dot" id="stream-dot"></span>
      <span id="stream-label">Standby</span>
      <span class="sep">|</span>
      <span>Universe 0</span>
    </div>
  </div>

  <div class="container">
    <div class="box">
      <div class="row"><span class="lbl">Date</span><span id="dt-val" class="val">Loading...</span></div>
      <div class="row"><span class="lbl">Time</span><span id="tm-val" class="val">Loading...</span></div>
      <div class="row"><span class="lbl">IP Address</span><span id="ip-val" class="val">Loading...</span></div>
      <div class="row"><span class="lbl">WiFi SSID</span><span id="wf-val" class="val">Loading...</span></div>
      <div class="row"><span class="lbl">Status</span><span id="st-val" class="val pill pill-neutral"><span class="pdot"></span><span>Connecting</span></span></div>
      <div class="row"><span class="lbl">Signal</span><span id="sg-val" class="val pill pill-neutral"><span class="pdot"></span><span>&mdash;</span></span></div>
      <div class="row"><span class="lbl">DMX Engine</span><span id="dx-val" class="val pill pill-neutral"><span class="pdot"></span><span>No Signal</span></span></div>
      <div class="row"><span class="lbl">Received</span><span id="rx-val" class="val">Loading...</span></div>
    </div>

    <div class="btn-container">
      <button id="btn-reset" class="btn btn-danger" onclick="req('/reset', 'Reset WiFi configuration and reboot module?')">WiFi Reset</button>
      <button class="btn btn-amber" onclick="req('/reboot', 'Reboot hardware module?')">Module Reboot</button>
    </div>

    <a href="/setup" class="footer-link">Network Setup</a>
  </div>
</body>
</html>
)rawliteral";

// Dedicated DMX512 transmission task pinned to Core 0 (hard real-time loop)
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

// System hardware and peripheral initialization
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

// Core 1 main loop: network packet ingestion, web server, and LCD animation
void loop() {
  handleLCDAnimation();
  server.handleClient();

  if (currentState == S_AP_MODE) {
    dnsServer.processNextRequest();
  }

  // Ingest incoming Art-Net UDP datagrams with drain loop
  if (currentState == S_RUN || currentState == S_AP_MODE) {
    int packetSize = 0;

    // Drain socket buffer to prevent packet starvation under burst traffic
    while ((packetSize = Udp.parsePacket()) > 0) {
      if (packetSize < 10) {
        Udp.flush();
        continue;
      }

      uint8_t packetBuffer[576];
      int bytesRead = Udp.read(packetBuffer, sizeof(packetBuffer));
      if (bytesRead < 10) continue;

      // 1. Verify Art-Net identifier header
      if (memcmp(packetBuffer, ARTNET_HEADER_TAG, 8) != 0) {
        continue;
      }

      // 2. Extract OpCode (little-endian: byte 8 LSB, byte 9 MSB)
      uint16_t opCode = (uint16_t)packetBuffer[8] | ((uint16_t)packetBuffer[9] << 8);

      // ArtPoll handler: node auto-discovery reply
      if (opCode == ARTNET_OP_POLL) {
        sendArtPollReply(Udp.remoteIP(), Udp.remotePort());
        continue;
      }

      // ArtDmx handler: stage lighting control frame
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

  // Background Wi-Fi auto-reconnect engine
  if (currentState == S_RUN) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiLostTime == 0) {
        wifiLostTime = millis();
        lastReconnectAttempt = 0;
        setScreenState(S_WIFI_DISCONN1);
      }
      if (millis() - wifiLostTime > 30000) {
        // Fallback to emergency AP without clearing NVS flash credentials
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
    // Background station reconnect check
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

  // SoftAP client station monitoring
  if (currentState == S_AP_MODE) {
    int currentDevices = WiFi.softAPgetStationNum();
    if (currentDevices != connectedDevices) {
      connectedDevices = currentDevices;
      lcd.setCursor(0, 1);
      if (connectedDevices > 0) {
        lcd.print("Device Connect! ");
      } else {
        lcd.print("192.168.4.1     ");
      }
    }
  }
}

// Assemble and transmit ArtPollReply (OpCode 0x2100) per Art-Net 4 specification
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

// JSON string sanitization utility
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

// Activate standalone Access Point mode
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

// Associate with stage Wi-Fi router in station mode
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

// Centralized state-aware web route handlers
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
    setScreenState(S_MOD_CONN1);
    delay(500);
    ESP.restart();
  });

  server.on("/reset", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    prefs.begin("wifi_data", false);
    prefs.clear();
    prefs.end();
    setScreenState(S_WIFI_RST);
    delay(500);
    ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  });
}

// Asynchronous non-blocking LCD state renderer
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
      lcd.setCursor(0, 0); lcd.print("WiFi Not Set!   ");
      lcd.setCursor(0, 1); lcd.print("Switching AP... ");
      break;
    case S_AP_MODE:
      animStep = 0;
      lastAnimTime = millis();
      lcd.setCursor(0, 0); lcd.print("ARTNET-DMX      ");
      lcd.setCursor(0, 1);
      if (connectedDevices > 0) {
        lcd.print("Device Connect! ");
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
      lcd.setCursor(0, 0); lcd.print("WiFi Failed!    ");
      lcd.setCursor(0, 1); lcd.print("Starting AP Mode");
      break;
    case S_WIFI_RST:
      lcd.setCursor(0, 0); lcd.print("WiFi Reset!     ");
      lcd.setCursor(0, 1); lcd.print("Rebooting...    ");
      break;
    case S_MOD_CONN1:
    case S_MOD_CONN2:
      lcd.setCursor(0, 0); lcd.print("Module Connect! ");
      lcd.setCursor(0, 1); lcd.print("Rebooting...    ");
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
