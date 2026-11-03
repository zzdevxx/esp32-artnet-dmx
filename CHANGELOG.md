# 📋 Changelog & Development Roadmap — ESP32 Art-Net to DMX512 Node (ZZLIGHT)

Dokumentasi komprehensif riwayat perancangan, evolusi kode firmware (v1.0 s/d v4.0 Final), aset visual panggung terindeks, peta jalan pengembangan **v5.0 (Community Edition)**, serta pesan hangat bagi komunitas maker, pengembang, dan kontributor (*forks & pull requests*).

Format pencatatan mengikuti standar industri [*Keep a Changelog*](https://keepachangelog.com/id/1.0.0/) dan penomoran versi semantik [*Semantic Versioning (SemVer)*](https://semver.org/).

> 💡 **Status Versi Saat Ini:** Versi **v4.0.0** (`firmware/artnet_dmx_final.ino`) adalah **versi final stabil (*production-ready*)** yang sudah teruji 100% di panggung fisik. Pengembangan versi resmi oleh pembuat asli dicukupkan sampai v4.0.0 ini. Namun, bagi siapa pun yang ingin **mengembangkan versi berikutnya (v5.0.0 dst.)**, pintu terbuka sangat lebar — *Silakan dikembangkan dan di-fork sesuka hati!* 🎉

---

## 🔧 [v4.0.1] — Timing Precision & Telemetry Observability Update — 2026-10
*Maintenance Release: Periodic Pacing via esp_timer, Atomic Art-Net Packet Reception Counter, and Extended Telemetry Status API.*

### 🔧 Fixed & Improved
- **Absolute-Time Periodic Pacing (`esp_timer`):**
  - Mengganti `vTaskDelay(22)` aditif pada Core 0 dengan penjadwalan periodik waktu absolut (`DMX_PERIOD_US = 23220`, ~43.07 FPS sinkron dengan STFT ZZLUXORA). Durasi transmisi serial 22.5 ms kini diserap ke dalam interval periode sehingga mengeliminasi akumulasi *jitter* dan *drift*.
- **Atomic Art-Net Packet Counter (`artnet_rx_count`):**
  - Menambahkan penghitung paket ArtDmx valid bertipe `volatile uint32_t` di dalam *critical section* atomik (`portENTER_CRITICAL`) untuk pengukuran empiris *packet loss* pada pengujian lapangan FASE 6.
- **Enhanced Telemetry Status API:**
  - Endpoint `/status` pada WebServer kini melaporkan nilai riil `rx_count`, `stations`, `clients`, dan `rssi` secara dinamis dalam format JSON untuk integrasi telemetri live dengan konsol FOH.

---

## 🌟 [v4.0.0] — Final Production Release (`firmware/artnet_dmx_final.ino` & `v4.txt`) — 2026-08
*Master Production Release: Dual-Core FreeRTOS SMP, Atomic Spinlock Double-Buffering, Full Captive Portal DNS Hijacking, 11-State Machine LCD, Fail-Safe Blackout 10 Detik, dan 58 Aset Visual Terindeks.*

### ✨ Added (Fitur Firmware & Dokumentasi)
- **FreeRTOS Symmetric Multiprocessing (SMP) Architecture:**
  - Task `DMXLoop` diikat secara eksklusif ke **Core 0** (`xTaskCreatePinnedToCore`) dengan prioritas tertinggi (`configMAX_PRIORITIES - 1`), menghasilkan sinyal fisik DMX512-A (250 kbps, 8N2) secara deterministik pada *refresh rate* stabil **44.1 Hz** dengan *zero-jitter* ($<0.05\,\mu\text{s}$).
  - **Core 1** menangani seluruh tumpukan jaringan (*WiFi UDP 6454*, *Captive Portal DNS*, *WebServer HTTP 80*, *Client-Side Clock*, dan *NVS Storage*).
- **Atomic Double Buffering (Ping-Pong Memory):**
  - Alokasi memori independen `bufferA[512]` dan `bufferB[512]`.
  - Pertukaran pointer `writeBuffer` dan `readBuffer` dilindungi *spinlock critical section* atomik (`portENTER_CRITICAL(&mux)` & `portEXIT_CRITICAL(&mux)`), mengeliminasi risiko *frame tearing* dan kedipan (*flicker*) pada lampu panggung.
- **11-State Machine Display LCD 16x2 I2C (PCF8574 Address `0x27`):**
  - `S_BOOT`: Animasi booting *"ZZLIGHT PROJECT"* (durasi 3 detik).
  - `S_CONNECTING`: Indikator pencarian dan koneksi ke Access Point tersimpan.
  - `S_WIFI_ERR`: Peringatan kegagalan sambungan WiFi router.
  - `S_AP_MODE`: Hotspot mandiri aktif (`ZZLIGHT-AP` / IP `192.168.4.1`) menunggu gawai teknisi.
  - `S_DEV_CONN`: Notifikasi real-time saat smartphone/laptop tersambung ke Hotspot.
  - `S_MOD_CONN1` & `S_MOD_CONN2`: Konfirmasi modul berhasil tersambung ke router panggung.
  - `S_RUN`: Dashboard operasional live dengan indikator Universe aktif, alamat IP lokal, dan penghitung paket *DMX FPS*.
  - `S_WIFI_RST`: Konfirmasi eksekusi reset WiFi via web interface.
  - `S_WIFI_DISCONN1` & `S_WIFI_DISCONN2`: Pemulihan otomatis saat sinyal WiFi drop (<30 detik reconnect, >30 detik fallback ke AP).
  - `S_WIFI_FAIL`: Indikator kegagalan permanen sebelum sistem me-reboot diri.
- **Enhanced Captive Portal (DNS Hijacking Port 53):**
  - Integrasi `DNSServer` dengan rute wildcard `*` menuju `192.168.4.1`.
  - Deteksi otomatis pop-up login pada Android (`/generate_204`), iOS / macOS (`/hotspot-detect.html`), dan Windows (`/ncsi.txt` & `/connecttest.txt`).
- **NVS (Non-Volatile Storage) Preferences:**
  - Migrasi penuh dari pustaka EEPROM legacy ke `Preferences.h` bawaan ESP32 untuk penyimpanan persisten SSID, Password, dan Art-Net Universe.
- **Fail-Safe & Safety Protections:**
  - **Blackout Safety Timeout (10 Detik):** Pelacakan `lastArtNetTime` otomatis mengosongkan seluruh 512 slot DMX menjadi nilai `0` jika software pengendali di laptop mati/terputus, mencegah panas berlebih pada fixture.
- **Suite Visual Terindeks Lengkap (58 Visual):**
  - 1 Master Result Banner 16:9 (`RESULT.png` — Gambar 1.1).
  - 22 Gambar Katalog Alat dan Bahan Putih Bersih (`images/` — Gambar 2.1 s/d 2.22).
  - 5 Skematik dan Diagram Arsitektur 300 DPI (`schematics/` — Gambar 3.1 s/d 3.5).
  - 30 Foto Fabrikasi Fisik Autentik (`docs/` — Gambar 4.1 s/d 4.30).
  - Laporan Riset Formal Komprehensif (`REPORT.pdf`).

---

## 🔄 [v3.0.0] (`firmware/artnet_dmx_v3.txt`) — Double Buffering & Timing Precision — 2026-07
*Double Buffering, Spinlock Mutex, & DMX Timing Precision.*

### ✨ Added
- Arsitektur *Ping-Pong Buffer* dengan pointer ganda `writeBuffer` dan `readBuffer`.
- Proteksi memori menggunakan `portMUX_TYPE` spinlock untuk menjamin integritas data lintas inti prosesor.
- Filter Sub-Net dan Universe pada verifikasi header biner paket ArtDmx (`OpCode 0x5000`).

### 🔧 Fixed
- Memperbaiki presisi durasi gelombang *Break* (min. $88\,\mu\text{s}$) dan *Mark After Break / MAB* (min. $8\,\mu\text{s}$) menggunakan teknik *GPIO Bit-Banging + Serial2.flush()* sebelum transmisi byte 512 channel serial.

---

## 🔄 [v2.0.0] (`firmware/artnet_dmx_v2.txt`) — Multi-Core Splitting — 2026-06
*FreeRTOS Multitasking Task Splitting & Custom WebServer.*

### ✨ Added
- Pemisahan beban kerja komputasi menggunakan `xTaskCreatePinnedToCore`:
  - **Core 0:** Khusus mengeksekusi *loop* transmisi serial RS-485 DMX512 pada baudrate 250.000 bps.
  - **Core 1:** Menjalankan `loop()` standar untuk penanganan socket UDP Art-Net dan WebServer konfigurasi.
- Eksplorasi otentikasi jaringan kampus WPA2-Enterprise (`WiFi.begin(ssid, WPA2_AUTH_PEAP, ...)`).

### ⚡ Changed
- Menghapus ketergantungan pada pustaka pihak ketiga `WiFiManager` dan beralih ke implementasi `WebServer.h` bawaan ESP32.

---

## 🔄 [v1.0.0] (`firmware/artnet_dmx_v1.txt`) — Proof of Concept (PoC) — 2026-05
*Proof of Concept (PoC) & Baseline Evaluation.*

### ✨ Added
- Implementasi awal modul konverter Art-Net ke DMX512 menggunakan pustaka open-source (`WiFiManager` dan `ArtnetWifi`).
- Pengujian komunikasi dasar paket UDP port 6454 menuju modul transceiver MAX485 pada channel 1–512.
- Pengujian fisik pertama pada fixture lampu panggung PAR LED RGBW.

---

## 🔮 Roadmap v5.0 (Community Edition) — Ide untuk Versi Berikutnya

Bagi Anda yang ingin melanjutkan tongkat estafet pengembangan ke **v5.0.0**, berikut adalah beberapa ide fitur keren yang siap dieksekusi:

1. 🌐 **Multi-Protocol Support (sACN / E1.31):** Menambahkan Streaming ACN (E1.31 multicast) berdampingan dengan Art-Net 4.
2. 🎛️ **Multi-Universe Hardware Outputs (2–4 Universes):** Menghasilkan 2–4 port XLR fisik independen (1024–2048 channel) menggunakan periferal UART0/UART1/RMT ESP32.
3. 🔄 **Bidirectional RDM (ANSI E1.20):** Diagnostik suhu fixture lampu panggung dan konfigurasi start address dari jarak jauh.
4. ⚡ **Hardwired Ethernet (W5500 / LAN8720 PoE):** Port kabel RJ45 fisik anti-interferensi RF untuk panggung stadion besar.
5. ☁️ **Over-The-Air (OTA) Web Firmware Update:** Pembaruan firmware drag-and-drop file `.bin` via browser.
6. 🔍 **Auto-Discovery Engine (ArtPoll & ArtPollReply):** Auto-deteksi nama node di konsol lighting (*grandMA3, Avolites, Chamsys*).
7. 📟 **OLED SSD1306 & NeoPixel RGB Status:** Tampilan visual bar-graph channel grafis dan cincin status RGB yang dinamis.
8. 💾 **Standalone DMX Scene Recorder & Playback:** Perekam adegan panggung ke Flash/MicroSD tanpa butuh laptop menyala.

---

## 💬 Pesan Hangat untuk Pengguna, Pembuat (Makers), & Forkers (Pengembang v5)

> Halo rekan-rekan sesama *developer*, *lighting designer*, teknisi panggung, mahasiswa, dan komunitas *DIY maker* di mana pun Anda berada! 👋✨

Proyek **ESP32 Wireless Art-Net to DMX512 Node (ZZLIGHT)** ini saya rilis sepenuhnya sebagai **perangkat lunak dan desain perangkat keras Open Source (MIT License)**. Bagi saya pribadi, pengembangan inti modul ini telah **tuntas sampai versi v4.0.0** dan sudah terbukti sangat stabil serta siap pakai di panggung.

### 🚀 Silakan Banget untuk Fork & Kembangkan v5.0!
Jika Anda memiliki ide untuk menambahkan fitur baru, mendesain custom PCB KiCad SMD, membuat versi multi-universe, atau menambahkan protokol sACN E1.31 untuk melahirkan **Versi 5 (v5.0.0)**:
* **Silakan banget  di-fork, dimodifikasi, dan dikembangkan sesuka hati!**
* Repositori ini bebas Anda jadikan basis riset skripsi, produk komersial, maupun perlengkapan panggung rental Anda sendiri.
* Kami sangat senang jika Anda membagikan kembali hasil inovasi Anda via **Pull Request** atau *showcase* di GitHub kami ([`@zzdevxx`](https://github.com/zzdevxx)).

### 🛠️ Tips Penting bagi yang Merakit Sendiri (*DIY Builders*):
1. **Suplai Daya 5V:** Gunakan kabel data tembaga tebal dan adaptor berkualitas minimal 1A–2A agar radio WiFi ESP32 tidak drop saat mengirim paket data intensif.
2. **Kapasitor Decoupling:** Sangat disarankan menyolder kapasitor keramik $100\,\text{nF}$ tepat di pin VCC-GND modul MAX485 untuk meredam noise frekuensi tinggi.
3. **Resistor Terminasi $120\,\Omega$:** Jika kabel DMX panggung Anda panjang (>30–50 meter), pasang resistor $120\,\Omega$ ($0.25\text{W}$) pada soket XLR lampu paling ujung untuk mencegah pantulan sinyal (*signal reflection*).

---

<div align="center">
  <b>⭐ Jika proyek ini bermanfaat untuk panggung atau penelitian Anda, jangan lupa tinggalkan Star di GitHub! ⭐</b><br/>
  <i>Happy Making, Happy Coding, and Let the Lights Shine Bright! 💡🎛️🔥</i>
  <br/><br/>
  <b>Salam hangat dari Semarang,</b><br/>
  <b>Andreas Restuawanta Christwara</b> (<a href="https://github.com/zzdevxx">@zzdevxx</a>)<br/>
  <i>Teknik Komputer — Universitas Negeri Semarang (UNNES)</i>
</div>