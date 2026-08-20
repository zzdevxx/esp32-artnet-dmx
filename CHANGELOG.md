# 📋 Changelog — ESP32 Art-Net to DMX512 Converter Module (ZZLIGHT PROJECT)
Dokumentasi komprehensif riwayat perancangan dan evolusi arsitektur firmware modul konverter **Wireless Art-Net Node ke DMX512** berbasis **ESP32 DevKit V1 + MAX485 + I2C LCD 16x2 + Enclosure Box X4**.

Format pencatatan mengikuti standar industri [*Keep a Changelog*](https://keepachangelog.com/id/1.0.0/) dan penomoran versi semantik [*Semantic Versioning (SemVer)*](https://semver.org/).

---

## 🌟 [Final Release] — v5.0.0 (`firmware/artnet_dmx_final.ino`) — 2026-08
*Production-Ready Firmware with Dual-Core FreeRTOS SMP, Atomic Spinlock Double-Buffering, Full Captive Portal, 11-State LCD Machine, and Enclosure Hardware Integration.*

### ✨ Added (Fitur Baru)
- **FreeRTOS Symmetric Multiprocessing (SMP) Architecture:**
  - `DMX_Task` diikat secara eksklusif ke **Core 0** dengan prioritas tertinggi (`configMAX_PRIORITIES - 1`), menjamin transmisi RS-485 berkecepatan 250 kbps (8N2) dengan *refresh rate* stabil 44 Hz tanpa jitter.
  - Core 1 menangani seluruh tumpukan jaringan (*WiFi UDP 6454*, *Captive Portal DNS*, *WebServer HTTP 80*, dan *NVS Storage*).
- **Atomic Double Buffering (Ping-Pong Memory):**
  - Alokasi memori independen `dmxBufferA[512]` dan `dmxBufferB[512]`.
  - Operasi pertukaran pointer aman menggunakan *spinlock critical section* atomik (`portENTER_CRITICAL(&dmxMux)` dan `portEXIT_CRITICAL(&dmxMux)`), mengeliminasi risiko *frame tearing* dan *data race*.
- **11-State Machine Display LCD 16x2 I2C (PCF8574 Address `0x27`):**
  - **State 1 (Boot Screen):** Animasi *typing effect* `"ZZLIGHT PROJECT"` di baris 1 & 2 (durasi 3 detik).
  - **State 2 (Connecting):** Menampilkan status mencoba koneksi ke Access Point tersimpan.
  - **State 3 (WiFi Not Connect):** Peringatan kegagalan sambungan sebelum membuka Hotspot Setup.
  - **State 4 (AP Mode / Setup):** Status AP Hotspot aktif (`ZZLIGHT-AP` / IP `192.168.4.1`) menunggu gawai teknisi.
  - **State 5 (Device Connect):** Notifikasi real-time saat smartphone/laptop tersambung ke Hotspot.
  - **State 6 (Save & Reboot):** Konfirmasi penyimpanan kredensial WiFi baru dan eksekusi reboot sistem.
  - **State 7 (Module Connect):** Konfirmasi keberhasilan koneksi ke jaringan WiFi lokal.
  - **State 8 (Run Screen / Live Monitor):** Dashboard operasional live dengan indikator Universe aktif, alamat IP lokal, dan penghitung paket DMX FPS.
  - **State 9 (WiFi Reset):** Status eksekusi reset jaringan via antarmuka web.
  - **State 10 (WiFi Disconnect):** Penanganan otomatis saat sinyal WiFi drop (<30 detik mencoba ulang, >30 detik fallback ke AP).
  - **State 11 (Blackout Mode):** Indikator perlindungan saat sinyal Art-Net terputus lebih dari 10 detik.
- **Enhanced Captive Portal (DNS Hijacking Port 53):**
  - Integrasi `DNSServer` dengan rute wildcard `*` menuju `192.168.4.1`.
  - Dukungan otomatis deteksi popup login pada Android (`/generate_204`), iOS / macOS (`/hotspot-detect.html`), dan Windows (`/ncsi.txt` & `/connecttest.txt`).
- **NVS (Non-Volatile Storage) Preferences:**
  - Migrasi penuh dari pustaka EEPROM legacy ke `Preferences.h` bawaan ESP32 untuk penyimpanan persisten SSID, Password, Universe, dan Konfigurasi Subnet.
- **Fail-Safe & Safety Protections:**
  - **Blackout Timeout (10 Detik):** Otomatis mengosongkan seluruh 512 slot DMX menjadi `0` jika laptop pengirim crash.
  - **Watchdog Timer (WDT):** Mencegah *deadlock* sistem pada operasi socket jaringan.

---

## 🔄 [v4.0.0] (`firmware/artnet_dmx_v4.txt`) — 2026-07
*Custom Captive Portal DNS Hijacking & Web UI Modernization.*

### ✨ Added
- Server DNS lokal pada port 53 untuk pengalihan otomatis (*Captive Portal*) gawai mobile ke portal setup tanpa perlu mengetikkan IP manual di browser.
- Antarmuka web modern responsif mobile dengan tombol *Scan WiFi*, pemilihan dropdown SSID otomatis, dan input universe dinamis.
- Mekanisme sinkronisasi waktu dan status modul via JavaScript *Client-Side Clock*.

### ⚡ Changed
- Mengoptimalkan buffer transmisi I2C untuk mengurangi beban komunikasi display LCD 16x2.

---

## 🔄 [v3.0.0] (`firmware/artnet_dmx_v3.txt`) — 2026-07
*Double Buffering, Spinlock Mutex, & DMX Timing Precision.*

### ✨ Added
- Arsitektur *Ping-Pong Buffer* dengan pointer ganda `writeBuffer` dan `readBuffer`.
- Proteksi memori menggunakan `portMUX_TYPE` spinlock untuk menjamin integritas data lintas inti prosesor.
- Filter Sub-Net dan Universe pada verifikasi header biner paket ArtDmx (`OpCode 0x5000`).

### 🔧 Fixed
- Memperbaiki presisi durasi gelombang *Break* (min. $88\,\mu	ext{s}$) dan *Mark After Break / MAB* (min. $8\,\mu	ext{s}$) menggunakan teknik *GPIO Bit-Banging + Serial2.flush()* sebelum transmisi byte 512 channel serial.

---

## 🔄 [v2.0.0] (`firmware/artnet_dmx_v2.txt`) — 2026-06
*FreeRTOS Multitasking Task Splitting & Custom WebServer.*

### ✨ Added
- Pemisahan beban kerja komputasi menggunakan `xTaskCreatePinnedToCore`:
  - **Core 0:** Khusus mengeksekusi *loop* transmisi serial RS-485 DMX512 pada baudrate 250.000 bps.
  - **Core 1:** Menjalankan `loop()` standar untuk penanganan socket UDP Art-Net dan WebServer konfigurasi.
- Eksplorasi otentikasi jaringan kampus WPA2-Enterprise (`WiFi.begin(ssid, WPA2_AUTH_PEAP, ...)`).

### ⚡ Changed
- Menghapus ketergantungan pada pustaka pihak ketiga `WiFiManager` dan beralih ke implementasi `WebServer.h` bawaan ESP32.

---

## 🔄 [v1.0.0] (`firmware/artnet_dmx_v1.txt`) — 2026-05
*Proof of Concept (PoC) & Baseline Evaluation.*

### ✨ Added
- Implementasi awal modul konverter Art-Net ke DMX512 menggunakan pustaka open-source (`WiFiManager` dan `ArtnetWifi`).
- Pengujian komunikasi dasar paket UDP port 6454 menuju modul transceiver MAX485 pada channel 1–512.
- Pengujian fisik pertama pada fixture lampu panggung PAR LED RGBW.

### ⚠️ Limitations Identified (Catatan Evaluasi)
- Model *single-threaded* menyebabkan kedipan (*flicker*) pada lampu panggung setiap kali terjadi fluktuasi latensi jaringan WiFi atau pemrosesan request HTTP.
- Memerlukan arsitektur *dual-core* dan proteksi *memory buffer* untuk mencapai standar kehandalan tingkat produksi / panggung langsung.
