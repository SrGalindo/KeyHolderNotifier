// Example
// mqtt: notificacion/play
// http://192.168.1.200:8123/local/SweetChild_intro-bit128.mp3

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
 
void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINT("Entered config mode");
  DEBUG_PRINT(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINT(myWiFiManager->getConfigPortalSSID());
}

void writeStringToEEPROM(uint16_t pos, char* str, uint16_t len){
  for (int i = 0; i < len; ++i){
    EEPROM.write(pos + i, str[i]);
    if (str[i] == 0) return;
  }
}

void readStringFromEEPROM(uint16_t pos, char* str, uint16_t len){
  for (int i = 0; i < len; ++i){
    str[i] = EEPROM.read(pos + i);
    if (str[i] == 0) return;
  }
  str[len] = 0; //make sure every string is properly terminated. str must be at least len +1 big.
}

void clearEEPROM(){
  for (int i = 0; i < 512; i++){
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}
 
void setup() {
  Serial.begin(115200);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);
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
  playBootSound();
}
 
void loop() {
  mqttReconnect();
  mqttClient.loop();
  if (mp3   && !mp3->loop())    stopPlaying();
  if (wav   && !wav->loop())    stopPlaying();
  if (rtttl && !rtttl->loop())  stopPlaying();

}

void mqttReconnect() {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN(F("Connecting to MQTT..."));
    String ChipId = String(random(0xffff), HEX);
    String thingName = String("Notificador - ") + ChipId;

    //if (mqttClient.connect(thingName.c_str(), mqtt_user, mqtt_password)) {
    if (mqttClient.connect(thingName.c_str(), mqtt_user, mqtt_password, mqttFullTopic(willTopic), willQoS, willRetain, willMessage)) {
      mqttClient.subscribe(mqttFullTopic("play"));
      mqttClient.subscribe(mqttFullTopic("stream"));
      mqttClient.subscribe(mqttFullTopic("tone"));
      mqttClient.subscribe(mqttFullTopic("say"));
      mqttClient.subscribe(mqttFullTopic("stop"));
      mqttClient.subscribe(mqttFullTopic("volume"));
      mqttClient.subscribe(mqttFullTopic("FACTORY-RESET"));
      DEBUG_PRINTLN(F("Connected to MQTT"));

      broadcastStatus("LWT", "online");
      broadcastStatus("ThingName", thingName.c_str());
      broadcastStatus("IPAddress", WiFi.localIP().toString());
      broadcastStatus("status", "pausa");
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int mlength)  {
  DEBUG_PRINTLN("In on MQTT message");
  char newMsg[MQTT_MSG_SIZE];

  if (mlength > 0) {
    memset(newMsg, '\0' , sizeof(newMsg));
    memcpy(newMsg, payload, mlength);
    DEBUG_PRINTLN();
    DEBUG_PRINT(F("Requested ["));
    DEBUG_PRINT(topic);
    DEBUG_PRINT(F("] "));
    DEBUG_PRINTLN(newMsg);
  
    // got a new URL to play ------------------------------------------------
    if (!strcmp(topic, mqttFullTopic("play") ) ) {
      stopPlaying();
      file_http = new AudioFileSourceHTTPStream();
      if ( file_http->open(newMsg)) {
        actionPrePlay();
        buff = new AudioFileSourceBuffer(file_http, preallocateBuffer, preallocateBufferSize);
        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out);
      } else {
        stopPlaying();
        broadcastStatus("status", "error");
      }
    }
    // got a new URL to play ------------------------------------------------
    if ( !strcmp(topic, mqttFullTopic("stream"))) {
      stopPlaying();
      file_icy = new AudioFileSourceICYStream();
      if ( file_icy->open(newMsg)) {
        actionPrePlay();
        buff = new AudioFileSourceBuffer(file_icy, preallocateBuffer, preallocateBufferSize);
        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out);
      } else {
        stopPlaying();
        broadcastStatus("status", "error");
      }
    }
    // got a tone request --------------------------------------------------
    if(!strcmp(topic, mqttFullTopic("tone"))){
      stopPlaying();
      actionPrePlay();
      file_progmem = new AudioFileSourcePROGMEM( newMsg, sizeof(newMsg) );
      rtttl = new AudioGeneratorRTTTL();
      rtttl->begin(file_progmem, out);
      stopPlaying();
    }
    //got a TTS request ----------------------------------------------------
    if ( !strcmp(topic, mqttFullTopic("say"))) {
      stopPlaying();
      actionPrePlay();
      ESP8266SAM *sam = new ESP8266SAM;
      sam->Say(out, newMsg);
      delete sam;
      stopPlaying();
    }
    // got a volume request, expecting double [0.0,1.0] ---------------------
    if ( !strcmp(topic, mqttFullTopic("volume"))) {
      volume_level = atof(newMsg);
      if ( volume_level < 0.0 ) volume_level = 0;
      if ( volume_level > 1.0 ) volume_level = 0.7;
      out->SetGain(volume_level);
    }
    // got a stop request  --------------------------------------------------
    if ( !strcmp(topic, mqttFullTopic("stop"))) {
      stopPlaying();
    }
    // reset configuration from factory
    if(!strcmp(topic, mqttFullTopic("FACTORY-RESET"))){
      DEBUG_PRINTLN("In factory reset");
      if(!strcmp(newMsg, "YES")){
        DEBUG_PRINTLN("Reseting from factory");
        clearEEPROM();
        wifiManager.resetSettings();
        ESP.reset();
      }
    }
  }
}

