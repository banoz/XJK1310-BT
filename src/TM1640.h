#pragma once

#include <stdint.h>

#define READPORT(pin, state) ((state >> pin) && 1UL)

void setupTM1640(uint32_t dio, uint32_t sclk, uint32_t both);

bool readTM1640(void);

uint8_t mapSegment(uint8_t);

extern void parsePayload(const char *);