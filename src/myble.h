#pragma once

#include <bluefruit.h>

#ifdef SERIAL_DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTDEC(x) Serial.print(x, DEC)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTLNF(x, y) Serial.println(x, y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTLNF(x, y)
#endif

bool setup_ble(void);

bool isConnected(void);
bool isNotifyEnabled(void);

bool isInTare(void);
bool isInTimer(void);

void setBattery(uint32_t);

void notifyTareDone(void);
void notifyWeight(int16_t weight);