#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "adc.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9



int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}


// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 12 and 13 for UART0
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 12
#define UART_RX_PIN 13

// servo Defines
const uint PWM_PIN = 11;
// const uint16_t STOP_PULSE_US = 1500; // 規定値　1500us
// const uint16_t CW_PULSE_US   = 1200; // 規定値　1500-700
// const uint16_t CCW_PULSE_US  = 1800; // 規定値　1500-2300
#define PWM_FREQ 400
#define PWM_DIVIDER 125.0f
#define WRAP ((clock_get_hz(clk_sys) / PWM_DIVIDER) / PWM_FREQ)

float deg = 90.0; //test

void update_servo_from_light_deg(float deg, int slice_num, int channel) {
        float goal = light_deg()-deg;
        printf("goal=%f\n", goal);
        if (goal >= -180 && goal <= -90) {
            pwm_set_chan_level(slice_num, channel,1800);// 1800
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1800\n");
        }
        if (goal >= -89 && goal <= -45) {
            pwm_set_chan_level(slice_num, channel,1700); //1700
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1700\n");
        }
        if (goal >= -44 && goal <= -21) {
            pwm_set_chan_level(slice_num, channel,1600); //1600
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1600\n");
        }
        if (goal >= -20 && goal <= 20) {
            pwm_set_chan_level(slice_num, channel,1500);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1500\n");
        }
        if (goal >= 21 && goal <= 45) {
            pwm_set_chan_level(slice_num, channel,1400); //1400
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1400\n");
        }
        if (goal >= 46 && goal <= 90) {
            pwm_set_chan_level(slice_num, channel,1300);// 1300
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1300\n"); 
        }
        if (goal >= 91 && goal <= 180) {
            pwm_set_chan_level(slice_num, channel,1200); //1200
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            printf("1200\n");
        }
    }


int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
    }

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c


    // Interpolator example code
    interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config 
    interp_set_config(interp0, 0, &cfg);
    // For examples of interpolator use see https://github.com/raspberrypi/pico-examples/tree/master/interp

    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    while (cyw43_arch_wifi_connect_timeout_ms("SPWH_L12_5b414e", "0f15b502ac61d", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect. Retrying in 5 seconds...\n");
        sleep_ms(5000); 
    }
    
    // 接続成功時の処理
    printf("Connected.\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // initialize the PWM hardware
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    uint channel = pwm_gpio_to_channel(PWM_PIN);
    pwm_set_clkdiv(slice_num, PWM_DIVIDER); 
    pwm_set_wrap(slice_num, WRAP);
    pwm_set_enabled(slice_num, true);
    printf("wrap=%f\n", WRAP);


    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart
    while (true) {
        update_servo_from_light_deg(90, slice_num, channel);



       // pwm_set_chan_level(slice_num, channel,2300);  
       // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
       // sleep_ms(1100);
       // pwm_set_chan_level(slice_num, channel,700);  
       // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
       // sleep_ms(200);
       // pwm_set_chan_level(slice_num, channel,1600);  
       // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
       // sleep_ms(50);
       // pwm_set_chan_level(slice_num, channel,1500);  
       //  sleep_ms(1000);
//
       //  for (uint16_t pulse = 700; pulse <= 2300; pulse += 1) {
       //      uint16_t level = pulse;
       //      printf("pulse=%d level=%d\n", pulse, level);
       //      pwm_set_chan_level(slice_num, channel, level);
       //      sleep_ms(10);
       //  }

         print_ch_data();
        printf("lv=%f\n",light_deg());
        //test_pwm(slice_num, channel);
        //sleep_ms(1000);

    }
}
