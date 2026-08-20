# 📋 Product Requirements Document (PRD)
# ZZLIGHT Art-Net to DMX512 Converter Module

> **Dokumen Spesifikasi Kebutuhan Produk & Arsitektur Sistem Node Perangkat Keras**  
> *Hardware Receiver Node Converter untuk Software Otomasi Pencahayaan Panggung Cerdas ZZLUXORA.*

---

## 📌 Metadata Dokumen
- **Nama Produk:** ZZLIGHT Art-Net to DMX512 Converter Module
- **Kode Proyek:** ZZLIGHT-HW-01
- **Versi Produk:** v4.0.0 (Final Production Release)
- **Target Board / Core:** ESP32 DevKit V1 (ESP-WROOM-32 30-Pin) / Arduino ESP32 Core v3.x
- **Peneliti / Penulis:** Andreas Restuawanta Christwara (`NIM: 5312422036`)
- **Dosen Pembimbing:** Khoirudin Fathoni, S.T., M.T. (`NIP: 19900929292015041001`)
- **Institusi:** Program Studi Teknik Komputer, Jurusan Teknik Elektro, Fakultas Teknik, Universitas Negeri Semarang (UNNES)
- **Sesi Riset AI:** [Google Gemini Research Session #9b15b21a9066dd07](https://gemini.google.com/app/9b15b21a9066dd07)
- **Status Dokumen:** **APPROVED & PRODUCTION READY (GRADE A)**

---

## 1. 📖 Executive Summary & Latar Belakang

### 1.1 Problem Statement
Pada instalasi tata cahaya panggung (*stage lighting*) modern seperti di gedung pertunjukan, gereja, dan konser musik, kabel transmisi sinyal DMX512 konvensional yang ditarik dari meja operator (*front of house / FOH*) ke panggung seringkali rentan putus, memakan waktu setup yang lama, dan menimbulkan kekacauan kabel (*cable clutter*). Modul konverter komersial yang ada di pasaran umumnya berharga mahal (jutaan rupiah), tertutup (*proprietary*), dan tidak memiliki indikator visual status jaringan lokal secara langsung.

### 1.2 Tujuan Produk (Objective)
Membangun modul konverter node **Art-Net to DMX512 nirkabel mandiri** yang:
1. Menerima paket data kontrol pencahayaan nirkabel **Art-Net 4 (UDP Port 6454)** dari software lighting (*ZZLUXORA*, *QLC+*, *grandMA3*).
2. Mengonversinya menjadi sinyal serial diferensial **RS-485 DMX512 (250 kbps, 8N2)** untuk mengendalikan fixture lampu fisik (*PAR LED RGBW, Moving Head Beam, Strobe*).
3. Menyediakan antarmuka monitoring status visual perangkat keras secara langsung melalui **Layar LCD 16x2 I2C** dan **Web Portal Responsif (Captive Portal)**.
4. Menjamin transmisi DMX bebas kedip (*jitter-free*) menggunakan arsitektur **FreeRTOS Dual-Core** dan **Double Buffering**.

---

## 2. 🛒 Hardware Specifications & Bill of Materials (BOM)

| Indeks Gambar | Komponen / Alat | Model / Tipe | Spesifikasi Kunci | Fungsi dalam Sistem |
|:---:|:---|:---|:---|:---|
| **Gambar 2.1** | Mikrokontroler | ESP32 DevKit V1 | Dual-Core 240 MHz, 520 KB SRAM, WiFi 802.11 b/g/n | Unit pemroses sentral jaringan & DMX engine |
| **Gambar 2.2** | Transceiver RS-485 | MAX485 Module | Half-Duplex TTL to RS-485, 5V VCC, 250 kbps | Mengubah sinyal UART TTL menjadi RS-485 diferensial |
| **Gambar 2.3** | Display Layar | LCD 16x2 + I2C | PCF8574 Backpack, Address `0x27`, Blue Backlight | Menampilkan status koneksi, IP, Universe, & FPS DMX |
| **Gambar 2.4** | Konektor Output | XLR 3-Pin Female | Panel Mount Metal Chassis, 3 Solder Lugs | Antarmuka fisik colokan kabel DMX standar industri |
| **Gambar 2.5** | Box Enclosure | Box Plastik X4 | Plastik ABS Hitam Matte, $120 \times 85 \times 40\text{ mm}$ | Kotak pelindung sirkuit modular siap pakai di panggung |
| **Gambar 2.6** | Papan Rangkaian | PCB Matrix (Rawboard) | Single-Sided Copper Dot Matrix $50 \times 70\text{ mm}$ | Papan penyolderan komponen modular & header |
| **Gambar 2.7** | Pin Header | Male & Female Strips | Pitch 2.54 mm, Straight Pins, Gold Plated | Dudukan soket lepasan modular ESP32 dan MAX485 |
| **Gambar 2.8** | Kabel Jumper | DuPont Wires | AWG 24–26, Multi-Warna, Panjang 20 cm | Penghubung jalur daya 5V, GND, UART2, dan I2C |
| **Gambar 2.9** | Kabel Daya & Data | USB to Micro-USB | Tembaga Murni High-Speed, Panjang 1.0 m | Penyuplai daya 5V DC dan jalur flashing firmware |
| **Gambar 2.10** | Timah Solder | Rosin Core Solder | Paduan 60/40 Tin/Lead, Diameter 0.8 mm | Perekat elektrikal konduktif antar komponen |
| **Gambar 2.11** | Solder Listrik | Soldering Iron 60W | Keramik Heater 60W, Temperature Controlled | Alat penyambung kaki komponen dan perkabelan |
| **Gambar 2.12** | Sedotan Timah | Desoldering Pump | Atraktor Manual Bodi Aluminium | Pembersih timah sisa dan perbaikan penyolderan |
| **Gambar 2.13** | Multimeter Digital | Digital Multimeter | Pengukur VDC, Arus, Resistansi, Kontinuitas Buzzer | Instrumen pengujian bebas hubung singkat jalur PCB |
| **Gambar 2.14** | Breadboard | Solderless Breadboard | Prototipe 830 Titik Koneksi | Media pengujian sirkuit sementara fase eksperimen |
| **Gambar 2.15** | Tang Potong Sisi | Diagonal Flush Cutters | Carbon Steel, Spring Loaded, Handle Karet | Pemotong kaki pin header dan perapian kawat |
| **Gambar 2.16** | Mata Bor Step Drill | Step Drill Bit HSS | Titanium Nitride HSS 4–22 mm Kerucut | Pembuat lubang sasis soket XLR pada box X4 |
| **Gambar 2.17** | Set Obeng Presisi | Precision Screwdrivers | Tip Phillips (+) & Flathead (-), Knurled Body | Pengencang baut sasis XLR dan penutup box X4 |
| **Gambar 2.18** | Kabel DMX Panggung | DMX Cable 3 Meter | 120 Ohm Shielded Twisted Pair, XLR M-F | Penghubung sinyal DMX dari modul ke fixture panggung |
| **Gambar 2.19** | Komputer Host | Laptop Acer Swift 3 | Intel Core / Ryzen, 14-inch, Windows 11 | Host pengirim Art-Net UDP (ZZLUXORA / QLC+) |
| **Gambar 2.20** | Beban Uji Panggung | PAR LED 54 Mata | 54x3W RGBW LEDs, Standar DMX512-A | Beban uji aktuasi fisik penerimaan channel DMX |
| **Gambar 2.21** | Pasta Solder Flux | Soldering Rosin Paste | Grease Flux Paste Non-Corrosive | Pembersih lapisan oksidasi tembaga saat menyolder |
| **Gambar 2.22** | Gunting Craft | Workshop Scissors | Stainless Steel Blades, Handle Ergonomis | Pemotong isolasi bakar dan perapian jalur kabel |

---

## 3. 🖼️ Visual Assets & Architecture Cross-Reference

| Indeks Visual | Nama Berkas | Lokasi File | Deskripsi & Peran Arsitektural |
|:---:|:---|:---|:---|
| **Gambar 1.1** | `RESULT.png` | `C:\ANDREAS\esp32_artnet_dmx\RESULT.png` | Banner Hasil Akhir 16:9 (Laptop Acer Swift 3, Box X4, & 4 PAR LED Ungu, Merah, Oranye, Hijau) |
| **Gambar 2.1 – 2.22** | 22 Berkas Katalog | `C:\ANDREAS\esp32_artnet_dmx\images\` | Katalog Alat dan Bahan Berlatar Belakang Putih Murni (`#FFFFFF`) |
| **Gambar 3.1 – 3.5** | 5 Berkas Skematik | `C:\ANDREAS\esp32_artnet_dmx\schematics\` | Diagram Blok Sistem, Skematik Wiring, Timing DMX512, FreeRTOS SMP, & Flowchart LCD |
| **Gambar 4.1 – 4.30** | 30 Foto Fabrikasi | `C:\ANDREAS\esp32_artnet_dmx\docs\` | Dokumentasi Autentik Perakitan Fisik, Pelubangan Box X4, Penyolderan, & Uji Coba |

---

## 4. 🔌 Hardware Wiring & Pinout Contract

```text
+-----------------------------------------------------------------------------------------+
|                                TABEL KONEKSI PIN ELEKTRONIKA                            |
+-------------------+--------------------+------------------------+-----------------------+
| Komponen Asal     | Pin Komponen Asal  | Komponen Tujuan        | Pin Komponen Tujuan   |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | Modul MAX485           | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul MAX485           | GND                   |
| ESP32 DevKit V1   | GPIO 4 (D4)        | Modul MAX485           | DE & /RE (Dijumper)   |
| ESP32 DevKit V1   | GPIO 17 (TX2)      | Modul MAX485           | DI (Driver Input)     |
| ESP32 DevKit V1   | GPIO 16 (RX2)      | Modul MAX485           | RO (Tidak Terpakai)   |
+-------------------+--------------------+------------------------+-----------------------+
| Modul MAX485      | Pin A (Data +)     | Konektor XLR 3-Pin     | Pin 3 (DMX Data +)    |
| Modul MAX485      | Pin B (Data -)     | Konektor XLR 3-Pin     | Pin 2 (DMX Data -)    |
| Modul MAX485      | Pin GND            | Konektor XLR 3-Pin     | Pin 1 (DMX Shield)    |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | Modul I2C LCD 16x2     | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul I2C LCD 16x2     | GND                   |
| ESP32 DevKit V1   | GPIO 21 (D21)      | Modul I2C LCD 16x2     | SDA                   |
| ESP32 DevKit V1   | GPIO 22 (D22)      | Modul I2C LCD 16x2     | SCL                   |
+-------------------+--------------------+------------------------+-----------------------+
```

---

## 5. 🧠 Arsitektur FreeRTOS Dual-Core SMP & Double Buffering

```text
+-----------------------------------------------------------------------------------------+
|                                ARSITEKTUR FREERTOS DUAL-CORE                            |
+-----------------------------------------------------------------------------------------+
  [CORE 0: DMX DEDICATED REAL-TIME TASK]          [CORE 1: NETWORKING & UI TASK]
  • Prioritas: configMAX_PRIORITIES - 1           • Prioritas: Standard Arduino loop()
  • Refresh Rate: 44.1 Hz Deterministik           • Socket: UDP Port 6454 (WiFiUDP)
  • Sinyal DMX512 Timing Generator:               • Parser: ArtDmx Payload Validation
    - Break Time : LOW selama >= 88 µs            • Mutex: portENTER_CRITICAL(&dmxMux)
    - MAB Time   : HIGH selama >= 8 µs            • WebServer: HTTP Port 80 & DNS Port 53
    - Start Code : 0x00 (Null Byte)               • UI Engine: LCD 16x2 State Machine (500ms)
    - Channels   : 512 Bytes (250 kbps 8N2)
            │                                             │
            └──────────────┬──────────────────────────────┘
                           ▼
              [DOUBLE BUFFERING / PING-PONG]
              • writeBuffer (Core 1) <---> readBuffer (Core 0)
              • Bebas Race Condition & Bebas Frame Tearing (<0.05 µs Jitter)
+-----------------------------------------------------------------------------------------+
```

---

## 6. 🌐 Captive Portal, 11-State LCD, & Fail-Safe Blackout

1. **Captive Portal DNS Hijacking:** Menggunakan `DNSServer` port 53 untuk mengarahkan seluruh query DNS (`*`) ke IP `192.168.4.1`, memicu pop-up login otomatis di Android, iOS, dan Windows.
2. **11-State Machine LCD:** Menampilkan status dinamis mulai dari Booting, Scanning, AP Mode, Connected, Live FPS Monitor, hingga Error States secara diferensial.
3. **Blackout Safety Timeout (10 Detik):** Jika paket Art-Net tidak diterima selama lebih dari 10 detik, modul otomatis mengirimkan nilai `0` ke seluruh 512 channel untuk memadamkan lampu panggung dan mencegah panas berlebih pada fixture.
