#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "DHT.h"

//Release/Binary version (increase this with each new release)
#define FIRMWARE_VERSION 1

// DHT 22 sensor
#define DHTTYPE DHT22

#define MICROS 1000000

//Thread on deep sleep - GPIO16 + RESET diode - https://www.esp8266.com/viewtopic.php?f=160&t=13625&start=12
//ESP8266 D0  = 16 // 1N5817 Schottky diode (cathode to D0/16, anode to reset pin/pad)
//ESP8266 D1  = 5  // Data pin for DHT22 sensor (can be changed in the config below)
//ESP8266 D2  = 4  // Power pin for the DHT22 sensor (optional - can be changed in the config below)
//ESP8266 D3  = 0
//ESP8266 D4  = 2
//ESP8266 D5  = 14
//ESP8266 D6  = 12
//ESP8266 D7  = 13
//ESP8266 D8  = 15
//ESP8266 D9  = 3
//ESP8266 D10 = 1
//DHT22 1 = Power pin. Either tied directly to the 5v rail or a software controlled pin D2/4 (see above)
//DHT22 2 = Ground pin. Also use a 10k resistor to the 5v rail
//DHT22 3 = Not connected
//DHT22 4 = Data pin. Connected to D1/5 (see above)

//InfluxDB
#define INFLUXDB_URL    "$PROTOCOL$://$HOST$:$PORT$:/write?db=$DB$"
//https://docs.influxdata.com/influxdb/v1.6/write_protocols/line_protocol_reference/
String TEMP_DATA_TEMPLATE = "$measurement$,deviceid=$deviceid$ temperature=$temperature$,humidity=$humidity$,firmware=$firmware$,wifi_rssi=$wifi_rssi$";

// DHT sensor
float sensorPrimeTime = 2500; //Time required for sensor to be ready after power on

//WiFi
long wifiConnectStart;

//EEPROM storage struct for settings
struct {
  //Sanity valid for a valid struct (checking for a single integer isn't sufficient)
  char validation_string[6];

  short firmware_version;

  //OTA - Auto update URL
  char ota_url[128];

  //Sensor settings
  short dht_pin;
  short dht_power_pin;
  float sensor_temp_adjustment;
  float sensor_humidity_adjustment;

  //WiFi settings
  char wifi_ssid[64];
  char wifi_pwd[64];

  //InfluxDB settings
  char influxdb_protocol[6];
  char influxdb_host[64];
  int influxdb_port;
  char influxdb_db[32];
  char influxdb_measurement[32];
  char device_id[64];
  short send_interval; //seconds
}
localSettings;

