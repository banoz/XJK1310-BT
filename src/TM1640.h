#pragma once

#include <stdint.h>

#define BUFFER_LENGTH 16

#define READPORT(pin, state) ((state >> pin) && 1UL)

void setupTM1640(uint32_t dio, uint32_t sclk, uint32_t both, char *data);

bool isBufferedTM1640(void);

void resetBufferTM1640(void);

void portEventHandle(uint32_t *latch);

void parseData(void);

int16_t currentWeight(void);

uint32_t lastDataUpdate(void);