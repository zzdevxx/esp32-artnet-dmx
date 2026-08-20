# 📋 Product Requirements Document (PRD)
# ZZLIGHT Art-Net to DMX512 Converter Module

> **Dokumen Spesifikasi Kebutuhan Produk & Arsitektur Sistem Node Perangkat Keras**  
> *Hardware Receiver Node Converter untuk Software Otomasi Pencahayaan Panggung Cerdas ZZLUXORA.*

---

## 📌 Metadata Dokumen
- **Nama Produk:** ZZLIGHT Art-Net to DMX512 Converter Module
- **Kode Proyek:** ZZLIGHT-HW-01
- **Versi Produk:** v5.0.0 (Final Release)
- **Target Board / Core:** ESP32 DevKit V1 (ESP-WROOM-32 30-Pin) / Arduino ESP32 Core v3.x
- **Peneliti / Penulis:** Andreas Restuawanta Christwara (`NIM: 5312422036`)
- **Dosen Pembimbing:** Khoirudin Fathoni, S.T., M.T. (`NIP: 19900929292015041001`)
- **Institusi:** Program Studi Teknik Komputer, Jurusan Teknik Elektro, Fakultas Teknik, Universitas Negeri Semarang (UNNES)
- **Status Dokumen:** **APPROVED & PRODUCTION READY**

---

## 1. 📖 Executive Summary & Latar Belakang

### 1.1 Problem Statement
Pada instalasi tata cahaya panggung (*stage lighting*) modern seperti di gereja dan gedung konser, kabel transmisi sinyal DMX512 konvensional yang ditarik dari meja operator (*front of house / FOH*) ke panggung seringkali rentan putus, memakan waktu setup yang lama, dan menimbulkan kekacauan kabel (*cable clutter*). Modul konverter komersial yang ada di pasaran umumnya berharga mahal, tertutup (*proprietary*), dan tidak memiliki indikator visual status jaringan lokal secara langsung.

### 1.2 Tujuan Produk (Objective)
Membangun modul konverter node **Art-Net to DMX512 nirkabel mandiri** yang:
1. Menerima paket data kontrol pencahayaan nirkabel **Art-Net 4 (UDP Port 6454)** dari software lighting (*ZZLUXORA*, *QLC+*, *grandMA3*).
2. Mengonversinya menjadi sinyal serial diferensial **RS-485 DMX512 (250 kbps, 8N2)** untuk mengendalikan fixture lampu fisik (*PAR LED RGBW, Moving Head Beam, Strobe*).
3. Menyediakan antarmuka monitoring status visual perangkat keras secara langsung melalui **Layar LCD 16x2 I2C** dan **Web Portal Responsif (Captive Portal)**.
4. Menjamin transmisi DMX bebas kedip (*jitter-free*) menggunakan arsitektur **FreeRTOS Dual-Core** dan **Double Buffering**.

---

## 2. 🛒 Hardware Specifications & Bill of Materials (BOM)

| No | Komponen | Model / Tipe | Spesifikasi Kunci | Fungsi dalam Sistem |
|:--:|:---|:---|:---|:---|
| 1 | **Mikrokontroler** | ESP32 DevKit V1 | Dual-Core 240 MHz, 520 KB SRAM, WiFi 802.11 b/g/n | Unit pemroses sentral jaringan & DMX engine |
| 2 | **Transceiver RS-485** | MAX485 TTL-to-RS485 | Half-Duplex, Baudrate s/d 2.5 Mbps, 5V VCC | Mengubah sinyal UART TTL menjadi RS-485 diferensial |
| 3 | **Display Layar** | LCD 16x2 + I2C Backpack | Modul PCF8574, I2C Address `0x27` / `0x3F` | Menampilkan status koneksi, IP, Universe, & FPS DMX |
| 4 | **Konektor Output** | XLR 3-Pin Female | Panel Mount Chassis XLR | Antarmuka fisik colokan kabel DMX standar industri |
| 5 | **Box Enclosure** | Box Plastik X4 (Hitam) | Dimensi $120 	imes 85 	imes 40	ext{ mm}$ | Kotak pelindung sirkuit modular siap pakai di panggung |
| 6 | **Papan Rangkaian** | PCB Matrix / Rawboard | Single-sided Perfboard $50 	imes 70	ext{ mm}$ | Papan penyolderan komponen modular & header |
| 7 | **Regulator Daya** | LM2596 / Adaptor 5V | Input 7–12V DC, Output 5.0V DC 2A | Pemasok daya stabil 5V ke Vin ESP32, MAX485, LCD |

