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