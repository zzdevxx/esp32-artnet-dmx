# 📋 Changelog — ESP32 Art-Net to DMX512 Converter Module (ZZLIGHT PROJECT)

Evolusi perancangan firmware modul node konverter **Art-Net ke DMX512** berbasis **ESP32 DevKit V1 + MAX485 + I2C LCD 16x2**.

---

## 🌟 [Final Release] — v5.0.0 (`artnet_dmx_final.ino`)
*Production-Ready Firmware with Dual-Core Task Splitting, Robust Captive Portal, & 8-State LCD State Machine.*

### ✨ Added
- **8-State LCD 16x2 State Machine:**
  - `Screen 1`: Boot screen dengan animasi *typing effect* `"ZZLIGHT"` & `"PROJECT"` di tengah layar (durasi 3 detik).
  - `Screen 2`: Layar proses koneksi WiFi dengan nama SSID aktif.
  - `Screen 3`: Peringatan kegagalan koneksi WiFi sebelum beralih ke Access Point mode.
  - `Screen 4`: AP Mode aktif (`ZZLIGHT-AP` / `192.168.4.1`) dengan indikator menunggu konfigurasi.
  - `Screen 5`: Indikator real-time saat pengguna terhubung ke web portal konfigurasi.
  - `Screen 6`: Layar pemilihan Universe Art-Net (0–15) via antarmuka web.
  - `Screen 7`: Konfirmasi koneksi sukses dengan menampilkan IP lokal & Universe aktif.
  - `Screen 8`: Dashboard monitoring operasional live (Menampilkan Status IP, Universe, dan Paket DMX FPS counter).
- **Double Buffering & Spinlock Mutex:**
  - Pemisahan `bufferA` dan `bufferB` untuk mencegah *race condition* dan *frame tearing* antara penerimaan paket Art-Net di Core 1 dan pengiriman DMX di Core 0.
  - Penggunaan `portMUX_TYPE` spinlock (`taskENTER_CRITICAL` / `taskEXIT_CRITICAL`).
- **Android-Optimized Captive Portal (`DNSServer` on Port 53):**
  - Redireksi otomatis pada Android (`/generate_204`), iOS (`/hotspot-detect.html`), dan Windows (`/ncsi.txt`).
- **Preferences (NVS) Flash Storage:**
  - Penyimpanan persisten untuk SSID, Password WiFi, Universe DMX, dan IP statis tanpa ketergantungan pada EEPROM legacy.

---

## 🔄 [v4.0.0] (`artnet_dmx_v4.txt`)
*Advanced Captive Portal & Web UI Refactor.*
- Menambahkan server DNS lokal pada port 53 untuk *Captive Portal wildcard redirection* (`*` ➔ `192.168.4.1`).
- Memperbarui antarmuka web UI dengan styling modern, responsif mobile, dan pemilih universe interaktif.
- Menambahkan *Client-Side Clock* pada antarmuka web untuk sinkronisasi waktu dan status modul.

---

## 🔄 [v3.0.0] (`artnet_dmx_v3.txt`)
*Double Buffering & FreeRTOS Concurrency Hardening.*
- Mengimplementasikan teknik *Ping-Pong Buffer* (`writeBuffer` dan `readBuffer`).
- Menambahkan filter subnet dan universe pada payload ArtDmx OpCode `0x5000`.
- Memperbaiki waktu DMX Break (88 µs) dan Mark After Break (8 µs) menggunakan `Serial2.flush()` dan manipulasi level GPIO.

---

## 🔄 [v2.0.0] (`artnet_dmx_v2.txt`)
*FreeRTOS Task Splitting & WPA2-Enterprise Exploration.*
- Memisahkan tugas ke dua core ESP32 via `xTaskCreatePinnedToCore`:
  - **Core 0:** `DMX_Task` khusus transmisi serial RS-485 250 kbps (44 Hz refresh rate).
  - **Core 1:** `loop()` untuk UDP socket Art-Net, web server HTTP, dan refresh LCD.
- Menambahkan percobaan otentikasi jaringan kampus WPA2-Enterprise (PEAP/MSCHAPv2).

---

## 🔄 [v1.0.0] (`artnet_dmx_v1.txt`)
*Proof of Concept (PoC).*
- Implementasi awal menggunakan pustaka open-source pihak ketiga (`WiFiManager` dan `ArtnetWifi`).
- Pengujian komunikasi dasar UDP port 6454 ke transceiver MAX485 pada channel 1–512.
- Mengidentifikasi keterbatasan *single-threaded* (terjadinya *flicker* pada lampu panggung saat ada jitter jaringan WiFi).
