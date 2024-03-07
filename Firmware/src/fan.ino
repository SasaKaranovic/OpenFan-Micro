#include <Arduino.h>
#include <esp32-hal.h>
#include <pins_arduino.h>
#include "config_openfan.h"

void IRAM_ATTR FAN_RPM_ISR() {
  fan_int_cnt++;
}

void start_rpm_measurement(void)
{
    bRPM_counting = true;
    fan_millis_start = millis();
    fan_int_cnt = 0;
    attachInterrupt(digitalPinToInterrupt(PIN_FAN_TACH), FAN_RPM_ISR, FALLING);
}

void end_rpm_measurement(void)
{
    detachInterrupt(digitalPinToInterrupt(PIN_FAN_TACH));
    fan_millis_end = millis();
    bRPM_counting = false;
    uint32_t period = fan_millis_end - fan_millis_start;
    if (period == 0)
    {
        Serial.println("Avoiding division by zero");
        return;
    }

    fan_int_cnt = fan_int_cnt/eepromData.impulse_per_rev;
    fan_rpm = fan_int_cnt * ( (float)60000 / (float)period );
    fan_int_cnt = 0;
}

void set_pwm(uint8_t percent)
{
    if(percent>100)
    {
        percent = 100;
    }
    uint8_t pwm_value;
    pwm_value = ( ( (float)percent*(float)255) / (float)100);
    u8FanPercent = percent;
    ledcWrite(PWM_CH_FAN, pwm_value);

    Serial.print("Setting fan PWM to ");
    Serial.print(percent);
    Serial.print("% (");
    Serial.print(pwm_value);
    Serial.println("/255)");

    // Reset nFanSave_Tick
    nFanSave_Tick = millis() + FAN_PWM_SAVE_PERIOD;
}

void fan_enable_12V(bool enable)
{
    if (enable)
    {
        Serial.println("Enabling 12V DC-DC");
        digitalWrite(PIN_LDO_EN, HIGH);
        digitalWrite(PIN_LDO_LED, HIGH);
    }
    else
    {
        Serial.println("DC-DC is off. Fan should get 5V");
        digitalWrite(PIN_LDO_EN, LOW);
        digitalWrite(PIN_LDO_LED, LOW);
    }
}

void fan_configure_as_5V(void)
{
    eepromData.enable_12v = false;
    eeprom_save();
    fan_enable_12V(eepromData.enable_12v);
}

void fan_configure_as_12V(void)
{
    eepromData.enable_12v = true;
    eeprom_save();
    fan_enable_12V(eepromData.enable_12v);
}

void fan_setup(void)
{
    // Setup Tach
    pinMode(PIN_FAN_TACH, INPUT);
    digitalWrite(PIN_FAN_TACH, HIGH);
    // Setup PWM
    ledcSetup(PWM_CH_FAN, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(PIN_FAN_PWM, PWM_CH_FAN);
    u8FanPercent = eepromData.last_percent;
    set_pwm(u8FanPercent);
    // Setup DC-DC
    fan_enable_12V(eepromData.enable_12v);
}

void fan_tick(void)
{
    if (nRPM_Tick <= millis())
    {
        // Start RPM counting and schedule end in FAN_RPM_SAMPLE_PERIOD
        if(!bRPM_counting)
        {
            start_rpm_measurement();
            nRPM_Tick = millis() + FAN_RPM_SAMPLE_PERIOD;
        }
        // End RPM counting and schedule next one in 1sec
        else
        {
            end_rpm_measurement();
            nRPM_Tick = millis() + FAN_RPM_PAUSE_PERIOD;
        }
    }

    // Every FAN_PWM_SAVE_PERIOD miliseconds, save PWM value
    // It will be reapplied on power up
    if (nFanSave_Tick <= millis())
    {
        if (eepromData.last_percent != u8FanPercent)
        {
            Serial.println("Backing up new PWM value.");
            eepromData.last_percent = u8FanPercent;
            eeprom_save();
        }
        nFanSave_Tick = millis() + FAN_PWM_SAVE_PERIOD;
    }
}
