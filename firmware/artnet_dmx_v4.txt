/*
 * ZZLIGHT PROJECT - ARTNET TO DMX MODULE
 * Hardware : ESP32 DevKit V1, MAX485, LCD 16x2 I2C
 * Core     : ESP32 Board v3.3.8
 * Features : Art-Net, DMX Engine, Captive Portal (Android Optimized), Client-Side Clock, Advanced Typography
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <string.h>

// --- PINS CONFIGURATION ---
#define DMX_TX_PIN 17
#define DMX_RX_PIN 16
#define DMX_EN_PIN 4

// --- DMX & ARTNET VARIABLES ---
uint8_t bufferA[512];
uint8_t bufferB[512];
uint8_t* writeBuffer = bufferA; 
uint8_t* readBuffer = bufferB;  
volatile bool frameReady = false;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
const uint8_t myUniverse = 0;

WiFiUDP Udp;
const int artNetPort = 6454;
volatile unsigned long lastArtNetTime = 0;
TaskHandle_t DMXTask;

// --- SYSTEM & WIFI VARIABLES ---
Preferences prefs;
WebServer server(80);
DNSServer dnsServer; // Objek untuk Captive Portal
const byte DNS_PORT = 53;

LiquidCrystal_I2C lcd(0x27, 16, 2);

String ssid = "";
String pass = "";

unsigned long wifiLostTime = 0;
int connectedDevices = 0;

// --- STATE MACHINE ---
enum ScreenState {
  S_BOOT, S_CONNECTING, S_WIFI_ERR, S_AP_MODE, S_DEV_CONN, 
  S_MOD_CONN1, S_MOD_CONN2, S_RUN, S_WIFI_RST, 
  S_WIFI_DISCONN1, S_WIFI_DISCONN2, S_WIFI_FAIL
};
ScreenState currentState = S_BOOT;

// --- FUNCTION PROTOTYPES ---
void changeScreen(ScreenState newState);
void handleLCDAnimation();
void startAP();
void connectWiFi();
void setupAPServer();
void setupRunServer();
void DMXCoreTask(void *pvParameters);

unsigned long lastAnimTime = 0;
int animStep = 0;

// --- HTML TEMPLATES ---

// 1. Setup Page (AP Mode)
const char AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Setup</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,300;0,400;0,700;0,900;1,300;1,400;1,700;1,900&display=swap');
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin: 0; padding: 40px 20px; display: flex; flex-direction: column; align-items: center; }
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; margin-bottom: 25px; } 
    .container { width: 100%; max-width: 340px; display: flex; flex-direction: column; align-items: center; }
    .wifi-box { background: #1e1e1e; border-radius: 8px; padding: 5px; width: 100%; max-height: 220px; overflow-y: auto; border: 1px solid #333; box-sizing: border-box; margin-bottom: 15px; font-weight: 400; } 
    .wifi-item { display: flex; justify-content: space-between; align-items: center; padding: 14px 10px; border-bottom: 1px solid #333; cursor: pointer; transition: 0.2s; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:hover { background: #2a2a2a; }
    .w-left { font-size: 15px; display: flex; align-items: center; gap: 10px; }
    .w-right { font-size: 14px; color: #aaa; display: flex; align-items: center; gap: 8px; }
    .btn-refresh { background-color: #e67e22; color: white; padding: 12px; border: none; border-radius: 6px; font-size: 15px; font-weight: 700; cursor: pointer; width: 100%; margin-bottom: 30px; } 
    form { width: 100%; }
    input[type="text"], input[type="password"] { background-color: #ffffff; color: #000000; padding: 14px; margin-bottom: 12px; width: 100%; border-radius: 6px; border: none; font-size: 16px; box-sizing: border-box; text-align: center; font-weight: 300; } 
    .btn-connect { background-color: #2ecc71; color: white; padding: 14px; border: none; border-radius: 6px; font-size: 16px; font-weight: 700; cursor: pointer; width: 100%; margin-top: 5px; } 
    .footer-link { color: #888888; font-weight: 400; font-style: italic; text-decoration: underline; font-size: 15px; margin-top: 40px; display: block; } 
  </style>
  <script>
    function fillSSID(el) { document.getElementById('ssid').value = el.getAttribute('data-ssid'); document.getElementById('pass').focus(); }
    function scan() {
      let wb = document.getElementById('wb');
      wb.innerHTML = '<div style="padding:20px;color:#aaa;">Scanning WiFi...</div>';
      fetch('/scan').then(r=>r.json()).then(d=>{
        let h = ''; if(d.length == 0) h = '<div style="padding:20px;color:#aaa;">No WiFi found</div>';
        d.forEach(w => {
          let safeSsid = w.ssid.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
          let lock = w.sec ? '🔒' : '🔓'; 
          h += `<div class="wifi-item" onclick="fillSSID(this)" data-ssid="${safeSsid}">
                  <div class="w-left"><span>📶</span> <span>${w.ssid}</span></div>
                  <div class="w-right"><span>${lock}</span> <span>${w.sig}%</span></div>
                </div>`;
        }); wb.innerHTML = h;
      }).catch(() => { wb.innerHTML = '<div style="padding:20px;color:#e74c3c;">Scan failed</div>'; });
    } window.onload = scan;
  </script>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>
  <div class="container">
    <div id="wb" class="wifi-box"></div>
    <button type="button" class="btn-refresh" onclick="scan()">Refresh</button>
    <form action="/connect" method="POST">
      <input type="text" name="ssid" id="ssid" placeholder="SSID" required>
      <input type="password" name="pass" id="pass" placeholder="Password">
      <button type="submit" class="btn-connect">Connect</button>
    </form>
    <a href="/monitor" class="footer-link">Monitor</a>
  </div>
</body>
</html>
)rawliteral";

// 2. Saved Page (Connect Mode)
const char SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Connect</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,400;0,900;1,400;1,900&display=swap'); 
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin-top: 100px; padding: 20px; } 
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; } 
    p { font-weight: 400; font-style: italic; font-size: 16px; margin-top: 20px; color: #ccc; } 
  </style>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>
  <p>WiFi and Password Saved, Rebooting...</p>
</body>
</html>
)rawliteral";

// 3. Monitor Page (Run Mode & AP Mode)
const char RUN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ARTNET-DMX Monitor</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,300;0,400;0,700;0,900;1,300;1,400;1,700;1,900&display=swap');
    body { background-color: #121212; color: #ffffff; font-family: 'Roboto', sans-serif; text-align: center; margin: 0; padding: 40px 20px; display: flex; flex-direction: column; align-items: center; }
    h1 { font-weight: 900; font-style: italic; letter-spacing: 2px; margin-bottom: 25px; } 
    
    .box { background-color: #1e1e1e; padding: 20px; border-radius: 8px; border: 1px solid #333; width: 100%; max-width: 340px; box-sizing: border-box; text-align: left; font-size: 15px; line-height: 1.8; margin-bottom: 25px; font-weight: 700; font-style: italic; } 
    .box div { white-space: nowrap; }
    .lbl { display: inline-block; width: 85px; color: #ccc; font-weight: 700; font-style: italic; } 
    
    .btn-container { display: flex; flex-direction: column; gap: 12px; width: 100%; max-width: 340px; }
    .btn { width: 100%; padding: 14px; border: none; border-radius: 6px; font-weight: 700; cursor: pointer; font-size: 16px; font-family: 'Roboto', sans-serif; display: block; } 
    .btn-red { background-color: #e74c3c; color: white; }
    .btn-orange { background-color: #e67e22; color: white; }
    
    .footer-link { color: #888888; font-weight: 400; font-style: italic; text-decoration: underline; font-size: 15px; margin-top: 30px; display: block; } 
    
    .clr-g { color: #2ecc71; } 
    .clr-y { color: #f1c40f; } 
    .clr-r { color: #e74c3c; }
    .val { color: #fff; }
  </style>
  <script>
    function updateClientTime() {
      const now = new Date();
      const days = ["Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"];
      const months = ["Januari","Februari","Maret","April","Mei","Juni","Juli","Agustus","September","Oktober","November","Desember"];
      let d = now.getDate().toString().padStart(2, '0');
      let m = months[now.getMonth()];
      let y = now.getFullYear();
      let hh = now.getHours().toString().padStart(2, '0');
      let mm = now.getMinutes().toString().padStart(2, '0');
      let ss = now.getSeconds().toString().padStart(2, '0');
      
      document.getElementById('dt-val').innerHTML = `${days[now.getDay()]}, ${d} ${m} ${y}`;
      document.getElementById('tm-val').innerHTML = `${hh}:${mm}:${ss}`;
    }

    function updateData() {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('ip-val').innerHTML = d.ip;
        document.getElementById('wf-val').innerHTML = d.wifi;
        
        let statColor = d.status == "Connected" ? "clr-g" : "clr-r";
        document.getElementById('st-val').innerHTML = `<span class="${statColor}">${d.status}</span>`;
        
        if(d.strength === "Direct Mode") {
           document.getElementById('sg-val').innerHTML = `<span class="val">${d.strength}</span>`;
           document.getElementById('setupLink').style.display = 'block'; 
           document.getElementById('btn-reset').style.display = 'none'; // Sembunyikan Reset WiFi di Mode AP
        } else {
           let dbm = parseInt(d.strength); let strColor = "clr-r", strTxt = "Weak";
           if(dbm > -60) { strColor = "clr-g"; strTxt = "Good"; } else if(dbm > -80) { strColor = "clr-y"; strTxt = "Medium"; }
           document.getElementById('sg-val').innerHTML = `<span class="${strColor}">${d.strength} dBm (${strTxt})</span>`;
           document.getElementById('setupLink').style.display = 'none'; 
           document.getElementById('btn-reset').style.display = 'block'; // Tampilkan Reset WiFi di Mode RUN
        }
        
        let dmxColor = d.dmx == "Active" ? "clr-g" : (d.dmx == "No Signal" ? "clr-y" : "clr-r");
        document.getElementById('dx-val').innerHTML = `<span class="${dmxColor}">${d.dmx}</span>`;
      }).catch(e => console.log(e));
    } 
    
    setInterval(updateData, 1000);
    setInterval(updateClientTime, 1000);
    window.onload = function() { updateClientTime(); updateData(); };

    function req(url, msg) {
      if(confirm(msg)) { fetch(url, {method: 'POST'}).then(() => { 
        if(url === '/reset') { setTimeout(() => { window.location.href = "http://192.168.4.1/setup"; }, 3500); } 
        else { setTimeout(() => { location.reload(); }, 3500); }
      }); }
    }
  </script>
</head>
<body>
  <h1>ZZLIGHT PROJECT</h1>
  
  <div class="box">
    <div><span class="lbl">Date</span>: <span id="dt-val" class="val">Loading...</span></div>
    <div><span class="lbl">Time</span>: <span id="tm-val" class="val">Loading...</span></div>
    <div><span class="lbl">IP</span>: <span id="ip-val" class="val">Loading...</span></div>
    <div><span class="lbl">Wifi</span>: <span id="wf-val" class="val">Loading...</span></div>
    <div><span class="lbl">Status</span>: <span id="st-val" class="val">Loading...</span></div>
    <div><span class="lbl">Strength</span>: <span id="sg-val" class="val">Loading...</span></div>
    <div><span class="lbl">DMX</span>: <span id="dx-val" class="val">Loading...</span></div>
  </div>

  <div class="btn-container">
    <button id="btn-reset" class="btn btn-red" onclick="req('/reset', 'Reset WiFi?')">WiFi Reset</button>
    <button class="btn btn-orange" onclick="req('/reboot', 'Reboot Module?')">Module Reboot</button>
  </div>
  
  <a href="http://192.168.4.1/setup" id="setupLink" class="footer-link" style="display: none;">Setup</a>

</body>
</html>
)rawliteral";


// --- DMX ENGINE TASK (CORE 0) ---
void DMXCoreTask(void *pvParameters) {
  for (;;) {
    digitalWrite(DMX_EN_PIN, HIGH);
    
    if (frameReady) {
      portENTER_CRITICAL(&mux); 
      uint8_t* temp = readBuffer;
      readBuffer = writeBuffer;
      writeBuffer = temp;
      frameReady = false;
      portEXIT_CRITICAL(&mux);  
    }
    
    Serial2.updateBaudRate(96000);
    Serial2.write(0);
    Serial2.flush();
    
    Serial2.updateBaudRate(250000);
    Serial2.write(0); 
    
    if (millis() - lastArtNetTime > 10000) {
      for (int i = 0; i < 512; i++) Serial2.write(0);
    } else {
      Serial2.write(readBuffer, 512); 
    }
    Serial2.flush();
    
    vTaskDelay(pdMS_TO_TICKS(22)); 
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  pinMode(DMX_EN_PIN, OUTPUT);
  digitalWrite(DMX_EN_PIN, LOW);
  Serial2.begin(250000, SERIAL_8N2, DMX_RX_PIN, DMX_TX_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  memset(bufferA, 0, 512);
  memset(bufferB, 0, 512);

  xTaskCreatePinnedToCore(DMXCoreTask, "DMX_T", 2048, NULL, 2, &DMXTask, 0);

  prefs.begin("wifi_data", false);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");

  changeScreen(S_BOOT);
  changeScreen(S_CONNECTING);
  
  if (ssid == "") {
    changeScreen(S_WIFI_ERR);
    startAP();
  } else {
    connectWiFi();
  }
}

// --- MAIN LOOP ---
void loop() {
  handleLCDAnimation();
  server.handleClient();
  
  // Captive Portal Hijacker
  if (currentState == S_AP_MODE) {
    dnsServer.processNextRequest();
  }
  
  // --- MESIN ART-NET (Aktif di Run Mode & AP Mode) ---
  if (currentState == S_RUN || currentState == S_AP_MODE) {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      char packetBuffer[530];
      Udp.read(packetBuffer, 530);
      
      if (strncmp(packetBuffer, "Art-Net\0", 8) == 0 && packetBuffer[9] == 0x50) {
        uint8_t incomingUniverse = packetBuffer[14]; 
        if (incomingUniverse == myUniverse) {
          lastArtNetTime = millis();
          int length = (packetBuffer[16] << 8) | packetBuffer[17];
          if (length > 512) length = 512;
          
          portENTER_CRITICAL(&mux);
          memset(writeBuffer, 0, 512); 
          for (int i = 0; i < length; i++) {
            writeBuffer[i] = packetBuffer[18 + i];
          }
          frameReady = true; 
          portEXIT_CRITICAL(&mux);
        }
      }
    }
  }
  
  // --- MESIN AUTO-RECONNECT WIFI (Hanya di Run Mode) ---
  if (currentState == S_RUN) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiLostTime == 0) {
        wifiLostTime = millis();
        changeScreen(S_WIFI_DISCONN1); 
      }
      if (millis() - wifiLostTime > 30000) {
        changeScreen(S_WIFI_DISCONN2);
        prefs.clear();
        ESP.restart(); 
      } else {
        WiFi.reconnect();
      }
    } else {
      if (wifiLostTime != 0) {
        changeScreen(S_CONNECTING);
        changeScreen(S_MOD_CONN2);
        changeScreen(S_RUN);
        wifiLostTime = 0; 
      }
    }
  }

  // --- DETEKSI DEVICE DI AP MODE ---
  if (currentState == S_AP_MODE) {
    int currentDevices = WiFi.softAPgetStationNum();
    if (currentDevices > connectedDevices) {
      changeScreen(S_CONNECTING); 
      changeScreen(S_DEV_CONN);   
      changeScreen(S_AP_MODE);    
    }
    connectedDevices = currentDevices;
  }
}


// --- WIFI & AP FUNCTIONS ---
void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ARTNET-DMX", "andreas123");
  connectedDevices = 0;
  
  // Aktifkan DNS Server untuk Captive Portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  Udp.begin(artNetPort); 
  setupAPServer();
  server.begin();
  changeScreen(S_AP_MODE); 
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startT = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startT < 10000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    changeScreen(S_MOD_CONN2);
    MDNS.begin("zzlight");
    Udp.begin(artNetPort);
    
    setupRunServer();
    server.begin();
    changeScreen(S_RUN);
  } else {
    changeScreen(S_WIFI_FAIL);
    startAP();
  }
}

// --- WEBSERVER ROUTING (MODE AP) ---
void setupAPServer() {
  // Tangkap semua URL acak dari Captive Portal
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
  });

  // OPTIMASI ANDROID (SAMSUNG) & APPLE CAPTIVE PORTAL
  server.on("/generate_204", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
  });
  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
  });
  
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
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
    String dmxStat = "Loss Signal";
    if (dmxDelta < 1000) dmxStat = "Active";
    else if (dmxDelta < 10000) dmxStat = "No Signal";

    String json = "{";
    json += "\"ip\":\"192.168.4.1\",";
    json += "\"wifi\":\"ARTNET-DMX\",";
    json += "\"status\":\"Connected\",";
    json += "\"strength\":\"Direct Mode\",";
    json += "\"dmx\":\"" + dmxStat + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
  });

  server.on("/scan", HTTP_GET, []() {
    WiFi.scanDelete(); 
    int n = WiFi.scanNetworks(false, true); 
    String json = "[";
    for (int i = 0; i < n; ++i) {
      if (i > 0) json += ",";
      int dbm = WiFi.RSSI(i);
      int quality = (dbm <= -100) ? 0 : ((dbm >= -50) ? 100 : 2 * (dbm + 100));
      bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"sec\":" + String(secure ? "true" : "false") + ",\"sig\":" + String(quality) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  
  server.on("/connect", HTTP_POST, []() {
    ssid = server.arg("ssid");
    pass = server.arg("pass");
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    server.send(200, "text/html", SAVED_HTML);
    changeScreen(S_MOD_CONN1);
    ESP.restart();
  });
  
  server.on("/reset", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    prefs.clear();
    changeScreen(S_WIFI_RST);
    ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  });
}

// --- WEBSERVER ROUTING (MODE RUN) ---
void setupRunServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html", RUN_HTML);
  });

  server.on("/status", HTTP_GET, []() {
    unsigned long dmxDelta = millis() - lastArtNetTime;
    String dmxStat = "Loss Signal";
    if (dmxDelta < 1000) dmxStat = "Active";
    else if (dmxDelta < 10000) dmxStat = "No Signal";

    String json = "{";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"wifi\":\"" + WiFi.SSID() + "\",";
    json += "\"status\":\"" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\",";
    json += "\"strength\":\"" + String(WiFi.RSSI()) + "\",";
    json += "\"dmx\":\"" + dmxStat + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
  });

  server.on("/reset", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    prefs.clear();
    changeScreen(S_WIFI_RST);
    ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  });
}

// --- LCD CONTROLLER ---
void changeScreen(ScreenState newState) {
  if(newState != S_WIFI_DISCONN1) {
    currentState = newState;
  }
  lcd.clear();
  
  switch(newState) {
    case S_BOOT: 
      for(int i=0; i<7; i++) { lcd.setCursor(5+i, 0); lcd.print("ZZLIGHT"[i]); delay(150); }
      for(int i=0; i<7; i++) { lcd.setCursor(5+i, 1); lcd.print("PROJECT"[i]); delay(150); }
      delay(900); 
      break;
      
    case S_CONNECTING: 
      lcd.setCursor(0, 0); lcd.print("Connecting...   ");
      lcd.setCursor(0, 1); lcd.print("                ");
      delay(3000);
      break;
      
    case S_WIFI_ERR: 
      lcd.setCursor(0, 0); lcd.print("WiFi Not Connect");
      lcd.setCursor(0, 1); lcd.print("Connecting...   ");
      delay(3000);
      break;
      
    case S_AP_MODE: 
      animStep = 0;
      lastAnimTime = millis();
      lcd.setCursor(0, 0); lcd.print("ARTNET-DMX      ");
      lcd.setCursor(0, 1); lcd.print("192.168.4.1     ");
      break;
      
    case S_DEV_CONN: 
      lcd.setCursor(0, 0); lcd.print("Device Connect! ");
      lcd.setCursor(0, 1); lcd.print("Loading...      ");
      delay(3000);
      break;
      
    case S_MOD_CONN1: 
      lcd.setCursor(0, 0); lcd.print("Module Connect! ");
      lcd.setCursor(0, 1); lcd.print("Rebooting...    ");
      delay(3000);
      break;
      
    case S_MOD_CONN2: 
      lcd.setCursor(0, 0); lcd.print("Module Connect! ");
      lcd.setCursor(0, 1); lcd.print("Loading...      ");
      delay(3000);
      break;
      
    case S_RUN: 
      animStep = 0;
      lastAnimTime = millis();
      lcd.setCursor(0, 0); lcd.print("ARTNET-DMX      ");
      lcd.setCursor(0, 1); 
      {
        String ipStr = WiFi.localIP().toString();
        while(ipStr.length() < 16) ipStr += " ";
        lcd.print(ipStr);
      }
      break;
      
    case S_WIFI_RST: 
      lcd.setCursor(0, 0); lcd.print("WiFi Reset!     ");
      lcd.setCursor(0, 1); lcd.print("Rebooting...    ");
      delay(3000);
      break;
      
    case S_WIFI_DISCONN1: 
      lcd.setCursor(0, 0); lcd.print("WiFi Disconnect!");
      lcd.setCursor(0, 1); lcd.print("Reconnecting... ");
      break;
      
    case S_WIFI_DISCONN2: 
      lcd.setCursor(0, 0); lcd.print("WiFi Disconnect!");
      lcd.setCursor(0, 1); lcd.print("Rebooting...    ");
      delay(3000);
      break;
      
    case S_WIFI_FAIL: 
      lcd.setCursor(0, 0); lcd.print("WiFi Failed!    ");
      lcd.setCursor(0, 1); lcd.print("Wrong Password..");
      delay(3000);
      break;
  }
}

void handleLCDAnimation() {
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
        for(int i=1; i<=animStep; i++) lcd.print(animChar);
        for(int i=animStep+1; i<=5; i++) lcd.print(" ");
      }
    }
  }
}