# esp32_weather_light
![IMG_0841.JPG](https://github.com/kouya17/esp32_weather_light/blob/master/doc/IMG_0841.JPG)

## Overview
Inform weather by illuminating LED.  
Use weather API : <a href="https://openweathermap.org/">https://openweathermap.org/</a>

## Prepare
- <a href="https://www.switch-science.com/catalog/3210/">ESPr® Developer 32</a>
- LEDs

## Pin assignment
IO25 : white led  
IO26 : red led  
IO23 : blue led  
IO4 : green led

## How to use
1. Upload this program to your ESP32.
2. Connect to SSID of your ESP32(default "ESP32AP").
3. Open the browser and access to IP Adress of your ESP32(default 192.168.4.1).
<img src="https://github.com/kouya17/esp32_weather_light/blob/master/doc/index.png" width="400px">  
4. Click the button "AP設定".
<img src="https://github.com/kouya17/esp32_weather_light/blob/master/doc/ssid.png" width="400px">  
5. Set SSID and password of your Wi-Fi router.
6. If you need to set area or forecast time, click the button "通知情報設定" and set them.
<img src="https://github.com/kouya17/esp32_weather_light/blob/master/doc/setinfo.png" width="400px">  
7. Complete!
