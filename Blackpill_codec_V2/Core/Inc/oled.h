#ifndef OLED_H
#define OLED_H
#include <stdint.h>
#include "waveforms.h"     // para el tipo waveform_t

// arranca la pantalla (llamar una vez en el setup)
void oled_init(void);

// dibuja el estado actual: canal, forma de onda, frecuencia (Hz) y amplitud (%)
void oled_show(uint8_t canal, waveform_t w, uint32_t freq_hz, uint8_t amp_pct);

#endif