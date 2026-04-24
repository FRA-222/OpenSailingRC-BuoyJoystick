# OpenSailingRC BuoyJoystick

🌐 [Version française](README.fr.md)

GPS autonomous buoy controller based on the **M5Stack Atom S3**.  
Sends navigation commands to buoys via **LoRa 920 MHz** (long range) or **ESP-NOW 2.4 GHz** (short range), and displays their real-time status on the built-in LCD screen.

This project is part of the **OpenSailingRC** ecosystem:

| Project | Role |
|---------|------|
| **OpenSailingRC-BuoyJoystick** | This project — joystick controller |
| OpenSailingRC-BoatGPS | Autonomous GPS buoy |
| OpenSailingRC-Display | Multi-buoy display |
| OpenSailingRC-Anemometer-v2 | Anemometer sensor |

---

## Required Hardware

| Component | Reference | Description |
|-----------|-----------|-------------|
| Controller | M5Stack Atom S3 | ESP32-S3, 128×128 LCD screen |
| Joystick | M5Stack Dual Atom JoyStick | 2 joysticks + 4 buttons via I2C (STM32 @ 0x59) |
| Radio module (LoRa) | M5Stack Unit LoRaE220-920 | 920 MHz, long range |

> The LoRa module is optional: **ESP-NOW** mode works with the Atom S3 alone.

---

## Software Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- Libraries installed automatically by PlatformIO:
  - `m5stack/M5GFX`
  - `m5stack/M5Unified`
  - `m5stack/M5-LoRa-E220-JP`

---

## Quick Start

### 1. Clone and build

```bash
git clone <repo-url>
cd OpenSailingRC-BuoyJoystick
platformio run
```

### 2. Flash

```bash
platformio run --target upload
```

### 3. Serial monitor (115 200 baud)

```bash
platformio device monitor
```

---

## Configuration

### Communication mode

The mode is selected in `src/main.cpp` by changing the `COMM_MODE` constant:

```cpp
// ESP-NOW: 2.4 GHz, short range (~100 m), no external module required
#define COMM_MODE CommMode::ESP_NOW

// LoRa: 920 MHz, long range (> 1 km), requires the LoRaE220-920 module
#define COMM_MODE CommMode::LORA
```

> **Default mode: LoRa.**  
> In LoRa mode, ESP-NOW is also enabled in passive listening mode to receive buoy status broadcasts (more responsive than LoRa polling).

### LoRa wiring (Atom S3 + Unit LoRaE220-920)

| Signal | Atom S3 pin | LoRaE220-920 pin |
|--------|-------------|-----------------|
| RX (Atom receives) | G1 | Module TX |
| TX (Atom sends) | G2 | Module RX |

> The M0/M1 switches on the module must be set to **OFF** (normal transmission mode).  
> Set M0/M1 to ON only to reconfigure the module using `LORA_MODE_CONFIGURATION`.

### LoRa parameters

| Parameter | Value |
|-----------|-------|
| Frequency | 920.6 MHz (channel 0x00, Japan ISM band) |
| Air data rate | 2.4 kbps |
| TX power | 22 dBm |
| Joystick address | 0x0007 |

---

## Controls

### Buttons

| Button | Index | Action |
|--------|-------|--------|
| BTN_LEFT_STICK | 0 | Short press → `CMD_INIT_HOME` |
| BTN_RIGHT_STICK | 1 | Short press → `CMD_HOME_VALIDATION` |
| BTN_LEFT | 2 | Short press → `CMD_NAV_HOLD` |
| BTN_RIGHT | 3 | Short press → `CMD_NAV_STOP` |
| BTN_ATOM_SCREEN | 4 | Short press → Select next buoy |

### Left joystick (navigation)

| Movement | Command sent |
|----------|-------------|
| Up | `CMD_NAV_CAP` — switch to heading navigation mode |
| Down | `CMD_NAV_HOME` — return to Home position |

### Right joystick (throttle / heading)

