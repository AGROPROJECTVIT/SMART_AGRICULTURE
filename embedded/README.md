# Smart Agriculture – Embedded System

This directory contains the Arduino/ESP32 firmware for the Smart Agriculture
controller.  It reads environmental sensors, automatically controls irrigation
and environment actuators, and publishes live data to an MQTT broker over WiFi.

---

## Hardware

### Microcontroller

| Item | Specification |
|------|---------------|
| Board | ESP32 DevKit v1 (Espressif ESP32-WROOM-32) |
| Clock | 240 MHz dual-core Xtensa LX6 |
| Flash | 4 MB |
| RAM  | 520 KB SRAM |
| Operating voltage | 3.3 V (5 V tolerant via onboard regulator) |
| WiFi | 802.11 b/g/n 2.4 GHz |

---

## Pin-Out

### Sensor Connections

| GPIO | Label | Sensor | Notes |
|------|-------|--------|-------|
| GPIO 4 | `DHT_PIN` | DHT22 Temperature & Humidity | Digital single-wire protocol; add 10 kΩ pull-up to 3.3 V |
| GPIO 34 | `SOIL_PIN` | Capacitive Soil Moisture Sensor | ADC1_CH6 – analogue input only; 3.3 V power rail |
| GPIO 35 | `PH_PIN` | Analogue pH Sensor | ADC1_CH7 – analogue input only; calibrate with pH 4 & pH 7 buffers |
| GPIO 32 | `LIGHT_PIN` | LDR Light Sensor | ADC1_CH4; use voltage divider (10 kΩ + LDR) |
| GPIO 33 | `RAIN_PIN` | Rain / Water-Level Sensor | ADC1_CH5; 3.3 V power |

> **ADC note:** Use ADC1 pins (GPIO 32–39) when WiFi is active.  ADC2 is
> unavailable while the WiFi radio is in use.

### Actuator Connections

| GPIO | Label | Actuator | Relay Logic |
|------|-------|----------|-------------|
| GPIO 26 | `PUMP_PIN` | Water Pump (12 V DC via relay) | Active-LOW – LOW = ON |
| GPIO 27 | `FERT_PIN` | Fertiliser Pump (12 V DC via relay) | Active-LOW – LOW = ON |
| GPIO 25 | `LIGHT_RELAY_PIN` | LED Grow-Light (mains / 12 V) | Active-LOW – LOW = ON |
| GPIO 14 | `FAN_PIN` | Ventilation Fan (12 V DC via relay) | Active-LOW – LOW = ON |

### Communication / Peripheral Connections

| GPIO | Label | Peripheral |
|------|-------|-----------|
| GPIO 21 | `I2C_SDA` | I²C data – OLED display / DS3231 RTC |
| GPIO 22 | `I2C_SCL` | I²C clock – OLED display / DS3231 RTC |
| GPIO 2  | `LED_PIN` | Onboard status LED (active-HIGH) |

---

## Wiring Diagram (text schematic)

```
                        3.3 V ──┬── DHT22 VCC
                                ├── Soil Sensor VCC
                                ├── pH Sensor VCC
                                ├── LDR + 10 kΩ (to LIGHT_PIN)
                                └── Rain Sensor VCC

                        GND  ──┬── DHT22 GND
                                ├── Soil Sensor GND
                                ├── pH Sensor GND
                                └── Rain Sensor GND

  GPIO 4  ── 10 kΩ ─── 3.3 V
          └──────────── DHT22 DATA

  GPIO 34 ──────────── Soil Sensor AOUT
  GPIO 35 ──────────── pH Sensor AOUT
  GPIO 32 ──────────── LDR Voltage Divider mid-point
  GPIO 33 ──────────── Rain Sensor AOUT

  GPIO 26 ── Relay IN1 (Water Pump relay)
  GPIO 27 ── Relay IN2 (Fertiliser Pump relay)
  GPIO 25 ── Relay IN3 (Grow-Light relay)
  GPIO 14 ── Relay IN4 (Fan relay)

  Relay VCC ─── 5 V
  Relay GND ─── GND
  Relay COM ─── 12 V supply (or mains via appropriate module)
  Relay NO  ─── Pump / Light / Fan positive terminal

  GPIO 21 (SDA) ──── OLED SDA / RTC SDA
  GPIO 22 (SCL) ──── OLED SCL / RTC SCL
```

