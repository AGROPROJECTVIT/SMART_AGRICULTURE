/**
 * @file actuators.cpp
 * @brief Smart Agriculture – Actuator Control Implementation
 */

#include "actuators.h"

/* -------------------------------------------------------------------------- */
/*  Module-private state                                                       */
/* -------------------------------------------------------------------------- */

/** Current state of all actuators. */
static ActuatorState state = { false, false, false, false };

/* -------------------------------------------------------------------------- */
/*  Helper: relay write (active-LOW relay modules)                            */
/* -------------------------------------------------------------------------- */

/**
 * Write to a relay pin.
 * Active-LOW: LOW turns the relay ON, HIGH turns it OFF.
 */
static inline void relay_write(uint8_t pin, bool activate)
{
    digitalWrite(pin, activate ? LOW : HIGH);
}

/* -------------------------------------------------------------------------- */
/*  Public functions                                                           */
/* -------------------------------------------------------------------------- */

void actuators_init(void)
{
    /* Configure relay pins as digital outputs */
    pinMode(PUMP_PIN,        OUTPUT);
    pinMode(FERT_PIN,        OUTPUT);
    pinMode(LIGHT_RELAY_PIN, OUTPUT);
    pinMode(FAN_PIN,         OUTPUT);

    /* Ensure all relays start in the OFF state */
    relay_write(PUMP_PIN,        false);
    relay_write(FERT_PIN,        false);
    relay_write(LIGHT_RELAY_PIN, false);
    relay_write(FAN_PIN,         false);

    /* Status LED */
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println(F("[ACTUATORS] Initialised: Pump, Fertiliser, Light, Fan"));
}

/* --------------------------------------------------------------------------- */

void actuators_auto_control(const SensorData &data)
{
    if (!data.valid) {
        Serial.println(F("[ACTUATORS] Sensor data invalid – skipping auto control"));
        return;
    }

    /* --- Water pump ------------------------------------------------------- */
    if (!state.pumpOn && data.soilMoisture < SOIL_MOISTURE_LOW && !data.isRaining) {
        actuators_set_pump(true);
        Serial.println(F("[ACTUATORS] Water pump ON  (soil dry)"));
    } else if (state.pumpOn && (data.soilMoisture > SOIL_MOISTURE_HIGH || data.isRaining)) {
        actuators_set_pump(false);
        Serial.println(F("[ACTUATORS] Water pump OFF (soil wet / raining)"));
    }

    /* --- Fertiliser pump -------------------------------------------------- */
    if (!state.fertOn && data.phValue < PH_LOW_THRESH) {
        actuators_set_fert(true);
        Serial.println(F("[ACTUATORS] Fertiliser pump ON  (pH low)"));
    } else if (state.fertOn && data.phValue > PH_HIGH_THRESH) {
        actuators_set_fert(false);
        Serial.println(F("[ACTUATORS] Fertiliser pump OFF (pH OK)"));
    }

    /* --- LED grow-light --------------------------------------------------- */
    if (!state.lightOn && data.lightLevel < LIGHT_LOW_THRESH) {
        actuators_set_light(true);
        Serial.println(F("[ACTUATORS] Grow-light ON  (low ambient light)"));
    } else if (state.lightOn && data.lightLevel > LIGHT_HIGH_THRESH) {
        actuators_set_light(false);
        Serial.println(F("[ACTUATORS] Grow-light OFF (sufficient light)"));
    }

    /* --- Ventilation fan -------------------------------------------------- */
    bool highTemp     = data.temperature > TEMP_HIGH_THRESH;
    bool highHumidity = data.humidity    > HUMIDITY_HIGH_THRESH;
    if (!state.fanOn && (highTemp || highHumidity)) {
        actuators_set_fan(true);
        Serial.println(F("[ACTUATORS] Fan ON  (high temp/humidity)"));
    } else if (state.fanOn && data.temperature < TEMP_LOW_THRESH && !highHumidity) {
        actuators_set_fan(false);
        Serial.println(F("[ACTUATORS] Fan OFF (conditions normal)"));
    }
}

/* --------------------------------------------------------------------------- */

void actuators_set_pump(bool on)
{
    state.pumpOn = on;
    relay_write(PUMP_PIN, on);
}

void actuators_set_fert(bool on)
{
    state.fertOn = on;
    relay_write(FERT_PIN, on);
}

void actuators_set_light(bool on)
{
    state.lightOn = on;
    relay_write(LIGHT_RELAY_PIN, on);
}

void actuators_set_fan(bool on)
{
    state.fanOn = on;
    relay_write(FAN_PIN, on);
}

/* --------------------------------------------------------------------------- */

ActuatorState actuators_get_state(void)
{
    return state;
}

/* --------------------------------------------------------------------------- */

void actuators_print(void)
{
    Serial.println(F("===== Actuator State ====="));
    Serial.print(F("  Water Pump  : ")); Serial.println(state.pumpOn  ? F("ON") : F("OFF"));
    Serial.print(F("  Fert. Pump  : ")); Serial.println(state.fertOn  ? F("ON") : F("OFF"));
    Serial.print(F("  Grow-Light  : ")); Serial.println(state.lightOn ? F("ON") : F("OFF"));
    Serial.print(F("  Fan         : ")); Serial.println(state.fanOn   ? F("ON") : F("OFF"));
    Serial.println(F("=========================="));
}
