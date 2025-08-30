# Este código es un ejemplo del uso del lector serial de python para obtener datos de un ESP32
# y guardarlos en un archivo de texto con un timestamp en el nombre del archivo.
import serial
import time
import os
from datetime import datetime

# Cambia COMXX por el puerto de tu ESP32 (por ejemplo, COM5 o /dev/ttyUSB0)
puerto = "COM14"
baudrate = 115200

ahora = datetime.now() #Obtencion de la hora para generación de archivos distintos cada vez
timestamp = ahora.strftime("%Y-%m-%d_%H-%M-%S")  # Formato: 2025-06-20_23-15-00

# Crea el nombre del archivo
nombre_archivo = f"vibracion_{timestamp}.txt"
ruta = "C:/Users/darki/Documents/DocumentosUNAM/Servicio Social/spProyectoSensores/" #Ruta de la carpeta
ruta_completa = os.path.join(ruta, nombre_archivo)

archivo = open(ruta_completa, "w") #Abrir en modo escritura "w"
#archivo.write("Nueva lectura\n\n")
print("Archivo creado en:", ruta)

arduino = serial.Serial(puerto, baudrate, timeout=1)
while True: 
    try:
        linea = arduino.readline().decode("utf-8").strip() #Obtención linea por linea del monitor serial
        if linea:
            print(linea)
            archivo.write(linea)
            archivo.write("\n")
    except KeyboardInterrupt:
            archivo.close() #Para interrumpir con ctrl + c
            break
    except Exception as e: #Interrupción por desconexión
        print("Error:", e)
        archivo.close()
        print("Datos guardados en archivo: " + nombre_archivo)
        break