void setup() {
  Serial.begin(115200);

  EEPROM.begin(512);

  long iterationStart = millis();

  Serial.println();
  Serial.println();
  Serial.println("-=ESP8266=-");
  Serial.println("Firmware version #: " + String(FIRMWARE_VERSION));
  Serial.println("Sketch date: " __DATE__ ", time: " __TIME__);
  Serial.printf("SDK version: %s\n", ESP.getSdkVersion());
  Serial.printf("Core Version: %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("Boot Version: %u\n", ESP.getBootVersion());
  Serial.printf("Boot Mode: %u\n", ESP.getBootMode());
  Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("WiFi MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("Reset reason: %s\n", ESP.getResetReason().c_str());

  Serial.println("-=Temperature Logger=-");
  bool readSettingsSuccess;
  readSettings(readSettingsSuccess);
  if (readSettingsSuccess)
    Serial.println("Successfully read and validated settings");
  else {
    Serial.println("Failed to read settings. Going into longest allowed deep sleep.");
    ESP.deepSleep(4294967295 * MICROS);
    return;
  }

  Serial.println("-=Settings=-");
  Serial.printf("Sensor data pin: %d\n", localSettings.dht_pin);
  Serial.printf("Sensor power pin: %d\n", localSettings.dht_power_pin);
  Serial.printf("OTA Update URL: %s\n", localSettings.ota_url);
  Serial.println("InfluxDB Protocol: " + String(localSettings.influxdb_protocol));
  Serial.println("InfluxDB Host: " + String(localSettings.influxdb_host));
  Serial.printf("InfluxDB Port: %d\n", localSettings.influxdb_port);
  Serial.println("InfluxDB DB: " + String(localSettings.influxdb_db));
  Serial.println("InfluxDB Measurement: " + String(localSettings.influxdb_measurement));
  Serial.println("DeviceID: " + String(localSettings.device_id));
  Serial.printf("Temperature adjustment: : %.2f\n", localSettings.sensor_temp_adjustment);
  Serial.printf("Humidity adjustment: : %.2f\n", localSettings.sensor_humidity_adjustment);

  //Init sensor power pin
  if (localSettings.dht_power_pin >= 0) {
    Serial.println("Energizing sensor power pin...");
    pinMode(localSettings.dht_power_pin, OUTPUT);
    digitalWrite(localSettings.dht_power_pin, HIGH);
  }

  DHT dht(localSettings.dht_pin, DHTTYPE);
  dht.begin();
  long sensorPrimeStart = millis();

  //Start the WiFi before priming the sensor to make good use of our time
  startWiFi();

  long remainingPrimeTime = sensorPrimeTime - (millis() - sensorPrimeStart);
  Serial.println("Priming sensor...");
  if (remainingPrimeTime > 0) {
    delay(remainingPrimeTime);
  }
  Serial.println("Sensor primed");

  connectToWiFi(1);

  if (WiFi.status() == WL_CONNECTED) {
    sendData(dht);
  }

  //Deenergize sensor power pin
  if (localSettings.dht_power_pin >= 0) {
    Serial.println("Denergizing sensor power pin...");
    digitalWrite(localSettings.dht_power_pin, LOW);
  }

  if (WiFi.status() == WL_CONNECTED)
    attemptFirmwareUpdate();

  stopWiFi();

  unsigned long iterationElapsed = millis() - iterationStart;
  Serial.printf("Iteration elapsed: %lums\n", iterationElapsed);

  long sleepMicros = (localSettings.send_interval * MICROS) - micros();
  if (sleepMicros <= 0) {
    Serial.println("No sleep needed. Restarting");
    ESP.restart();
  } else {
    int sleepSeconds = sleepMicros / MICROS;
    Serial.printf("Entering deep sleep (%dsecs)...\n", sleepSeconds);
    ESP.deepSleep(sleepMicros);
  }
}

void loop() {
}

void readSettings(bool &success) {
  Serial.println("Reading settings from EEPROM...");

  for (unsigned int t = 0; t < sizeof(localSettings); t++)
    *((char*)&localSettings + t) = EEPROM.read(t);

  boolean needNewSettings = true;

  if (String(localSettings.validation_string) != "Valid") {
    Serial.println("Validation of exisiting settings failed. Validation header missing. New settings needed.");
    needNewSettings = true;
  }

  if (!needNewSettings && localSettings.firmware_version != FIRMWARE_VERSION) {
    Serial.println("Validaton of existing settings failed. Config firmware version does not match current firmware version. New settings needed");
    needNewSettings = true;
  }

  if (needNewSettings) {
    Serial.println("New settings needed. Attempting to load new settings.");
    bool writeSettingsSuccess;
    writeNewDeviceSettings(writeSettingsSuccess);
    if (!writeSettingsSuccess) {
      success = false;
      return;
    }
  }

  //Validate WiFi settings
  String ssid = String(localSettings.wifi_ssid);
  String pwd = String(localSettings.wifi_pwd);
  if (ssid.length() == 0 || pwd.length() == 0) {
    Serial.println("Failed to validate WiFi settings");
    success = false;
    return;
  }

  //Validate InfluxDB settings
  String protocol = String(localSettings.influxdb_protocol);
  String host = String(localSettings.influxdb_host);
  String db = String(localSettings.influxdb_db);
  String measurement = String(localSettings.influxdb_measurement);
  String deviceid = String(localSettings.device_id);
  if (protocol.length() == 0 || host.length() == 0 || db.length() == 0
      || measurement.length() == 0 || deviceid.length() == 0) {
    Serial.println("Failed to validate InfluxDB settings");
    success = false;
    return;
  }

  success = true;
}

//Validate existing EEPROM settings are up to date and commit new settings if needed
void writeNewDeviceSettings(bool &success) {
  Serial.println("Attempting to write new device settings...");

  char* otaUrl = "http://otaserverhostname:otaserverport/ota/tempsensor.bin";
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

  String mac_address = WiFi.macAddress().c_str();

  float sensorTempAdjustment;
  float sensorHumdidityAdjustment;
  String deviceId;

  if (mac_address == "AA:AA:AA:AA:AA:AA") {
    sensorTempAdjustment = 0.0;
    sensorHumdidityAdjustment = 0.0;
    deviceId = "SensorA";
  } else if (mac_address == "BB:BB:BB:BB:BB:BB") {
    sensorTempAdjustment = 0.0;
    sensorHumdidityAdjustment = 0.0;
    deviceId = "SensorB";
  } else if (mac_address == "CC:CC:CC:CC:CC:CC") {
    sensorTempAdjustment = 0.0;
    sensorHumdidityAdjustment = 0.0;
    deviceId = "SensorC";
  } else {
    Serial.printf("No configuration data found MAC address: %s\n", WiFi.macAddress().c_str());
    success = false;
    return;
  }

  //TODO bind config version to firmware version so they're updated together
  strcpy(localSettings.validation_string, "Valid"); //DO NOT CHANGE THIS
  localSettings.firmware_version = FIRMWARE_VERSION;

  //OTA URL
  strcpy(localSettings.ota_url, otaUrl);

  //Sensor settings
  localSettings.dht_pin = dhtPin;
  localSettings.dht_power_pin = dhtPowerPin;
  localSettings.sensor_temp_adjustment = sensorTempAdjustment;
  localSettings.sensor_humidity_adjustment = sensorHumdidityAdjustment;
  //WiFi settings
  strcpy(localSettings.wifi_ssid, wifiSSID);
  strcpy(localSettings.wifi_pwd, wifiPwd);
  //InfluxDB settings
  strcpy(localSettings.influxdb_protocol, influxDBProtocol);
  strcpy(localSettings.influxdb_host, influxDBHost);
  localSettings.influxdb_port = influxDBPort;
  strcpy(localSettings.influxdb_db, influxDBDB);
  strcpy(localSettings.influxdb_measurement, influxDBMeasurement);
  strcpy(localSettings.device_id, deviceId.c_str());
  localSettings.send_interval = sendInterval;

  Serial.println("Writing settings to EEPROM...");

  for (unsigned int t = 0; t < sizeof(localSettings); t++)
    EEPROM.write(t, *((char*)&localSettings + t));

  EEPROM.commit();

  success = true;
}

int maxWiFiConnectAttemps = 2;
void connectToWiFi(int attemptNumber) {
  Serial.println("Waiting for WiFi connection...");

  int maxWaitMs = 15 * 1000;
  unsigned long waitAbortMs = millis() + maxWaitMs;
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() >= waitAbortMs) {
      break;
    }
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected to WiFi - IP: %d.%d.%d.%d\n", localIP[0], localIP[1], localIP[2], localIP[3]);
    Serial.printf("WiFi connection time %lums\n", millis() - wifiConnectStart);
    Serial.printf("WiFi signal stregnth: %d dBm\n",  WiFi.RSSI());
    Serial.printf("WiFi channel: %d\n",  WiFi.channel());
  } else if (attemptNumber >= maxWiFiConnectAttemps) {
    Serial.println("Final attempt to connect to WiFi failed.");
  } else {
    Serial.println("Failed to connect to WiFi. Retrying.");
    stopWiFi();
    startWiFi();
    connectToWiFi(attemptNumber + 1);
  }
}

