/**
 * @file config.h
 * @brief Smart Agriculture ESP32 – Pin Definitions, Thresholds & Configuration
 *
 * =============================================================================
 *  PIN-OUT TABLE  (ESP32 DevKit v1)
 * =============================================================================
 *
 *  ┌─────────┬──────────┬────────────────────────────────────────────────────┐
 *  │ GPIO    │ Label    │ Description                                        │
 *  ├─────────┼──────────┼────────────────────────────────────────────────────┤
 *  │ GPIO 4  │ DHT_PIN  │ DHT22 Temperature & Humidity sensor data line      │
 *  │ GPIO 34 │ SOIL_PIN │ Capacitive soil moisture sensor (ADC1_CH6, input)  │
 *  │ GPIO 35 │ PH_PIN   │ Analog pH sensor (ADC1_CH7, input only)            │
 *  │ GPIO 32 │ LIGHT_PIN│ LDR light sensor (ADC1_CH4)                        │
 *  │ GPIO 33 │ RAIN_PIN │ Rain / water-level sensor (ADC1_CH5)               │
 *  │ GPIO 26 │ PUMP_PIN │ Water pump relay (active-LOW relay module)         │
 *  │ GPIO 27 │ FERT_PIN │ Fertiliser pump relay (active-LOW relay module)    │
 *  │ GPIO 25 │ LIGHT_RELAY_PIN │ LED grow-light relay                       │
 *  │ GPIO 14 │ FAN_PIN  │ Ventilation fan relay                              │
 *  │ GPIO 21 │ SDA      │ I²C data  (OLED display / RTC)                    │
 *  │ GPIO 22 │ SCL      │ I²C clock (OLED display / RTC)                    │
 *  │ GPIO 2  │ LED_PIN  │ Onboard status LED                                 │
 *  └─────────┴──────────┴────────────────────────────────────────────────────┘
 *
 *  Power supply notes:
 *   • ESP32 operates at 3.3 V logic; relay module VCC = 5 V.
 *   • Soil, pH and LDR sensors powered from 3.3 V rail.
 *   • Water/fertiliser pumps: 12 V DC via separate supply.
 * =============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* -------------------------------------------------------------------------- */
/*  Sensor pins                                                                */
/* -------------------------------------------------------------------------- */
#define DHT_PIN        4    /* DHT22 data – digital I/O                       */
#define DHT_TYPE       22   /* DHT22 sensor type identifier                   */

#define SOIL_PIN       34   /* Capacitive soil-moisture – ADC input           */
#define PH_PIN         35   /* pH sensor analogue output – ADC input          */
#define LIGHT_PIN      32   /* LDR light sensor – ADC input                   */
#define RAIN_PIN       33   /* Rain / water-level sensor – ADC input          */

/* -------------------------------------------------------------------------- */
/*  Actuator pins  (relay modules: LOW = ON, HIGH = OFF)                      */
/* -------------------------------------------------------------------------- */
#define PUMP_PIN        26   /* Water pump relay                               */
#define FERT_PIN        27   /* Fertiliser pump relay                          */
#define LIGHT_RELAY_PIN 25   /* LED grow-light relay                           */
#define FAN_PIN         14   /* Ventilation fan relay                          */

/* -------------------------------------------------------------------------- */
/*  Miscellaneous                                                              */
/* -------------------------------------------------------------------------- */
#define LED_PIN         2    /* Onboard LED for status indication              */
#define I2C_SDA        21    /* I²C SDA (OLED / DS3231 RTC)                   */
#define I2C_SCL        22    /* I²C SCL (OLED / DS3231 RTC)                   */

/* -------------------------------------------------------------------------- */
/*  ADC calibration                                                            */
/*  Soil sensor: 4095 = dry air, 1500 = saturated soil (tune to hardware)    */
/* -------------------------------------------------------------------------- */
#define SOIL_DRY_VALUE   3200  /* ADC count when soil is completely dry       */
#define SOIL_WET_VALUE   1500  /* ADC count when soil is saturated            */

/*  pH sensor: raw ADC maps linearly to 0–14 pH range                        */
#define PH_SENSOR_VREF   3.3f  /* Reference voltage (V)                       */
#define PH_OFFSET        0.0f  /* Calibration offset  (tune with buffer)      */

/* -------------------------------------------------------------------------- */
/*  Automation thresholds                                                      */
/* -------------------------------------------------------------------------- */
#define SOIL_MOISTURE_LOW    30   /* % – irrigate when moisture drops below   */
#define SOIL_MOISTURE_HIGH   70   /* % – stop irrigation above this level     */

#define TEMP_HIGH_THRESH     35.0f  /* °C – turn on ventilation fan            */
#define TEMP_LOW_THRESH      30.0f  /* °C – turn off ventilation fan           */

#define HUMIDITY_HIGH_THRESH 85.0f  /* % RH – reduce humidity (fan/vent)      */

#define LIGHT_LOW_THRESH     300    /* ADC – supplement with grow-light below  */
#define LIGHT_HIGH_THRESH    500    /* ADC – turn off grow-light above         */

#define PH_LOW_THRESH        5.5f   /* pH – add alkaline fertiliser            */
#define PH_HIGH_THRESH       7.0f   /* pH – add acidic fertiliser              */

#define RAIN_THRESHOLD       1000   /* ADC – skip irrigation when raining      */

/* -------------------------------------------------------------------------- */
/*  Timing (milliseconds)                                                      */
/* -------------------------------------------------------------------------- */
#define SENSOR_READ_INTERVAL   5000UL   /* Read sensors every 5 s             */
#define MQTT_PUBLISH_INTERVAL  30000UL  /* Publish MQTT data every 30 s       */
#define PUMP_MAX_ON_TIME       60000UL  /* Pump auto-off safety timer (60 s)  */

/* -------------------------------------------------------------------------- */
/*  WiFi credentials                                                           */
/*  Create secrets.h from secrets.h.example and fill in your credentials.    */
/*  secrets.h is listed in .gitignore and must never be committed.            */
/* -------------------------------------------------------------------------- */
#if __has_include("secrets.h")
#  include "secrets.h"
#else
#  ifndef WIFI_SSID
#    define WIFI_SSID     "YOUR_WIFI_SSID"
#  endif
#  ifndef WIFI_PASSWORD
#    define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#  endif
#endif

/* -------------------------------------------------------------------------- */
/*  MQTT broker settings                                                       */
/* -------------------------------------------------------------------------- */
#define MQTT_BROKER   "broker.hivemq.com"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "smart_agri_esp32"

/* MQTT topics */
#define TOPIC_SENSOR_DATA   "smartagri/sensors"
#define TOPIC_PUMP_CMD      "smartagri/actuators/pump"
#define TOPIC_FERT_CMD      "smartagri/actuators/fertiliser"
#define TOPIC_LIGHT_CMD     "smartagri/actuators/light"
#define TOPIC_FAN_CMD       "smartagri/actuators/fan"
#define TOPIC_STATUS        "smartagri/status"

#endif /* CONFIG_H */
