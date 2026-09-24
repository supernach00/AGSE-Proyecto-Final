#include "waveforms.h"
#include "sine_table.h"

// estas viven en main.c: las "tomamos prestadas"
extern uint32_t acc[2];            // fase de cada canal
extern volatile uint32_t ftw[2];   // FTW (frecuencia) de cada canal

#define FS  48828.125              // OJO: igual que el FS del main.c

volatile waveform_t waveform[2] = { WAVE_SINE, WAVE_SINE };  // arranca en seno

// --- estado propio del chirp, uno por canal ---
static uint32_t chirp_ftw[2];      // FTW instantanea (la que va subiendo)
static uint32_t chirp_start[2];    // FTW inicial (f0)
static uint32_t chirp_end[2];      // FTW final   (f1)
static uint32_t chirp_step[2];     // cuanto sube la FTW por muestra

void chirp_config(uint8_t c, float f0, float f1, float dur_s)
{
    chirp_start[c] = (uint32_t)(f0 * 4294967296.0 / FS);   // FTW de f0
    chirp_end[c]   = (uint32_t)(f1 * 4294967296.0 / FS);   // FTW de f1
    uint32_t nsamp = (uint32_t)(dur_s * FS);               // muestras que dura el barrido
    if (nsamp == 0) nsamp = 1;
    chirp_step[c]  = (chirp_end[c] - chirp_start[c]) / nsamp;
    chirp_ftw[c]   = chirp_start[c];                        // empieza en f0
}

int32_t wave_next(uint8_t c)
{
    int32_t v;

    switch (waveform[c])
    {
    case WAVE_SINE:                              // ---- SENO ----
        acc[c] += ftw[c];                        // avanzo la fase con la FTW, y busco el valor en la tabla.
        v = sine_table[acc[c] >> 19];
        break;

    case WAVE_SQUARE:                                       // ---- CUADRADA
        acc[c] += ftw[c];                                   // avanzo la fase, y pregunto en qué mitad estoy. 0x80000000 es 
        v = (acc[c] < 0x80000000u) ? 8388607 : -8388608;    // 2^31 = la mitad del acumulador. Primera mitad → máximo (+); 
        break;                                              // segunda mitad → mínimo (−).

    case WAVE_TRIANGLE:                                             // ---- TRIANGULAR ----
        acc[c] += ftw[c];                                           // avanzo la fase y la uso como rampa, Primera mitad (acc < 2^31): subo de −máx hacia +máx.
        if (acc[c] < 0x80000000u)                                   // Segunda mitad: bajo de +máx hacia −máx.
            v = -8388608 + (int32_t)(acc[c] >> 7);                  // El >> 7: la media fase tiene 2^31 valores, pero la amplitud que quiero tiene 2^24 (±2^23).
        else                                                        // Como 2^31 / 2^7 = 2^24, corro 7 bits para "encoger" la rampa al rango de amplitud.
            v = 8388608 - (int32_t)((acc[c] - 0x80000000u) >> 7);   // El clamp evita que en el pico justo se pase 1 unidad
        if (v > 8388607) v = 8388607;
        break;

     case WAVE_CHIRP:                             // ---- CHIRP ----
        chirp_ftw[c] += chirp_step[c];           // la frecuencia sube un poquito
        if (chirp_ftw[c] >= chirp_end[c])
            chirp_ftw[c] = chirp_start[c];       // llego a f1 -> vuelve a f0
        acc[c] += chirp_ftw[c];                  // avanzo con la FTW del momento
        v = sine_table[acc[c] >> 19];            // es un seno que barre
        break;

     default:
        v = 0;
    }
    return v;
}