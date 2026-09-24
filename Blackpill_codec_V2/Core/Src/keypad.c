#include "keypad.h"
#include "main.h"          // HAL + GPIO
#include "waveforms.h"     // waveform[] y el tipo
#include <stdlib.h>        // atoi

// --- pines, segun la config de CubeMX ---
typedef struct { GPIO_TypeDef *port; uint16_t pin; } pin_t;

static const pin_t rows[4] = {                 // salidas (filas)
    {GPIOA, GPIO_PIN_0}, {GPIOA, GPIO_PIN_1},
    {GPIOB, GPIO_PIN_0}, {GPIOB, GPIO_PIN_1}
};
static const pin_t cols[4] = {                 // entradas pull-up (columnas)
    {GPIOB, GPIO_PIN_8},  {GPIOB, GPIO_PIN_9},
    {GPIOB, GPIO_PIN_13}, {GPIOB, GPIO_PIN_14}
};

static const char keymap[4][4] = {             // que tecla es cada cruce
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// --- variables que el teclado modifica (viven en otros archivos) ---
extern volatile uint8_t canal_seleccionado;    // main.c
extern float    frecuencia[2];                 // usbd_cdc_if.c (uno por canal)
extern uint32_t amplitud[2];                   // usbd_cdc_if.c (uno por canal)

// --- estado del ingreso de numeros ---
static char    numbuf[8];
static uint8_t nlen = 0;
static uint8_t edit_amp = 0;    // 0 = editando frecuencia, 1 = editando amplitud

void keypad_init(void)
{
    for (int r = 0; r < 4; r++)                // arranco con todas las filas en alto
        HAL_GPIO_WritePin(rows[r].port, rows[r].pin, GPIO_PIN_SET);
}

// escanea la matriz, devuelve la tecla RECIEN apretada (o 0 si nada nuevo)
static char keypad_scan(void)
{
    static char last = 0;
    char found = 0;

    for (int r = 0; r < 4 && !found; r++) {
        HAL_GPIO_WritePin(rows[r].port, rows[r].pin, GPIO_PIN_RESET);   // activo (bajo) esta fila
        for (volatile int d = 0; d < 200; d++);                        // dejo asentar
        for (int c = 0; c < 4; c++) {
            if (HAL_GPIO_ReadPin(cols[c].port, cols[c].pin) == GPIO_PIN_RESET) {
                found = keymap[r][c];                                   // columna en 0 -> tecla apretada
                break;
            }
        }
        HAL_GPIO_WritePin(rows[r].port, rows[r].pin, GPIO_PIN_SET);     // la devuelvo a alto
    }

    char result = (found && found != last) ? found : 0;   // flanco: 1 sola vez por apretada
    last = found;
    return result;
}

// escanea + aplica la accion sobre el generador
void keypad_process(void)
{
    char k = keypad_scan();
    if (!k) return;

    if (k >= '0' && k <= '9') {                        // digito -> lo acumulo
        if (nlen < sizeof(numbuf) - 1) numbuf[nlen++] = k;
    }
    else if (k == 'A') waveform[canal_seleccionado] = WAVE_SINE;
    else if (k == 'B') waveform[canal_seleccionado] = WAVE_SQUARE;
    else if (k == 'C') waveform[canal_seleccionado] = WAVE_TRIANGLE;
    else if (k == 'D') waveform[canal_seleccionado] = WAVE_CHIRP;
    else if (k == '*') { edit_amp ^= 1; nlen = 0; }    // cambia freq <-> amp
    else if (k == '#') {                               // ENTER: aplica el numero
        numbuf[nlen] = '\0';
        uint32_t val = atoi(numbuf);
        if (edit_amp) amplitud[canal_seleccionado] = val;
        else          frecuencia[canal_seleccionado] = (float)val;
        nlen = 0;
    }
}