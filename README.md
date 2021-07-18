# KeyHolderNotifier
Arduino project which reads the value of the analog pin and calculates who is at home, plays a sound and sends a notification via MQTT

Example
mqtt: notificacion/play
http://192.168.1.200:8123/local/SweetChild_intro-bit128.mp3

Rele on pin D5
Speaker G to Wemos G, Speaker + to Wemos RX
AP password NOT-1234

MQTT:

notificador, notificador, 1883, notificacion

cut songs: https://mp3cut.net/es/

reduce song quality to 32000Khz Sample Rate: https://online-audio-converter.com/

original idea: https://www.instructables.com/MQTT-Audio-Notifier-for-ESP8266-Play-MP3-TTS-RTTL/