void startWiFi() {
  Serial.println("Waking WiFi...");
  WiFi.forceSleepWake();
  delay(1);

  Serial.println("Connecting to WiFi network...");

  WiFi.hostname("Sensor - " + String(localSettings.device_id));
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false); //Eliminate flash writes
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.begin(localSettings.wifi_ssid, localSettings.wifi_pwd);
  wifiConnectStart = millis();

  //DO NOT USE WiFi.waitForConnectResult() as current implementations use large timeouts
}

// https://github.com/esp8266/Arduino/issues/644
void stopWiFi() {
  Serial.println("Putting WiFi to sleep...");

  WiFi.disconnect();
  delay(1000); //See: https://github.com/esp8266/Arduino/issues/4082
  WiFi.mode(WIFI_OFF);

  WiFi.forceSleepBegin();
  delay(1);
}

void sendData(DHT &dht) {
  Serial.println("-=Send Data Start=-");

  String deviceIdFormatted = localSettings.device_id;
  formatLineProtocol(deviceIdFormatted);

  String temp_influx_url = INFLUXDB_URL;
  temp_influx_url.replace("$PROTOCOL$", localSettings.influxdb_protocol);
  temp_influx_url.replace("$HOST$", localSettings.influxdb_host);
  temp_influx_url.replace("$PORT$", String(localSettings.influxdb_port));
  temp_influx_url.replace("$DB$", localSettings.influxdb_db);

  float temp_data[2];
  readTemp(dht, temp_data);
  float humidity_data = temp_data[0];
  float temperature_data = temp_data[1];

  if (isnan(humidity_data)
      || humidity_data <= 0
      || isnan(temperature_data)
      || temperature_data <= 0) {
    Serial.println("-=Send Data End=-");
    return;
  }

  String temp_data_str = TEMP_DATA_TEMPLATE;
  temp_data_str.replace("$measurement$", localSettings.influxdb_measurement);
  temp_data_str.replace("$deviceid$", deviceIdFormatted);
  temp_data_str.replace("$temperature$", String(temperature_data, 2));
  temp_data_str.replace("$humidity$", String(humidity_data, 2));
  temp_data_str.replace("$firmware$", String(FIRMWARE_VERSION));
  temp_data_str.replace("$wifi_rssi$", String(WiFi.RSSI()));

  Serial.println("InfluxDB URL: " + temp_influx_url);
  Serial.println("InfluxDB Record: " + temp_data_str);

  HTTPClient http;
  http.begin(temp_influx_url);

  int statusCode = http.POST(temp_data_str);
  String rspBody = http.getString();

  Serial.printf("Response Status Code: %d (Success = 204)\n", statusCode);

  if (rspBody != NULL) {
    rspBody.trim();

    if (rspBody.length() > 0) {
      Serial.println("Response Body: " + rspBody);
    }
  }

  http.end();
  Serial.println("-=Send Data End=-");
}

