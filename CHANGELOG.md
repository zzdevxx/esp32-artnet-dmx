# 📋 Changelog & Development Roadmap — ESP32 Art-Net to DMX512 Node (ZZLIGHT)

Dokumentasi komprehensif riwayat perancangan, evolusi kode firmware (v1.0 s/d v4.0 Final), aset visual panggung terindeks, peta jalan pengembangan masa depan (*future roadmap*), serta panduan bagi komunitas maker, pengembang, dan kontributor (*forks & pull requests*).

Format pencatatan mengikuti standar industri [*Keep a Changelog*](https://keepachangelog.com/id/1.0.0/) dan penomoran versi semantik [*Semantic Versioning (SemVer)*](https://semver.org/).

> 💡 **Catatan Berkas:** Berkas `firmware/artnet_dmx_final.ino` merupakan versi resmi (*production-ready*) dari `firmware/artnet_dmx_v4.txt` yang siap dikompilasi langsung di Arduino IDE.

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

### ⚠️ Limitations Identified (Catatan Evaluasi)
- Model *single-threaded* menyebabkan kedipan (*flicker*) pada lampu panggung setiap kali terjadi fluktuasi latensi jaringan WiFi atau pemrosesan request HTTP.
- Memerlukan arsitektur *dual-core* dan proteksi *memory buffer* untuk mencapai standar kehandalan tingkat produksi / panggung langsung.

---

## 🔮 Roadmap & Ide Pengembangan Masa Depan (Future Roadmap)

Bagi siapa pun yang ingin mengembangkan proyek ini lebih jauh, berikut adalah daftar fitur dan peningkatan arsitektur yang sangat direkomendasikan untuk versi berikutnya:

1. 🌐 **Multi-Protocol Support (sACN / E1.31):** Streaming Architecture for Control Networks (E1.31 multicast) berdampingan dengan Art-Net 4.
2. 🎛️ **Multi-Universe Hardware Outputs (2–4 Universes):** Menggunakan periferal UART0/UART1/RMT untuk 2–4 port XLR fisik independen (1024–2048 channel).
3. 🔄 **Bidirectional RDM (ANSI E1.20):** Diagnostik suhu fixture lampu panggung dan konfigurasi start address dari jarak jauh.
4. ⚡ **Hardwired Ethernet (W5500 / LAN8720 PoE):** Jalur kabel RJ45 fisik bebas interferensi radio WiFi untuk panggung stadion besar.
5. ☁️ **Over-The-Air (OTA) Web Firmware Update:** Pembaruan firmware drag-and-drop `.bin` file via web browser.
6. 🔍 **Auto-Discovery Engine (ArtPoll & ArtPollReply):** Auto-deteksi nama node di konsol lighting (*grandMA3, Avolites, Chamsys*).
7. 📟 **OLED SSD1306 & NeoPixel RGB Status:** Tampilan visual bar-graph channel grafis dan cincin status RGB yang dinamis.
8. 💾 **Standalone DMX Scene Recorder & Playback:** Perekam adegan panggung ke Flash/MicroSD tanpa butuh laptop menyala.

---

## 💬 Pesan Hangat untuk Pengguna, Pembuat (Makers), & Kontributor (Forks)

> Halo rekan-rekan sesama *developer*, *lighting designer*, teknisi panggung, mahasiswa, dan komunitas *DIY maker* di mana pun Anda berada! 👋✨

Proyek **ESP32 Wireless Art-Net to DMX512 Node (ZZLIGHT)** ini saya dedikasikan dan rilis sepenuhnya sebagai **perangkat lunak dan desain perangkat keras Open Source (MIT License)**. Proyek ini berawal dari kebutuhan nyata di lapangan akan modul konverter DMX nirkabel yang **andal, berkecepatan tinggi, bebas jitter (*zero-jitter*), namun dapat dirakit sendiri dengan biaya sangat terjangkau ($<\$10 / <\text{Rp }150.000$)**.

### 🤝 Untuk Anda yang Ingin Merakit Sendiri (*DIY Builders*):
* **Silakan rakit dan gunakan secara bebas!** Proyek ini telah dilengkapi daftar komponen lengkap (22 item katalog), skematik wiring presisi, dan panduan foto langkah demi langkah.
* **Tips Perakitan Penting:**
  1. Pastikan jalur suplai daya **5V** ke ESP32 dan MAX485 menggunakan kabel tembaga berkualitas agar tegangan tidak drop saat transmisi WiFi berdaya penuh.
  2. Tambahkan kapasitor *decoupling* keramik ($100\,\text{nF}$) dekat pin VCC-GND MAX485 untuk menyaring *noise* frekuensi tinggi.
  3. Untuk kabel DMX panggung yang sangat panjang (>50 meter), pasang resistor terminasi $120\,\Omega$ ($0.25\text{W}$) di antara Pin 2 ($D-$) dan Pin 3 ($D+$) pada fixture lampu terakhir untuk mencegah pantulan sinyal (*signal reflection*).

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
