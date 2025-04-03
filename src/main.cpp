#include <Arduino.h>
#include "myble.h"
#include "TM1640.h"

#ifndef PDA
#define PDA 13 // 25  // P0.13
#endif
#ifndef DNC
#define DNC 15 // 24  // P0.15
#endif
#ifndef PCL
#define PCL 17 // 29  // P0.17
#endif
#ifndef BUT_P
#define BUT_P 3 // 19  // P0.03
#endif
#ifndef BUT_T
#define BUT_T 28 // 17  // P0.28
#endif
#ifndef VDIV
#define VDIV 29 // 18  // P0.29
#endif

char bufferTM1640[BUFFER_LENGTH] = {0};

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

  setupTM1640(PDA, PCL, DNC, bufferTM1640);

  setup_ble();
}

uint32_t weightUpdateMillis = 5000UL;
uint32_t inTareUpdateMillis = 0UL;
uint32_t batteryUpdateMillis = 0UL;

void loop()
{
  parseData();

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
      int16_t weight = currentWeight();

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

      setBattery(battery);

      batteryUpdateMillis = millis() + 1000UL;
    }
  }

  if (millis() > (lastDataUpdate() + 3000))
  {
    if (isConnected())
    {
      Bluefruit.disconnect(Bluefruit.connHandle());
    }
    else
    {
      // this code will change SPIS pin setup so it is expected for the MCU to go into a reset after the wake up
      (void)nrf_gpio_pin_read(PCL);
      if (nrf_gpio_pin_read(PCL))
      {
        nrf_gpio_cfg_sense_set(PCL, NRF_GPIO_PIN_SENSE_LOW);
      }
      else
      {
        nrf_gpio_cfg_sense_set(PCL, NRF_GPIO_PIN_SENSE_HIGH);
      }
      if (sd_power_system_off() == NRF_SUCCESS)
      {
        while (1)
          ;
      }
    }
  }
}