# Review Modul ARTNET-DMX (ZZLIGHT PROJECT)

## 📁 File dalam Workspace

| File | Ukuran | Deskripsi |
|------|--------|-----------|
| [artnet-dmx_prompt.txt](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx_prompt.txt) | 9.5 KB | Prompt lengkap spesifikasi modul |
| [artnet-dmx_v1.txt](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx_v1.txt) | 3 KB | Versi awal, pakai library WiFiManager & ArtnetWifi |
| [artnet-dmx_v2.txt](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx_v2.txt) | 20 KB | Tambah DMX Core Task, WPA2-Enterprise, WebServer custom |
| [artnet-dmx_v3.txt](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx_v3.txt) | 24 KB | Tambah Double Buffering + Spinlock, Universe Filter |
| [artnet-dmx_v4.txt](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx_v4.txt) | 24 KB | Tambah Captive Portal + DNS Server, Client-Side Clock |
| [artnet-dmx.ino](file:///c:/Users/andre/OneDrive/Documents/SCRIPT/artnet-dmx.ino) | 23 KB | **Final** — sama persis dengan v4, siap upload |

---

## 🏗️ Arsitektur Modul (Final)

### Hardware
- **ESP32 DevKit V1** — dual-core MCU
- **MAX485** → RS-485 → XLR 3-pin → DMX512 fixture
- **LCD 16x2 I2C** — status monitor

### Wiring
```
ESP32 → MAX485:  VIN→VCC, GND→GND, D4→DE/RE, D17(TX2)→DI
MAX485 → XLR:    GND→Pin1, A→Pin3, B→Pin2
ESP32 → LCD I2C: VIN→VCC, GND→GND, D21→SDA, D22→SCL
```

### Fitur Software
| Fitur | Detail |
|-------|--------|
| **Art-Net Receiver** | Port 6454, Universe 0, raw packet parsing |
| **DMX Engine** | Double-buffering + Spinlock, dedicated Core 0, 22ms refresh (~45 FPS) |
| **WiFi Manager** | Custom AP mode + Captive Portal (Android/iOS optimized) |
| **WebServer** | Setup page (scan WiFi, autofill), Monitor page (realtime status) |
| **Auto Reconnect** | <30s reconnect, >30s reboot + reset ke AP mode |
| **Blackout Safety** | Auto blackout jika no Art-Net signal >10 detik |
| **LCD State Machine** | 12 screen states dengan animasi typing & looping |
| **mDNS** | Akses via `zzlight.local` |

### Evolusi Versi
```
v1 → v2: WiFiManager library → custom WiFi manager + WPA2-Enterprise
v2 → v3: Single buffer → Double Buffering + Spinlock (enterprise-grade DMX)
v3 → v4/final: NTP clock → Client-Side clock, tambah Captive Portal + DNS hijack
```

---

## 📊 Posisi Modul dalam Arsitektur Skripsi

```
┌─────────────────────────────────────────── LAPTOP ─────────────────────────────────────┐
│                                                                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐         ┌──────────────┐      │
│  │  Audio File  │───▶│    Audio     │───▶│  Scene/Chase │──Play──▶│  App Art-Net │      │
│  │  (.mp3/.wav) │    │   Analysis   │    │   Generator  │         │   Sender     │      │
│  └──────────────┘    │  (FFT, BPM)  │    │   (RGBW)     │         └──────┬───────┘      │
│                      └──────────────┘    └──────────────┘                │               │
│                                                                   Art-Net UDP            │
│                                                              (virtual adapter)           │
│                                                                         │               │
│                                                                  ┌──────▼───────┐       │
│                                                                  │    QLC+      │       │
│                                                                  │  (optional)  │       │
│                                                                  └──────┬───────┘       │
│                                                                         │               │
└─────────────────────────────────────────────────────────────────────────┼───────────────┘
                                                                          │
                                                                   Art-Net UDP
                                                                  via WiFi/Hotspot
                                                                          │
                                                                   ┌──────▼───────┐
                                                                   │  ESP32 Modul │  ◄── SUDAH SELESAI ✅
                                                                   │  ARTNET-DMX  │
                                                                   └──────┬───────┘
                                                                          │ DMX512
                                                                   ┌──────▼───────┐
                                                                   │   Lampu      │
                                                                   │  RGBW/PAR    │
                                                                   └──────────────┘
```

> [!IMPORTANT]
> **Modul ESP32 Art-Net → DMX sudah selesai dan production-ready.**
> Yang perlu dibuat untuk skripsi adalah **aplikasi desktop/web di laptop** (bagian atas diagram):
> - Audio analysis (FFT, beat detection, frequency band analysis)
> - Scene generator (mapping audio → RGBW values)
> - Chase sequencer (multiple scenes + timing)
> - Art-Net sender (UDP broadcast ke virtual adapter / langsung ke modul)

---

## 🚀 Langkah Selanjutnya

Modul hardware sudah solid. Sekarang fokus ke **aplikasi utama skripsi**:

1. **Tentukan tech stack** — Python (desktop) atau Web App?
2. **Audio analysis engine** — Library apa yang dipakai?
3. **UI/UX** — Scene editor, chase builder, play/preview
4. **Art-Net sender** — Integrasi dari app ke modul

> [!NOTE]
> Kualitas modul ini sangat bagus untuk level skripsi. Double-buffering, spinlock, captive portal, dan state machine menunjukkan pemahaman teknis yang kuat di sisi embedded system.
