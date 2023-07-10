#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DFRobot_MLX90614.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <SimpleKalmanFilter.h>
//----------------------------
Adafruit_MPU6050 mpu;
WiFiClient client;
DFRobot_MLX90614_I2C mlx;  // instantiate an object to drive our sensor
SimpleKalmanFilter filter(1, 1, 0.001);

//---------------------------
const char *server = "api.thingspeak.com";
const char *ssid = "P406";  // replace with your WiFi SSID and WPA2 key
const char *pass = "khongnoiday";
const char *apiKey = "27JKXX1XXAF7JULC";  // Enter your Write API key from ThingSpeak
unsigned long channelID = 2199351;        // Replace with your ThingSpeak channel ID
const char *host = "192.168.30.100";
int port = 5000;
// -------------------------
float ambientTemp_Kalman;
float objectTemp_Kalman;
float ambientTemp;
float objectTemp;
float x_Kalman, y_Kalman, z_Kalman;
float x, y, z;
unsigned long MPUTime, MLXTime, thingSpeakTime;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("Connected!!!");
  // initialize the sensor
  while (NO_ERR != mlx.begin()) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }
  mlx.enterSleepMode();
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("Begin ok!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  ThingSpeak.begin(client);  // Initialize ThingSpeak library
  MPUTime = MLXTime = thingSpeakTime = millis();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  if ((unsigned long)millis() - MPUTime > 20) {
    Wire.beginTransmission(0x68);
    x = a.acceleration.x;
    y = a.acceleration.y;
    z = a.acceleration.z;
    Wire.endTransmission();
    MPUTime = millis();
  }
  if ((unsigned long)millis() - MLXTime > 1000) {
    mlx.enterSleepMode(false);
    Wire.beginTransmission(0x5A);
    ambientTemp = mlx.getAmbientTempCelsius();
    objectTemp = mlx.getObjectTempCelsius();
    Wire.endTransmission();
    mlx.enterSleepMode();
    MLXTime = millis();
  }

  ambientTemp_Kalman = filter.updateEstimate(ambientTemp);
  objectTemp_Kalman = filter.updateEstimate(objectTemp);

  x_Kalman = filter.updateEstimate(x);
  y_Kalman = filter.updateEstimate(y);
  z_Kalman = filter.updateEstimate(z);

  if ((unsigned long)millis() - thingSpeakTime > 15000) {
    // Data send to thingspeak
    ThingSpeak.setField(1, objectTemp);
    ThingSpeak.setField(2, ambientTemp);
    ThingSpeak.setField(3, x_Kalman);
    ThingSpeak.setField(4, y_Kalman);
    ThingSpeak.setField(5, z_Kalman);
    int httpCode = ThingSpeak.writeFields(channelID, apiKey);

    if (httpCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully.");
    } else {
      Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(httpCode));
    }
    thingSpeakTime = millis();
  }
//Kalman Filter
  Serial.print(objectTemp);
  Serial.print("    ");
  Serial.print(x_Kalman);
  Serial.print("    ");
  Serial.print(y_Kalman);
  Serial.print("    ");
  Serial.print(z_Kalman);
  Serial.println();
  Serial.println(millis());

  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
    return;
  }

  client.print(objectTemp);
  client.print("    ");
  client.print(x_Kalman);
  client.print("    ");
  client.print(y_Kalman);
  client.print("    ");
  client.println(z_Kalman);
}
