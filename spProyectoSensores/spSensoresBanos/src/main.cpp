#include <Arduino.h> //Libreria para programar en placas arduino y ESP32
#include <WiFi.h>    //Permite conectar el ESP32 a las redes WIFI
#include <time.h>    // Permite ajustar la fecha y hora del sistema
#include <HTTPClient.h> //Permite consultar HTTP a servidores web en el ESP32
#include <string.h>        //Proporciona funciones para manipulacion de caracteres en C
#include <iostream>        // Permite entrada y salida de datos en C++
#include <list>            //Permite manejar listas
#include <map>            //Permite manejas mapas

// Multitaskiing
TaskHandle_t Task0;

// Configuracion del servidor NTP para obtener la hora
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -6 * 3600; // Ajusta según tu zona horaria
const int daylightOffset_sec = 0;

// WiFi
/* const char *ssid = "Opticom";
const char *passwd = "%L0$.ErR0re$.$oN.d3.Lui$%"; */

//! CAMBIAR POR LA RED Y CONTRASEÑA
/* const char *ssid = "Opticom_EXT2G";
const char *passwd = "%L0$.ErR0re$.$oN.d3.Lui$%"; */
const char *ssid = ".:PC Puma FI:.";
const char *passwd = "";
const char *serverName = "https://script.google.com/macros/s/AKfycbxqPtzQlKgmTssQis5jEaXHgqINdUAlmMHyxOrEvKtQyAgTRCFThnJOXAfWq2djfV0/exec";

//! UNICAMENTE CAMBIAR POR EL SERVIDOR QUE SE PONGA NUEVO
const char *serverAndre = "http://10.66.66.178:8080/guardar_datos.php";

// Tiempo de espera para la bandera de sensores
int flag_time = 200000; // El tiempo varía dependiendo de cuántos sensores se tengan, si son muchos, disminuir el tiempo

// Structura del sensor
struct Sensor
{
    String ID;        //identificador del sensor
    String type;        //tipo de sensor
    int bandera;        //bandera de estado
};

// Mapa para almacenar los sensores, clave: pin, valor: estructura del sensor
std::map<int, Sensor> sensores;

// Lista para almacenar las descargas
std::list<String> Descargas;    // Para el servidor de datos
std::list<String> descargas;    // ??

// Variables globales para enviar datos
String postData;
String postSheets;
String fecha_y_hora;

// Funciones
String getDateTime();
void sendtodatalogger();
void sendtosheets();
void task0(void *parameter);
void beginSensores();

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, passwd);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi ");
    delay(500);
    beginSensores();
    delay(200);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Hora sincronizada con NTP.");
    xTaskCreatePinnedToCore(
        task0,        /* Function to implement the task */
        "sendData", /* Name of the task */
        10000,        /* Stack size in words */
        NULL,        /* Task input parameter */
        0,            /* Priority of the task */
        &Task0,        /* Task handle. */
        0);            /* Core where the task should run */
}

void task0(void *parameter)
{

    for (;;)
    {

        for (auto &sensor : sensores)
        {
            if (digitalRead(sensor.first) && sensor.second.bandera == 0)
            {
                fecha_y_hora = getDateTime();

                //Descargas.push_front("sensorID=" + sensor.second.ID + "&dischargeTime=" + fecha_y_hora);

                //descargas.push_front("{\"descarga\":\"" + sensor.second.ID + "\",\"date\":\"" + fecha_y_hora + "\",\"type\":\"" + sensor.second.type + "\"}");

                Serial.println("Descarga del sensor: " + sensor.second.ID + "\t Hora: " + fecha_y_hora + "\n");

                sensor.second.bandera = 1;
            }

            if (sensor.second.bandera > 0 && sensor.second.bandera < flag_time)
            {
                ++sensor.second.bandera;
            }
            else
            {
                sensor.second.bandera = 0;
            }
        }

    }
    
}