---

## Automation Logic

| Actuator | Turn ON condition | Turn OFF condition |
|----------|-------------------|-------------------|
| Water Pump | Soil moisture < 30 % **and** not raining | Soil moisture > 70 % **or** raining |
| Fertiliser Pump | pH < 5.5 | pH > 7.0 |
| Grow-Light | Light ADC < 300 | Light ADC > 500 |
| Fan | Temperature > 35 °C **or** Humidity > 85 % | Temperature < 30 °C **and** Humidity ≤ 85 % |

All thresholds are configurable in [`config.h`](main/config.h).

---

## MQTT Topics

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `smartagri/sensors` | Publish | JSON | All sensor readings + actuator states |
| `smartagri/actuators/pump` | Subscribe | `"1"` / `"0"` | Remote pump override |
| `smartagri/actuators/fertiliser` | Subscribe | `"1"` / `"0"` | Remote fertiliser override |
| `smartagri/actuators/light` | Subscribe | `"1"` / `"0"` | Remote grow-light override |
| `smartagri/actuators/fan` | Subscribe | `"1"` / `"0"` | Remote fan override |
| `smartagri/status` | Publish | `"online"` | Device heartbeat (retained) |

### Example JSON payload (`smartagri/sensors`)

```json
{
  "temperature": 27.4,
  "humidity": 62.5,
  "soil_moisture": 38,
  "ph": 6.3,
  "light": 890,
  "rain": 3100,
  "is_raining": false,
  "pump": true,
  "fertiliser": false,
  "light_relay": false,
  "fan": false
}
```

---

## Software Setup

### Prerequisites

1. Install **Arduino IDE 2.x** or **PlatformIO**.
2. Add ESP32 board support:
   - Arduino IDE → Preferences → Additional Boards Manager URLs:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Boards Manager → search *esp32* → install **esp32 by Espressif Systems ≥ 2.0.0**
3. Install libraries via Library Manager:
   - **DHT sensor library** by Adafruit ≥ 1.4.4
   - **Adafruit Unified Sensor** by Adafruit ≥ 1.1.9
   - **PubSubClient** by Nick O'Leary ≥ 2.8.0
   - **ArduinoJson** by Benoit Blanchon ≥ 6.21.0

### Configure WiFi credentials

Copy the template and fill in your credentials:

```bash
cp embedded/main/secrets.h.example embedded/main/secrets.h
```

Edit `embedded/main/secrets.h`:

```c
#define WIFI_SSID     "your_network_name"
#define WIFI_PASSWORD "your_network_password"
```

> **Security:** `secrets.h` is listed in `.gitignore` and must **never** be
> committed.  Only `secrets.h.example` (containing no real credentials) is
> tracked in version control.

### Flash the firmware

1. Open `embedded/main/main.ino` in Arduino IDE.
2. Select board: **Tools → Board → ESP32 Dev Module**.
3. Select port: **Tools → Port → COMx / /dev/ttyUSBx**.
4. Upload speed: **115200** (Serial monitor).
5. Click **Upload** (Ctrl+U).

### Monitor output

Open Serial Monitor at **115200 baud** to see live sensor readings and
actuator state changes.

---

## File Structure

```
embedded/
├── README.md          ← this file
└── main/
    ├── main.ino       ← Arduino sketch (setup / loop, WiFi, MQTT)
    ├── config.h       ← pin definitions, thresholds, MQTT settings
    ├── sensors.h      ← sensor API header
    ├── sensors.cpp    ← sensor implementation (DHT22, ADC reads)
    ├── actuators.h    ← actuator API header
    └── actuators.cpp  ← actuator implementation (relay control, auto-logic)
```
