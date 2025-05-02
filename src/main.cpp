#include <Arduino.h>

#include "main.h"
#include "myble.h"
#include "TM1640.h"

volatile bool weightChanged, timeChanged;
volatile int16_t previousWeight = 0, currentWeight = 0;
volatile int16_t previousTime = 0, currentTime = 0;

void setT(bool push)
{
  if (push)
  {
    pinMode(BUT_T, OUTPUT_S0D1);
    digitalWrite(BUT_T, LOW);
  }
  else
  {
    digitalWrite(BUT_T, HIGH);
    pinMode(BUT_T, INPUT);
  }
}

void setP(bool push)
{
  if (push)
  {
    pinMode(BUT_P, OUTPUT_S0D1);
    digitalWrite(BUT_P, LOW);
  }
  else
  {
    digitalWrite(BUT_P, HIGH);
    pinMode(BUT_P, INPUT);
  }
}

void setup()
{
  pinMode(PDA, INPUT);
  pinMode(DNC, INPUT);
  pinMode(PCL, INPUT);
  pinMode(BUT_T, INPUT);
  pinMode(BUT_P, INPUT);
  pinMode(VDIV, INPUT);

  setT(false);
  setP(false);

  Serial.begin(921600, SERIAL_8N1);

  setupTM1640(PDA, PCL, DNC);

  setup_ble();
}

volatile uint32_t weightUpdateMillis = 1000UL;
volatile uint32_t inTareUpdateMillis = 0UL;
volatile uint32_t batteryUpdateMillis = 0UL;

volatile uint32_t lastDataUpdate = 0UL;

void loop()
{
  if (readTM1640())
  {
    lastDataUpdate = millis();
  }

  if (isInTare())
  {
    if (inTareUpdateMillis == 0UL)
    {
      setP(true);
      inTareUpdateMillis = millis() + 250UL;
    }
    else if (inTareUpdateMillis < millis())
    {
      setP(false);
      notifyTareDone();

      inTareUpdateMillis = millis() + 1000UL;
    }
  }
  else if (inTareUpdateMillis < millis())
  {
    inTareUpdateMillis = 0UL;
  }

  if (isNotifyEnabled())
  {
    if (weightUpdateMillis < millis())
    {
      int16_t weight = currentWeight;

      if (weight < INT16_MAX)
      {
        notifyWeight(weight);
      }

      weightUpdateMillis = millis() + 100UL;
    }
  }
  else
  {
    weightUpdateMillis = 0;
  }

  if (isConnected())
  {
    if (batteryUpdateMillis < millis())
    {
      uint32_t battery = analogRead(PIN_VBAT);

      setBattery((battery - 666) / 2.34);

      batteryUpdateMillis = millis() + 1000UL;
    }
  }

  if (millis() > (lastDataUpdate + 3000))
  {
    if (isConnected())
    {
      disconnect();
    }
    else
    {
      digitalWrite(LED_CONN, LOW);
      uint32_t ulPin = g_ADigitalPinMap[PCL];
      (void)nrf_gpio_pin_read(ulPin);
      if (nrf_gpio_pin_read(ulPin))
      {
        nrf_gpio_cfg_sense_set(ulPin, NRF_GPIO_PIN_SENSE_LOW);
      }
      else
      {
        nrf_gpio_cfg_sense_set(ulPin, NRF_GPIO_PIN_SENSE_HIGH);
      }
      if (sd_power_system_off() == NRF_SUCCESS)
      {
        while (1)
          ;
      }
    }
  }
}

void parsePayload(const char *payload)
{
  int16_t parsedWeight = INT16_MAX;

  if (payload[0] == 0x00)
  {
    bool g = (payload[10] & 0x40) != 0;
    bool Oz = (payload[11] & 0x01) != 0;
    bool M1 = (payload[11] & 0x02) != 0;
    bool M2 = (payload[11] & 0x08) != 0;

    if (g)
    {
      parsedWeight = mapSegment(payload[6]) * 1000 + mapSegment(payload[7]) * 100 + mapSegment(payload[8]) * 10 + mapSegment(payload[9]);

      if (payload[6] & 0x40)
      {
        parsedWeight = -parsedWeight;
      }

      if (parsedWeight < -2000 || parsedWeight > 2000)
      {
        parsedWeight = INT16_MAX;
      }
    }

#ifdef SERIAL_DEBUG_ENABLED
    for (int i = 0; i < BUFFER_LENGTH; i++)
    {
      Serial.printf("%02X ", payload[i]);
    }
    Serial.print("\n\r\n\r");
#endif
  }

  currentWeight = parsedWeight;
}