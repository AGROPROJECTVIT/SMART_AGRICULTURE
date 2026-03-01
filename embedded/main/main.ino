#include <Arduino.h>
#include <STM32LowPower.h>

/*==================================================
                PIN DEFINITIONS
==================================================*/
#define GSM_PWR   PB_0


/*==================================================
                FUNCTION PROTOTYPES
==================================================*/
void GSM_PowerON(void);
void GSM_PowerOFF(void);
void enterSleep(uint32_t time_ms);


/*==================================================
                GSM POWER ON
==================================================*/
void GSM_PowerON(void)
{
    Serial.println("GSM POWER ON");

    digitalWrite(GSM_PWR, HIGH);

    delay(5000);   // GSM boot time
}


/*==================================================
                GSM POWER OFF
==================================================*/
void GSM_PowerOFF(void)
{
    Serial.println("GSM POWER OFF");

    digitalWrite(GSM_PWR, LOW);
}


/*==================================================
                LOW POWER MODE
==================================================*/
void enterSleep(uint32_t time_ms)
{
    Serial.println("Entering Sleep Mode");

    LowPower.deepSleep(time_ms);

    Serial.println("Wakeup Complete");
}


/*==================================================
                    SETUP
==================================================*/
void setup(void)
{
    Serial.begin(9600);
    delay(2000);

    pinMode(GSM_PWR, OUTPUT);

    GSM_PowerOFF();

    LowPower.begin();

    Serial.println("SYSTEM READY");
}


/*==================================================
                    MAIN LOOP
==================================================*/
void loop(void)
{
    GSM_PowerON();

    delay(10000);      // GSM active time

    GSM_PowerOFF();

    enterSleep(15000); // sleep 15 sec
}