char* mqttFullTopic(const char action[]) {
  strcpy (mqtt_FullTopic, mqtt_topic);
  strcat (mqtt_FullTopic, "/");
  strcat (mqtt_FullTopic, action);
  return mqtt_FullTopic;
}

void broadcastStatus(const char topic[], String msg) {
  if ( playing_status != msg) {
    char charBuf[msg.length() + 1];
    msg.toCharArray(charBuf, msg.length() + 1);
    mqttClient.publish(mqttFullTopic(topic), charBuf);
    playing_status = msg;
    DEBUG_PRINT(F("[MQTT] "));
    DEBUG_PRINT(mqtt_topic);
    DEBUG_PRINT(F("/"));
    DEBUG_PRINT(topic);
    DEBUG_PRINT(F("\t\t"));
    DEBUG_PRINTLN(msg);
  }
}

void wifiSetup(){
  EEPROM.begin(512);
  //For testing
  //clearEEPROM();
   
  readStringFromEEPROM(0, mqtt_server, 40);
  readStringFromEEPROM(40,mqtt_topic,34);
  readStringFromEEPROM(74,mqtt_user,34);
  readStringFromEEPROM(108,mqtt_password,34);
  readStringFromEEPROM(142,mqtt_port,6);
  readStringFromEEPROM(148,plug1,80);
  readStringFromEEPROM(228,plug2,80);
  readStringFromEEPROM(308,plug3,80);
  readStringFromEEPROM(388,plug4,80);
  
  DEBUG_PRINT(F("Read EEPROM server --> "));
  DEBUG_PRINTLN(mqtt_server);
  DEBUG_PRINT(F("Read EEPROM server --> "));
  DEBUG_PRINTLN(mqtt_port);
  DEBUG_PRINT(F("Read EEPROM topic --> "));
  DEBUG_PRINTLN(mqtt_topic);
  DEBUG_PRINT(F("Read EEPROM user --> "));
  DEBUG_PRINTLN(mqtt_user);
  DEBUG_PRINT(F("Read EEPROM Pass --> "));
  DEBUG_PRINTLN(mqtt_password);
  DEBUG_PRINT(F("plug1 --> "));
  DEBUG_PRINTLN(plug1);
  DEBUG_PRINT(F("plug2 --> "));
  DEBUG_PRINTLN(plug2);
  DEBUG_PRINT(F("plug3 --> "));
  DEBUG_PRINTLN(plug3);
  DEBUG_PRINT(F("plug4 --> "));
  DEBUG_PRINTLN(plug4);

  WiFiManagerParameter custom_mqtt_server("server", "MQTT server IP", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT server port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 32);
  WiFiManagerParameter custom_mqtt_password("password", "MQTT password", mqtt_password, 32);
  WiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT topic notification", mqtt_topic, 32);
  WiFiManagerParameter custom_plug1("plug1", "URL song for Plug 1", plug1, 80);
  WiFiManagerParameter custom_plug2("plug2", "URL song for Plug 2", plug2, 80);
  WiFiManagerParameter custom_plug3("plug3", "URL song for Plug 3", plug3, 80);
  WiFiManagerParameter custom_plug4("plug4", "URL song for Plug 4", plug4, 80);

  //reset settings - for testing
  //wifiManager.resetSettings();
 
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
 
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_plug1);
  wifiManager.addParameter(&custom_plug2);
  wifiManager.addParameter(&custom_plug3);
  wifiManager.addParameter(&custom_plug4);
 
  if(!wifiManager.autoConnect("Notificador-AP","NOT-1234")) {
    DEBUG_PRINT("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  } 
 
  //if you get here you have connected to the WiFi
  DEBUG_PRINT(F("WIFIManager connected!"));
 
  DEBUG_PRINT(F("IP --> "));
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT(F("GW --> "));
  DEBUG_PRINTLN(WiFi.gatewayIP());
  DEBUG_PRINT(F("SM --> "));
  DEBUG_PRINTLN(WiFi.subnetMask());
  DEBUG_PRINT(F("DNS 1 --> "));
  DEBUG_PRINTLN(WiFi.dnsIP(0));
  DEBUG_PRINT(F("DNS 2 --> "));
  DEBUG_PRINTLN(WiFi.dnsIP(1));
  
  //read updated parameters
  if(mqtt_server != custom_mqtt_server.getValue()){
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    writeStringToEEPROM(0, mqtt_server, 40);
  }
  if(mqtt_topic != custom_mqtt_topic.getValue()){
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    writeStringToEEPROM(40,mqtt_topic,34);
  }
  if(mqtt_user != custom_mqtt_user.getValue()){
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    writeStringToEEPROM(74,mqtt_user,34);
  }
  if(mqtt_password != custom_mqtt_password.getValue()){
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    writeStringToEEPROM(108,mqtt_password,34);
  }
  if(mqtt_port != custom_mqtt_port.getValue()){
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    writeStringToEEPROM(142,mqtt_port,6);
  }
  if(plug1 != custom_plug1.getValue()){
    strcpy(plug1, custom_plug1.getValue());
    writeStringToEEPROM(148,plug1,80);
  }
  if(plug2 != custom_plug2.getValue()){
    strcpy(plug2, custom_plug2.getValue());
    writeStringToEEPROM(228,plug2,80);
  }
  if(plug3 != custom_plug3.getValue()){
    strcpy(plug3, custom_plug3.getValue());
    writeStringToEEPROM(308,plug3,80);
  }
  if(plug4 != custom_plug4.getValue()){
    strcpy(plug4, custom_plug4.getValue());
    writeStringToEEPROM(388,plug4,80);
  }
  EEPROM.commit();
}

///////////////AUdio methods/////////////////////////////////////
void playBootSound() {
  DEBUG_PRINTLN("playing boot sound");
  stopPlaying();
  actionPrePlay();
  file_progmem = new AudioFileSourcePROGMEM(boot_sound, sizeof(boot_sound));
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file_progmem, out);
}

void stopPlaying() {
  DEBUG_PRINTLN(F("...#"));
  DEBUG_PRINTLN(F("Interrupted!"));
  DEBUG_PRINTLN();
  digitalWrite(relePin, HIGH);
  delay(500);
  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = NULL;
  }
  if (wav) {
    wav->stop();
    delete wav;
    wav = NULL;
  }
  if (rtttl) {
    rtttl->stop();
    delete rtttl;
    rtttl = NULL;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file_http) {
    file_http->close();
    delete file_http;
    file_http = NULL;
  }
  if (file_progmem) {
    file_progmem->close();
    delete file_progmem;
    file_progmem = NULL;
  }
  if (file_icy) {
    file_icy->close();
    delete file_icy;
    file_icy = NULL;
  }
  broadcastStatus("status", "stop");
}

void actionPrePlay(){
  digitalWrite(relePin, LOW);
  delay(300);
  broadcastStatus("status", "playing");
}