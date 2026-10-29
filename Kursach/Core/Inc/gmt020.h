#ifndef GMT020_H
#define GMT020_H
#include "stm32f4xx_hal.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Caller owns this structure. Zero-initialize, assign pins, then call Init. */
typedef struct {
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *cs_port, *dc_port, *rst_port;
    uint16_t cs_pin, dc_pin, rst_pin;
    uint16_t width, height; /* Set by Init/SetRotation. */
    uint8_t rotation, bgr, inversion, ready;
} GMT020;
#define GMT_BLACK   0x0000u
#define GMT_WHITE   0xFFFFu
#define GMT_RED     0xF800u
#define GMT_GREEN   0x07E0u
#define GMT_BLUE    0x001Fu
#define GMT_YELLOW  0xFFE0u
#define GMT_CYAN    0x07FFu
static inline uint16_t GMT_RGB(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
}
HAL_StatusTypeDef GMT_Init(GMT020 *d);
HAL_StatusTypeDef GMT_SetRotation(GMT020 *d, uint8_t rotation);
HAL_StatusTypeDef GMT_FillRect(GMT020 *d, int16_t x, int16_t y,
                             uint16_t w, uint16_t h, uint16_t color);
HAL_StatusTypeDef GMT_Fill(GMT020 *d, uint16_t color);
HAL_StatusTypeDef GMT_Pixel(GMT020 *d, int16_t x, int16_t y, uint16_t color);
HAL_StatusTypeDef GMT_Rect(GMT020 *d, int16_t x, int16_t y,
                         uint16_t w, uint16_t h, uint16_t color);
HAL_StatusTypeDef GMT_Line(GMT020 *d, int16_t x0, int16_t y0,
                         int16_t x1, int16_t y1, uint16_t color);
/* 5x7 font in 6x8 opaque cells; scale 1..8. ASCII letters/digits,
   lowercase displayed as uppercase; unsupported characters become '?'. */
HAL_StatusTypeDef GMT_Text(GMT020 *d, int16_t x, int16_t y, const char *text,
                         uint16_t fg, uint16_t bg, uint8_t scale);

void GMT_PrintPIDValues(GMT020 *d, float v, float cur, float output, float setpoint, float error);
#ifdef __cplusplus
}
#endif
#endif
