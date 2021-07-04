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

void audioTtsSam(char* newMsg){
    stopPlaying();
    actionPrePlay();
    ESP8266SAM *sam = new ESP8266SAM;
    sam->Say(out, newMsg);
    delete sam;
    stopPlaying();
}

void audioPlay(char* url){
    stopPlaying();
    file_http = new AudioFileSourceHTTPStream();
    if ( file_http->open(url)) {
        actionPrePlay();
        buff = new AudioFileSourceBuffer(file_http, preallocateBuffer, preallocateBufferSize);
        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out);
    } else {
        stopPlaying();
        broadcastStatus("status", "error");
    }
}

void audioStream(char* stream){
    stopPlaying();
    file_icy = new AudioFileSourceICYStream();
    if ( file_icy->open(stream)) {
        actionPrePlay();
        buff = new AudioFileSourceBuffer(file_icy, preallocateBuffer, preallocateBufferSize);
        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out);
    } else {
        stopPlaying();
        broadcastStatus("status", "error");
    }
}

void audioTone(char* toneMessage){
    stopPlaying();
    actionPrePlay();
    file_progmem = new AudioFileSourcePROGMEM( toneMessage, sizeof(toneMessage) );
    rtttl = new AudioGeneratorRTTTL();
    rtttl->begin(file_progmem, out);
    stopPlaying();
}