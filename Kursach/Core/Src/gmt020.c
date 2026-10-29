#include "gmt020.h"
#include <stddef.h>
#include <stdlib.h>
#include "string.h"
#include "stdio.h"

#define TRY(expr) do { HAL_StatusTypeDef s_ = (expr); if (s_ != HAL_OK) return s_; } while (0)
static void select_display(GMT020 *d, GPIO_PinState state) {
    if (d->cs_port != NULL) HAL_GPIO_WritePin(d->cs_port, d->cs_pin, state);
}
static HAL_StatusTypeDef tx(GMT020 *d, const uint8_t *p, uint16_t n) {
    return HAL_SPI_Transmit(d->spi, (uint8_t *)p, n, 100);
}
/* One command + parameters; CS is released even on a transport error. */
static HAL_StatusTypeDef command(GMT020 *d, uint8_t c, const uint8_t *p, uint16_t n) {
    select_display(d, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(d->dc_port, d->dc_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef s = tx(d, &c, 1);
    if (s == HAL_OK && n != 0) {
        HAL_GPIO_WritePin(d->dc_port, d->dc_pin, GPIO_PIN_SET);
        s = tx(d, p, n);
    }
    select_display(d, GPIO_PIN_SET);
    return s;
}
static HAL_StatusTypeDef reg8(GMT020 *d, uint8_t c, uint8_t v) {
    return command(d, c, &v, 1);
}
HAL_StatusTypeDef GMT_SetRotation(GMT020 *d, uint8_t r) {
    static const uint8_t madctl[4] = {0x00, 0x60, 0xC0, 0xA0};
    if (d == NULL || !d->ready || r > 3) return HAL_ERROR;
    TRY(reg8(d, 0x36, (uint8_t)(madctl[r] | (d->bgr ? 0x08 : 0))));
    d->rotation = r;
    d->width = (r & 1) ? 320 : 240;
    d->height = (r & 1) ? 240 : 320;
    return HAL_OK;
}
HAL_StatusTypeDef GMT_Init(GMT020 *d) {
    if (d == NULL) return HAL_ERROR;
    d->ready = 0;
    if (!d->spi || !d->dc_port || !d->rst_port || !d->dc_pin || !d->rst_pin ||
        (d->cs_port && !d->cs_pin) || d->rotation > 3) return HAL_ERROR;
    if (d->spi->Init.DataSize != SPI_DATASIZE_8BIT ||
        d->spi->Init.FirstBit != SPI_FIRSTBIT_MSB ||
        d->spi->Init.CLKPolarity != SPI_POLARITY_LOW ||
        d->spi->Init.CLKPhase != SPI_PHASE_1EDGE ||
        d->spi->Init.Mode != SPI_MODE_MASTER ||
        d->spi->Init.NSS != SPI_NSS_SOFT) return HAL_ERROR;
    select_display(d, GPIO_PIN_SET);
    HAL_GPIO_WritePin(d->rst_port, d->rst_pin, GPIO_PIN_SET); HAL_Delay(10);
    HAL_GPIO_WritePin(d->rst_port, d->rst_pin, GPIO_PIN_RESET); HAL_Delay(20);
    HAL_GPIO_WritePin(d->rst_port, d->rst_pin, GPIO_PIN_SET); HAL_Delay(120);
    TRY(command(d, 0x01, NULL, 0)); HAL_Delay(150); /* Software reset */
    TRY(command(d, 0x11, NULL, 0)); HAL_Delay(120); /* Sleep out */
    TRY(reg8(d, 0x3A, 0x55)); /* RGB565: two bytes per pixel */
    /* Common ST7789V panel setup. Analog/gamma tuning is panel dependent. */
    const uint8_t porch[] = {0x0C,0x0C,0x00,0x33,0x33};
    TRY(command(d, 0xB2, porch, sizeof porch));
    TRY(reg8(d, 0xB7, 0x35)); TRY(reg8(d, 0xBB, 0x19));
    TRY(reg8(d, 0xC0, 0x2C)); TRY(reg8(d, 0xC2, 0x01));
    TRY(reg8(d, 0xC3, 0x12)); TRY(reg8(d, 0xC4, 0x20));
    TRY(reg8(d, 0xC6, 0x0F));
    const uint8_t power[] = {0xA4,0xA1};
    TRY(command(d, 0xD0, power, sizeof power));
    TRY(command(d, d->inversion ? 0x21 : 0x20, NULL, 0));
    TRY(command(d, 0x13, NULL, 0)); /* Normal mode */
    d->ready = 1;
    HAL_StatusTypeDef s = GMT_SetRotation(d, d->rotation);
    if (s == HAL_OK) s = GMT_Fill(d, GMT_BLACK);
    if (s == HAL_OK) s = command(d, 0x29, NULL, 0); /* Display on */
    if (s != HAL_OK) d->ready = 0;
    HAL_Delay(20);
    return s;
}
static HAL_StatusTypeDef window(GMT020 *d, uint16_t x, uint16_t y, uint16_t x1, uint16_t y1) {
    uint8_t p[4] = {(uint8_t)(x>>8),(uint8_t)x,(uint8_t)(x1>>8),(uint8_t)x1};
    TRY(command(d, 0x2A, p, 4)); /* Column address */
    p[0]=(uint8_t)(y>>8); p[1]=(uint8_t)y; p[2]=(uint8_t)(y1>>8); p[3]=(uint8_t)y1;
    TRY(command(d, 0x2B, p, 4)); /* Row address */
    return command(d, 0x2C, NULL, 0); /* RAM write */
}
HAL_StatusTypeDef GMT_FillRect(GMT020 *d, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (d == NULL || !d->ready) return HAL_ERROR;
    int32_t left=x, top=y, right=(int32_t)x+w, bottom=(int32_t)y+h;
    if (left<0) left=0;
    if (top<0) top=0;
    if (right>d->width) right=d->width;
    if (bottom>d->height) bottom=d->height;
    if (right<=left || bottom<=top) return HAL_OK;
    TRY(window(d,(uint16_t)left,(uint16_t)top,(uint16_t)(right-1),(uint16_t)(bottom-1)));
    uint8_t block[256]; /* Only 256 bytes, no framebuffer. */
    for (unsigned i=0;i<sizeof block;i+=2) {
        block[i]=(uint8_t)(color>>8); block[i+1]=(uint8_t)color;
    }
    uint32_t remaining=(uint32_t)(right-left)*(uint32_t)(bottom-top);
    select_display(d, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(d->dc_port,d->dc_pin,GPIO_PIN_SET);
    HAL_StatusTypeDef s=HAL_OK;
    while (remaining && s==HAL_OK) {
        uint16_t count=(remaining>128) ? 128 : (uint16_t)remaining;
        s=tx(d,block,(uint16_t)(count*2)); remaining-=count;
    }
    select_display(d,GPIO_PIN_SET);
    return s;
}
HAL_StatusTypeDef GMT_Fill(GMT020 *d,uint16_t c) {
    if (!d) return HAL_ERROR;
    return GMT_FillRect(d,0,0,d->width,d->height,c);
}
HAL_StatusTypeDef GMT_Pixel(GMT020 *d,int16_t x,int16_t y,uint16_t c) {
    return GMT_FillRect(d,x,y,1,1,c);
}
HAL_StatusTypeDef GMT_Rect(GMT020 *d,int16_t x,int16_t y,uint16_t w,uint16_t h,uint16_t c) {
    if (!d || !d->ready) return HAL_ERROR;
    if (!w || !h) return HAL_OK;
    /* Keep intermediate coordinates representable by the public int16_t API. */
    if ((int32_t)x+w-1>INT16_MAX || (int32_t)y+h-1>INT16_MAX) return HAL_ERROR;
    TRY(GMT_FillRect(d,x,y,w,1,c));
    TRY(GMT_FillRect(d,x,(int16_t)(y+h-1),w,1,c));
    TRY(GMT_FillRect(d,x,y,1,h,c));
    return GMT_FillRect(d,(int16_t)(x+w-1),y,1,h,c);
}
HAL_StatusTypeDef GMT_Line(GMT020 *d,int16_t x0,int16_t y0,int16_t x1,int16_t y1,uint16_t c) {
    if (!d || !d->ready) return HAL_ERROR;
    int32_t x=x0,y=y0,dx=labs((long)x1-x0),dy=-labs((long)y1-y0);
    int32_t sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx+dy;
    for (;;) {
        TRY(GMT_Pixel(d,(int16_t)x,(int16_t)y,c));
        if (x==x1 && y==y1) break;
        int32_t e2=2*err;
        if (e2>=dy) {err+=dy; x+=sx;}
        if (e2<=dx) {err+=dx; y+=sy;}
    }
    return HAL_OK;
}
/* Original compact row font. Each byte holds five pixels, MSB at left. */
static const char alphabet[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ .,:-+/%=?";
static const uint8_t glyphs[][7]={
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14},
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{15,16,16,19,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{0,0,0,0,0,0,0},{0,0,0,0,0,6,6},{0,0,0,0,6,4,8},
{0,6,6,0,6,6,0},{0,0,0,31,0,0,0},{0,4,4,31,4,4,0},
{1,2,2,4,8,8,16},{24,25,2,4,8,19,3},{0,0,31,0,31,0,0},
{14,17,1,2,4,0,4}};

HAL_StatusTypeDef GMT_Text(GMT020 *d,int16_t x,int16_t y,const char *t,uint16_t fg,uint16_t bg,uint8_t scale) {
    if (!d || !d->ready || !t || !scale || scale>8) return HAL_ERROR;
    int32_t cx=x,cy=y;
    for (;*t;++t) {
        if (*t=='\r') continue;
        if (*t=='\n') {cx=x;cy+=8*scale;continue;}
        if (cx+6*scale>d->width) {cx=x;cy+=8*scale;}
        if (cy>=d->height) break;
        char c=*t;
        if (c>='a' && c<='z') c=(char)(c-'a'+'A');
        unsigned k=0;
        while (alphabet[k] && alphabet[k]!=c) ++k;
        if (!alphabet[k]) k=sizeof glyphs/sizeof glyphs[0]-1;
        for (unsigned row=0;row<8;++row) {
            for (unsigned col=0;col<6;++col) {
                uint16_t color=(row<7 && col<5 && (glyphs[k][row] & (16u>>col)))?fg:bg;
                int32_t px=cx+(int32_t)col*scale,py=cy+(int32_t)row*scale;
                if (px<d->width && py<d->height && px>=INT16_MIN && py>=INT16_MIN)
                    TRY(GMT_FillRect(d,(int16_t)px,(int16_t)py,scale,scale,color));
            }
        }
        cx+=6*scale;
    }
    return HAL_OK;
}

void GMT_PrintPIDValues(GMT020 *d, float v, float cur, float output, float setpoint, float error)
{
  char val_buf[32] = {0};

  GMT_Text(d, 10, 20, "Output voltage: ", GMT_YELLOW, GMT_BLACK, 2);
  sprintf(val_buf, "%.4f", v);
  GMT_Text(d, 10+strlen("Output voltage: ")*12, 20, val_buf, GMT_WHITE, GMT_BLACK,2);
  memset(val_buf, 0, sizeof(val_buf));

  GMT_Text(d, 10, 40, "Output current: ", GMT_YELLOW, GMT_BLACK, 2);
  sprintf(val_buf, "%.4f", cur);
  GMT_Text(d, 10+strlen("Output current: ")*12, 40, val_buf, GMT_WHITE, GMT_BLACK,2);
  memset(val_buf, 0, sizeof(val_buf));

  GMT_Text(d, 10, 60, "Duty cycle: ", GMT_YELLOW, GMT_BLACK, 2);
  sprintf(val_buf, "%.4f", output);
  GMT_Text(d, 10+strlen("Duty cycle: ")*12, 60, val_buf, GMT_WHITE, GMT_BLACK,2);
  memset(val_buf, 0, sizeof(val_buf));

  GMT_Text(d, 10, 80, "Setpoint: ", GMT_YELLOW, GMT_BLACK, 2);
  sprintf(val_buf, "%.4f", setpoint);
  GMT_Text(d, 10+strlen("Setpoint: ")*12, 80, val_buf, GMT_WHITE, GMT_BLACK,2);
  memset(val_buf, 0, sizeof(val_buf));

  GMT_Text(d, 10, 100, "Error: ", GMT_YELLOW, GMT_BLACK, 2);
  sprintf(val_buf, "%.4f", error);
  GMT_Text(d, 10+strlen("Error: ")*12, 100, val_buf, GMT_WHITE, GMT_BLACK,2);
  memset(val_buf, 0, sizeof(val_buf));
}