| Movement | Command sent |
|----------|-------------|
| Up | `CMD_THROTTLE_INCREASE` |
| Down | `CMD_THROTTLE_DECREASE` |
| Right | `CMD_HEADING_INCREASE` |
| Left | `CMD_HEADING_DECREASE` |

> Detection threshold: `±1500` on a `±2048` range.

---

## Software Architecture

```
main.cpp  (10 Hz loop — Core 1)
│
├── JoystickManager        I2C reading of axes and buttons (STM32 @ 0x59)
│
├── ESPNowCommunication    Implements ICommunication — ESP-NOW broadcast mode
├── LoRaCommunication      Implements ICommunication — UART to LoRaE220-920
│       └── loraRxTask()   FreeRTOS task dedicated to LoRa reception (Core 0)
│
├── BuoyStateManager       Active buoy selection, data consolidation
│                          (merges LoRa + passive ESP-NOW)
│
├── CommandManager         Command generation and sending (with retry + ACK)
│
├── DisplayManager         128×128 LCD display (modes, heading, battery, GPS…)
│
└── Logger                 Centralised logging → USB Serial + optional LCD
```

### `ICommunication` interface

Both radio modes (`ESPNowCommunication` and `LoRaCommunication`) implement the same `ICommunication` interface. `BuoyStateManager` and `CommandManager` are fully mode-agnostic.

---

## Available Commands

```cpp
enum BuoyCommand : uint8_t {
    CMD_INIT_HOME = 0,      // Set Home to current GPS position
    CMD_THROTTLE_INCREASE,  // Increase throttle
    CMD_THROTTLE_DECREASE,  // Decrease throttle
    CMD_HEADING_INCREASE,   // Increase heading
    CMD_HEADING_DECREASE,   // Decrease heading
    CMD_NAV_HOLD,           // Hold current position
    CMD_NAV_CAP,            // Navigate by heading
    CMD_NAV_HOME,           // Navigate to Home position
    CMD_NAV_STOP,           // Stop all movement
    CMD_HOME_VALIDATION,    // Validate Home position
    CMD_HEARTBEAT           // Keepalive (sent automatically every 3 s)
};
```

---

## Data Structures

### Command packet (Joystick → Buoy)

```cpp
struct __attribute__((packed)) CommandPacket {
    uint8_t  targetBuoyId;   // Target buoy ID (0–7)
    BuoyCommand command;     // Command type (BuoyCommand)
    uint32_t timestamp;      // millis() timestamp
    uint16_t sequenceNumber; // Sequence number for deduplication
    uint8_t  ttl;            // 1 = original packet, 0 = relayed by Hub
};
```

### Buoy state (Buoy → Joystick)

```cpp
struct BuoyState {
    uint8_t        buoyId;
    uint32_t       timestamp;
    tEtatsGeneral  generalMode;              // INIT / READY / MAINTENANCE / HOME_DEFINITION / NAV
    tEtatsNav      navigationMode;           // NAV_NOTHING / NAV_HOME / NAV_HOLD / NAV_STOP / NAV_CAP / NAV_BASIC / NAV_TARGET
    bool           gpsOk;
    bool           headingOk;
    bool           yawRateOk;
    double         latitude;
    double         longitude;
    float          temperature;
    float          remainingCapacity;        // Remaining capacity in mAh
    float          distanceToCons;           // Distance to waypoint (m)
    int8_t         autoPilotThrottleCmde;    // Autopilot throttle (-100 to +100 %)
    float          autoPilotTrueHeadingCmde; // Autopilot heading (degrees)
    uint16_t       sequenceNumber;
    uint8_t        ttl;
};
```

### Navigation mode hierarchy

```
tEtatsGeneral (level 1)     tEtatsNav (level 2, active when generalMode == NAV)
───────────────────────     ──────────────────────────────────────────────────
INIT                        NAV_NOTHING
READY                       NAV_HOME
MAINTENANCE                 NAV_HOLD
HOME_DEFINITION             NAV_STOP
NAV ──────────────────────→ NAV_CAP
                            NAV_BASIC
                            NAV_TARGET
```

