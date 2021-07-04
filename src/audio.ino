void ttsSAM(char* newMsg){
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