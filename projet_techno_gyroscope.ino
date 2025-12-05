// Basic demo for accelerometer readings from Adafruit MPU6050

// ESP32 Guide: https://RandomNerdTutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/
// Wifi guide: https://randomnerdtutorials.com/esp32-mpu-6050-web-server/

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino_JSON.h>
#include "WiFi.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"


Adafruit_MPU6050 mpu;

constexpr char* ssid = "gyroscope_proj";
constexpr char* pass = "gyro1234";

AsyncWebServer server(80);
AsyncEventSource events("/events");

sensors_event_t a, g, temp;

uint32_t lastTime = 0;
uint32_t gyroDelay = 10;

float gyroX;
float gyroY;
float gyroZ;
//float accX;
//float accY;
//float accZ;

constexpr float gyroXError = 0.05f;
constexpr float gyroYError = 0.05f;
constexpr float gyroZError = 0.05f;

JSONVar readings;

/* """"""""""""""""""""""""""""""""""""""""""""""""
 * Initalisation de LittleFS et du wifi
"""""""""""""""""""""""""""""""""""""""""""""""" */
void init_wifi()
{
  // Lancer LittleFS
  if(!LittleFS.begin())
    Serial.println("Une erreur avec LittleFS s'est produite.");  

  // Connection au Wifi
  WiFi.begin(ssid, pass);

  // Attendre la connection au Wifi
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connexion au Wifi...");
  }

  // Afficher address IP lorsque la connexion reussi
  Serial.print("Connected to following wifi IP: ");
  Serial.println(WiFi.localIP());

  /* *******************************************************
   * Gestion des requetes web provenant de l'utilisateur
  * *******************************************************/
  // écoute requête "/" (=page d'accueil)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) 
  {
    //envoi du fichier HTML au client
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Requete pour reset l'axe X
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest* request){
    gyroX = 0;
    gyroY = 0;
    gyroZ = 0;
    request->send(200, "text/plain", "OK");
  });

  // Ajoute le support pour des evenement serveur 
  // qui seront envoyes a l'utilisateur, puis 
  // demmarage du serveur.
  server.addHandler(&events);
  server.begin();
}

/* """"""""""""""""""""""""""""""""""""""""""""""""
 * SETUP
"""""""""""""""""""""""""""""""""""""""""""""""" */
void setup() {
  Serial.begin(115200);

  // Attendre que le moniteur serie soit demmarre
  while (!Serial)
    delay(10);
    
  // Initialize le Wifi
  init_wifi();
  
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  Serial.println("Gyroscope Initialized");
  delay(100);
}

/* """"""""""""""""""""""""""""""""""""""""""""""""  
 * LOOP
"""""""""""""""""""""""""""""""""""""""""""""""" */

void loop() {
  // Coute les evenements des senseurs du gyroscope
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Recuperer les variations depui la derniere seconde
  float gyroX_temp = g.gyro.x;
  float gyroY_temp = g.gyro.y;
  float gyroZ_temp = g.gyro.z;
  
  if(abs(gyroX_temp)> gyroXError) 
    gyroX += gyroX_temp / 50.0f;
  if(abs(gyroY_temp) > gyroYError) 
    gyroY += gyroY_temp / 70.0f;
  if(abs(gyroZ_temp) > gyroZError) 
    gyroZ += gyroZ_temp / 90.0f;
    

  // Convertir chaque donnes numeriques en string
  readings["gyroX"] = String(gyroX);
  readings["gyroY"] = String(gyroY);
  readings["gyroZ"] = String(gyroZ);

  // Convertir toutes les donnes en format JSON string
  String jsonString = JSON.stringify(readings);

  // Envoyer les donnes JSON a l'utilisateur connecte au site web
  // seulement si 10ms se sont ecoule dpuis la derniere fois que cela a ete envoye.
  if((millis() - lastTime) > gyroDelay)
  {
    events.send(jsonString.c_str(), "gyro_readings", millis());
    lastTime = millis();
  }
}
