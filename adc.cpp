#include <stdio.h>
#include "adc.h"

// グローバル変数の定義
const float unitVectorY[4] = {0.707, -0.707, -0.707, 0.707};
const float unitVectorX[4] = {-0.707, -0.707, 0.707, 0.707};
float Vref = 3.2562;

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

int readADC(uint8_t ch) {
    uint8_t writeData[3] = {};
    uint8_t buffer[3] = {};
    writeData[0] = MCP3008_START_BIT;
    writeData[1] = MCP3008_MODE_SINGLE | (ch << 4);
#if _DEBUG_MCP3008
    printf("\n %08b %08b %08b\n", writeData[0], writeData[1], writeData[2]);
#endif
    cs_select();
    sleep_ms(1);
    spi_write_read_blocking(SPI_PORT, writeData, buffer, 3);
    sleep_ms(1);
    cs_deselect();
    return (buffer[1] & 0b00000011) << 8 | buffer[2];
}

float light_deg() {
    uint8_t raw_data[4] = {};
    float V_x = 0;
    float V_y = 0;
    float deg = 0;
    for(int i = 0; i < 4; i++) {
        raw_data[i] = readADC(i);
    }
    
    for(int i = 0; i < 4; i++) {
        V_x += raw_data[i] * unitVectorX[i];
        V_y += raw_data[i] * unitVectorY[i];
    }
    
    // deg = atan2(V_y, V_x) / PI * 180.0;
    deg = 55.5;
    printf("deg=%f\n", deg);
    
    return deg;
}

void print_ch_data() {
    for (uint8_t i = 0; i < 8; i++) {
        printf("%.4f", Vref * readADC(i) / 1024);
        if (i < 7) {
            printf(",");
        }
    }
    printf(" \n");
}