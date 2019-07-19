# ESP8266 Power efficient temperature sensor

![Assembled sensor](/sensor-case/ESP8266_Temp_Sensor_Assembled.png?raw=true)

## Sensor design and features
- Utilizes DHT22 temperature + humidity sensor
    - Changing to a different sensor should only require changes to a few lines of code
- OTA (Over The Air) firmware updates
    - No need to physically connect to device to flash new firmware	
- Utilizes InfluxDB time series DB for data logging
- Utilizes the board's deep sleep (low power) mode to minimize power consumption 
    - For maximum efficient/lowest power consumption, the board needs to be powered directly and not via the USB connector (which utilizes the on-board voltage regulator)
- Per device configuration for temperature and humidity adjustments
    - Adjustments can be made globally (it's all code) but for my purposes I used the ESP8266  WiFI adapter MAC address
    - It's reccomended to calibrate (adjust the software offsets) relative to known accurate sensors

## BOM (Bill of Materials)
### Sensor
- 1x ESP8266
    - The provided case leverages NodeMCU v3 w/o attached headers 
- 1x 1N5817 Schottky diode
- 1x 10kΩ resistor

### Case
- 4x M2x4 self tapping screws
- 2x M2x8 self tapping screws
- 1x M2 nylon washer (optional)
- 1x 3-wire Micro JST connector (optional to simplify disassembly/reassembly)

## Wiring diagram
![Wiring diagram](/sensor/ESP8266_Temp_Sensor_Wiring_Diagram.png?raw=true)

## Sensor OTA (Over The Air) update server

## Sensor case
- stl, step, and f3d models provided
- Flame retartent filament recommended
    - I personally use eSun ePC (Flame retardant level: UL94：V2) for electronics projects
- Remember to scale the 3D model to compensate for filament shrinkage
- This case relies on the ESP8266 NodeMCU v3 dimensions and a headerless (no pre-soldered pin headers) board. While headerless boards are more difficult to find, their use does allow for a much shorter, uniform, and convenient case height
- Board dimensions vary wildly between ESP8266 version and manufacturer. I highly reccomend comparing the dimensions of your board and screw hole sizes/locations to the 3D model to avoid wasting unnecessary time and filament
- Assembly
    - Use the 4x M2x4 screws to attach the ESP8266 to the case
    - Use the 2x M2x8 screws to attach the sensor lid to the case
    - If the sensor does not sit flush, use the nylon washer (any thin washer would do but I used a nylon washer as I had one available) to fill the empty space between the sensor and the lid
    - As long as the tolerances of your 3D print are within reason, the sliding lid should attach without force but should also not slide out on its own

![Sensor case parts](/sensor-case/ESP8266_Temp_Sensor_Case_Parts.png?raw=true)
![Open sensor case](/sensor-case/ESP8266_Temp_Sensor_Open_Case.png?raw=true)


## Getting started

1. Install the ES8266 boards
    * Instructions [here](https://github.com/esp8266/Arduino#installing-with-boards-manager)
2. Select the ESP8266 board
    * Tools --> Boards --> *NodeMCU 1.0 (ESP-12E Module)*
3. Install the Adafruit DHT sensor library
    * Instructions [here](https://learn.adafruit.com/dht/using-a-dhtxx-sensor)
    * Don't forget to install both the *DHT sensor library* **and** the *Adafruit Unified Sensor* libraries
4. Locate the *writeNewDeviceSettings* function and make updates as necessary
```cpp
  char* otaUrl = "http://otaserverhostname:otaserverport/ota/tempsensor.bin"; //Blank if not using OTA updates
  short dhtPin = 5; //ESP8266 D1
  short dhtPowerPin = 4; //ESP8266 D2 (Not required if you use a power pin directly)
  char* wifiSSID = "yourwifissid";
  char* wifiPwd = "yourwifipwd";
  char* influxDBProtocol = "http";
  char* influxDBHost = "yourinfluxdbhost";
  int influxDBPort = 8086;
  char* influxDBDB = "sensordata";
  char* influxDBMeasurement = "temperature";
  short sendInterval = 60; //seconds
```
