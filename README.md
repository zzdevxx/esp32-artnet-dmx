# 💡 ESP32 Art-Net to DMX512 Transceiver Module (ZZLIGHT PROJECT)

> **Tutorial & Dokumentasi Lengkap Pembuatan Modul Node Art-Net DMX512 Nirkabel Berbasis ESP32 DevKit V1, Transceiver MAX485, dan Layar LCD 16x2 I2C.**  
> *Hardware Node Converter untuk Software Otomasi Pencahayaan Panggung Cerdas ZZLUXORA (Universitas Negeri Semarang).*

---

![ESP32](https://img.shields.io/badge/MCU-ESP32_DevKit_V1-red?style=flat-square&logo=espressif&logoColor=white)
![Protocol](https://img.shields.io/badge/Protocol-Art--Net_4_%2F_DMX512-orange?style=flat-square)
![Baud Rate](https://img.shields.io/badge/Baud_Rate-250_kbps_(8N2)-blue?style=flat-square)
![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS_Dual--Core-green?style=flat-square)
![Display](https://img.shields.io/badge/Display-LCD_16x2_I2C_(0x27)-purple?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-brightgreen?style=flat-square)

---

## 📑 Daftar Isi
1. [Gambaran Umum Modul](#-1-gambaran-umum-modul)
2. [Daftar Komponen Hardware (BOM)](#-2-daftar-komponen-hardware-bom)
3. [Skematik & Tabel Wiring Lengkap](#-3-skematik--tabel-wiring-lengkap)
4. [Arsitektur Firmware & FreeRTOS Dual-Core](#-4-arsitektur-firmware--freertos-dual-core)
5. [State Machine & Tampilan LCD 16x2 (8 Layar)](#-5-state-machine--tampilan-lcd-16x2-8-layar)
6. [Fitur Captive Portal & Web Configuration](#-6-fitur-captive-portal--web-configuration)
7. [Panduan Instalasi & Flashing via Arduino IDE](#-7-panduan-instalasi--flashing-via-arduino-ide)
8. [Panduan Pengujian dengan Software Lighting](#-8-panduan-pengujian-dengan-software-lighting)
9. [Troubleshooting & Solusi Masalah Hardware](#-9-troubleshooting--solusi-masalah-hardware)
10. [Lisensi & Hak Cipta](#-10-lisensi--hak-cipta)

---

## 📖 1. Gambaran Umum Modul

Modul **ZZLIGHT Art-Net to DMX512** adalah perangkat keras konverter (*node receiver*) yang menerima paket data pencahayaan **Art-Net (ArtDmx OpCode 0x5000)** melalui jaringan WiFi nirkabel pada port **UDP 6454**, kemudian mengonversinya secara *real-time* menjadi sinyal serial diferensial **RS-485 DMX512 standar industri (USITT DMX512-A, 250 kbps, 8N2)** untuk menggerakkan fixture lampu panggung (*Moving Head, PAR LED RGBW, Strobe, Laser*).

Modul ini dirancang dengan arsitektur **FreeRTOS Dual-Core**, di mana penerimaan jaringan WiFi dan pengiriman serial DMX diisolasi pada inti prosesor yang berbeda guna menjamin sinyal DMX512 tetap stabil tanpa *flicker* (*jitter-free* 44 Hz).

---

## 🛒 2. Daftar Komponen Hardware (BOM)

| No | Komponen | Spesifikasi / Model | Keterangan |
|:--:|:---|:---|:---|
| 1 | **Mikrokontroler** | ESP32 DevKit V1 (30-Pin / 38-Pin ESP-WROOM-32) | Dual-Core 240 MHz, 520 KB SRAM, WiFi 802.11 b/g/n |
| 2 | **Transceiver RS-485** | Modul MAX485 TTL-to-RS485 | Konverter UART TTL 3.3V/5V ke diferensial RS-485 |
| 3 | **Display Layar** | LCD 16x2 Character Display + Modul I2C PCF8574 | Alamat I2C `0x27` (atau `0x3F`), 5V VCC |
| 4 | **Konektor Output** | XLR 3-Pin Female Panel Mount (atau XLR 5-Pin) | Pin 1: GND, Pin 2: Data- (B), Pin 3: Data+ (A) |
| 5 | **Regulator Daya** | Buck Converter DC-DC LM2596 (atau Adaptor 5V 2A) | Suplai tegangan stabil 5.0V ke Vin ESP32, MAX485, LCD |
| 6 | **Kabel & Protoboard** | Jumper Wires / PCB Custom / Box Enclosure X6 | Kabel shielded twisted pair untuk jalur DMX |

---

## 🔌 3. Skematik & Tabel Wiring Lengkap

### A. Tabel Sambungan Pin Antar-Komponen

```text
+-----------------------------------------------------------------------------------------+
|                                TABEL KONEKSI PIN MODUL                                  |
+-------------------+--------------------+------------------------+-----------------------+
| Komponen Asal     | Pin Komponen Asal  | Komponen Tujuan        | Pin Komponen Tujuan   |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | 5V / VIN           | Modul MAX485           | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul MAX485           | GND                   |
| ESP32 DevKit V1   | GPIO 4 (D4)        | Modul MAX485           | DE & RE (Dijumper)    |
| ESP32 DevKit V1   | GPIO 17 (TX2)      | Modul MAX485           | DI (Driver Input)     |
| ESP32 DevKit V1   | GPIO 16 (RX2)      | Modul MAX485           | RO (Receiver Output)* |
+-------------------+--------------------+------------------------+-----------------------+
| Modul MAX485      | Pin A              | Konektor XLR 3-Pin     | Pin 3 (DMX Data +)    |
| Modul MAX485      | Pin B              | Konektor XLR 3-Pin     | Pin 2 (DMX Data -)    |
| Modul MAX485      | Pin GND            | Konektor XLR 3-Pin     | Pin 1 (DMX Ground)    |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | 5V / VIN           | Modul I2C LCD 16x2     | VCC (5V)              |
| ESP32 DevKit V1   | GND                | Modul I2C LCD 16x2     | GND                   |
| ESP32 DevKit V1   | GPIO 21 (D21)      | Modul I2C LCD 16x2     | SDA                   |
| ESP32 DevKit V1   | GPIO 22 (D22)      | Modul I2C LCD 16x2     | SCL                   |
+-------------------+--------------------+------------------------+-----------------------+
*Catatan: Pin RO pada MAX485 dapat dibiarkan tidak terhubung karena modul berfungsi sebagai Transmitter (DMX Out).
```

### B. Diagram Blok Sistem

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                 DIAGRAM BLOK PERANGKAT                                 │
└────────────────────────────────────────────────────────────────────────────────────────┘

 [Lighting Software]            [WiFi Network]               [ESP32 Node Converter]
  • ZZLUXORA                      • Router / AP               • Core 1: WiFi UDP 6454
  • QLC+                     ───► • 2.4 GHz 802.11n      ───► • Core 0: DMX512 Engine
  • grandMA3 onPC                   UDP Broadcast             • I2C Display Monitor
                                                                     │
                                                                     ▼
                                                              [MAX485 Transceiver]
                                                              • UART2 (250 kbps 8N2)
                                                              • GPIO 4 DE/RE High
                                                                     │
                                                                     ▼
                                                              [XLR 3-Pin Output]
                                                              • Pin 1: GND
                                                              • Pin 2: Data- (B)
                                                              • Pin 3: Data+ (A)
                                                                     │
                                                                     ▼
                                                           [Physical DMX512 Fixtures]
                                                           • PAR LED RGBW (8ch/4ch)
                                                           • Moving Head Beam 230W
```

---

## 🧠 4. Arsitektur Firmware & FreeRTOS Dual-Core

Firmware **`artnet_dmx_final.ino`** menggunakan pembagian beban kerja berbasis **FreeRTOS SMP**:

1. **Core 0 (DMX Dedicated Real-Time Core):**
   - Menjalankan task `DMX_Task` dengan prioritas tertinggi (`configMAX_PRIORITIES - 1`).
   - Bertanggung jawab memformat sinyal USITT DMX512-A:
     - **Break Time:** Menarik jalur data LOW selama minimal **88 µs** (menggunakan `Serial2.begin(..., 8N2)` manipulasi baudrate atau GPIO toggle).
     - **Mark After Break (MAB):** Jalur data HIGH selama **8 µs**.
     - **Start Code:** Mengirim byte `0x00` (Null Start Code).
     - **Payload Data:** Mengirim 512 channel DMX secara serial beruntun pada baud rate **250.000 bps (8 data bit, 2 stop bit, No parity)**.
     - **Refresh Rate:** 40–44 Hz terus-menerus tanpa jeda.

2. **Core 1 (Network, WebServer, & UI Core):**
   - Menjalankan `loop()` standar Arduino:
     - Mendengarkan paket UDP port 6454 (`WiFiUDP`).
     - Memeriksa header string Art-Net: `'A' 'r' 't' '-' 'N' 'e' 't' 0x00`.
     - Memverifikasi OpCode: `0x5000` (ArtDmx).
     - Menyalin payload 512-byte ke `writeBuffer` di bawah perlindungan spinlock `taskENTER_CRITICAL(&mux)`.
     - Melayani WebServer HTTP konfigurasi pada port 80 dan DNS Server port 53.
     - Memperbarui status layar LCD 16x2 setiap 500 ms.

---

## 📟 5. State Machine & Tampilan LCD 16x2 (8 Layar)

Layar LCD 16x2 I2C menyajikan navigasi status operasional modul melalui 8 state visual:

| State | Nama Layar | Tampilan Baris 1 | Tampilan Baris 2 | Fungsi & Deskripsi |
|:--:|:---|:---|:---|:---|
| **1** | **Boot Screen** | `     ZZLIGHT    ` | `     PROJECT    ` | Animasi *typing effect* saat startup (3 detik). |
| **2** | **Connecting** | `Connecting WiFi ` | `SSID: [NamaSSID]` | Menampilkan proses koneksi ke router target. |
| **3** | **WiFi Failed** | `WiFi Not Connect` | `Starting AP Mode` | Pemberitahuan koneksi gagal, beralih ke Access Point. |
| **4** | **AP Mode Active**| `AP: ZZLIGHT-AP  ` | `IP: 192.168.4.1 ` | Modul membuka hotspot konfigurasi mandiri. |
| **5** | **Web Client On** | `Web Config Mode ` | `Client Connected` | Ada smartphone/laptop membuka web portal konfigurasi. |
| **6** | **Universe Set**  | `Universe Config ` | `Selected Uni: [X]`| Konfirmasi perubahan universe Art-Net dari web portal. |
| **7** | **Connected OK**  | `Connected!      ` | `IP: 192.168.1.X ` | Konfirmasi koneksi WiFi berhasil didapatkan. |
| **8** | **Live Monitor**  | `IP:192.168.1.50 ` | `U:00  DMX:44 FPS` | Dashboard real-time (IP, Universe, dan DMX FPS Counter). |

---

## 🌐 6. Fitur Captive Portal & Web Configuration

Ketika modul tidak dapat terhubung ke WiFi yang tersimpan, modul secara otomatis mengaktifkan **Captive Portal**:
1. Hubungkan smartphone atau laptop ke WiFi **`ZZLIGHT-AP`** (tanpa password).
2. Layar login portal otomatis terbuka (*Android, iOS, Windows automatic popup redirection*).
3. Jika tidak otomatis terbuka, buka browser dan ketik **`http://192.168.4.1`**.
4. Di antarmuka web, Anda dapat:
   - Memilih SSID WiFi dari daftar hasil scan jaringan.
   - Memasukkan password WiFi.
   - Memilih Universe Art-Net yang ingin dimonitor (0 s/d 15).
   - Menekan tombol **Save & Reboot** untuk menyimpan konfigurasi ke memori flash NVS.

---

## 🛠️ 7. Panduan Instalasi & Flashing via Arduino IDE

### A. Persiapan Software & Board:
1. Unduh dan install **[Arduino IDE 2.x](https://www.arduino.cc/en/software)**.
2. Tambahkan URL Board Manager ESP32 di `File` ➔ `Preferences`:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Buka `Tools` ➔ `Board` ➔ `Boards Manager`, cari **esp32** by Espressif, dan install versi **3.x.x**.

### B. Library yang Dibutuhkan:
Install library berikut via Arduino Library Manager (`Ctrl + Shift + I`):
- **`LiquidCrystal_I2C`** by Frank de Brabander (atau John Rickman)
- **`WiFi`** (Bawaan ESP32 Core)
- **`WebServer`** (Bawaan ESP32 Core)
- **`DNSServer`** (Bawaan ESP32 Core)
- **`Preferences`** (Bawaan ESP32 Core)

### C. Pengaturan Menu Tools di Arduino IDE:
- **Board:** `DOIT ESP32 DEVKIT V1` (atau `ESP32 Dev Module`)
- **CPU Frequency:** `240MHz (WiFi/BT)`
- **Flash Frequency:** `80MHz`
- **Flash Mode:** `QIO`
- **Flash Size:** `4MB (32Mb)`
- **Partition Scheme:** `Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)`
- **Port:** Pilih COM Port yang sesuai dengan kabel USB ESP32 Anda.

### D. Flashing:
Buka berkas `firmware/artnet_dmx_final.ino`, lalu klik tombol **Upload (Ctrl + U)**.

---


---

## 📸 8. Dokumentasi Perakitan Fisik & Fabrikasi Hardware

Berikut adalah dokumentasi visual langkah-demi-langkah perakitan modul node hardware ZZLIGHT Art-Net to DMX512:

### A. Komponen Utama & Pengujian Breadboard:
| ESP32 DevKit V1 | Modul Transceiver MAX485 | Pengujian Protoboard |
|:---:|:---:|:---:|
| ![ESP32](documentations/esp32_devkit_v1_top.jpeg) | ![MAX485](documentations/max_485.jpeg) | ![Breadboard](documentations/breadboard.jpeg) |

### B. Fabrikasi Box Enclosure X4 & Pemasangan Soket XLR:
| Pelubangan Enclosure X4 | Pemasangan Soket XLR 3-Pin | Pemasangan Modul ESP32 |
|:---:|:---:|:---:|
| ![Box Dilubangi](documentations/box_x4_dilubangi_01.jpeg) | ![Soket XLR](documentations/box_x4_dipasang_xlr_01.jpeg) | ![Pasang ESP32](documentations/box_x4_dipasang_esp32.jpeg) |

### C. Perakitan Rangkaian & Integrasi Akhir:
| Penyolderan Rangkaian | Rangkaian Masuk Box X4 | Produk Jadi (ZZLIGHT Node) |
|:---:|:---:|:---:|
| ![Rangkaian](documentations/rangkaian_01.jpeg) | ![Box Plus Rangkaian](documentations/box_x4_plus_rangkaian_01.jpeg) | ![Hasil Produk](documentations/hasil_produk.jpeg) |

### D. Pengujian Live dengan Fixture Lampu Panggung:
| Pengujian Sinyal DMX 44 FPS | Uji Operasional Lampu PAR LED RGBW |
|:---:|:---:|
| ![Uji Coba 1](documentations/uji_coba_01.jpeg) | ![Uji Coba 2](documentations/uji_coba_02.jpeg) |


## 🎭 9. Panduan Pengujian dengan Software Lighting

### A. Pengujian dengan Software ZZLUXORA:
1. Pastikan PC dan ESP32 terhubung pada satu jaringan WiFi / Router yang sama.
2. Buka software **ZZLUXORA** (`python -m zzluxora`).
3. Masuk ke panel **Settings**, pastikan **Universe = 0** dan **IP Target = Broadcast (`255.255.255.255`)** atau IP Statis ESP32.
4. Buka tab **Mixer**, naikkan fader Channel 1–4 (Red, Green, Blue, White).
5. Amati respon lampu PAR LED dan indikator DMX FPS pada layar LCD ESP32.

### B. Pengujian dengan QLC+ (Q Light Controller Plus):
1. Buka QLC+, masuk ke tab **Input / Output**.
2. Pada Universe 1 Output, centang plugin **ArtNet**.
3. Klik ikon obeng (Konfigurasi), pilih interface jaringan WiFi Anda, dan arahkan IP ke Broadcast.
4. Masuk ke tab **Simple Desk**, gerakkan fader DMX 1–512.

---

## 🔧 10. Troubleshooting & Solusi Masalah Hardware

| Gejala Masalah | Kemungkinan Penyebab | Solusi Penanganan |
|:---|:---|:---|
| **Lampu DMX tidak merespon sama sekali** | Kabel DMX Data+ (Pin 3) dan Data- (Pin 2) terbalik pada XLR. | Tukar posisi pin A dan B pada modul MAX485. |
| **Lampu berkedip acak (flickering)** | Pin DE dan RE pada MAX485 tidak dijumper atau mengambang (*floating*). | Pastikan Pin DE dan RE terhubung bersama ke GPIO 4 ESP32. |
| **Layar LCD 16x2 hanya menampilkan kotak hitam** | Kontras LCD belum disesuaikan atau alamat I2C salah. | Putar potensiometer biru (Trimpot) di belakang modul I2C dan verifikasi alamat I2C (`0x27` atau `0x3F`). |
| **ESP32 sering reboot saat menyalakan WiFi** | Penurunan tegangan (*brownout*) akibat suplai USB tidak mencukupi arus 500mA. | Gunakan adaptor daya 5V 2A eksternal melalui pin Vin. |
| **Paket Art-Net tidak diterima** | Universe yang dikirim oleh software berbeda dengan Universe di ESP32. | Pastikan pengaturan Universe pada software dan ESP32 keduanya bernilai `0`. |

---

## 📜 11. Lisensi & Hak Cipta

Proyek ini dilisensikan di bawah **MIT License**.  
Hak Cipta (c) 2026 **Andreas Restuawanta Christwara ([@zzdree](https://github.com/zzdree))** — Jurusan Teknik Elektro, Universitas Negeri Semarang.
