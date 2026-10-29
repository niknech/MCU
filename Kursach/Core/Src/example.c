/* Integration snippets, not a replacement for CubeMX main.c.
 * Copy functions into app.c, call Display_Init after MX_SPI1_Init.
 * Configure the pins exactly as described in README_RU.md. */
#include "main.h"
#include "spi.h"
#include "gmt020.h"
#include <stdio.h>
static GMT020 display = {
    .spi = &hspi1,
    .cs_port = GPIOB, .cs_pin = GPIO_PIN_6,
    .dc_port = GPIOC, .dc_pin = GPIO_PIN_7,
    .rst_port = GPIOA, .rst_pin = GPIO_PIN_9,
    .rotation = 0, .bgr = 0, .inversion = 1
};
HAL_StatusTypeDef Display_Init(void) {
    HAL_StatusTypeDef s = GMT_Init(&display);
    if (s != HAL_OK) return s;
    return GMT_Text(&display, 10, 10, "POWER SUPPLY", GMT_CYAN, GMT_BLACK, 2);
}
/* Call from main loop with a stable local snapshot, at most 5-10 Hz.
 * Integer formatting avoids the linker option for printf float support.
 * Accepts 0..99999 mV. */
HAL_StatusTypeDef Display_Voltage(uint32_t millivolts) {
    char line[24];
    if (millivolts > 99999u) return HAL_ERROR;
    (void)snprintf(line, sizeof line, "U: %2lu.%03lu V ",
                   (unsigned long)(millivolts / 1000u),
                   (unsigned long)(millivolts % 1000u));
    return GMT_Text(&display, 10, 50, line, GMT_WHITE, GMT_BLACK, 2);
}
