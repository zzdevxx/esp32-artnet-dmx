# 📋 Changelog & Development Roadmap — ESP32 Art-Net to DMX512 Node (ZZLIGHT)

Dokumentasi komprehensif riwayat perancangan, evolusi kode firmware (v1.0 s/d Final v5.2), aset visual panggung terindeks, peta jalan pengembangan masa depan (*future roadmap*), serta panduan bagi komunitas maker, pengembang, dan kontributor (*forks & pull requests*).

Format pencatatan mengikuti standar industri [*Keep a Changelog*](https://keepachangelog.com/id/1.0.0/) dan penomoran versi semantik [*Semantic Versioning (SemVer)*](https://semver.org/).

---

## 🌟 [v5.2.0] — Final Production & Open-Source Release — 2026-08
*Master Production Release: Integrasi Aset Visual Lengkap (58 Visual), Modernisasi Open-Source, dan Laporan Riset Formal (`REPORT.pdf`).*

### ✨ Added (Fitur & Dokumentasi Baru)
- **Visual Asset & Engineering Indexing Suite (58 Visual):**
  - **1 Master Live Setup Banner (`RESULT.png`):** Format 16:9 ($1024 \times 576\text{ px}$) sudut lebar tanpa cropping pada laptop Acer Swift 3, modul Box X4 presisi (LCD 16x2 biru & XLR 3-pin perak), dan 4 fixture PAR LED 54 mata menyala: 🟣 **Ungu**, 🔴 **Merah**, 🟠 **Oranye**, dan 🟢 **Hijau** (**Gambar 1.1**).
  - **22 Studio Catalog Product Images (`images/` — Gambar 2.1 s/d 2.22):** Seluruh alat, bahan sirkuit, perkabelan, laptop host, lampu panggung, solder, dan instrumen bor/potong berlatar belakang putih bersih murni (`#FFFFFF`).
  - **5 Monochrome 300 DPI Engineering Schematics (`schematics/` — Gambar 3.1 s/d 3.5):** Diagram blok sistem, skematik wiring pinout detail, diagram waktu USITT DMX512-A, arsitektur FreeRTOS Dual-Core SMP, dan flowchart mesin status LCD 16x2.
  - **30 Authentic Physical Fabrication Photos (`docs/` — Gambar 4.1 s/d 4.30):** Dokumentasi autentik tahapan pengeboran step drill box X4, penyolderan rawboard, penataan soket sasis XLR, perakitan internal, dan uji coba fixture panggung live.
- **Formal Technical & Academic Report (`REPORT.pdf`):**
  - Dokumen laporan riset teknis 19.8 MB dengan layout resmi UNNES, running header-footer berpenomoran dinamis (*Halaman X dari Y* via `NumberedCanvas`), dan preservasi skala gambar asli tanpa distorsi rasio.
- **Modern Open-Source README:**
  - Transformasi penuh pada [`README.md`](file:///C:/ANDREAS/esp32_artnet_dmx/README.md) menjadi *showcase* open-source modern kelas dunia (bergaya WLED/ESPHome/QLC+) dengan navigasi interaktif, tabel BOM, galeri visual, panduan flashing Arduino IDE, dan matriks kompatibilitas software pencahayaan universal.

---

## 🌟 [v5.0.0] (`firmware/artnet_dmx_final.ino`) — Production Firmware Release — 2026-08
*Firmware Produksi Utama: Dual-Core FreeRTOS SMP, Atomic Spinlock Double-Buffering, Full Captive Portal, 11-State Machine LCD, dan Fail-Safe Blackout 10 Detik.*

### ✨ Added (Fitur Firmware)
- **FreeRTOS Symmetric Multiprocessing (SMP) Architecture:**
  - Task `DMXLoop` diikat ke **Core 0** (`xTaskCreatePinnedToCore`) dengan prioritas tertinggi (`configMAX_PRIORITIES - 1`), menghasilkan sinyal fisik DMX512-A (250 kbps, 8N2) secara deterministik pada *refresh rate* stabil **44.1 Hz** dengan *zero-jitter* ($<0.05\,\mu\text{s}$).
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

---

## 🔄 [v4.0.0] (`firmware/artnet_dmx_v4.txt`) — Captive Portal & Web UI — 2026-07
*Custom Captive Portal DNS Hijacking & Web UI Modernization.*

### ✨ Added
- Server DNS lokal pada port 53 untuk pengalihan otomatis (*Captive Portal*) gawai mobile ke portal setup tanpa perlu mengetikkan IP manual di browser.
- Antarmuka web modern responsif mobile dengan tombol *Scan WiFi*, pemilihan dropdown SSID otomatis, dan input universe dinamis.
- Mekanisme sinkronisasi waktu dan status modul via JavaScript *Client-Side Clock*.

### ⚡ Changed
- Mengoptimalkan buffer transmisi I2C untuk mengurangi beban komunikasi display LCD 16x2.

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

### ⚠️ Limitations Identified (Catatan Evaluasi)
- Model *single-threaded* menyebabkan kedipan (*flicker*) pada lampu panggung setiap kali terjadi fluktuasi latensi jaringan WiFi atau pemrosesan request HTTP.
- Memerlukan arsitektur *dual-core* dan proteksi *memory buffer* untuk mencapai standar kehandalan tingkat produksi / panggung langsung.

---

## 🔮 Roadmap & Ide Pengembangan Masa Depan (Future Roadmap)

Bagi siapa pun yang ingin mengembangkan proyek ini lebih jauh, berikut adalah daftar fitur dan peningkatan arsitektur yang sangat direkomendasikan untuk versi berikutnya:

### 1. 🌐 Dukungan Multi-Protokol (sACN / E1.31)
* Menambahkan dukungan protokol **sACN (Streaming Architecture for Control Networks - ANSI E1.31)** dengan fitur *multicast* (Universe 1-63999) dan *priority handling* berdampingan dengan Art-Net 4.

### 2. 🎛️ Multi-Universe Hardware Outputs (2 s/d 4 Universe)
* Memanfaatkan periferal perangkat keras ESP32 lainnya (seperti **UART1 / UART0** atau **RMT / Remote Control Peripheral**) untuk menghasilkan **2 hingga 4 Port XLR DMX512 fisik independen** dari satu modul ESP32 tunggal (1024–2048 channel).

### 3. 🔄 Komunikasi Dua Arah RDM (Remote Device Management - ANSI E1.20)
* Mengimplementasikan protokol **RDM** pada bus RS-485 untuk membaca data diagnostik dari lampu panggung (suhu fixture, jam kerja lampu, voltase) dan mengubah alamat DMX (*start address*) lampu dari jarak jauh tanpa perlu memanjat rigging truss panggung.

### 4. ⚡ Dukungan Port Ethernet Hardwired (W5500 / LAN8720 PoE)
* Menyediakan opsi modul **Ethernet SPI W5500** atau **RMII LAN8720** dengan dukungan *Power over Ethernet (PoE 802.3af)* untuk instalasi panggung festival/stasioner besar yang bebas dari interferensi gelombang radio WiFi.

### 5. ☁️ Over-The-Air (OTA) Web Firmware Update
* Menambahkan fitur pembaruan firmware langsung melalui antarmuka web (*Drag-and-Drop `.bin` File Upload*) sehingga teknisi panggung dapat memperbarui fitur modul tanpa perlu membuka baut box atau mencolokkan kabel USB ke PC.

### 6. 🔍 Respon ArtPoll & ArtPollReply (Auto-Discovery Engine)
* Menambahkan parser biner untuk paket `ArtPoll` (`OpCode 0x2000`) dan membalas dengan `ArtPollReply` (`OpCode 0x2100`), sehingga software konsol profesional (seperti *grandMA3, Avolites Titan, Chamsys MagicQ*) dapat mendeteksi nama node `"ZZLIGHT Node"` dan status port secara otomatis di jaringan panggung.

### 7. 📟 Antarmuka Layar OLED SSD1306 / NeoPixel RGB Status
* Menyediakan opsi firmware untuk layar grafis **OLED 0.96-inch I2C (SSD1306)** atau cincin **NeoPixel WS2812B RGB** untuk visualisasi level channel DMX bar-graph grafis yang lebih dinamis.

### 8. 💾 Standalone DMX Scene Recorder & Playback (Tanpa Komputer)
* Menambahkan fitur penyimpanan adegan (*scene recorder*) ke memori Flash ESP32 / MicroSD Card, sehingga modul dapat memutar adegan pencahayaan panggung statis/animasi secara otomatis saat saklar dinyalakan tanpa perlu laptop yang menyala.

---

## 💬 Pesan Hangat untuk Pengguna, Pembuat (Makers), & Kontributor (Forks)

> Halo rekan-rekan sesama *developer*, *lighting designer*, teknisi panggung, mahasiswa, dan komunitas *DIY maker* di mana pun Anda berada! 👋✨

Proyek **ESP32 Wireless Art-Net to DMX512 Node (ZZLIGHT)** ini saya dedikasikan dan rilis sepenuhnya sebagai **perangkat lunak dan desain perangkat keras Open Source (MIT License)**. Proyek ini berawal dari kebutuhan nyata di lapangan akan modul konverter DMX nirkabel yang **andal, berkecepatan tinggi, bebas jitter (*zero-jitter*), namun dapat dirakit sendiri dengan biaya sangat terjangkau ($<\$10 / <	ext{Rp }150.000$)**.

### 🤝 Untuk Anda yang Ingin Merakit Sendiri (*DIY Builders*):
* **Silakan rakit dan gunakan secara bebas!** Proyek ini telah dilengkapi daftar komponen lengkap (22 item katalog), skematik wiring presisi, dan panduan foto langkah demi langkah.
* **Tips Perakitan Penting:**
  1. Pastikan jalur suplai daya **5V** ke ESP32 dan MAX485 menggunakan kabel tembaga berkualitas agar tegangan tidak drop saat transmisi WiFi berdaya penuh.
  2. Tambahkan kapasitor *decoupling* keramik ($100\,	ext{nF}$) dekat pin VCC-GND MAX485 untuk menyaring *noise* frekuensi tinggi.
  3. Untuk kabel DMX panggung yang sangat panjang (>50 meter), pasang resistor terminasi $120\,\Omega$ ($0.25	ext{W}$) di antara Pin 2 ($D-$) dan Pin 3 ($D+$) pada fixture lampu terakhir untuk mencegah pantulan sinyal (*signal reflection*).

### 🍴 Untuk Anda yang Ingin Memodifikasi atau Mem-Fork (*Forkers & Modders*):
* **Anda sangat dipersilakan untuk mem-fork repositori ini!** Buatlah inovasi baru: ubah desain PCB ke custom SMD PCB KiCad, tambahkan layar OLED, integrasikan protokol sACN E1.31, atau buatkan versi multi-universe.
* **Etika Open-Source:** Kami sangat menghargai jika Anda tetap mencantumkan nama pembuat asli (*credit attribution*) dan membagikan kembali perbaikan atau fitur baru Anda ke komunitas!

### 🚀 Untuk Anda yang Ingin Berkontribusi (*Contributors*):
* Temukan bug atau punya ide optimasi? Silakan buka **Issue** atau kirimkan **Pull Request (PR)** di repositori GitHub kami ([`@zzdree`](https://github.com/zzdree)).
* Kami sangat terbuka terhadap masukan, dokumentasi terjemahan bahasa lain, perbaikan timing serial, atau penambahan fitur baru di roadmap di atas.

---

<div align="center">
  <b>⭐ Jangan lupa berikan bintang (Star) pada repositori ini jika bermanfaat bagi pertunjukan atau proyek Anda! ⭐</b><br/>
  <i>Happy Making, Happy Lighting, and Keep Rocking the Stage! 💡🎛️🚀</i>
  <br/><br/>
  <b>Salam hangat,</b><br/>
  <b>Andreas Restuawanta Christwara</b> (<a href="https://github.com/zzdree">@zzdree</a>)<br/>
  <i>Teknik Komputer — Universitas Negeri Semarang (UNNES)</i>
</div>
