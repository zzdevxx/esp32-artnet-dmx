# 🎛️ ESP32 Wireless Art-Net to DMX512 Node (ZZLIGHT)

[![Hardware Status](https://img.shields.io/badge/Hardware-Production--Ready%20(Grade%20A)-success?style=for-the-badge&logo=espressif&logoColor=white)](file:///C:/ANDREAS/esp32-artnet-dmx)
[![Protocol Support](https://img.shields.io/badge/Protocols-Art--Net%204%20%7C%20USITT%20DMX512--A-blue?style=for-the-badge)](file:///C:/ANDREAS/esp32-artnet-dmx)
[![Architecture](https://img.shields.io/badge/OS-FreeRTOS%20SMP%20Dual--Core-8E44AD?style=for-the-badge)](file:///C:/ANDREAS/esp32-artnet-dmx)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](file:///C:/ANDREAS/esp32-artnet-dmx/LICENSE)
[![Creator](https://img.shields.io/badge/Made%20by-Andreas%20Christwara%20(%40zzdree)-orange?style=for-the-badge&logo=github)](https://github.com/zzdree)

> **A high-performance, ultra-reliable, zero-jitter DIY Wireless Art-Net 4 to DMX512-A Converter Node powered by ESP32 Dual-Core (FreeRTOS SMP), built-in Captive Portal Web Config, and 16x2 LCD Status Monitor.**

---

## 📁 Visual Assets & Documentation Links
* 🎬 **[RESULT.png](RESULT.png)** — Live stage setup banner (Acer Swift 3 running QLC+, Box X4 enclosure, 4 PAR LED 54-eye fixtures).
* 📄 **[REPORT.pdf](REPORT.pdf)** — Complete PDF project report with all 58 indexed figures, schematics, and build logs.
* 📦 **[images/](images/)** — Catalog of 22 hardware tools & components.
* 📐 **[schematics/](schematics/)** — 5 High-resolution engineering schematics & architecture diagrams (300 DPI).
* 📸 **[docs/](docs/)** — 30 Step-by-step fabrication & testing photos.
* 📋 **[PRD.md](PRD.md)** — Product Requirements Document.
* 📑 **[REVIEW.md](REVIEW.md)** — Architectural & Signal Integrity Review.
* 📝 **[CHANGELOG.md](CHANGELOG.md)** — Version history and roadmap notes.

---

## ⚡ Quick Navigation

- [✨ Key Features](#-key-features)
- [🛒 Hardware & Bill of Materials (BOM)](#-hardware--bill-of-materials-bom)
- [📐 Schematics & Pinout Wiring](#-schematics--pinout-wiring)
- [🛠️ Fabrication & Assembly Summary](#️-fabrication--assembly-summary)
- [🧠 Firmware & Dual-Core Architecture](#-firmware--dual-core-architecture)
- [🚀 Quick Start: Flashing the Firmware](#-quick-start-flashing-the-firmware)
- [🌐 WiFi Setup via Captive Portal](#-wifi-setup-via-captive-portal)
- [🎛️ Universal Software Compatibility](#️-universal-software-compatibility)
- [📊 Test Results & Performance](#-test-results--performance)
- [🔮 Roadmap v5.0 (Community Edition)](#-roadmap-v50-community-edition--invitation-to-fork)
- [💬 Message to Makers & Forkers](#-message-to-makers--forkers)
- [📜 License & Credits](#-license--credits)

---

## ✨ Key Features

- **🚀 FreeRTOS Dual-Core SMP Engine:**
  - **Core 0 (Dedicated DMX):** Generates strict USITT DMX512-A serial timing (Break $\ge 88\,\mu\text{s}$, MAB $\ge 8\,\mu\text{s}$) at an ultra-stable **44.1 Hz refresh rate** with **zero jitter** ($<0.05\,\mu\text{s}$).
  - **Core 1 (Networking & UI):** Handles Art-Net UDP (Port 6454), Captive Portal DNS (Port 53), HTTP WebServer (Port 80), and NVS flash storage.
- **🔄 Atomic Double-Buffering (Ping-Pong Memory):**
  - Thread-safe pointer swapping between `writeBuffer` and `readBuffer` protected by an atomic spinlock (`portENTER_CRITICAL`), totally eliminating frame tearing and stage light flicker.
- **📱 Smart Captive Portal (Auto Pop-up):**
  - When disconnected from WiFi, the node automatically broadcasts an Access Point (`ZZLIGHT-AP` at `192.168.4.1`). Connecting with any smartphone (Android/iOS) or PC instantly opens the web setup portal.
- **📟 11-State Machine 16x2 LCD Display:**
  - Real-time status display with differential I2C rendering: Booting animation, WiFi status, connected IP, active Art-Net Universe, and live DMX packet counter (FPS).
- **🛡️ 10-Second Fail-Safe Blackout:**
  - Automatically blacks out all 512 DMX channels if the host lighting software stops broadcasting for >10 seconds, protecting stage fixtures from overheating.
- **📦 Rugged Box X4 DIY Enclosure:**
  - Compact, road-ready ABS plastic case with metal 3-pin XLR female chassis socket and top-mounted blue LCD screen.

---

## 🛒 Hardware & Bill of Materials (BOM)

Total estimated cost to build this module is under **\$10 / Rp 150.000**. Here are all 22 parts and tools:

| No | Component / Tool | Specification & Details | Purpose |
|:--:|:---|:---|:---|
| 1 | **ESP32 DevKit V1** | 30-Pin, Dual-Core 240MHz, 520KB SRAM | Main MCU & WiFi Engine |
| 2 | **MAX485 Module** | 5V TTL to RS-485 Differential Transceiver | Converts UART TX to DMX512 |
| 3 | **LCD 16x2 + I2C** | Blue Backlight, PCF8574 Backpack (`0x27`) | Real-time Status Display |
| 4 | **XLR 3-Pin Female Socket** | Round Metal Panel-Mount Chassis | DMX Cable Output Socket |
| 5 | **Box X4 Enclosure** | Black Matte ABS ($120 \times 85 \times 40\text{ mm}$) | Road-ready Protective Casing |
| 6 | **Perfboard / Rawboard** | Single-Sided Matrix PCB ($50 \times 70\text{ mm}$) | Circuit Board Base |
| 7 | **Pin Headers** | 2.54mm Pitch Male & Female Strips | Modular Socket for ESP32 & MAX485 |
| 8 | **DuPont Jumper Wires** | AWG 24-26 Multi-Color Wires | Internal Power & Signal Wiring |
| 9 | **USB to Micro-USB Cable** | High-Quality Copper Data Cable (1.0m) | 5V Power Supply & Flashing |
| 10 | **Solder Wire** | 60/40 Rosin Core Tin (0.8mm) | Electrical Soldering |
| 11 | **Soldering Iron** | 60W Adjustable Temperature Ceramic | Tool for Soldering |
| 12 | **Desoldering Pump** | Manual Aluminum Solder Sucker | Solder Cleaning & Fixes |
| 13 | **Digital Multimeter** | Voltage, Continuity Buzzer & Resistance | Short-Circuit & Rail Verification |
| 14 | **Breadboard 830 Points** | Solderless Prototyping Breadboard | Testing Circuit Before Assembly |
| 15 | **Side Wire Cutters** | Precision Flush Cutters | Trimming Header Pins & Wires |
| 16 | **Step Drill Bit** | Titanium Nitride HSS 4–22mm Cone | Drilling XLR Socket Hole on Box X4 |
| 17 | **Precision Screwdrivers** | Phillips (+) & Flathead (-) | Fastening XLR & Box Screws |
| 18 | **3m DMX XLR Cable** | 120-Ohm Shielded Twisted Pair | Signal Transmission to Lights |
| 19 | **Host Laptop (Acer Swift 3)**| Windows 11 / Linux Host Running QLC+ | Art-Net Software Controller |
| 20 | **PAR LED 54 Fixtures** | 54x3W RGBW Stage Wash Lights | Physical DMX Actuation Load |
| 21 | **Soldering Flux Paste** | Rosin Soldering Flux Grease Paste | Clean & High-Quality Solder Joints |
| 22 | **Craft Scissors** | Sharp Stainless Steel Scissors | Cable & Heat-Shrink Trimming |

*(All component photos are accessible in the [`images/`](images/) folder and indexed in [`REPORT.pdf`](REPORT.pdf)).*

---

## 📐 Schematics & Pinout Wiring

### 🔌 Pinout Connection Table

```text
+-------------------+--------------------+------------------------+-----------------------+
| Source Pin        | Source Label       | Target Component       | Target Pin            |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | MAX485 Module          | VCC (5V)              |
| ESP32 DevKit V1   | GND                | MAX485 Module          | GND                   |
| ESP32 DevKit V1   | GPIO 4 (D4)        | MAX485 Module          | DE & /RE (Tied HIGH)  |
| ESP32 DevKit V1   | GPIO 17 (TX2)      | MAX485 Module          | DI (Data In)          |
+-------------------+--------------------+------------------------+-----------------------+
| MAX485 Module     | Pin A (Data +)     | XLR 3-Pin Female       | Pin 3 (DMX Data +)    |
| MAX485 Module     | Pin B (Data -)     | XLR 3-Pin Female       | Pin 2 (DMX Data -)    |
| MAX485 Module     | Pin GND (Shield)   | XLR 3-Pin Female       | Pin 1 (DMX Ground)    |
+-------------------+--------------------+------------------------+-----------------------+
| ESP32 DevKit V1   | VIN (5V)           | I2C LCD 16x2 Display   | VCC (5V)              |
| ESP32 DevKit V1   | GND                | I2C LCD 16x2 Display   | GND                   |
| ESP32 DevKit V1   | GPIO 21 (D21)      | I2C LCD 16x2 Display   | SDA                   |
| ESP32 DevKit V1   | GPIO 22 (D22)      | I2C LCD 16x2 Display   | SCL                   |
+-------------------+--------------------+------------------------+-----------------------+
```

### 📊 Available Schematics (in [`schematics/`](schematics/))
* **[01_system_block_diagram.png](schematics/01_system_block_diagram.png)** — End-to-end signal flow from lighting software to ESP32 receiver, MAX485 driver, and stage PAR LEDs.
* **[02_esp32_max485_wiring_schematic.png](schematics/02_esp32_max485_wiring_schematic.png)** — Complete wiring schematic between ESP32 DevKit V1, MAX485 transceiver, I2C LCD, and 3-pin XLR socket.
* **[03_dmx512_timing_diagram.png](schematics/03_dmx512_timing_diagram.png)** — High-precision DMX512-A frame waveform at 250 kbps (Break >= 88µs, MAB >= 8µs, Start Code 0x00, 512 channels, 44.1 Hz).
* **[04_freertos_dual_core_architecture.png](schematics/04_freertos_dual_core_architecture.png)** — Task separation between Core 0 (Real-Time DMX Generation) and Core 1 (Networking & Web UI) via Spinlock Double Buffering.
* **[05_lcd16x2_state_machine_flowchart.png](schematics/05_lcd16x2_state_machine_flowchart.png)** — State machine transition diagram for LCD 16x2 status indicators, auto-recovery, and blackout modes.

---

## 🛠️ Fabrication & Assembly Summary

The complete step-by-step physical build process (with 30 photos in [`docs/`](docs/)) consists of 4 main phases:

1. **Breadboard Prototyping & Testing:** Validating UART2 DMX transmission and I2C LCD address (`0x27`).
2. **Box X4 Enclosure Machining:** Drilling front panel hole with a step drill bit (4–22mm) for the round XLR 3-pin female chassis and rectangular top cutout for LCD 16x2.
3. **Perfboard Soldering & Internal Wiring:** Soldering header sockets, routing 5V power rails, hooking up MAX485 DE/RE pins to GPIO 4, and attaching XLR pins (1: GND, 2: Data-, 3: Data+).
4. **Final Assembly & Bench Testing:** Fastening the PCB inside Box X4 with screws, closing the lid, and connecting to stage lighting fixtures for live actuation.

---

## 🧠 Firmware & Dual-Core Architecture

The production firmware [`firmware/artnet_dmx_final.ino`](firmware/artnet_dmx_final.ino) is optimized for zero-latency stage performance:

```cpp
// Core 0 Dedicated DMX Generation Loop (44.1 Hz)
void DMX_Task(void *pvParameters) {
    for (;;) {
        // 1. Generate Break Signal (LOW >= 88 us)
        digitalWrite(DMX_TX_PIN, LOW);
        esp_rom_delay_us(88);

        // 2. Generate Mark After Break (HIGH >= 8 us)
        digitalWrite(DMX_TX_PIN, HIGH);
        esp_rom_delay_us(8);

        // 3. Atomically transmit 513 bytes (Start Code + 512 channels)
        portENTER_CRITICAL(&dmxMux);
        Serial2.write((uint8_t*)readBuffer, 513);
        portEXIT_CRITICAL(&dmxMux);

        Serial2.flush();
        vTaskDelay(pdMS_TO_TICKS(1)); // ~22.7ms period = 44.1 Hz
    }
}
```

---

## 🚀 Quick Start: Flashing the Firmware

1. Install **[Arduino IDE 2.x](https://www.arduino.cc/en/software)**.
2. In Arduino IDE Settings, add ESP32 Board URL:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Install **`esp32 by Espressif Systems`** via Boards Manager.
4. Install library **`LiquidCrystal_I2C by Frank de Brabander`** via Library Manager.
5. Connect your ESP32 to PC with a Micro-USB cable.
6. Select board **`DOIT ESP32 DEVKIT V1`** and the correct COM port.
7. Open [`firmware/artnet_dmx_final.ino`](firmware/artnet_dmx_final.ino) and click **Upload**.

---

## 🌐 WiFi Setup via Captive Portal

1. Power on the module.
2. On your phone or laptop, connect to the WiFi network: **`ZZLIGHT-AP`**.
3. A web configuration portal will automatically open (or navigate to `http://192.168.4.1`).
4. Click **Scan WiFi**, pick your stage WiFi router, type the password, and choose your **Art-Net Universe** (default: `0`).
5. Click **Save & Connect**. The node will reboot and join your stage network. The LCD will display its newly assigned IP address!

---

## 🎛️ Universal Software Compatibility

The node accepts standard **Art-Net 4 DMX packets (OpCode `0x5000`) on UDP Port 6454** from any lighting console:

* **[QLC+](https://www.qlcplus.org/):** Go to *Inputs/Outputs* ➔ Select *ArtNet* output plugin ➔ Enter Node IP or Broadcast `255.255.255.255`.
* **[grandMA3 onPC](https://www.malighting.com/):** Go to *Menu* ➔ *Network* ➔ *DMX Protocols* ➔ *Art-Net* (Enabled).
* **[Resolume Arena](https://resolume.com/):** Go to *Preferences* ➔ *DMX* ➔ Add Lumiverse ➔ Target Node IP.
* **[TouchDesigner](https://derivative.ca/):** Use `DMX Out CHOP` in Art-Net mode.
* **[ZZLUXORA](https://github.com/zzdree):** Automated audio-reactive AI lighting control engine.

---

## 📊 Test Results & Performance

| Metric | Target Requirement | Measured Test Result | Status |
|:---|:---:|:---:|:---:|
| **DMX Refresh Rate** | $40 - 44\text{ Hz}$ | **$44.1\text{ Hz}$ Deterministik** | 🌟 Pass |
| **Serial Timing Jitter** | $< 1.0\,\mu\text{s}$ | **$< 0.05\,\mu\text{s}$ (Zero-Jitter)** | 🌟 Pass |
| **WiFi UDP Latency** | $< 20\text{ ms}$ | **$1.8 - 4.2\text{ ms}$ (30m range)** | 🌟 Pass |
| **Fail-Safe Blackout** | $10\text{ seconds}$ | **10.0s Auto Blackout Trigger** | 🌟 Pass |
| **Current Consumption**| $< 500\text{ mA}$ | **$180\text{ mA}$ @ 5V DC** | 🌟 Pass |

---

## 🔮 Roadmap v5.0 (Community Edition) & Invitation to Fork

> 💡 **Project Status:** This project is officially finalized and stable at **v4.0.0 (Production-Ready)**. For anyone wanting to take this project further and build **v5.0.0**, you are warmly invited to fork, experiment, and contribute! (*Silakan dikembangkan sesuka hati!*) 🎉

Recommended features for v5:
1. 🌐 **sACN (ANSI E1.31) Protocol Support** alongside Art-Net 4.
2. 🎛️ **Multi-Universe Outputs (2–4 XLR Ports)** using ESP32 UART/RMT peripherals.
3. 🔄 **Bidirectional RDM (ANSI E1.20)** for remote fixture configuration and sensor telemetry.
4. ⚡ **Hardwired PoE Ethernet (W5500 / LAN8720)** for stadium/arena setups.
5. ☁️ **Web OTA Firmware Update (Drag & Drop .bin)**.
6. 🔍 **ArtPoll & ArtPollReply Auto-Discovery** for seamless grandMA3 / Avolites integration.
7. 📟 **OLED SSD1306 Graphic Display & NeoPixel RGB Status Indicators**.
8. 💾 **Standalone DMX Scene Recorder & Playback** without needing a PC.

---

## 💬 Message to Makers & Forkers

* **Feel free to DIY build this node:** All schematics, BOM, and step-by-step photos are provided under the open-source MIT license.
* **Feel free to fork & create v5:** Build custom KiCad SMD PCBs, 3D printed enclosures, or multi-port nodes. We'd love to see your creations!

---

## 📜 License & Credits

* **Author:** **Andreas Restuawanta Christwara** ([@zzdree](https://github.com/zzdree))
* **Institution:** Universitas Negeri Semarang (UNNES), Computer Engineering.
* **Academic Reference:** Part of Bachelor's Thesis research in Intelligent Stage Lighting Automation.
* **License:** Distributed under the **[MIT License](LICENSE)**. Free for commercial, personal, and educational use!

---

<div align="center">
  <b>⭐ If you find this project helpful, give it a star on GitHub! ⭐</b><br/>
  <i>Built with ❤️ for Stage Lighting Designers, DIY Makers, and Engineers Worldwide.</i>
</div>
