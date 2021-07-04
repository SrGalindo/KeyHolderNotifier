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
      audioPlay(newMsg);
    }
    // got a new URL to play ------------------------------------------------
    if ( !strcmp(topic, mqttFullTopic("stream"))) {
      audioStream(newMsg);
    }
    // got a tone request --------------------------------------------------
    if(!strcmp(topic, mqttFullTopic("tone"))){
      audioTone(newMsg);
    }
    //got a TTS request ----------------------------------------------------
    if ( !strcmp(topic, mqttFullTopic("say"))) {
      audioTtsSam(newMsg);
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