/**
 * @file main.ino
 * @brief Smart Agriculture – ESP32 Main Sketch
 *
 * Overview
 * --------
 * This sketch runs on an ESP32 DevKit v1 and implements a fully autonomous
 * smart-agriculture controller.  Every SENSOR_READ_INTERVAL milliseconds it
 * reads all sensors, applies the automatic actuator-control rules, and
 * publishes a JSON payload to an MQTT broker over WiFi.
 *
 * Remote override commands can be sent to the MQTT actuator topics at any time
 * (payload "1" = ON, "0" = OFF).
 *
 * Libraries required (install via Arduino Library Manager):
 *   - DHT sensor library   by Adafruit (≥ 1.4.4)
 *   - Adafruit Unified Sensor (≥ 1.1.9)
 *   - PubSubClient         by Nick O'Leary (≥ 2.8.0)
 *   - ArduinoJson          by Benoit Blanchon (≥ 6.21.0)
 *
 * Board: "ESP32 Dev Module"  (esp32 by Espressif, ≥ 2.0.0)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "sensors.h"
#include "actuators.h"

/* -------------------------------------------------------------------------- */
/*  Module-private objects                                                     */
/* -------------------------------------------------------------------------- */

static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

static SensorData   sensorData;

static unsigned long lastSensorRead  = 0;
static unsigned long lastMqttPublish = 0;

/* -------------------------------------------------------------------------- */
/*  WiFi helpers                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Connect (or reconnect) to the configured WiFi network.
 *        Blocks until connected.
 */
static void wifi_connect(void)
{
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.print(F("[WiFi] Connecting to "));
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }

    Serial.println();
    Serial.print(F("[WiFi] Connected – IP: "));
    Serial.println(WiFi.localIP());
}

/* -------------------------------------------------------------------------- */
/*  MQTT helpers                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief MQTT message callback.
 *        Handles incoming remote-override commands on actuator topics.
 *
 * @param topic    Topic string.
 * @param payload  Byte payload ("1" = ON, "0" = OFF).
 * @param length   Length of payload.
 */
static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    bool on = (length > 0 && payload[0] == '1');

    if (strcmp(topic, TOPIC_PUMP_CMD) == 0) {
        actuators_set_pump(on);
        Serial.print(F("[MQTT] Pump command: ")); Serial.println(on ? F("ON") : F("OFF"));
    } else if (strcmp(topic, TOPIC_FERT_CMD) == 0) {
        actuators_set_fert(on);
        Serial.print(F("[MQTT] Fertiliser command: ")); Serial.println(on ? F("ON") : F("OFF"));
    } else if (strcmp(topic, TOPIC_LIGHT_CMD) == 0) {
        actuators_set_light(on);
        Serial.print(F("[MQTT] Light command: ")); Serial.println(on ? F("ON") : F("OFF"));
    } else if (strcmp(topic, TOPIC_FAN_CMD) == 0) {
        actuators_set_fan(on);
        Serial.print(F("[MQTT] Fan command: ")); Serial.println(on ? F("ON") : F("OFF"));
    }
}

/**
 * @brief Connect (or reconnect) to the MQTT broker.
 *        Subscribes to all actuator command topics on success.
 */
static void mqtt_connect(void)
{
    while (!mqttClient.connected()) {
        Serial.print(F("[MQTT] Connecting to "));
        Serial.println(MQTT_BROKER);

        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println(F("[MQTT] Connected"));

            /* Publish online status */
            mqttClient.publish(TOPIC_STATUS, "online", true /* retain */);

            /* Subscribe to remote actuator command topics */
            mqttClient.subscribe(TOPIC_PUMP_CMD);
            mqttClient.subscribe(TOPIC_FERT_CMD);
            mqttClient.subscribe(TOPIC_LIGHT_CMD);
            mqttClient.subscribe(TOPIC_FAN_CMD);
        } else {
            Serial.print(F("[MQTT] Failed, rc="));
            Serial.print(mqttClient.state());
            Serial.println(F(" – retry in 5 s"));
            delay(5000);
        }
    }
}

/**
 * @brief Build and publish a JSON sensor+actuator payload.
 *
 * Publishes to TOPIC_SENSOR_DATA in the form:
 * @code
 * {
 *   "temperature": 25.3,
 *   "humidity": 65.1,
 *   "soil_moisture": 45,
 *   "ph": 6.8,
 *   "light": 1024,
 *   "rain": 3200,
 *   "is_raining": false,
 *   "pump": false,
 *   "fertiliser": false,
 *   "light_relay": false,
 *   "fan": false
 * }
 * @endcode
 */
static void mqtt_publish_data(void)
{
    StaticJsonDocument<256> doc;

    doc["temperature"]  = sensorData.temperature;
    doc["humidity"]     = sensorData.humidity;
    doc["soil_moisture"]= sensorData.soilMoisture;
    doc["ph"]           = sensorData.phValue;
    doc["light"]        = sensorData.lightLevel;
    doc["rain"]         = sensorData.rainLevel;
    doc["is_raining"]   = sensorData.isRaining;

    ActuatorState aState = actuators_get_state();
    doc["pump"]         = aState.pumpOn;
    doc["fertiliser"]   = aState.fertOn;
    doc["light_relay"]  = aState.lightOn;
    doc["fan"]          = aState.fanOn;

    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    mqttClient.publish(TOPIC_SENSOR_DATA, reinterpret_cast<const uint8_t *>(buffer),
                       static_cast<unsigned int>(n));

    Serial.println(F("[MQTT] Published sensor data"));
}

/* -------------------------------------------------------------------------- */
/*  Arduino setup / loop                                                       */
/* -------------------------------------------------------------------------- */

void setup(void)
{
    Serial.begin(115200);
    delay(200);
    Serial.println(F("\n=== Smart Agriculture Controller – ESP32 ==="));

    /* Initialise sensors and actuators */
    sensors_init();
    actuators_init();

    /* Connect to WiFi and MQTT broker */
    wifi_connect();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    mqtt_connect();

    /* Status LED: solid ON once initialisation is complete */
    digitalWrite(LED_PIN, HIGH);
    Serial.println(F("[SETUP] Initialisation complete"));
}

void loop(void)
{
    /* Maintain WiFi and MQTT connections */
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connect();
    }
    if (!mqttClient.connected()) {
        mqtt_connect();
    }
    mqttClient.loop();

    unsigned long now = millis();

    /* --- Periodic sensor read and auto-control ---------------------------- */
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = now;

        bool ok = sensors_read(sensorData);
        if (ok) {
            sensors_print(sensorData);
            actuators_auto_control(sensorData);
            actuators_print();
        } else {
            Serial.println(F("[LOOP] Sensor read error – skipping control cycle"));
        }
    }

    /* --- Periodic MQTT publish -------------------------------------------- */
    if (now - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
        lastMqttPublish = now;
        if (sensorData.valid) {
            mqtt_publish_data();
        }
    }
}
