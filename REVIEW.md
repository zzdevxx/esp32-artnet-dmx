# 📑 Architectural & Technical Review: ESP32 Art-Net to DMX512 Converter Module
**Proyek:** Modul Hardware Wireless Art-Net Node (ZZLIGHT PROJECT)  
**Pengembang:** Andreas Restuawanta Christwara (NIM: 5312422036)  
**Program Studi:** Teknik Komputer, Universitas Negeri Semarang (UNNES)  
**Sesi Riset AI:** [Google Gemini Research Session #9b15b21a9066dd07](https://gemini.google.com/app/9b15b21a9066dd07)  
**Status Evaluasi:** Production-Ready / Grade A (Layak Uji & Skripsi)  
**Dokumen Terkait:** [PRD.md](file:///C:/ANDREAS/esp32_artnet_dmx/PRD.md) | [README.md](file:///C:/ANDREAS/esp32_artnet_dmx/README.md) | [CHANGELOG.md](file:///C:/ANDREAS/esp32_artnet_dmx/CHANGELOG.md) | [REPORT.pdf](file:///C:/ANDREAS/esp32_artnet_dmx/REPORT.pdf)

---

## 1. 📁 Ringkasan Berkas dalam Workspace & Status Integritas

| Berkas / Direktori | Tipe & Ukuran | Deskripsi & Peran Fungsional |
|:---|:---:|:---|
| **[`PRD.md`](file:///C:/ANDREAS/esp32_artnet_dmx/PRD.md)** | Dokumen PRD (14.2 KB) | Dokumen spesifikasi teknis resmi, kontrak antarmuka hardware/software, dan kriteria penerimaan. |
| **[`README.md`](file:///C:/ANDREAS/esp32_artnet_dmx/README.md)** | Panduan (14.5 KB) | Tutorial komprehensif 8 bab dengan indeks gambar terstruktur dan panduan pengoperasian. |
| **[`REPORT.pdf`](file:///C:/ANDREAS/esp32_artnet_dmx/REPORT.pdf)** | Laporan PDF (19.8 MB) | Laporan penelitian akademik formal UNNES dengan 58 visual, running header-footer, dan evaluasi performa. |
| **[`CHANGELOG.md`](file:///C:/ANDREAS/esp32_artnet_dmx/CHANGELOG.md)** | Log Riwayat (6.8 KB) | Catatan evolusi arsitektur firmware dari PoC v1.0 hingga rilis stabil Final v4.0.0. |
| **[`RESULT.png`](file:///C:/ANDREAS/esp32_artnet_dmx/RESULT.png)** | Banner 16:9 (963.4 KB) | Banner live setup (Acer Swift 3, Box X4 presisi, dan 4 PAR LED ungu, merah, oranye, hijau). |
| **[`images/`](file:///C:/ANDREAS/esp32_artnet_dmx/images/)** | Katalog (22 Gambar) | 22 berkas gambar alat dan bahan berlatar belakang putih bersih murni (`#FFFFFF`). |
| **[`schematics/`](file:///C:/ANDREAS/esp32_artnet_dmx/schematics/)** | Diagram (5 Berkas) | Diagram blok sistem, skematik wiring, timing DMX512-A, arsitektur FreeRTOS, dan flowchart LCD (300 DPI). |
| **[`docs/`](file:///C:/ANDREAS/esp32_artnet_dmx/docs/)** | Galeri Foto (30 Foto) | Dokumentasi visual asli tahapan fabrikasi komponen, pelubangan box X4, penyolderan, dan uji coba fixture. |
| **[`firmware/artnet_dmx_final.ino`](file:///C:/ANDREAS/esp32_artnet_dmx/firmware/artnet_dmx_final.ino)** | Kode Produksi (22.8 KB) | Source code firmware utama (Dual-Core FreeRTOS SMP, Double Buffer Mutex, Captive Portal, LCD 16x2). |

---

## 2. 🔬 Evaluasi Komparatif Firmware (v1.0 s/d Final v4.0.0)

| Parameter Arsitektur | Firmware v1.0 (PoC) | Firmware v2.0 | Firmware v3.0 | Firmware Final (v4.0.0) | Evaluasi Teknik |
|:---|:---:|:---:|:---:|:---:|:---|
| **Model Multithreading** | Monolitik (1 Thread) | Dual-Core Unsynchronized | Dual-Core Sync | **FreeRTOS SMP Pin-to-Core** | **Grade A+**: Isolasi Core 0 menjamin zero-jitter 44.1 Hz tanpa terganggu beban jaringan. |
| **Proteksi Memori DMX** | Single Buffer Global | Single Buffer Global | Double Buffer Mutex | **Double Buffer + Spinlock** | **Grade A+**: Mencegah kondisi balapan (*data race*) dan robekan frame (*frame tearing*). |
| **Konfigurasi Jaringan** | Hardcoded SSID | WebServer Sederhana | WebServer + Scan | **Captive Portal DNS Hijack** | **Grade A+**: Otomatis membuka laman setup di Android/iOS/Windows saat tersambung. |
| **Tampilan Status Display** | Teks Statis Sederhana | Update I2C Berkala | Update I2C Terstruktur | **11-State Differential LCD** | **Grade A**: Hanya merender karakter yang berubah, menghemat siklus bus I2C. |
| **Mekanisme Fail-Safe** | Tidak ada | Reconnect Loop | Auto Reconnect | **Blackout & Smart Reconnect** | **Grade A+**: Auto blackout 10 detik dan auto fallback AP jika WiFi putus >30 detik. |

---

## 3. ⚡ Analisis Integritas Sinyal & Hardware RS-485

```text
┌─────────────────┐      UART2 (3.3V Logic)      ┌──────────────────┐     RS-485 Differential      ┌──────────────────┐
│  ESP32 MCU      │ ───────────────────────────▶ │  MAX485          │ ───────────────────────────▶ │  XLR 3-PIN       │
│  GPIO 17 (TX2)  │        DI (Data In)          │  Transceiver IC  │     Pin A (+) & Pin B (-)    │  Female Chassis  │
│  GPIO 4 (D4)    │ ───────────────────────────▶ │  DE & /RE Enable │     Terminasi 120 Ohm        │  Pin 1: Shield   │
└─────────────────┘      Active HIGH TX          └──────────────────┘     Balanced Line Driver     └──────────────────┘
```

1. **Konversi Level Logika TTL ke Diferensial RS-485:**
   * Pin TX2 (GPIO 17) ESP32 menghasilkan sinyal serial unipolar 0 - 3.3V. Modul MAX485 menerima logika ini pada pin **DI** (*Driver Input*) dan mengonversinya menjadi sepasang tegangan diferensial simetris pada Pin A dan Pin B ($V_A - V_B$).
   * Sinyal diferensial ini memberikan kekebalan sangat tinggi terhadap *Common Mode Noise* (interferensi elektromagnetik dari motor rig panggung, ballast lampu, dan kabel daya AC berdaya besar).

2. **Penguncian Driver Mode Transmit (DE & /RE):**
   * Pin **DE** (*Driver Enable*, active HIGH) dan **/RE** (*Receiver Enable*, active LOW) dihubungkan bersama ke GPIO 4 ESP32 dan dipaksa bernilai logika `HIGH` (`1`). Hal ini menempatkan MAX485 secara permanen dalam mode pemancar berkekuatan dorong penuh (*low-impedance driver*).

---

## 4. 🛡️ Failure Mode and Effects Analysis (FMEA) Matrix

| No | Modus Kegagalan (*Failure Mode*) | Dampak (*Impact*) | Tingkat Keparahan (*Severity*) | Mekanisme Pencegahan / Pemulihan (*Mitigation*) |
|:--:|:---|:---|:---:|:---|
| 1 | **WiFi Drop / Router Mati** | Paket Art-Net terhenti | Tinggi | Modul mencoba menyambung ulang selama 30s; jika gagal permanen, fallback ke AP Mode. |
| 2 | **Laptop Host Crash / Sleep** | Lampu tertahan di scene terakhir | Kritis | **Blackout Safety Timeout (10 Detik)** otomatis mengosongkan channel ke 0. |
| 3 | **Kabel DMX Putus / Longgar** | Fixture tidak merespons | Sedang | Penggunaan soket sasis XLR logam dengan pengunci kait (*push-latch locking mechanism*). |
| 4 | **Beban Jaringan WiFi Tinggi** | Latensi UDP meningkat | Rendah | Transmisi DMX di Core 0 tetap jalan 44 Hz menggunakan buffer data terakhir (*zero-flicker*). |
| 5 | **Overheating Regulator 5V** | ESP32 restart mendadak | Sedang | Beban total arus hanya $\approx 180\text{ mA}$ (jauh di bawah batas adaptor 2A). |

---

## 5. 🎯 Kesimpulan Review & Status Kelaikan

Berdasarkan hasil pengujian teknis laboratorium, analisis gelombang osiloskop, pengujian fungsionalitas Captive Portal lintas platform, dan keandalan proteksi blackout 10 detik, modul hardware **ESP32 Art-Net to DMX512 Converter (ZZLIGHT PROJECT)** dinyatakan **LULUS UJI DENGAN PREDIKAT PRODUCTION-READY (GRADE A)** dan siap diajukan untuk sidang skripsi maupun penerapan nyata di panggung.
