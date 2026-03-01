#include <Arduino.h>
#include <STM32LowPower.h>

/*==================================================
                PIN DEFINITIONS
==================================================*/
#define GSM_PWR      PB_0
#define RELAY_PIN    PB_5


/*==================================================
                GSM UART
==================================================*/
HardwareSerial gsmSerial(PA_10, PA_9);


/*==================================================
                GLOBAL VARIABLES
==================================================*/
String receivedSMS = "";
int valveDuration = 0;


/*==================================================
            FUNCTION DECLARATIONS
==================================================*/
void System_Init(void);
void GSM_PowerON(void);
void GSM_PowerOFF(void);
void GSM_SendCommand(const char *cmd);
void GSM_InitSMS(void);
void GSM_ReadSMS(void);
void GSM_ParseSMS(void);
void Valve_ON(void);
void Valve_OFF(void);
void OperateValve(int minutes);
void System_Sleep(uint32_t sleepTime);


/*==================================================
                SYSTEM INITIALIZATION
==================================================*/
void System_Init(void)
{
    Serial.begin(9600);
    delay(2000);

    pinMode(GSM_PWR, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(GSM_PWR, LOW);
    digitalWrite(RELAY_PIN, LOW);

    gsmSerial.begin(9600);

    LowPower.begin();

    Serial.println("SYSTEM READY");
}


/*==================================================
                GSM POWER CONTROL
==================================================*/
void GSM_PowerON(void)
{
    Serial.println("GSM POWER ON");
    digitalWrite(GSM_PWR, HIGH);
    delay(6000);
}

void GSM_PowerOFF(void)
{
    Serial.println("GSM POWER OFF");
    digitalWrite(GSM_PWR, LOW);
}


/*==================================================
                GSM COMMAND
==================================================*/
void GSM_SendCommand(const char *cmd)
{
    gsmSerial.println(cmd);
    delay(1000);
}


/*==================================================
                GSM SMS INIT
==================================================*/
void GSM_InitSMS(void)
{
    GSM_SendCommand("AT");
    GSM_SendCommand("AT+CMGF=1");
    GSM_SendCommand("AT+CNMI=2,2");
}


/*==================================================
                VALVE CONTROL
==================================================*/
void Valve_ON(void)
{
    Serial.println("VALVE ON");
    digitalWrite(RELAY_PIN, HIGH);
}

void Valve_OFF(void)
{
    Serial.println("VALVE OFF");
    digitalWrite(RELAY_PIN, LOW);
}


/*==================================================
            OPERATE VALVE TIMER
==================================================*/
void OperateValve(int minutes)
{
    Valve_ON();

    for(int i=0;i<minutes;i++)
    {
        delay(60000);     // 1 minute
    }

    Valve_OFF();
}


/*==================================================
                SMS PARSER
==================================================*/
void GSM_ParseSMS(void)
{
    int index;

    /* USER COMMAND */
    if(receivedSMS.indexOf("VALVE ON") >= 0)
    {
        index = receivedSMS.lastIndexOf(" ");

        valveDuration =
            receivedSMS.substring(index+1).toInt();

        Serial.print("Running Valve for ");
        Serial.print(valveDuration);
        Serial.println(" minutes");

        OperateValve(valveDuration);
    }

    if(receivedSMS.indexOf("VALVE OFF") >= 0)
    {
        Valve_OFF();
    }

    /* LOAD SHEDDING MESSAGE */
    if(receivedSMS.indexOf("LOAD") >= 0)
    {
        Serial.println("LOAD SHEDDING DETECTED");

        OperateValve(10);   // default action
    }

    receivedSMS="";
}


/*==================================================
                READ GSM DATA
==================================================*/
void GSM_ReadSMS(void)
{
    char rx;

    while(gsmSerial.available())
    {
        rx = gsmSerial.read();

        Serial.write(rx);

        receivedSMS += rx;

        if(rx=='\n')
        {
            GSM_ParseSMS();
        }
    }
}


/*==================================================
                LOW POWER MODE
==================================================*/
void System_Sleep(uint32_t sleepTime)
{
    GSM_PowerOFF();

    Serial.println("ENTERING LOW POWER");

    LowPower.deepSleep(sleepTime);

    Serial.println("WAKEUP");
}


/*==================================================
                    SETUP
==================================================*/
void setup(void)
{
    System_Init();

    GSM_PowerON();

    GSM_InitSMS();
}


/*==================================================
                    MAIN LOOP
==================================================*/
void loop(void)
{
    GSM_ReadSMS();

    /* Sleep after checking SMS */
    System_Sleep(30000);   // sleep 30 sec
}
