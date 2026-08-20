# 📋 Changelog — ESP32 Art-Net to DMX512 Converter Module (ZZLIGHT PROJECT)

Dokumentasi komprehensif riwayat perancangan, pengembangan firmware, fabrikasi hardware, penyusunan aset visual terindeks, dan laporan penelitian formal modul konverter **Wireless Art-Net Node ke DMX512** berbasis **ESP32 DevKit V1 + MAX485 + I2C LCD 16x2 + Enclosure Box X4**.

Format pencatatan mengikuti standar industri [*Keep a Changelog*](https://keepachangelog.com/id/1.0.0/) dan penomoran versi semantik [*Semantic Versioning (SemVer)*](https://semver.org/).

---

## 🌟 [v5.2.0] — Final Academic & Production Release — 2026-08
*Master Production Release: Formal Academic Research Report (`REPORT.pdf`), 58 Indexed Figures & Diagrams, 16:9 Master Result Banner, and Publication-Grade Documentation Suite.*

### ✨ Added (Fitur & Aset Baru)
- **Comprehensive Visual Asset Catalog & Figure Indexing Suite (58 Visuals):**
  - **1 Master Live Setup Banner (`RESULT.png`):** Diformat dalam rasio sinematik **16:9 ($1024 \times 576\text{ px}$)** dengan sudut pandang lebar (*wide master shot*), menampilkan laptop **Acer Swift 3** utuh tanpa terpotong menjalankan QLC+, modul **Box X4** presisi dengan layar LCD 16x2 biru menyala di tutup atas dan soket XLR 3-pin perak di panel depan, serta 4 unit lampu **PAR LED 54 mata** yang menyala dengan warna: 🟣 **Ungu**, 🔴 **Merah**, 🟠 **Oranye**, dan 🟢 **Hijau** (**Gambar 1.1**).
  - **22 Studio Catalog Product Images (`images/` — Gambar 2.1 s/d 2.22):** Seluruh alat, bahan, komponen sirkuit, perkabelan, instrumen solder/bor/multimeter, laptop host, dan beban uji panggung di-render dengan latar belakang putih bersih (*seamless pure white background* `#FFFFFF`), unslop, dan realistis.
  - **5 Monochrome 300 DPI Engineering Schematics (`schematics/` — Gambar 3.1 s/d 3.5):** Diagram blok sistem, skematik wiring pinout detail, diagram waktu USITT DMX512-A, arsitektur FreeRTOS Dual-Core SMP, dan flowchart mesin status LCD 16x2.
  - **30 Authentic Physical Fabrication Photos (`docs/` — Gambar 4.1 s/d 4.30):** Dokumentasi asli tahapan pembuatan mekanikal enclosure Box X4, pelubangan step drill, penyolderan rawboard, penataan soket, perakitan internal, dan pengujian live fixture panggung.
- **Formal Academic Research Report (`REPORT.pdf`):**
  - Disusun menggunakan arsitektur ReportLab dengan layout resmi Universitas Negeri Semarang (UNNES), Fakultas Teknik, Jurusan Teknik Elektro, Program Studi Teknik Komputer.
  - Dilengkapi *running headers* & *running footers* berpenomoran halaman dinamis (*Halaman X dari Y* via `NumberedCanvas`), tipografi terstruktur, tabel spesifikasi BOM, dan penyisipan seluruh 58 visual dengan preservasi rasio aspek asli (*zero-distortion aspect ratio scaling*).
- **Academic Citation & Research Traceability:**
  - Integrasi metadata resmi peneliti: **Andreas Restuawanta Christwara (NIM: 5312422036)**.
  - Tautan sesi riset kecerdasan buatan terverifikasi: [Google Gemini Research Session #9b15b21a9066dd07](https://gemini.google.com/app/9b15b21a9066dd07).

---

## 🌟 [v5.0.0] (`firmware/artnet_dmx_final.ino`) — 2026-08
*Production-Ready Firmware with Dual-Core FreeRTOS SMP, Atomic Spinlock Double-Buffering, Full Captive Portal, 11-State LCD Machine, and Enclosure Hardware Integration.*

### ✨ Added
- **FreeRTOS Symmetric Multiprocessing (SMP) Architecture:**
  - `DMX_Task` diikat secara eksklusif ke **Core 0** dengan prioritas tertinggi (`configMAX_PRIORITIES - 1`), menjamin transmisi RS-485 berkecepatan 250 kbps (8N2) dengan *refresh rate* stabil 44 Hz tanpa jitter.
  - Core 1 menangani seluruh tumpukan jaringan (*WiFi UDP 6454*, *Captive Portal DNS*, *WebServer HTTP 80*, dan *NVS Storage*).
- **Atomic Double Buffering (Ping-Pong Memory):**
  - Alokasi memori independen `dmxBufferA[513]` dan `dmxBufferB[513]`.
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
  - **Blackout Timeout (10 Detik):** Otomatis mengosongkan seluruh 512 slot DMX menjadi `0` jika laptop pengirim crash atau tertutup.
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
- Memperbaiki presisi durasi gelombang *Break* (min. $88\,\mu\text{s}$) dan *Mark After Break / MAB* (min. $8\,\mu\text{s}$) menggunakan teknik *GPIO Bit-Banging + Serial2.flush()* sebelum transmisi byte 512 channel serial.

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
