# 💡 ESP32 Art-Net to DMX512 Transceiver Module (ZZLIGHT PROJECT)

> **Tutorial & Dokumentasi Lengkap Pembuatan Modul Node Art-Net DMX512 Nirkabel Berbasis ESP32 DevKit V1, Transceiver MAX485, dan Layar LCD 16x2 I2C.**  
> *Hardware Node Converter untuk Software Otomasi Pencahayaan Panggung Cerdas ZZLUXORA (Universitas Negeri Semarang).*

---

![ESP32](https://img.shields.io/badge/MCU-ESP32_DevKit_V1-red?style=flat-square&logo=espressif&logoColor=white)
![Protocol](https://img.shields.io/badge/Protocol-Art--Net_4_%2F_DMX512-orange?style=flat-square)
![Baud Rate](https://img.shields.io/badge/Baud_Rate-250_kbps_(8N2)-blue?style=flat-square)
![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS_Dual--Core-green?style=flat-square)
![Display](https://img.shields.io/badge/Display-LCD_16x2_I2C_(0x27)-purple?style=flat-square)
![Photos](https://img.shields.io/badge/Documentation-30_Photos_Cataloged-brightgreen?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

---

## 📑 Daftar Isi
1. [Gambaran Umum Modul](#-1-gambaran-umum-modul)
2. [Daftar Komponen Hardware (BOM)](#-2-daftar-komponen-hardware-bom)
3. [Skematik & Tabel Wiring Lengkap](#-3-skematik--tabel-wiring-lengkap)
4. [Arsitektur Firmware & FreeRTOS Dual-Core](#-4-arsitektur-firmware--freertos-dual-core)
5. [State Machine & Tampilan LCD 16x2 (8 Layar)](#-5-state-machine--tampilan-lcd-16x2-8-layar)
6. [Fitur Captive Portal & Web Configuration](#-6-fitur-captive-portal--web-configuration)
7. [Dokumentasi Bahan & Komponen (9 Foto)](#-7-dokumentasi-bahan--komponen-9-foto)
8. [Dokumentasi Perakitan & Fabrikasi (16 Foto)](#-8-dokumentasi-perakitan--fabrikasi-16-foto)
9. [Dokumentasi Produk Jadi & Pengujian (5 Foto)](#-9-dokumentasi-produk-jadi--pengujian-5-foto)
10. [Panduan Instalasi & Flashing Arduino IDE](#-10-panduan-instalasi--flashing-arduino-ide)
11. [Troubleshooting & Solusi Masalah Hardware](#-11-troubleshooting--solusi-masalah-hardware)
12. [Lisensi & Hak Cipta](#-12-lisensi--hak-cipta)

---

## 📖 1. Gambaran Umum Modul

Modul **ZZLIGHT Art-Net to DMX512** adalah perangkat keras konverter (*node receiver*) yang menerima paket data pencahayaan **Art-Net (ArtDmx OpCode 0x5000)** melalui jaringan WiFi nirkabel pada port **UDP 6454**, kemudian mengonversinya secara *real-time* menjadi sinyal serial diferensial **RS-485 DMX512 standar industri (USITT DMX512-A, 250 kbps, 8N2)** untuk menggerakkan fixture lampu panggung (*Moving Head, PAR LED RGBW, Strobe, Laser*).

Modul ini dirancang dengan arsitektur **FreeRTOS Dual-Core**, di mana penerimaan jaringan WiFi dan pengiriman serial DMX diisolasi pada inti prosesor yang berbeda guna menjamin sinyal DMX512 tetap stabil tanpa *flicker* (*jitter-free* 44 Hz).

---

## 🛒 2. Daftar Komponen Hardware (BOM)

| No | Komponen | Spesifikasi / Model | Keterangan |
|:--:|:---|:---|:---|
| 1 | **Mikrokontroler** | ESP32 DevKit V1 (ESP-WROOM-32 30-Pin) | Dual-Core 240 MHz, 520 KB SRAM, WiFi 802.11 b/g/n |
| 2 | **Transceiver RS-485** | Modul MAX485 TTL-to-RS485 | Konverter UART TTL 3.3V/5V ke diferensial RS-485 |
| 3 | **Display Layar** | LCD 16x2 Character Display + Backpack I2C PCF8574 | Alamat I2C `0x27` (atau `0x3F`), 5V VCC |
| 4 | **Konektor Output** | XLR 3-Pin Female Panel Mount | Pin 1: GND, Pin 2: Data- (B), Pin 3: Data+ (A) |
| 5 | **Box Enclosure** | Box Plastik X4 (Hitam) | Enclosure pelindung modul elektronika |
| 6 | **Papan Rangkaian** | PCB Matrix / Rawboard / Breadboard | Papan tempat penyolderan komponen & header |
| 7 | **Regulator Daya** | Buck Converter LM2596 / Adaptor 5V 2A | Suplai tegangan stabil 5.0V ke Vin ESP32, MAX485, LCD |

---

## 🔌 3. Skematik & Tabel Wiring Lengkap

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
```

---

## 🧠 4. Arsitektur Firmware & FreeRTOS Dual-Core

- **Core 0 (DMX Task):** Transmisi serial RS-485 250 kbps deterministik 44 Hz (Break 88 µs, MAB 8 µs, Start Code `0x00`, 512 channels payload).
- **Core 1 (Network & UI Task):** Socket UDP port 6454, Captive Portal DNS Server (Port 53), WebServer HTTP (Port 80), Double Buffering (`writeBuffer` / `readBuffer`), dan refresh display LCD 16x2.

---

## 📟 5. State Machine & Tampilan LCD 16x2 (8 Layar)

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
4. Di antarmuka web, Anda dapat memilih SSID WiFi, memasukkan password, dan memilih Universe Art-Net (0–15).

---

## 📸 7. Dokumentasi Bahan & Komponen (9 Foto)

| ESP32 DevKit V1 (Tampak Atas) | ESP32 DevKit V1 (Tampak Bawah) | Modul Transceiver MAX485 |
|:---:|:---:|:---:|
| ![ESP32 Top](documentations/esp32_devkit_v1_top.jpeg) | ![ESP32 Bottom](documentations/esp32_dekit_v1_bottom.jpeg) | ![MAX485](documentations/max_485.jpeg) |

| Breadboard Prototipe | PCB Matrix / Rawboard | Soket XLR 3-Pin (Tampak Belakang) |
|:---:|:---:|:---:|
| ![Breadboard](documentations/breadboard.jpeg) | ![Rawboard](documentations/rawboard.jpeg) | ![XLR Back](documentations/xlr_socket_back.jpeg) |

| Soket XLR 3-Pin (Tampak Kaki) | Box Enclosure X4 (Tertutup) | Box Enclosure X4 (Terbuka) |
|:---:|:---:|:---:|
| ![XLR Rear](documentations/xlr_socket_rear.jpeg) | ![Box Tertutup](documentations/box_x4_tertutup.jpeg) | ![Box Terbuka](documentations/box_x4_terbuka.jpeg) |

---

## 🛠️ 8. Dokumentasi Perakitan & Fabrikasi (16 Foto)

### A. Tahap Pelubangan Box & Mounting Komponen:
| 1. Persiapan Box X4 | 2. Pelubangan Sisi XLR | 3. Pelubangan Port USB |
|:---:|:---:|:---:|
| ![Box Dibuka](documentations/box_x4_dibuka.jpeg) | ![Dilubangi 1](documentations/box_x4_dilubangi_01.jpeg) | ![Dilubangi 2](documentations/box_x4_dilubangi_02.jpeg) |

| 4. Hasil Pelubangan Box | 5. Pasang XLR (Tampak Luar) | 6. Pasang XLR (Tampak Dalam) |
|:---:|:---:|:---:|
| ![Dilubangi 3](documentations/box_x4_dilubangi_03.jpeg) | ![Pasang XLR 1](documentations/box_x4_dipasang_xlr_01.jpeg) | ![Pasang XLR 2](documentations/box_x4_dipasang_xlr_02.jpeg) |

| 7. Fitting ESP32 ke Box | - | - |
|:---:|:---:|:---:|
| ![Pasang ESP32](documentations/box_x4_dipasang_esp32.jpeg) | - | - |

### B. Tahap Penyolderan Rangkaian Elektronika:
| 8. Solder Jalur Daya 5V | 9. Solder Jalur MAX485 | 10. Solder Header I2C LCD |
|:---:|:---:|:---:|
| ![Rangkaian 1](documentations/rangkaian_01.jpeg) | ![Rangkaian 2](documentations/rangkaian_02.jpeg) | ![Rangkaian 3](documentations/rangkaian_03.jpeg) |

| 11. Inspeksi Bawah Board | 12. Solder Kabel Output XLR | - |
|:---:|:---:|:---:|
| ![Rangkaian 4](documentations/rangkaian_04.jpeg) | ![Rangkaian 5](documentations/rangkaian_05.jpeg) | - |

### C. Tahap Integrasi Box & Finishing:
| 13. Rakitan Masuk Box | 14. Manajemen Kabel Box | 15. Pasang Penutup Box |
|:---:|:---:|:---:|
| ![Box Plus 1](documentations/box_x4_plus_rangkaian_01.jpeg) | ![Box Plus 2](documentations/box_x4_plus_rangkaian_02.jpeg) | ![Box Tutup 1](documentations/box_x4_plus_rangkaian+tutup_01.jpeg) |

| 16. Baut & Finishing Box | - | - |
|:---:|:---:|:---:|
| ![Box Tutup 2](documentations/box_x4_plus_rangkaian+tutup_02.jpeg) | - | - |

---

## 🌟 9. Dokumentasi Produk Jadi & Pengujian (5 Foto)

### A. Produk Jadi & Detail Internal Box:
| Produk Jadi (ZZLIGHT Node) | Tampilan Internal Box (Sudut 1) | Tampilan Internal Box (Sudut 2) |
|:---:|:---:|:---:|
| ![Hasil Produk](documentations/hasil_produk.jpeg) | ![Box Rangkaian 1](documentations/box+rangkaian_01.jpeg) | ![Box Rangkaian 2](documentations/box+rangkaian_02.jpeg) |

### B. Pengujian Live dengan Fixture Lampu Panggung:
| Pengujian Sinyal DMX 44 FPS di LCD | Uji Respon Fixture PAR LED RGBW |
|:---:|:---:|
| ![Uji Coba 1](documentations/uji_coba_01.jpeg) | ![Uji Coba 2](documentations/uji_coba_02.jpeg) |

---

## 🛠️ 10. Panduan Instalasi & Flashing Arduino IDE

1. **Board Manager:** Install **ESP32 by Espressif** (versi 3.x.x).
2. **Library yang Dibutuhkan:**
   - `LiquidCrystal_I2C` by Frank de Brabander
   - `WiFi`, `WebServer`, `DNSServer`, `Preferences` (bawaan ESP32 core)
3. **Pengaturan Tools:**
   - Board: `DOIT ESP32 DEVKIT V1`
   - CPU Frequency: `240MHz`
   - Flash Size: `4MB (32Mb)`
   - Partition Scheme: `Default 4MB with spiffs`
4. Buka `firmware/artnet_dmx_final.ino` dan klik **Upload**.

---

## 🔧 11. Troubleshooting & Solusi Masalah Hardware

| Gejala Masalah | Kemungkinan Penyebab | Solusi Penanganan |
|:---|:---|:---|
| **Lampu DMX tidak merespon sama sekali** | Kabel DMX Data+ (Pin 3) dan Data- (Pin 2) terbalik pada XLR. | Tukar posisi pin A dan B pada modul MAX485. |
| **Lampu berkedip acak (flickering)** | Pin DE dan RE pada MAX485 tidak dijumper atau mengambang (*floating*). | Pastikan Pin DE dan RE terhubung bersama ke GPIO 4 ESP32. |
| **Layar LCD 16x2 hanya menampilkan kotak hitam** | Kontras LCD belum disesuaikan atau alamat I2C salah. | Putar potensiometer biru (Trimpot) di belakang modul I2C dan verifikasi alamat I2C (`0x27` atau `0x3F`). |
| **ESP32 sering reboot saat menyalakan WiFi** | Penurunan tegangan (*brownout*) akibat suplai USB tidak mencukupi arus 500mA. | Gunakan adaptor daya 5V 2A eksternal melalui pin Vin. |

---


---

## 🔗 Referensi & Sumber Riset Sesi AI

Pengembangan spesifikasi, arsitektur FreeRTOS dual-core, dan logika firmware modul ini didokumentasikan dalam sesi riset interaktif:
* 🤖 **Sesi Google Gemini AI:** [https://gemini.google.com/app/9b15b21a9066dd07](https://gemini.google.com/app/9b15b21a9066dd07)
* 📄 **Laporan Lengkap PDF:** [REPORT.pdf](REPORT.pdf)


## 📜 12. Lisensi & Hak Cipta

Proyek ini dilisensikan di bawah **MIT License**.  
Hak Cipta (c) 2026 **Andreas Restuawanta Christwara ([@zzdree](https://github.com/zzdree))** — Universitas Negeri Semarang.