---

## 3. 🔌 Hardware Wiring & Pinout Contract

```text
+-----------------------------------------------------------------------------------------+
|                                TABEL KONEKSI PIN ELEKTRONIKA                            |
+-------------------+--------------------+------------------------+-----------------------+
| Komponen Asal     | Pin Komponen Asal  | Komponen Tujuan        | Pin Komponen Tujuan   |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | Modul MAX485           | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul MAX485           | GND                   |
| ESP32 DevKit V1   | GPIO 4 (D4)        | Modul MAX485           | DE & RE (Dijumper)    |
| ESP32 DevKit V1   | GPIO 17 (TX2)      | Modul MAX485           | DI (Driver Input)     |
| ESP32 DevKit V1   | GPIO 16 (RX2)      | Modul MAX485           | RO (Tidak Terpakai)   |
+-------------------+--------------------+------------------------+-----------------------+
| Modul MAX485      | Pin A              | Konektor XLR 3-Pin     | Pin 3 (DMX Data +)    |
| Modul MAX485      | Pin B              | Konektor XLR 3-Pin     | Pin 2 (DMX Data -)    |
| Modul MAX485      | Pin GND            | Konektor XLR 3-Pin     | Pin 1 (DMX Shield)    |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | Modul I2C LCD 16x2     | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul I2C LCD 16x2     | GND                   |
| ESP32 DevKit V1   | GPIO 21 (D21)      | Modul I2C LCD 16x2     | SDA                   |
| ESP32 DevKit V1   | GPIO 22 (D22)      | Modul I2C LCD 16x2     | SCL                   |
+-------------------+--------------------+------------------------+-----------------------+
```

---

## 4. 🧠 Arsitektur Sistem & Konkurensi FreeRTOS

Firmware modul mengimplementasikan pembagian beban kerja berbasis **FreeRTOS Symmetric Multiprocessing (SMP)**:

```text
+-----------------------------------------------------------------------------------------+
|                                ARSITEKTUR FREERTOS DUAL-CORE                            |
+-----------------------------------------------------------------------------------------+
  [CORE 0: DMX DEDICATED REAL-TIME TASK]          [CORE 1: NETWORKING & UI TASK]
  • Prioritas: configMAX_PRIORITIES - 1           • Prioritas: Standard Arduino loop()
  • Refresh Rate: 44 Hz Deterministik             • Socket: UDP Port 6454 (WiFiUDP)
  • Sinyal DMX512 Timing Generator:               • Parser: ArtDmx Payload Validation
    - Break Time : LOW selama >= 88 µs            • Mutex: taskENTER_CRITICAL(&mux)
    - MAB Time   : HIGH selama >= 8 µs            • WebServer: HTTP Port 80 & DNS Port 53
    - Start Code : 0x00 (Null Byte)               • UI Engine: LCD 16x2 State Machine (500ms)
    - Channels   : 512 Bytes (250 kbps 8N2)
            │                                             │
            └──────────────┬──────────────────────────────┘
                           ▼
              [DOUBLE BUFFERING / PING-PONG]
              • writeBuffer (Core 1) <---> readBuffer (Core 0)
              • Bebas Race Condition & Bebas Frame Tearing
+-----------------------------------------------------------------------------------------+
```

---

## 5. 📟 Spesifikasi Lengkap State Machine LCD 16x2 (11 State)

Layar LCD 16x2 menyajikan transisi status operasional modul melalui 11 kondisi tampilan visual:

