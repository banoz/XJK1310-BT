#pragma once

// define SERIAL_DEBUG_ENABLED

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

#define SCALES_NAME "Tencent Scales"
#define UUID16_SVC_SCALES 0xFFF0
#define UUID16_CHR_SCALES_READ 0x36F5
#define UUID16_CHR_SCALES_WRITE 0xFFF4

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

#define BUFFER_LENGTH 16