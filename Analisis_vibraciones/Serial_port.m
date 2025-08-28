% --- DEBUG + Lectura serial robusta para ESP32 ---
% Ajusta puerto si es necesario
puerto = "COM14";
baud   = 115200;

% Mostrar puertos disponibles
disp("Puertos disponibles:");
disp(serialportlist("available"));

% Intentar abrir puerto con manejo de errores
try
    arduino = serialport(puerto, baud);
catch ME
    error("No se pudo abrir el puerto %s a %d baudios.\nError: %s\nAsegúrate de que el puerto existe y que ningún otro programa lo está usando.", puerto, baud, ME.message);
end

% Configuraciones útiles
configureTerminator(arduino, "LF");  % ESP32 con Serial.println usa LF (\n). Si ves CRLF usa "CR/LF"
arduino.Timeout = 5;                 % tiempo de espera (segundos) para readline
pause(2);                            % esperar a que la ESP32 reinicie/arranque si aplica

% Vaciar cualquier dato viejo en buffer (si hay)
while arduino.NumBytesAvailable > 0
    discard = readline(arduino);
end

% Preparar gráfico
figure;
h = animatedline('Color','b','LineWidth',1.5);
xlabel("Tiempo (s)");
ylabel("Detección de vibración (0/1)");
ylim([-0.2 1.2]);
grid on;

% Tiempo inicial
t0 = tic;

% Bucle de lectura con protección try/catch para limpiar puerto al salir
disp("Iniciando lectura. Presiona Ctrl+C para detener.");
try
    while true
        % Solo intentar leer si hay datos disponibles para evitar bloqueos
        if arduino.NumBytesAvailable > 0
            raw = readline(arduino);        % devuelve string/char sin el terminador
            raw_trim = strtrim(raw);        % quitar espacios/CR
            valor = str2double(raw_trim);   % convertir a número

            t = toc(t0);                    % segundos

            if isnan(valor)
                % Mensaje de depuración si no es numérico
                fprintf("Recibido no numérico: '%s' (bytes: %d)\n", raw_trim, arduino.NumBytesAvailable);
            else
                % actualizar gráfico
                addpoints(h, t, valor);
                drawnow limitrate; % limitrate para evitar sobrecarga gráfica

                % imprimimos en consola (opcional)
                fprintf("%.3f\t%d\n", t, round(valor));
            end
        else
            pause(0.005); % reducir CPU cuando no hay datos
        end
    end
catch ME
    % Limpieza al terminar o al recibir Ctrl+C
    disp("Terminando lectura o ocurrió un error:");
    disp(ME.message);
    % cerrar y liberar objeto serial
    try
        clear arduino;
    catch
    end
end
