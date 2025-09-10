#ifndef ADC_MCP3008_H
#define ADC_MCP3008_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <math.h>

// SPI Defines
#define SPI_PORT spi0
#define PIN_CS   17

// add features
#define MCP3008_START_BIT 0b00000001
#define MCP3008_MODE_SINGLE 0b10000000
#define _DEBUG_MCP3008 false

#define PI 3.14
extern const float unitVectorX[4];
extern const float unitVectorY[4];

extern float Vref;

#ifdef __cplusplus
extern "C" {
#endif

// 関数プロトタイプ
static inline void cs_select();
static inline void cs_deselect();
int readADC(uint8_t ch);
float light_deg();
void print_ch_data();

#ifdef __cplusplus
}
#endif

#endif