---

## LCD Display

```
┌────────────────────────┐
│  Buoy #1   ●           │  Header (cyan) + status indicator
├────────────────────────┤
│   CONNECTED            │  Green / Red based on connection
│                        │
│   NAV                  │  General mode
│   NAV_CAP              │  Navigation mode
│                        │
│   Hdg: 045°            │  Autopilot heading
│   75%                  │  Throttle
│                        │
│  GPS  BAT:87%  -72dBm  │  Sensors and signal
└────────────────────────┘
```

**Colour codes:**
- Green: connected, battery > 50 %
- Orange: battery 20–50 %
- Red: disconnected or battery < 20 %
- Blue: command being sent (waiting for ACK)
- Cyan: header
- Yellow: NAV_HOLD mode

---

## Timings

| Item | Value |
|------|-------|
| Main loop | 10 Hz (100 ms) |
| Automatic heartbeat | every 3 s |
| LoRa RX task (Core 0) | 1 ms pause between listen cycles |
| Serial debug output | every 2 s |

---

## Serial Logs at Startup

```
===========================================
  OpenSailingRC - Buoy Joystick v1.0
===========================================

1. Initialisation Joystick...
   -> Joystick: OK

2. Initialisation LoRa 920...
   -> LoRa 920: OK

2b. Initialisation ESP-NOW (écoute passive)...
   -> ESP-NOW passif: OK (réception broadcasts bouées)

3. Attente découverte automatique des bouées...
4. Configuration du mode de communication...
   -> Mode: LoRa 920 MHz
5. Initialisation BuoyStateManager...
6. Initialisation Display...
7. Création tâche LoRa RX sur Core 0...

===========================================
  SYSTEM READY
===========================================
```

---

## Troubleshooting

| Symptom | Likely cause | Solution |
|---------|-------------|---------|
| `ERREUR: Echec initialisation joystick` | STM32 not responding on I2C | Check SDA=38, SCL=39 wiring; STM32 power supply |
| `ERREUR CRITIQUE: Echec initialisation LoRa` | Module not connected or RX/TX swapped | Check G1/G2 wiring; try `LORA_MODE_CONFIGURATION` |
| Buoys never detected in ESP-NOW mode | Buoy out of range or not broadcasting | Check buoy firmware; distance < 100 m |
| Buoys never detected in LoRa mode | Mismatched address or channel | Check `LORA_ADDRESS_L`, `LORA_CHANNEL` in LoRaCommunication.h |
| Black screen | Display not initialised | Check `M5.begin()`; check power supply |

---

## Additional Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — Detailed architecture overview
- [API_DOCUMENTATION.md](API_DOCUMENTATION.md) — Public API documentation
- [LORA_INTEGRATION_GUIDE.md](LORA_INTEGRATION_GUIDE.md) — LoRa integration guide
- [LORA_TROUBLESHOOTING.md](LORA_TROUBLESHOOTING.md) — LoRa troubleshooting
- [DYNAMIC_BUOY_DISCOVERY.md](DYNAMIC_BUOY_DISCOVERY.md) — Automatic buoy discovery
- [BUOY_ESPNOW_CONFIG.md](BUOY_ESPNOW_CONFIG.md) — ESP-NOW configuration on the buoy side

---

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| Microcontroller | ESP32-S3 (M5Stack Atom S3) |
| Screen | 128×128 px LCD |
| Joystick interface | I2C, STM32F030F4P6 @ 0x59 |
| Radio modules | ESP-NOW 2.4 GHz / LoRa 920 MHz |
| Simultaneously supported buoys | 8 (`MAX_BUOYS` constant) |
| Framework | Arduino (PlatformIO, espressif32 @ 6.5.0) |

---

## License

This project is distributed under the **GNU General Public License v3.0**.

- Full license text: **[LICENSE](LICENSE)**
- Official reference: <https://www.gnu.org/licenses/gpl-3.0.html>
