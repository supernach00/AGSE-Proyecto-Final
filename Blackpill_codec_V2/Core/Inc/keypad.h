#ifndef KEYPAD_H
#define KEYPAD_H

void keypad_init(void);      // deja las filas en alto (llamar una vez)
void keypad_process(void);   // escanea y actua (llamar seguido en el loop)

#endif
