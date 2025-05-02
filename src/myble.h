#pragma once

bool setup_ble(void);

bool isConnected(void);
bool isNotifyEnabled(void);

bool isInTare(void);
bool isInTimer(void);

void setBattery(uint8_t);
void disconnect(void);

void notifyTareDone(void);
void notifyWeight(int16_t weight);