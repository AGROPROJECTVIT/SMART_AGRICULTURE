/**
 * @file actuators.h
 * @brief Smart Agriculture – Actuator Control Header
 *
 * Actuators supported:
 *   - Water pump       (relay, GPIO 26)
 *   - Fertiliser pump  (relay, GPIO 27)
 *   - LED grow-light   (relay, GPIO 25)
 *   - Ventilation fan  (relay, GPIO 14)
 *
 * All relay modules are active-LOW: writing LOW turns the relay ON,
 * writing HIGH turns it OFF.
 */

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include "config.h"
#include "sensors.h"

/* -------------------------------------------------------------------------- */
/*  State structure                                                            */
/* -------------------------------------------------------------------------- */

/** Tracks the current on/off state of every actuator. */
struct ActuatorState {
    bool pumpOn;        /**< Water pump is active           */
    bool fertOn;        /**< Fertiliser pump is active      */
    bool lightOn;       /**< LED grow-light is active       */
    bool fanOn;         /**< Ventilation fan is active      */
};

/* -------------------------------------------------------------------------- */
/*  Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Configure all actuator GPIO pins as outputs and set them OFF.
 *        Call once from setup().
 */
void actuators_init(void);

/**
 * @brief Apply automatic control logic based on the latest sensor data.
 *
 *  Rules implemented:
 *   - Water pump   ON  when soil moisture < SOIL_MOISTURE_LOW  and not raining.
 *   - Water pump   OFF when soil moisture > SOIL_MOISTURE_HIGH or raining.
 *   - Fert. pump   ON  when pH < PH_LOW_THRESH  (alkaline correction needed).
 *   - Fert. pump   OFF when pH > PH_HIGH_THRESH (acceptable range reached).
 *   - Grow-light   ON  when light level < LIGHT_LOW_THRESH.
 *   - Grow-light   OFF when light level > LIGHT_HIGH_THRESH.
 *   - Fan          ON  when temperature > TEMP_HIGH_THRESH or
 *                                humidity > HUMIDITY_HIGH_THRESH.
 *   - Fan          OFF when temperature < TEMP_LOW_THRESH.
 *
 * @param data  Sensor readings to base decisions on.
 */
void actuators_auto_control(const SensorData &data);

/**
 * @brief Manually set the water pump on or off.
 *
 * @param on  true to activate, false to deactivate.
 */
void actuators_set_pump(bool on);

/**
 * @brief Manually set the fertiliser pump on or off.
 *
 * @param on  true to activate, false to deactivate.
 */
void actuators_set_fert(bool on);

/**
 * @brief Manually set the LED grow-light on or off.
 *
 * @param on  true to activate, false to deactivate.
 */
void actuators_set_light(bool on);

/**
 * @brief Manually set the ventilation fan on or off.
 *
 * @param on  true to activate, false to deactivate.
 */
void actuators_set_fan(bool on);

/**
 * @brief Return a copy of the current actuator state.
 *
 * @return ActuatorState snapshot.
 */
ActuatorState actuators_get_state(void);

/**
 * @brief Print the current actuator state to the Serial monitor.
 */
void actuators_print(void);

#endif /* ACTUATORS_H */
