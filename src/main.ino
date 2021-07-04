#define DEBUG_FLAG              // uncomment to enable debug mode & Serial output
  
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include "PubSubClient.h"
#include <ESP8266WiFi.h>

//Audio libraries
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioGeneratorRTTTL.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourcePROGMEM.h"
#include "ESP8266SAM.h"
#include <SD.h>
#include "boot_sound.h"
#include "sweetChildIntro_sound.h"
 
//debug macros
#ifdef DEBUG_FLAG
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_PRINTLN(x) Serial.println (x)
 #define DEBUG_PRINTF(x) Serial.printf (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x)
#endif

//Start plug section
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

const float INICIO_1M                = 255;
const float FIN_1M                   = 264;
const float INICIO_180K              = 341;
const float FIN_180K                 = 355;
const float INICIO_470K              = 432;
const float FIN_470K                 = 438;
const float INICIO_1M_180K           = 469;
const float FIN_1M_180K              = 480;
const float INICIO_1M_470K           = 532;
const float FIN_1M_470K              = 537;
const float INICIO_300K              = 543;
const float FIN_300K                 = 554;
const float INICIO_470K_180K         = 567;
const float FIN_470K_180K            = 580;
const float INICIO_1M_300K           = 612;
const float FIN_1M_300K              = 621;
const float INICIO_1M_470K_180K      = 629;
const float FIN_1M_470K_180K         = 639;
const float INICIO_300K_180K         = 640;
const float FIN_300K_180K            = 652;
const float INICIO_300K_470K         = 674;
const float FIN_300K_470K            = 679;
const float INICIO_1M_300K_180K      = 687;
const float FIN_1M_300K_180K         = 700;
const float INICIO_1M_300K_470K      = 712;
const float FIN_1M_300K_470K         = 722;
const float INICIO_300K_470K_180K    = 730;
const float FIN_300K_470K_180K       = 740;
const float INICIO_1M_300K_470K_180K = 756;
const float FIN_1M_300K_470K_180K    = 770;

long previousMillis = 0;
const long intervalRead = 2000;
boolean previousPlayedStatusPlug[4];
boolean justReadStatusPlug[4];
//End plug section

WiFiManager wifiManager;
WiFiClient                wifiClient;
PubSubClient              mqttClient(wifiClient);
const char* willTopic     = "LWT";
const char* willMessage   = "offline";
boolean willRetain        = false;
byte willQoS              = 0;
#define  MQTT_MSG_SIZE    512

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[34] = "notificacion";
char mqtt_user[34] = "notificador";
char mqtt_password[34] = "notificador";
char mqtt_FullTopic[MQTT_MSG_SIZE];

//Audio globals
AudioGeneratorMP3         *mp3;
AudioGeneratorWAV         *wav;
AudioGeneratorRTTTL       *rtttl;
AudioFileSourceHTTPStream *file_http;
AudioFileSourcePROGMEM    *file_progmem;
AudioFileSourceICYStream  *file_icy;
AudioFileSourceBuffer     *buff;
AudioOutputI2SNoDAC       *out;

float volume_level = 1.0;
String playing_status;
const int preallocateBufferSize = 2048;
void *preallocateBuffer = NULL;

//plugs globals
char plug1[80];
char plug2[80];
char plug3[80];
char plug4[80];

//Rele
#define relePin D5
 
void setup() {
  Serial.begin(115200);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);

  for(int i=0; i<4; i++){
    previousPlayedStatusPlug[i]=0;
    justReadStatusPlug[i]=0;
  }

  wifiSetup();

  DEBUG_PRINT(F("server --> "));
  DEBUG_PRINTLN(mqtt_server);
  DEBUG_PRINT(F("port --> "));
  DEBUG_PRINTLN(mqtt_port);
  DEBUG_PRINT(F("user --> "));
  DEBUG_PRINTLN(mqtt_user);
  DEBUG_PRINT(F("password --> "));
  DEBUG_PRINTLN(mqtt_password);
  DEBUG_PRINT(F("topic --> "));
  DEBUG_PRINTLN(mqtt_topic);

  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(MQTT_MSG_SIZE);
  mqttReconnect();

  out = new AudioOutputI2SNoDAC();
  DEBUG_PRINTLN("Using No DAC - using Serial port Rx pin");
  out->SetGain(volume_level);
  DEBUG_PRINTLN("Before playing boot sound");
  playBootSound();
  DEBUG_PRINTLN("After playing boot sound");
}
 
