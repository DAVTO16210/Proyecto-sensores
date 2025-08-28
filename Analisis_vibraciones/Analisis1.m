% --- Configuración serial (ESP32) ---
puerto = "COM14";     % Ajustar según corresponda
baud   = 115200;      % Baudrate de la ESP32 (verifica con Serial.begin)
arduino = serialport(puerto, baud);

% Configuración terminador y timeout
configureTerminator(arduino, "LF");  % ESP32 con Serial.println usa LF (\n)
arduino.Timeout = 5;                 % espera de hasta 5s en cada readline
pause(2);                            % dar tiempo a que arranque la ESP32

% Limpiar buffer inicial
while arduino.NumBytesAvailable > 0
    discard = readline(arduino);
end

% --- Parámetros de análisis ---
ventana_silencio = 100; % ms sin '1' para declarar fin de vibración
min_duracion_ms  = 20;  % opcional: descartar vibraciones muy cortas

% --- Crear archivo con timestamp ---
timestamp = datestr(now,'yyyy-mm-dd_HH-MM-SS'); 
filename  = sprintf("episodios_vibracion_%s.txt", timestamp);
fileID    = fopen(filename, "w");

% --- Variables de estado ---
t0 = tic;              % reloj
en_vibracion = false;  % estado: estamos dentro de vibración?
t_inicio = 0;          % inicio del episodio actual
duraciones = [];       % vector con duraciones
t_ultimo1 = 0;         % tiempo del último '1'

% --- Gráfica en tiempo real ---
figure;
h = animatedline('Color','b','LineWidth',1.5);
xlabel("Tiempo (s)");
ylabel("Detección de vibración (0/1)");
ylim([-0.2 1.2]);
grid on;

disp("Iniciando lectura... Presiona Ctrl+C para detener.");

try
    while true
        if arduino.NumBytesAvailable > 0
            raw   = readline(arduino);      % leer línea
            valor = str2double(strtrim(raw));
            t     = toc(t0) * 1000;         % tiempo en ms

            % --- Gráfico en tiempo real ---
            addpoints(h, t/1000, valor);    % eje X en segundos
            drawnow limitrate;

            % --- Algoritmo de detección de episodios ---
            if valor == 1
                if ~en_vibracion
                    en_vibracion = true;
                    t_inicio = t;
                end
                t_ultimo1 = t; % actualizar último pulso
            else
                if en_vibracion && (t - t_ultimo1 > ventana_silencio)
                    en_vibracion = false;
                    duracion = t_ultimo1 - t_inicio;
                    if duracion >= min_duracion_ms
                        duraciones(end+1) = duracion; %#ok<SAGROW>
                        fprintf(fileID, "%.2f ms\n", duracion);
                        fprintf("Duración vibración: %.2f ms\n", duracion);
                    end
                end
            end
        else
            pause(0.005); % evitar usar 100% CPU cuando no hay datos
        end
    end
catch ME
    % --- Cierre seguro ---
    fclose(fileID);
    clear arduino;
    disp("Lectura terminada.");
    disp(ME.message);

    % --- Estadística final ---
    if ~isempty(duraciones)
        fprintf("\nResumen estadístico:\n");
        fprintf("Promedio: %.2f ms\n", mean(duraciones));
        fprintf("Máxima:   %.2f ms\n", max(duraciones));
        fprintf("Mínima:   %.2f ms\n", min(duraciones));

        figure;
        histogram(duraciones, 'FaceColor','b');
        xlabel("Duración (ms)");
        ylabel("Frecuencia");
        title("Distribución de duraciones de vibración");
    else
        disp("No se detectaron episodios.");
    end
end
