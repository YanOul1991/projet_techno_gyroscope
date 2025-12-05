// Basic demo for accelerometer readings from Adafruit MPU6050

// ESP32 Guide: https://RandomNerdTutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/
// Wifi guide: https://randomnerdtutorials.com/esp32-mpu-6050-web-server/

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino_JSON.h>

// Header pour Wifi
#include "WiFi.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "LittleFS.h"

//#include <iostream>
//#include <sstream>

Adafruit_MPU6050 mpu;

/*
  Wifi
*/
const char* ssid = "gyroscope_ssid";
const char* pass = "gyro1234";
AsyncWebServer server(80);
AsyncEventSource events("/events");

/*
  Gyroscope
*/
JSONVar readings;

unsigned long lastTime = 0;
unsigned long lastTimeTemp = 0;
unsigned long lastTimeAcc = 0;
unsigned long gyroDelay = 10;
unsigned long tempDelay = 1000;
unsigned long accDelay = 200;

sensors_event_t a, g, temp;
float gyroX;
float gyroY;
float gyroZ;
float accX;
float accY;
float accZ;

const float gyroXError = 0.07f;
const float gyroYError = 0.03f;
const float gyroZError = 0.01f;


/*
  ------------------ Initialisation du Wifi ------------------
*/
void init_wifi()
{
  // Lancer LittleFS
  if(!LittleFS.begin())
  {
    Serial.println("Une erreur avec LittleFS s'est produite.");  
  }

  // Connection au Wifi
  WiFi.begin(ssid, pass);

  // Attendre la connection au Wifi
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to Wifi...");
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
  server.on("/resetX", HTTP_GET, [](AsyncWebServerRequest* request){
    Serial.print("ResetX Request from client");
    request->send(200, "text/plain", "OK");
  });

  // Ajoute le support pour des evenement serveur 
  // qui seront envoyes a l'utilisateur, puis 
  // demmarage du serveur.
  server.addHandler(&events);
  server.begin();
}

/* """"""""""""""""""""""""""""""""""""""""""""""""
 *  
 *  
 * SETUP
 *
 *
"""""""""""""""""""""""""""""""""""""""""""""""" */
void setup() {
  Serial.begin(115200);

  // Attendre que le moniteur serie soit demmarre
  while (!Serial)
    delay(10);
    
  // Initialize le Wifi
  init_wifi();
  Serial.println("---------- Wifi initialise ----------");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }
  Serial.println("Gyroscope Initialized");
  delay(100);
}

/* """"""""""""""""""""""""""""""""""""""""""""""""
 *  
 *  
 * LOOP
 *
 *
"""""""""""""""""""""""""""""""""""""""""""""""" */

void loop() {
  // Coute les evenements des senseurs du gyroscope
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");

  // Recuperer les variations depui la derniere seconde
  float gyroX_temp = g.gyro.x;
  float gyroY_temp = g.gyro.y;
  float gyroZ_temp = g.gyro.z;
  
  if(abs(gyroX_temp)> gyroXError)
    gyroX += gyroX_temp/50.0f;
    
  if(abs(gyroY_temp) > gyroYError)
    gyroY += gyroY_temp/70.0f;
    
  if(abs(gyroZ_temp) > gyroZError)
    gyroZ += gyroZ_temp/90.0f;

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
