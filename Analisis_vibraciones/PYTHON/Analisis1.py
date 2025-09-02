import serial
import time
import datetime
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# --- Configuración serial (ESP32) ---
puerto = "COM14"    # Ajustar según corresponda
baud   = 115200
arduino = serial.Serial(port=puerto, baudrate=baud, timeout=5)

time.sleep(2)  # dar tiempo a que arranque la ESP32

# Limpiar buffer inicial
arduino.reset_input_buffer()

# --- Parámetros de análisis ---
ventana_silencio = 100   # ms sin '1' para declarar fin de vibración
min_duracion_ms  = 20    # descartar vibraciones muy cortas

# --- Crear archivo con timestamp ---
timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
filename  = f"episodios_vibracion_{timestamp}.txt"
fileID    = open(filename, "w")

# --- Variables de estado ---
t0 = time.time()
en_vibracion = False
t_inicio = 0
duraciones = []
t_ultimo1 = 0

# --- Gráfica en tiempo real ---
plt.ion()
fig, ax = plt.subplots()
ax.set_xlabel("Tiempo (s)")
ax.set_ylabel("Detección de vibración (0/1)")
ax.set_ylim([-0.2, 1.2])
ax.grid(True)
linea, = ax.plot([], [], 'b-', linewidth=1.5)

tiempos = []
valores = []

print("Iniciando lectura... Presiona Ctrl+C para detener.")

try:
    while True:
        if arduino.in_waiting > 0:
            raw = arduino.readline().decode('utf-8').strip()
            try:
                valor = float(raw)
            except ValueError:
                continue  # ignorar si no es número válido

            t = (time.time() - t0) * 1000  # tiempo en ms

            # --- Gráfico en tiempo real ---
            tiempos.append(t/1000)  # eje X en segundos
            valores.append(valor)
            linea.set_data(tiempos, valores)
            ax.set_xlim(0, max(10, tiempos[-1]))
            plt.pause(0.001)

            # --- Algoritmo de detección de episodios ---
            if valor == 1:
                if not en_vibracion:
                    en_vibracion = True
                    t_inicio = t
                t_ultimo1 = t
            else:
                if en_vibracion and (t - t_ultimo1 > ventana_silencio):
                    en_vibracion = False
                    duracion = t_ultimo1 - t_inicio
                    if duracion >= min_duracion_ms:
                        duraciones.append(duracion)
                        fileID.write(f"{duracion:.2f} ms\n")
                        print(f"Duración vibración: {duracion:.2f} ms")

        else:
            time.sleep(0.005)  # evitar 100% CPU

except KeyboardInterrupt:
    print("\nLectura terminada.")
    fileID.close()
    arduino.close()

    # --- Estadística final ---
    if len(duraciones) > 0:
        print("\nResumen estadístico:")
        print(f"Promedio: {sum(duraciones)/len(duraciones):.2f} ms")
        print(f"Máxima:   {max(duraciones):.2f} ms")
        print(f"Mínima:   {min(duraciones):.2f} ms")

        plt.ioff()
        plt.figure()
        plt.hist(duraciones, bins=10, color='b', edgecolor='black')
        plt.xlabel("Duración (ms)")
        plt.ylabel("Frecuencia")
        plt.title("Distribución de duraciones de vibración")
        plt.show()
    else:
        print("No se detectaron episodios.")
