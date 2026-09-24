#ifndef WAVEFORMS_H
#define WAVEFORMS_H
#include <stdint.h>

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_CHIRP
} waveform_t;

extern volatile waveform_t waveform[2];   // forma de onda de cada canal

int32_t wave_next(uint8_t c);             // da la muestra del canal c y avanza su fase
void chirp_config(uint8_t c, float f0, float f1, float dur_s);  // configura el barrido

#endif