%% Tabla senoidal DDS de 24 bits - version 2 (codec PCM3060)
N    = 8192;              % puntos de la tabla (2^13). NO cambia el motor DDS (>>19)
bits = 24;                % resolucion vertical (eje Y)
A    = 2^(bits-1) - 1;    % pico maximo con signo = 8388607

n = 0:N-1;                              % indices 0..8191
y = round(A * sin(2*pi*n / N));        % seno de fondo de escala, CON SIGNO

% --- chequeos en consola ---
fprintf('N    = %d puntos\n', N);
fprintf('pico = +-%d  (medido: min=%d  max=%d)\n', A, min(y), max(y));

% --- opcional: verlo ---
plot(n, y); grid on; title('Tabla senoidal 24 bits'); xlabel('indice'); ylabel('valor');

% --- exportar a sine_table.h (int32_t) ---
fid = fopen('sine_table.h', 'w');
fprintf(fid, '// Tabla senoidal DDS: %d puntos, %d bits con signo (+-%d)\n', N, bits, A);
fprintf(fid, '// Generada en MATLAB para el codec PCM3060 (24 bits)\n');
fprintf(fid, '#ifndef SINE_TABLE_H\n#define SINE_TABLE_H\n#include <stdint.h>\n\n');
fprintf(fid, '#define SINE_N %d\n\n', N);
fprintf(fid, 'static const int32_t sine_table[%d] = {\n', N);
for k = 1:N
    fprintf(fid, '%d, ', y(k));
    if mod(k, 8) == 0, fprintf(fid, '\n'); end
end
fprintf(fid, '};\n\n#endif\n');
fclose(fid);
disp('OK -> sine_table.h generado en la carpeta actual.');