| No | Nama State Layar | Baris 1 (Line Atas) | Baris 2 (Line Bawah) | Durasi | Kondisi Pemicu & Perilaku |
|:--:|:---|:---|:---|:---:|:---|
| **1** | **Boot Screen** | `     ZZLIGHT    ` | `     PROJECT    ` | 3 Detik | Inisialisasi awal modul. Animasi *typing effect* huruf muncul satu persatu rata tengah. |
| **2** | **Connecting Screen** | `Connecting...   ` | *(Kosong)* | $\ge$ 3 Detik | Modul mencoba terhubung ke router WiFi atau saat client menghubungkan ke hotspot AP. |
| **3** | **WiFi Not Connect** | `WiFi Not Connect!` | `Connecting...   ` | 3 Detik | Modul belum punya data WiFi tersimpan atau gagal konek ke router. Selanjutnya beralih ke State 4 (AP Mode). |
| **4** | **WiFi Setup (AP Mode)** | `ARTNET-DMX ) ) )` | `192.168.4.1     ` | Loop Aktif | Hotspot mandiri aktif (`ZZLIGHT-AP`). Baris 1 menampilkan animasi sinyal looping 6-step (`ARTNET-DMX` s/d `ARTNET-DMX )))))`). |
| **5** | **Device Connect** | `Device Connect! ` | `Loading...      ` | 3 Detik | Smartphone / laptop berhasil terhubung ke hotspot WiFi modul. Setelah 3 detik kembali ke State 4. |
| **6** | **Module Connect 1** | `Module Connect! ` | `Rebooting...    ` | 3 Detik | Modul menerima konfigurasi WiFi baru dari web portal, lalu melakukan restart sistem. |
| **7** | **Module Connect 2** | `Module Connect! ` | `Loading...      ` | 3 Detik | Konfirmasi koneksi WiFi berhasil didapatkan setelah proses auto-connect saat boot. |
| **8** | **Run Screen (Standby)** | `ARTNET-DMX >>>>>` | `192.168.X.X     ` | Loop Aktif | Mode operasional utama. Baris 1 animasi panah looping 6-step (`ARTNET-DMX` s/d `ARTNET-DMX >>>>>`), Baris 2 menampilkan IP lokal. |
| **9** | **WiFi Reset Screen** | `WiFi Reset!     ` | `Rebooting...    ` | 3 Detik | Tombol Reset pada Web Monitor ditekan. Modul menghapus kredensial WiFi dan reboot menuju State 4. |
| **10a**| **WiFi Disconnect (<30s)**| `WiFi Disconnect!` | `Reconnecting... ` | Max 30s | Sinyal WiFi terputus sementara. Modul mencoba reconnecting otomatis. |
| **10b**| **WiFi Disconnect (>30s)**| `WiFi Disconnect!` | `Rebooting       ` | 3 Detik | Sinyal WiFi mati $>30$ detik. Modul melakukan reboot otomatis menuju State 4 (AP Mode). |
| **11**| **WiFi Failed** | `WiFi Failed!    ` | `Wrong Password..` | 3 Detik | Pengguna salah memasukkan password WiFi di web portal. Modul kembali ke State 4. |

---

## 6. 🌐 Spesifikasi Web Portal & Captive Portal

### 6.1 Parameter Hotspot Access Point:
* **SSID Hotspot:** `ARTNET-DMX`
* **Password Hotspot:** `andreas123`
* **IP Default AP:** `192.168.4.1`
* **Captive Portal:** Menggunakan `DNSServer` port 53 untuk meredireksi semua domain (`*`) ke `192.168.4.1`.

### 6.2 Halaman Konfigurasi WiFi (`http://192.168.4.1/setup` atau `/`):
- **Desain:** Dark theme, typography tebal miring (*Black Italic*) pada header bertuliskan **"ZZLIGHT PROJECT"**.
- **Fitur Scan Jaringan:** Menampilkan daftar SSID WiFi di sekitar dengan ikon sinyal, indikator gembok keamanan, dan persentase kekuatan sinyal (0–100%).
- **Autofill Cerdas:** Mengetuk salah satu nama WiFi otomatis mengisi kotak input SSID.
- **Form Input:** Kolom teks putih untuk SSID dan Password.
- **Tombol Aksi:** Tombol **Connect** (Hijau) dan tombol **Refresh Scan** (Oranye).
- **Footer Link:** Tautan teks abu-abu menuju halaman `/monitor`.

