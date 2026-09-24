#include "oled.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "sine_table.h"     // reusamos la tabla para dibujar el seno/chirp
#include <stdio.h>          // sprintf

// --- caja donde va el iconito de la onda (arriba a la derecha) ---
#define ICON_X    80
#define ICON_Y    0
#define ICON_W    46
#define ICON_H    28
#define ICON_MID  (ICON_Y + ICON_H/2)
#define ICON_AMP  (ICON_H/2 - 2)

// ====================== ICONOS DE CADA ONDA ======================

static void icon_sine(void)              // seno: 2 ciclos, usando la tabla
{
    int px = ICON_X, py = ICON_MID;
    for (int i = 1; i <= ICON_W; i++) {
        uint32_t k = ((uint32_t)i * 2 * 8192 / ICON_W) & 8191;    // 2 ciclos
        int y = ICON_MID - (int)((int32_t)sine_table[k] * ICON_AMP / 8388608);
        int x = ICON_X + i;
        ssd1306_Line(px, py, x, y, White);
        px = x; py = y;
    }
}

static void icon_square(void)            // cuadrada: puras lineas
{
    int top = ICON_Y + 2, bot = ICON_Y + ICON_H - 2, q = ICON_W/4, x = ICON_X;
    ssd1306_Line(x,     top, x+q,   top, White);
    ssd1306_Line(x+q,   top, x+q,   bot, White);
    ssd1306_Line(x+q,   bot, x+2*q, bot, White);
    ssd1306_Line(x+2*q, bot, x+2*q, top, White);
    ssd1306_Line(x+2*q, top, x+3*q, top, White);
    ssd1306_Line(x+3*q, top, x+3*q, bot, White);
    ssd1306_Line(x+3*q, bot, x+4*q, bot, White);
}

static void icon_triangle(void)          // triangular: zigzag
{
    int top = ICON_Y + 2, bot = ICON_Y + ICON_H - 2, q = ICON_W/4, x = ICON_X;
    ssd1306_Line(x,     bot, x+q,   top, White);
    ssd1306_Line(x+q,   top, x+2*q, bot, White);
    ssd1306_Line(x+2*q, bot, x+3*q, top, White);
    ssd1306_Line(x+3*q, top, x+4*q, bot, White);
}

static void icon_chirp(void)             // chirp: seno que se comprime
{
    int px = ICON_X, py = ICON_MID;
    uint32_t idx = 0;
    for (int i = 1; i <= ICON_W; i++) {
        uint32_t step = 40 + (uint32_t)i * 12;      // el paso crece -> mas frecuencia
        idx += step;
        uint32_t k = (idx >> 4) & 8191;
        int y = ICON_MID - (int)((int32_t)sine_table[k] * ICON_AMP / 8388608);
        int x = ICON_X + i;
        ssd1306_Line(px, py, x, y, White);
        px = x; py = y;
    }
}

static void draw_icon(waveform_t w)
{
    switch (w) {
        case WAVE_SINE:     icon_sine();     break;
        case WAVE_SQUARE:   icon_square();   break;
        case WAVE_TRIANGLE: icon_triangle(); break;
        case WAVE_CHIRP:    icon_chirp();    break;
        default: break;
    }
}

static const char* wave_name(waveform_t w)
{
    switch (w) {
        case WAVE_SINE:     return "Seno";
        case WAVE_SQUARE:   return "Cuadrada";
        case WAVE_TRIANGLE: return "Triang.";
        case WAVE_CHIRP:    return "Chirp";
        default:            return "?";
    }
}

// ====================== API PUBLICA ======================

void oled_init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void oled_show(uint8_t canal, waveform_t w, uint32_t freq_hz, uint8_t amp_pct)
{
    char buf[20];
    ssd1306_Fill(Black);                    // borro todo

    ssd1306_SetCursor(0, 0);                // linea 1: canal
    sprintf(buf, "Canal %u", canal);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_SetCursor(0, 14);               // linea 2: nombre de la onda
    ssd1306_WriteString((char*)wave_name(w), Font_7x10, White);

    draw_icon(w);                           // iconito arriba a la derecha

    ssd1306_SetCursor(0, 34);               // linea 3: frecuencia
    sprintf(buf, "F: %lu Hz", (unsigned long)freq_hz);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_SetCursor(0, 14);               // linea 2: nombre de la onda
    ssd1306_WriteString((char*)wave_name(w), Font_7x10, White);

    draw_icon(w);                           // iconito arriba a la derecha

    ssd1306_SetCursor(0, 34);               // linea 3: frecuencia
    sprintf(buf, "F: %lu Hz", (unsigned long)freq_hz);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_SetCursor(0, 48);               // linea 4: amplitud
    sprintf(buf, "A: %u %%", amp_pct);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_UpdateScreen();                 // <-- vuelca todo al OLED
}