void loop()
{

    if (Descargas.empty() && descargas.empty())
    {
        Serial.println("...\n");
    }
    if (!Descargas.empty())
    {

        postData = Descargas.back();
        Descargas.pop_back();
        Serial.println("Saqué una descarga para el server\n");

        sendtodatalogger();
    }
    if (!descargas.empty())
    {

        postSheets = descargas.back();
        descargas.pop_back();
        Serial.println("Saqué una descarga para sheets\n");

        sendtosheets();
    }

    delay(500);
}

void sendtodatalogger()
{

    if (WiFi.status() == WL_CONNECTED)
    {

        HTTPClient http;
        http.begin(serverAndre);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        int httpResponeCode = http.POST(postData);

        if (httpResponeCode > 0)
        {

            String response = http.getString();
            Serial.println("Responde code:" + httpResponeCode);
            Serial.println(response);
        }
        else
        {

            Serial.println("Error in sending data " + httpResponeCode);
        }

        http.end();
    }
    else
    {

        Serial.println("Wifi disconnected");
        WiFi.reconnect();
    }
}

void sendtosheets()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");
        int httpResponeCode = http.POST(postSheets);

        if (httpResponeCode > 0)
        {
            /* String response = http.getString();
            Serial.println("Responde code:" + httpResponeCode);
            Serial.println(response); */
            Serial.println("Se subió correctamente al sheets");
        }
        else
        {
            Serial.println("Error in sending data");
        }
        http.end();
    }
    else
    {
        Serial.println("Wifi disconnected");
        WiFi.reconnect();
    }
}

String getDateTime()
{
    struct tm timeinfo;
    String datetime;
    String ano;
    String mes;
    String dia;
    String hora;
    String minuto;
    String segundo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Error al obtener la hora");
        datetime = "0000-00-00 00:00:00";
    }

    ano = String(timeinfo.tm_year + 1900);
    if (timeinfo.tm_mon < 10)
    {
        mes = "0" + String(timeinfo.tm_mon + 1);
    }
    else
    {
        mes = String(timeinfo.tm_mon + 1);
    }

    if (timeinfo.tm_mday < 10)
    {
        dia = "0" + String(timeinfo.tm_mday);
    }
    else
    {
        dia = String(timeinfo.tm_mday);
    }

    if (timeinfo.tm_hour < 10)
    {
        hora = "0" + String(timeinfo.tm_hour);
    }
    else
    {
        hora = String(timeinfo.tm_hour);
    }

    if (timeinfo.tm_min < 10)
    {
        minuto = "0" + String(timeinfo.tm_min);
    }
    else
    {
        minuto = String(timeinfo.tm_min);
    }

    if (timeinfo.tm_sec < 10)
    {
        segundo = "0" + String(timeinfo.tm_sec);
    }
    else
    {
        segundo = String(timeinfo.tm_sec);
    }

    datetime = ano + "-" + mes + "-" + dia + " " + hora + ":" + minuto + ":" + segundo;
    return datetime;
}

void beginSensores()
{
    //* Agreggar sensores: sensores[ <pinSensor> ] = { "<ID>" , "<Type>" , 0};
    
    sensores[12] = {"12", "WC", 0};
    sensores[13] = {"13", "WC", 0};
    sensores[14] = {"14", "WC", 0};
    sensores[15] = {"15", "WC", 0};
    sensores[18] = {"18", "WC", 0};
    sensores[19] = {"19", "WC", 0};
    sensores[21] = {"21", "WC", 0};
    sensores[22] = {"22", "WC", 0};
    sensores[23] = {"23", "WC", 0};
    sensores[25] = {"25", "WC", 0};
    sensores[26] = {"26", "WC", 0};
    sensores[27] = {"27", "WC", 0};
    sensores[32] = {"32", "WC", 0};
    sensores[33] = {"33", "WC", 0};
    sensores[34] = {"34", "WC", 0};
    sensores[35] = {"35", "WC", 0};

    for (auto &sensor : sensores)
    {
        pinMode(sensor.first, INPUT);
        Serial.println("Sensor: " + sensor.second.ID + "\tPin: " + String(sensor.first) + "\t Type: " + sensor.second.type);
    }
}