void loop() {
  mqttReconnect();
  mqttClient.loop();
  if (mp3   && !mp3->loop())    stopPlaying();
  if (wav   && !wav->loop())    stopPlaying();
  if (rtttl && !rtttl->loop())  stopPlaying();
  long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalRead) {
    previousMillis = currentMillis;
    checkPlug();
    compareNewPlug();
  }
}

//////////////////////plugs methods/////////////////////////////////
void compareNewPlug(){
  for(int i=0; i<4; i++){
    if(justReadStatusPlug[i] != previousPlayedStatusPlug[i]){
      if(justReadStatusPlug[i] == 1){
        DEBUG_PRINT("Playing sound");
        DEBUG_PRINTLN(i);
      }else{
        DEBUG_PRINT("Playing sound 5");
      }
    }
    previousPlayedStatusPlug[i] = justReadStatusPlug[i];
  }
}

void checkPlug(){
  int sensorValue = analogRead(analogInPin);
  DEBUG_PRINT(" sensorValue=");
  DEBUG_PRINTLN(sensorValue);
  // char textNum[4];
  // String auxStr=String(sensorValue);
  // auxStr.toCharArray(textNum,4);
  // audioTtsSam(textNum);
  if(sensorValue < INICIO_1M){
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=0;
  }else if(INICIO_1M < sensorValue && sensorValue < FIN_1M){
    DEBUG_PRINTLN("1M");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=0;
  }else if(INICIO_180K < sensorValue && sensorValue < FIN_180K){
    DEBUG_PRINTLN("180K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=1;
  }else if(INICIO_470K < sensorValue && sensorValue < FIN_470K){
    DEBUG_PRINTLN("470K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=0;
  }else if(INICIO_1M_180K < sensorValue && sensorValue < FIN_1M_180K){
    DEBUG_PRINTLN("1M 180K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=1;
  }else if(INICIO_1M_470K < sensorValue && sensorValue < FIN_1M_470K){
    DEBUG_PRINTLN("1M 470K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=0;
  }else if(INICIO_300K < sensorValue && sensorValue < FIN_300K){
    DEBUG_PRINTLN("300K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=0;
  }else if(INICIO_470K_180K < sensorValue && sensorValue < FIN_470K_180K){
    DEBUG_PRINTLN("470K 180K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=1;
  }else if(INICIO_1M_300K < sensorValue && sensorValue < FIN_1M_300K){
    DEBUG_PRINTLN("1M 300K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=0;
  }else if(INICIO_1M_470K_180K < sensorValue && sensorValue < FIN_1M_470K_180K ){
    DEBUG_PRINTLN("1M 470K 180K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=0;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=1;
  }else if(INICIO_300K_180K < sensorValue && sensorValue < FIN_300K_180K){
    DEBUG_PRINTLN("300K 180K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=1;
  }else if(INICIO_300K_470K < sensorValue && sensorValue < FIN_300K_470K){
    DEBUG_PRINTLN("300K 470K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=0;
  }else if(INICIO_1M_300K_180K < sensorValue && sensorValue < FIN_1M_300K_180K){
    DEBUG_PRINTLN("1M 300K 180K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=0;
    justReadStatusPlug[3]=1;
  }else if(INICIO_1M_300K_470K < sensorValue && sensorValue < FIN_1M_300K_470K){
    DEBUG_PRINTLN("1M 300K 470K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=0;
  }else if(INICIO_300K_470K_180K < sensorValue && sensorValue < FIN_300K_470K_180K){
    DEBUG_PRINTLN("300K 470K 180K");
    justReadStatusPlug[0]=0;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=1;
  }else if(INICIO_1M_300K_470K_180K < sensorValue && sensorValue < FIN_1M_300K_470K_180K){
    DEBUG_PRINTLN("1M 300K 470K 180K");
    justReadStatusPlug[0]=1;
    justReadStatusPlug[1]=1;
    justReadStatusPlug[2]=1;
    justReadStatusPlug[3]=1;
  }
}
