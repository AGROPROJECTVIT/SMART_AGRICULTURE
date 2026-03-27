/**
 * @file sensors.cpp
 * @brief Smart Agriculture – Sensor Interfacing Implementation
 */

#include "sensors.h"

/* -------------------------------------------------------------------------- */
/*  Module-private objects                                                     */
/* -------------------------------------------------------------------------- */

/** DHT22 sensor instance (pin and type from config.h). */
static DHT dht(DHT_PIN, DHT_TYPE);

/* -------------------------------------------------------------------------- */
/*  Public functions                                                           */
/* -------------------------------------------------------------------------- */

void sensors_init(void)
{
    /* Configure analogue input pins (ESP32 ADC1 – safe to use alongside WiFi) */
    pinMode(SOIL_PIN,  INPUT);
    pinMode(PH_PIN,    INPUT);
    pinMode(LIGHT_PIN, INPUT);
    pinMode(RAIN_PIN,  INPUT);

    /* Start DHT22 library */
    dht.begin();

    Serial.println(F("[SENSORS] Initialised: DHT22, Soil, pH, Light, Rain"));
}

/* --------------------------------------------------------------------------- */

bool sensors_read(SensorData &data)
{
    /* --- DHT22: temperature & humidity ------------------------------------ */
    data.temperature = dht.readTemperature();   /* °C                        */
    data.humidity    = dht.readHumidity();      /* % RH                      */

    /* Validate DHT22 readings */
    if (isnan(data.temperature) || isnan(data.humidity)) {
        Serial.println(F("[SENSORS] DHT22 read failed"));
        data.valid = false;
        return false;
    }

    /* --- Soil moisture ---------------------------------------------------- */
    int soilRaw        = analogRead(SOIL_PIN);
    data.soilMoisture  = soil_adc_to_percent(soilRaw);

    /* --- pH sensor -------------------------------------------------------- */
    int phRaw          = analogRead(PH_PIN);
    data.phValue       = ph_adc_to_value(phRaw);

    /* --- Light sensor ----------------------------------------------------- */
    data.lightLevel    = analogRead(LIGHT_PIN);

    /* --- Rain sensor ------------------------------------------------------ */
    data.rainLevel     = analogRead(RAIN_PIN);
    data.isRaining     = (data.rainLevel < RAIN_THRESHOLD);

    /* Basic range validation */
    bool ok = (data.temperature > -40.0f && data.temperature < 80.0f)
           && (data.humidity    >=   0.0f && data.humidity    <= 100.0f)
           && (data.soilMoisture >= 0 && data.soilMoisture <= 100)
           && (data.phValue     >=  0.0f && data.phValue     <=  14.0f);

    data.valid = ok;
    return ok;
}

/* --------------------------------------------------------------------------- */

int soil_adc_to_percent(int rawAdc)
{
    /*
     * Capacitive sensor output:
     *   SOIL_DRY_VALUE (≈3200) → 0 %
     *   SOIL_WET_VALUE (≈1500) → 100 %
     * Clamp to [0, 100] to handle slight over/under-reads.
     */
    int percent = map(rawAdc, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
    return constrain(percent, 0, 100);
}

/* --------------------------------------------------------------------------- */

float ph_adc_to_value(int rawAdc)
{
    /*
     * Linear mapping from ADC voltage to pH:
     *   voltage = rawAdc * VREF / 4095
     *   pH      = 3.5 * voltage + PH_OFFSET   (empirical slope from datasheet)
     *
     * Tune PH_OFFSET in config.h by calibrating with pH 4 and pH 7 buffers.
     */
    float voltage = (float)rawAdc * PH_SENSOR_VREF / 4095.0f;
    float ph      = (3.5f * voltage) + PH_OFFSET;
    return constrain(ph, 0.0f, 14.0f);
}

/* --------------------------------------------------------------------------- */

void sensors_print(const SensorData &data)
{
    if (!data.valid) {
        Serial.println(F("[SENSORS] Last reading invalid – skipping print"));
        return;
    }

    Serial.println(F("===== Sensor Readings ====="));
    Serial.print(F("  Temperature  : ")); Serial.print(data.temperature, 1); Serial.println(F(" °C"));
    Serial.print(F("  Humidity     : ")); Serial.print(data.humidity,    1); Serial.println(F(" % RH"));
    Serial.print(F("  Soil Moisture: ")); Serial.print(data.soilMoisture);   Serial.println(F(" %"));
    Serial.print(F("  pH Value     : ")); Serial.print(data.phValue,     2); Serial.println();
    Serial.print(F("  Light Level  : ")); Serial.print(data.lightLevel);     Serial.println(F(" (ADC)"));
    Serial.print(F("  Rain Level   : ")); Serial.print(data.rainLevel);      Serial.println(F(" (ADC)"));
    Serial.print(F("  Raining      : ")); Serial.println(data.isRaining ? F("YES") : F("NO"));
    Serial.println(F("==========================="));
}
