# ESP8266 Power efficient temperature sensor
![Assembled sensor](/sensor-case/ESP8266_Temp_Sensor_Assembled.png?raw=true)


## Sensor design and features
- Utilizes DHT22 temperature + humidity sensor
    - Changing to a different sensor should only require changes to a few lines of code
- OTA (Over The Air) firmware updates
    - No need to physically connect to device to flash new firmware	
- Utilizes InfluxDB time series DB for data logging
- Per device configuration for temperature and humidity adjustments
    - It's reccomended to calibrate (adjust the software offsets) relative to known accurate sensors

## BOM (Bill of Materials)
- 1x ESP8266
    - The provided case leverages NodeMCU v3 w/o attached headers
- 1x 1N5817 Schottky diode
- 1x 10kΩ resistor
- 1x 3-wire Micro JST connector (optional to simplify disassembly/reassembly)

## Wiring diagram
![Wiring diagram](/sensor/ESP8266_Temp_Sensor_Wiring_Diagram.png?raw=true)

## Sensor OTA (Over The Air) update server

## Sensor case
- stl, step, and f3d models provided
- Flame retartent filament recommended
    - I personally use eSun ePC (Flame retardant level: UL94：V2) for electronics projects
- Remember to scale the 3D model to compensate for filament shrinkage
- Screws
- Nylon washer
- JST connector

![Sensor case parts](/sensor-case/ESP8266_Temp_Sensor_Case_Parts.png?raw=true)

![Open sensor case](/sensor-case/ESP8266_Temp_Sensor_Open_Case.png?raw=true)


## How do I get started?