### 6.3 Halaman Web Monitor (`http://192.168.X.X` / `http://zzlight.local` / `/monitor`):
- **Indikator Real-Time:**
  - **Date & Time:** Tanggal (Format: *Senin, 24 September 2026*) dan Jam Digital 24-jam sinkron client-side (*XX:XX:XX*).
  - **IP Address:** Alamat IP lokal aktif.
  - **WiFi SSID:** Nama access point yang terhubung.
  - **Connection Status:** `Connected` (Hijau) / `Disconnected` (Merah).
  - **Signal Strength:** Nilai RSSI dBm dengan status `Good` (Hijau) / `Medium` (Kuning) / `Weak` (Merah).
  - **DMX Output Status:**
    - `Active` (Hijau) — Paket Art-Net diterima dalam $< 1	ext{ detik}$.
    - `No Signal` (Kuning) — Tidak ada paket selama $1	ext{ s/d }10	ext{ detik}$.
    - `Loss Signal` (Merah) — Tidak ada paket selama $> 10	ext{ detik}$ (Blackout Safety).
- **Tombol Kontrol Pemeliharaan:**
  - **WiFi Reset:** Tombol merah dengan dialog konfirmasi (*"Reset WiFi?"*).
  - **Module Reboot:** Tombol oranye dengan dialog konfirmasi (*"Reboot Module?"*).

---

## 7. ⚡ Non-Functional Requirements & Standar Kualitas

1. **Kecepatan & Refresh Rate DMX:** 
   - Transmisi DMX512 berjalan stabil pada **44 Hz (22.7 ms per frame)** untuk 512 channel penuh tanpa interupsi.
2. **Latensi Transmisi Jaringan:** 
   - Latensi penerimaan paket UDP Art-Net dari PC ke output RS-485 terukur **$< 40	ext{ ms}$**.
3. **Blackout Safety Engine:** 
   - Jika sinyal Art-Net terputus lebih dari 10 detik, modul otomatis mengirim sinyal nilai 0 ke seluruh channel DMX untuk mencegah lampu menyala liar.
4. **Ketahanan Memori Flash (NVS Preferences):** 
   - Menggunakan partisi NVS terstruktur untuk mencegah keausan flash (*wear leveling*) dan korupsi data saat terjadi mati listrik mendadak.
5. **Anti-Flicker LCD Rendering:** 
   - Update LCD 16x2 dibatasi hanya saat data berubah (*differential update*) untuk menghindari flicker backlight dan glitch karakter.

---

## 8. 🧪 Kriteria Penerimaan & Verifikasi Pengujian (Acceptance Criteria)

| ID Uji | Skenario Pengujian | Hasil yang Diharapkan | Status |
|:---:|:---|:---|:---:|
| **TC-01** | Booting awal perangkat dari kondisi mati | LCD menampilkan animasi typing 3 detik "ZZLIGHT PROJECT" | ✅ PASSED |
| **TC-02** | Perangkat belum memiliki data WiFi | LCD masuk State 3 (WiFi Not Connect), lalu membuka AP `192.168.4.1` | ✅ PASSED |
| **TC-03** | Smartphone terhubung ke hotspot `ARTNET-DMX` | Captive portal terbuka otomatis, scan WiFi menampilkan SSID sekitar | ✅ PASSED |
| **TC-04** | Pengguna input password dan klik Connect | Data tersimpan ke NVS, LCD menampilkan Module Connect 1, lalu reboot | ✅ PASSED |
| **TC-05** | Perangkat terhubung ke router WiFi | LCD menampilkan IP lokal dan FPS DMX (State 8), web monitor aktif | ✅ PASSED |
| **TC-06** | Software ZZLUXORA / QLC+ mengirim paket ArtDmx | Modul mengonversi ke RS-485, lampu PAR LED RGBW merespon instan | ✅ PASSED |
| **TC-07** | Router WiFi dimatikan saat lampu beroperasi | LCD menampilkan WiFi Disconnect, jika >30s modul reboot ke AP mode | ✅ PASSED |

---

## 9. 📜 Hak Cipta & Kepemilikan Intelektual

Dokumen PRD ini dan seluruh source code yang menyertainya merupakan karya intelektual dari:  
**Andreas Restuawanta Christwara ([@zzdree](https://github.com/zzdree))**  
NIM: 5312422036 — Program Studi Teknik Komputer, Universitas Negeri Semarang.  
Dilisensikan di bawah **MIT License**.
