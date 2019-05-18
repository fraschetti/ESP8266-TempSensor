# ESP8266 Power efficient temperature sensor

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
- Recommended filament
- Reminder to scale the model
- Screws
- Nylon washer
- JST connector

## How do I get started?
