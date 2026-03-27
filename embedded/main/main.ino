/*
 * ============================================================
 *  Smart Irrigation System — NodeMCU (ESP8266)
 *  Converted from Industrial Project Algorithm
 *
 *  Hardware:
 *   - NodeMCU ESP8266
 *   - SIM800L  GSM module  (Serial1 / SoftwareSerial)
 *   - NEO-6M   GPS module  (SoftwareSerial)
 *   - DS3231   RTC         (I2C: SDA=D2, SCL=D1)
 *   - ACS712   Current sensor (A0 via voltage divider)
 *   - L298N    Motor driver   (IN1=D5, IN2=D0, EN=D5-PWM)
 *     Motor runs FORWARD to pump water, REVERSE/BRAKE to stop
 *   - Voltage divider for battery monitoring (A0 switched)
 * ============================================================
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

// ─── Pin Definitions ────────────────────────────────────────
#define RELAY_PIN       D5
#define ADC_PIN         A0
#define CURRENT_SEL_PIN D6
#define POWER_SENSE_PIN D7
#define GSM_RX          D3
#define GSM_TX          D4
#define GPS_RX          D8
#define GPS_TX          D9

// ─── Constants ──────────────────────────────────────────────
#define BATTERY_LOW_THRESHOLD   650
#define CURRENT_JAM_THRESHOLD   800
#define GSM_RETRY_INTERVAL_MS   30000
#define GPS_TIMEOUT_MS          120000
#define WATCHDOG_TIMEOUT_MS     8000
#define EEPROM_SIZE             64
#define ADDR_MODE               0
#define ADDR_SESSION_COUNT      1
#define ADDR_REMAINING_TIME     2
#define ADDR_LAST_RUN_H         6
#define ADDR_LAST_RUN_M         7

// ─── Forward declarations ────────────────────────────────────
class BatteryMonitor;
class GSMModule;
class GPSModule;
class RTCManager;
class PumpController;
class FlashStorage;
class SystemSafety;
class IrrigationScheduler;

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
RTC_DS3231     rtc;

// ============================================================
//  CLASS: FlashStorage  (Steps 9, 29)
// ============================================================
class FlashStorage {
public:
  void begin() { EEPROM.begin(EEPROM_SIZE); }

  void    saveMode(uint8_t mode)          { EEPROM.write(ADDR_MODE, mode); EEPROM.commit(); }
  uint8_t loadMode()                      { return EEPROM.read(ADDR_MODE); }

  void     saveRemainingTime(uint32_t s)  { EEPROM.put(ADDR_REMAINING_TIME, s); EEPROM.commit(); }
  uint32_t loadRemainingTime()            { uint32_t v=0; EEPROM.get(ADDR_REMAINING_TIME,v); return v; }

  void    saveSessionCount(uint8_t c)     { EEPROM.write(ADDR_SESSION_COUNT, c); EEPROM.commit(); }
  uint8_t loadSessionCount()              { return EEPROM.read(ADDR_SESSION_COUNT); }

  void saveLastRun(uint8_t h, uint8_t m) {
    EEPROM.write(ADDR_LAST_RUN_H, h);
    EEPROM.write(ADDR_LAST_RUN_M, m);
    EEPROM.commit();
  }
  void loadLastRun(uint8_t &h, uint8_t &m) {
    h = EEPROM.read(ADDR_LAST_RUN_H);
    m = EEPROM.read(ADDR_LAST_RUN_M);
  }
};

// ============================================================
//  CLASS: BatteryMonitor  (Steps 5, 6, 15, 26)
// ============================================================
class BatteryMonitor {
public:
  void begin() { pinMode(CURRENT_SEL_PIN, OUTPUT); selectBattery(); }

  void selectBattery() { digitalWrite(CURRENT_SEL_PIN, LOW); }
  void selectCurrent() { digitalWrite(CURRENT_SEL_PIN, HIGH); }

  int  readBatteryRaw() { selectBattery(); delay(5); return analogRead(ADC_PIN); }
  bool isBatteryOK()    { return readBatteryRaw() >= BATTERY_LOW_THRESHOLD; }

  int  readCurrentRaw()    { selectCurrent(); delay(5); return analogRead(ADC_PIN); }
  bool isCurrentJammed()   { return readCurrentRaw() > CURRENT_JAM_THRESHOLD; }
};

// ============================================================
//  CLASS: GSMModule  (Steps 4, 7, 24)
// ============================================================
class GSMModule {
  SoftwareSerial &_serial;
  bool _networkReady = false;

public:
  GSMModule(SoftwareSerial &s) : _serial(s) {}

  void begin() { _serial.begin(9600); delay(1000); }

  bool sendAT(const char *cmd, const char *expected, uint32_t timeout = 3000) {
    _serial.println(cmd);
    uint32_t start = millis();
    String response = "";
    while (millis() - start < timeout) {
      while (_serial.available()) response += (char)_serial.read();
      if (response.indexOf(expected) != -1) return true;
      yield();
    }
    return false;
  }

  bool initNetwork() {
    if (!sendAT("AT", "OK", 2000)) return false;
    sendAT("AT+CMGF=1", "OK");
    sendAT("AT+CNMI=1,2,0,0,0", "OK");
    uint8_t retries = 0;
    while (retries < 20) {
      if (sendAT("AT+CREG?", "+CREG: 0,1") ||
          sendAT("AT+CREG?", "+CREG: 0,5")) {
        _networkReady = true;
        return true;
      }
      delay(3000); retries++; yield();
    }
    return false;
  }

  bool sendSMS(const char *number, const char *message) {
    if (!_networkReady) return false;
    _serial.print("AT+CMGS=\""); _serial.print(number); _serial.println("\"");
    delay(500);
    _serial.print(message);
    _serial.write(26);  // Ctrl+Z
    return sendAT("", "OK", 5000);
  }

  String readSMS() {
    String sms = "";
    while (_serial.available()) sms += (char)_serial.read();
    return sms;
  }

  bool isNetworkReady() { return _networkReady; }

  void retryNetwork() { _networkReady = false; initNetwork(); }
};

// ============================================================
//  CLASS: GPSModule  (Steps 4, 8, 25)
// ============================================================
class GPSModule {
  SoftwareSerial &_serial;

public:
  GPSModule(SoftwareSerial &s) : _serial(s) {}

  void begin()   { _serial.begin(9600); }
  void enable()  { _serial.listen(); }
  void disable() { }

  bool getTime(uint8_t &h, uint8_t &m, uint8_t &s,
               uint32_t timeoutMs = GPS_TIMEOUT_MS) {
    enable();
    uint32_t start = millis();
    String line = "";
    while (millis() - start < timeoutMs) {
      while (_serial.available()) {
        char c = _serial.read();
        if (c == '\n') {
          if (line.startsWith("$GPRMC") || line.startsWith("$GPGGA")) {
            int comma1 = line.indexOf(',');
            String t = line.substring(comma1 + 1, comma1 + 7);
            if (t.length() == 6 && isDigit(t[0])) {
              h = t.substring(0,2).toInt();
              m = t.substring(2,4).toInt();
              s = t.substring(4,6).toInt();
              disable(); return true;
            }
          }
          line = "";
        } else { line += c; }
      }
      yield();
    }
    disable(); return false;
  }
};

// ============================================================
//  CLASS: RTCManager  (Steps 4, 8)
// ============================================================
class RTCManager {
  RTC_DS3231 &_rtc;

public:
  RTCManager(RTC_DS3231 &r) : _rtc(r) {}

  bool begin() {
    if (!_rtc.begin()) return false;
    if (_rtc.lostPower()) _rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
    return true;
  }

  void setTime(uint8_t h, uint8_t m, uint8_t s) {
    DateTime n = _rtc.now();
    _rtc.adjust(DateTime(n.year(), n.month(), n.day(), h, m, s));
  }

  DateTime now() { return _rtc.now(); }

  bool matchesSchedule(uint8_t schedH, uint8_t schedM) {
    DateTime n = _rtc.now();
    return (n.hour() == schedH && n.minute() == schedM);
  }
};

// ============================================================
//  CLASS: PumpController  (Steps 3, 14, 16, 19, 20, 22, 23, 28)
//  - start()     : energises relay → pump ON
//  - stop()      : cuts relay     → pump OFF
//  - isRunning() : true while pump is active
//  - isJammed()  : overcurrent → blockage or dry-run detected
// ============================================================
class PumpController {
  BatteryMonitor &_batt;
  uint8_t _relayPin;
  bool    _running = false;

public:
  PumpController(BatteryMonitor &batt, uint8_t pin)
    : _batt(batt), _relayPin(pin) {}

  void begin() {
    pinMode(_relayPin, OUTPUT);
    stop();  // Default pump OFF at startup
  }

  bool start() {
    if (_running) return false;
    if (!_batt.isBatteryOK()) return false;
    digitalWrite(_relayPin, HIGH);   // Relay ON → pump ON
    _running = true;
    return true;
  }

  void stop() {
    digitalWrite(_relayPin, LOW);    // Relay OFF → pump OFF
    _running = false;
  }

  bool isRunning() { return _running; }

  // Overcurrent = pipe blocked OR pump running dry — cut power either way
  bool isJammed() { return _running && _batt.isCurrentJammed(); }
};

// ============================================================
//  CLASS: SystemSafety  (Step 27)
// ============================================================
class SystemSafety {
public:
  void begin() { ESP.wdtEnable(WATCHDOG_TIMEOUT_MS); }
  void feed()  { ESP.wdtFeed(); yield(); }
};

// ============================================================
//  CLASS: IrrigationScheduler  (Steps 10–31)
// ============================================================
class IrrigationScheduler {
public:
  enum Mode { AUTO = 0, MANUAL = 1 };

  struct Session {
    uint8_t  hour;
    uint8_t  minute;
    uint32_t durationSec;
  };

private:
  GSMModule      &_gsm;
  GPSModule      &_gps;
  RTCManager     &_rtc;
  PumpController &_pump;
  BatteryMonitor &_batt;
  FlashStorage   &_flash;
  SystemSafety   &_safety;

  Mode     _mode;
  Session  _schedule[3];
  const char *_adminNumber = "+91XXXXXXXXXX";  // <<< YOUR NUMBER HERE

  uint32_t _irrigationStart  = 0;
  uint32_t _irrigationTarget = 0;
  bool     _irrigating       = false;
  uint8_t  _currentSession   = 0;

public:
  IrrigationScheduler(GSMModule &gsm, GPSModule &gps, RTCManager &rtc,
                      PumpController &pump, BatteryMonitor &batt,
                      FlashStorage &flash, SystemSafety &safety)
    : _gsm(gsm), _gps(gps), _rtc(rtc), _pump(pump),
      _batt(batt), _flash(flash), _safety(safety)
  {
    _schedule[0] = {6,  0,  1800};  // 06:00 — 30 min
    _schedule[1] = {13, 0,  1200};  // 13:00 — 20 min
    _schedule[2] = {18, 0,  1800};  // 18:00 — 30 min
  }

  // Step 9: Restore state from EEPROM
  void restoreState() {
    _mode = (Mode)_flash.loadMode();
    uint32_t rem = _flash.loadRemainingTime();
    if (rem > 0 && rem < 7200) {
      Serial.println("[RESTORE] Resuming incomplete irrigation");
      _irrigationTarget = rem;
      _irrigationStart  = millis() / 1000;
      _irrigating       = true;
      _pump.start();
    }
  }

  // Steps 13–16 / 17: Parse and act on incoming SMS
  void processSMS(const String &sms) {
    if (sms.indexOf("MANUAL") != -1) {
      _mode = MANUAL;
      _flash.saveMode(MANUAL);
      _gsm.sendSMS(_adminNumber, "Switched to Manual Mode");

    } else if (sms.indexOf("AUTO") != -1) {
      _mode = AUTO;
      _flash.saveMode(AUTO);
      _gsm.sendSMS(_adminNumber, "Switched to Automatic Mode");

    } else if (_mode == MANUAL && sms.indexOf("OPEN") != -1) {
      int idx     = sms.indexOf("OPEN");
      int minutes = sms.substring(idx + 5).toInt();
      if (minutes <= 0) minutes = 60;

      if (_pump.isRunning()) return;
      if (!_batt.isBatteryOK()) {
        _gsm.sendSMS(_adminNumber, "Low Battery - Cannot start pump");
        return;
      }
      if (_pump.start()) {
        _irrigationTarget = (uint32_t)minutes * 60;
        _irrigationStart  = millis() / 1000;
        _irrigating       = true;
      }

    } else if (sms.indexOf("CLOSE") != -1) {
      stopIrrigation("Manual Stop Command - Pump Off");
    }
  }

  // Steps 17–22: Auto schedule check
  void checkAutoSchedule() {
    for (uint8_t i = 0; i < 3; i++) {
      if (_rtc.matchesSchedule(_schedule[i].hour, _schedule[i].minute)
          && !_irrigating) {
        if (!isPowerAvailable()) {
          _gsm.sendSMS(_adminNumber, "Power Cut - Irrigation delayed");
          return;
        }
        _currentSession   = i;
        _irrigationTarget = _schedule[i].durationSec;
        _irrigationStart  = millis() / 1000;
        _irrigating       = true;
        _pump.start();
        Serial.println("[AUTO] Session started");
      }
    }
  }

  // Steps 15–16 / 20–23: Monitor active irrigation
  void monitorIrrigation() {
    if (!_irrigating) return;
    uint32_t elapsed = (millis() / 1000) - _irrigationStart;

    // Step 23: Jam / dry-run detection
    if (_pump.isJammed()) {
      _pump.stop();
      _irrigating = false;
      _gsm.sendSMS(_adminNumber, "Pump Blocked - Check system");
      _flash.saveRemainingTime(0);
      return;
    }

    // Step 20: Power failure mid-irrigation
    if (!isPowerAvailable()) {
      uint32_t remaining = (_irrigationTarget > elapsed)
                            ? _irrigationTarget - elapsed : 0;
      _pump.stop();
      _irrigating = false;
      _flash.saveRemainingTime(remaining);
      Serial.println("[SAFETY] Power cut — pump stopped, time saved");
      return;
    }

    // Step 26 during irrigation: battery check
    if (!_batt.isBatteryOK()) {
      stopIrrigation("Low Battery During Irrigation");
      return;
    }

    // Steps 16 / 22: Timer expired
    if (elapsed >= _irrigationTarget) {
      stopIrrigation(_mode == MANUAL
                     ? "Manual Irrigation Completed"
                     : "Auto Irrigation Completed");
    }
  }

  // Steps 28–29: Shut down pump and save session data
  void stopIrrigation(const char *reason) {
    _pump.stop();
    _irrigating = false;
    _flash.saveRemainingTime(0);
    DateTime now = _rtc.now();
    _flash.saveLastRun(now.hour(), now.minute());
    _flash.saveSessionCount(_flash.loadSessionCount() + 1);
    _gsm.sendSMS(_adminNumber, reason);
    Serial.print("[STOP] "); Serial.println(reason);
  }

  // Step 26: Periodic low battery SMS alert
  void checkBatteryAlert() {
    static uint32_t lastAlert = 0;
    if (!_batt.isBatteryOK() && millis() - lastAlert > 300000) {
      _gsm.sendSMS(_adminNumber, "Low Battery - Replace Battery");
      lastAlert = millis();
    }
  }

  bool isPowerAvailable() { return digitalRead(POWER_SENSE_PIN) == HIGH; }
  Mode getMode()          { return _mode; }
};

// ============================================================
//  GLOBAL INSTANCES
// ============================================================
FlashStorage        flash;
BatteryMonitor      battery;
GSMModule           gsm(gsmSerial);
GPSModule           gps(gpsSerial);
RTC_DS3231          ds3231;
RTCManager          rtcMgr(ds3231);
PumpController      pump(battery, RELAY_PIN);
SystemSafety        safety;
IrrigationScheduler scheduler(gsm, gps, rtcMgr, pump, battery, flash, safety);

// ============================================================
//  SETUP — Steps 1–9
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[SYSTEM] Starting Smart Irrigation System");

  flash.begin();
  battery.begin();
  pump.begin();
  pinMode(POWER_SENSE_PIN, INPUT);
  safety.begin();

  // Step 6: Battery warning at boot
  if (!battery.isBatteryOK()) {
    gsm.begin();
    gsm.initNetwork();
    gsm.sendSMS("+91XXXXXXXXXX", "Low Battery - Replace Battery");
    Serial.println("[WARN] Low battery at startup");
  }

  // Step 7: GSM network
  gsm.begin();
  Serial.println("[GSM] Initializing...");
  while (!gsm.initNetwork()) {
    Serial.println("[GSM] Retrying network...");
    delay(3000);
    safety.feed();
  }
  Serial.println("[GSM] Network ready");

  // Step 8: GPS time sync → RTC
  rtcMgr.begin();
  Serial.println("[GPS] Attempting time sync...");
  uint8_t h, m, s;
  if (gps.getTime(h, m, s)) {
    rtcMgr.setTime(h, m, s);
    Serial.println("[GPS] RTC updated");
  } else {
    Serial.println("[GPS] Timeout — using stored RTC time");
  }

  // Step 9: Restore saved session state
  scheduler.restoreState();
  Serial.println("[SYSTEM] Init complete");
}

// ============================================================
//  LOOP — Steps 10–31
// ============================================================
void loop() {
  safety.feed();  // Step 27: Watchdog feed

  // Step 12: Incoming SMS
  String sms = gsm.readSMS();
  if (sms.length() > 0) {
    Serial.print("[SMS] "); Serial.println(sms);
    scheduler.processSMS(sms);
  }

  // Step 17: Auto schedule
  if (scheduler.getMode() == IrrigationScheduler::AUTO)
    scheduler.checkAutoSchedule();

  // Steps 15, 20–23: Active irrigation monitoring
  scheduler.monitorIrrigation();

  // Step 26: Battery alert
  scheduler.checkBatteryAlert();

  // Step 24: GSM reconnect if lost
  if (!gsm.isNetworkReady()) {
    Serial.println("[GSM] Network lost — retrying...");
    gsm.retryNetwork();
  }

  // Step 30: Poll every 500ms
  delay(500);
}
