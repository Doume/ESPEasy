# ESPEasy
This is a fork of Easy MultiSensor device based on ESP8266
Forked from the Build 113
Regarding ESPEasy stable versions including libraries are currently on SoureForge:
- http://sourceforge.net/projects/espeasy/
- Wiki: http://www.esp8266.nu
- Forum: http://www.esp8266.nu/forum

# My Features
- Generic HTTP request :
  * Allow you to have a full control over the HTTP Request being sent to the backend
  * Set the HTTP method :
    - GET (for compatibility purpose with previous version of ESPEasy)
    - POST
    - PUT
  * Define your own HTTP Path : /<xxx>/<yyy>
  * Define your own HTTP Header : <key> : <value>
  * Define your own HTTP Body : JSON, XML, string or whatever you want
  * Key token could be anywhere within the HTTP Path / Header / Body :
    - %sysname% is replaced by the name of the device
    - %valname% is replaced by the name of the variable that sent his value
    - %value% is replaced by the value of the variable
- Wifi Access Point with System Name :
  * When turning into the AP mode, if the name of the device is already Define the the AP name is define with this value.
- Build version with my Name :-) :
  * Build version is appended with the "Fork olileger" token