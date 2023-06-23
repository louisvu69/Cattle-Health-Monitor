#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DFRobot_MLX90614.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
//----------------------------
Adafruit_MPU6050 mpu;
WiFiClient client;
DFRobot_MLX90614_I2C sensor;  // instantiate an object to drive our sensor
//---------------------------
const char *server = "api.thingspeak.com";
const char *ssid = "ASUS";  // replace with your WiFi SSID and WPA2 key
const char *pass = "123123123";
const char *apiKey = "27JKXX1XXAF7JULC";  // Enter your Write API key from ThingSpeak
unsigned long channelID = 2199351;        // Replace with your ThingSpeak channel ID
const char *host = "192.168.2.222";
int port = 50000;
// -------------------------
float ambientTemp_Kalman;
float avgTemp_Kalman;
float ambientTemp;
float avgTemperature;
float x, y, z;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("Connected!!!");
  // initialize the sensor
  while (NO_ERR != sensor.begin()) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }
  Serial.println("Begin ok!");
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  // ThingSpeak.begin(client);  // Initialize ThingSpeak library
  /**
   * adjust sensor sleep mode
   * mode select to enter or exit sleep mode, it's enter sleep mode by default
   *      true is to enter sleep mode
   *      false is to exit sleep mode (automatically exit sleep mode after power down and restart)
   */
  sensor.enterSleepMode();
  delay(50);
  sensor.enterSleepMode(false);
  delay(200);
  ThingSpeak.begin(client);  // Initialize ThingSpeak library
}

void loop() {
  Wire.beginTransmission(0x5A);

  float ambientTemp = sensor.getAmbientTempCelsius();

  float objectTemp = sensor.getObjectTempCelsius();
  Wire.endTransmission(0x5A);

  delay(10);
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Wire.beginTransmission(0x68);
  x = a.acceleration.x;
  y = a.acceleration.y;
  z = a.acceleration.z;
  Wire.endTransmission(0x68);

  // print measured data in Celsius
  Serial.print(ambientTemp);
  Serial.print(" ");
  Serial.print(objectTemp);
  Serial.print(" ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.print(z);
  Serial.println();
  /*
  // Data send to thingspeak
  ThingSpeak.setField(1, objectTemp);
  ThingSpeak.setField(2, ambientTemp);
  ThingSpeak.setField(3, x);
  ThingSpeak.setField(4, y);
  ThingSpeak.setField(5, z);
  int httpCode = ThingSpeak.writeFields(channelID, apiKey);
  
  
  if (httpCode == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(httpCode));
  }
  */

  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
    return;
  }

  client.print(objectTemp);
  client.print("    ");
  client.print(ambientTemp);
  client.print("    ");
  client.print(x);
  client.print("    ");
  client.print(y);
  client.print("    ");
  client.println(z);
  delay(10);
}