void readTemp(DHT &dht, float temp_data[]) {
  // Grab the current state of the sensor,
  // allowing for a couple of read errors (it happens from time to time)
  long startRead = millis();
  Serial.println("Reading temperature...");
  for (int i = 0; i < 10; i++) {
    float temperature_data = dht.readTemperature(true);
    float humidity_data = dht.readHumidity();


    if (!isnan(temperature_data) && !isnan(humidity_data)) {
      Serial.printf("Sensor read elapsed: %lums\n", millis() - startRead);
      Serial.printf("Orig Temperature: %.2f\n", temperature_data);
      Serial.printf("Orig Humidity: %.2f\n", humidity_data);

      temperature_data += localSettings.sensor_temp_adjustment;
      humidity_data += localSettings.sensor_humidity_adjustment;

      Serial.printf("Adjusted Temperature: %.2f\n", temperature_data);
      Serial.printf("Adjusted Humidity: %.2f\n", humidity_data);

      temp_data[0] = humidity_data;
      temp_data[1] = temperature_data;

      return;
    }
    delay(2000);
  }

  Serial.println("Failed to read temperature & humidity data.");

  temp_data[0] = 0;
  temp_data[1] = 0;
}

void formatLineProtocol(String &input) {
  input.replace(",", "\\,");
  input.replace("=", "\\=");
  input.replace(" ", "\\ ");
  input.replace("\"", "\\\"");
}

void attemptFirmwareUpdate() {
  Serial.println("Checking for firmware update...");

  String OTAUrl = String(localSettings.ota_url);
  if (OTAUrl == NULL || OTAUrl.length() == 0) {
    Serial.println("No configured OTA update URL. Skipping update check.");
    return;
  }

  OTAUrl = OTAUrl + "?deviceid=" + urlencode(String(localSettings.device_id))
           + "&firmware_version=" + String(FIRMWARE_VERSION)
           + "&temp_adjustment=" + String(localSettings.sensor_temp_adjustment, 2)
           + "&humidity_adjustment=" + String(localSettings.sensor_humidity_adjustment, 2)
           + "&wifi_rssi=" + String(WiFi.RSSI());


  //TODO call handleUpdate directly to pass in a HTTPClient with manual timeouts?
  t_httpUpdate_return ret = ESPhttpUpdate.update(OTAUrl, String(FIRMWARE_VERSION));
  Serial.println("Update check complete.");

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Auto-update - Failed - Error(%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Auto-update - No update required");
      break;

    case HTTP_UPDATE_OK:
      //This path will apparently never actually be called (auto reset is part of the update)
      Serial.println("Auto-update - Success");
      break;
  }
}

//Borrowed from https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino
//I needed something quick to get a job done but haven't fully vetted this logic
String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}
