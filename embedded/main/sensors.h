/**
 * @file sensors.h
 * @brief Smart Agriculture – Sensor Interfacing Header
 *
 * Sensors supported:
 *   - DHT22   : Temperature & relative humidity (digital, single-wire)
 *   - Soil    : Capacitive soil-moisture sensor (analogue, ADC)
 *   - pH      : Analogue pH sensor (ADC)
 *   - Light   : LDR light sensor (ADC)
 *   - Rain    : Resistive rain/water-level sensor (ADC)
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

/* -------------------------------------------------------------------------- */
/*  Data structures                                                            */
/* -------------------------------------------------------------------------- */

/** Holds a single snapshot of all sensor readings. */
struct SensorData {
    float   temperature;    /**< Air temperature (°C)                         */
    float   humidity;       /**< Relative humidity (% RH)                     */
    int     soilMoisture;   /**< Soil moisture (%, 0 = dry, 100 = saturated)  */
    float   phValue;        /**< Soil / solution pH (0–14)                    */
    int     lightLevel;     /**< Ambient light (raw ADC, 0–4095)              */
    int     rainLevel;      /**< Rain sensor (raw ADC, 0–4095)                */
    bool    isRaining;      /**< true when rain detected above threshold       */
    bool    valid;          /**< true when all readings are plausible          */
};

/* -------------------------------------------------------------------------- */
/*  Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialise all sensor pins and the DHT22 library.
 *        Call once from setup().
 */
void sensors_init(void);

/**
 * @brief Read all sensors and populate a SensorData structure.
 *
 * @param[out] data  Reference to a SensorData structure to fill.
 * @return true  if all readings are within plausible ranges.
 * @return false if any critical reading is NaN or out of range.
 */
bool sensors_read(SensorData &data);

/**
 * @brief Convert a raw ADC soil-moisture value to a percentage.
 *
 * @param rawAdc  Raw ADC reading (0–4095).
 * @return Soil moisture percentage (0–100).
 */
int soil_adc_to_percent(int rawAdc);

/**
 * @brief Convert a raw ADC voltage to a calibrated pH value.
 *
 * @param rawAdc  Raw ADC reading (0–4095).
 * @return Calibrated pH value (0.0–14.0).
 */
float ph_adc_to_value(int rawAdc);

/**
 * @brief Print all sensor readings to the Serial monitor.
 *
 * @param data  Reference to a populated SensorData structure.
 */
void sensors_print(const SensorData &data);

#endif /* SENSORS_H */
