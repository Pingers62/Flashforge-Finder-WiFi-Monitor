# Flashforge-Finder-WiFi-Monitor
WiFi Monitor project for Flashforge Finder 3D printer
This project wasstarted because i needed to monitor print progress on long prints remotely.
it uses an ESP8266 micro controller (Wemos D1 Mini / NodeMCU etc) and talks to the printer over wifi to get print status and send the details to a BLYNK project running on a mobile device.
Its been tested on Wemos D1 mini and NodeMCU - both gave same results.
The code here is my initial attempt for this project - i may get time to modify / correct at a later date.
The project in its current form here does what i need it to do.
The WiFi settings on the 3D printer need to be configured which can pose some risks if not done correctly - there is a YouTube video for this project showing which settings to change.
The project will not work without these WiFi configuration changes.
