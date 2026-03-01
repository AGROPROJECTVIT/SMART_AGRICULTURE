# SMART_AGRICULTURE

An IoT-based smart agriculture system that uses an ESP32 microcontroller to
automate irrigation, fertilisation, lighting and ventilation based on real-time
sensor readings.

## Features

- **Sensors:** DHT22 (temperature & humidity), capacitive soil-moisture, pH,
  LDR light sensor, rain sensor
- **Actuators:** Water pump, fertiliser pump, LED grow-light, ventilation fan
  (all relay-controlled)
- **Connectivity:** WiFi + MQTT for remote monitoring and control
- **Automation:** Rule-based auto-control with configurable thresholds

## Repository Layout

```
SMART_AGRICULTURE/
├── embedded/          ← ESP32 firmware
│   ├── README.md      ← Wiring, pin-out, setup guide
│   └── main/          ← Arduino sketch + source files
└── README.md          ← This file
```

## Quick Start

See **[embedded/README.md](embedded/README.md)** for the full pin-out table,
wiring diagram, library requirements, and flashing instructions.