#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DFRobot_MLX90614.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <SimpleKalmanFilter.h>
// Telegram libraries
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
//----------------------------
#define BOT_TOKEN "6333950648:AAHCNhHU8hWNpt-I1VXjdkWHlGSv9bNfmTw"  // Enter the bottoken you got from botfather
#define CHAT_ID "1503923214"                                        // Enter your chatID you got from chatid bot
//----------------------------
Adafruit_MPU6050 mpu;
WiFiClient client;
SimpleKalmanFilter filter(1, 1, 0.001);
DFRobot_MLX90614_I2C mlx;  // instantiate an object to drive our sensor

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

//---------------------------
const char *server = "api.thingspeak.com";
const char *ssid = "P406";  // replace with your WiFi SSID and WPA2 key
const char *pass = "khongnoiday";
const char *apiKey = "27JKXX1XXAF7JULC";  // Enter your Write API key from ThingSpeak
unsigned long channelID = 2199351;        // Replace with your ThingSpeak channel ID
const char *host = "192.168.30.100";
int port = 5000;
//---------------------------
float objectTemp, x, y, z;
float x_Kalman, y_Kalman, z_Kalman;
unsigned long startTime, thingSpeakTimer, MLXTime, MPUTime;
//---------------------------
const float safeThreshold = 29;
const float safeThreshold2 = 38.5;
const unsigned long lowTempDuration = 10 * 1000;  // 5 minutes
const float lowTempThreshold = 0.75;              // 75%
unsigned int lowTempCount = 0;
unsigned int totalCount = 0;



void setup() {
  Serial.begin(115200);
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  secured_client.setTrustAnchors(&cert);  // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  // initialize the sensor
  while (NO_ERR != mlx.begin()) {
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

  ThingSpeak.begin(client);  // Initialize ThingSpeak library

  mlx.enterSleepMode();
  delay(50);
  mlx.enterSleepMode(false);
  delay(200);
  startTime = thingSpeakTimer = MLXTime = MPUTime = millis();
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
  x_Kalman = filter.updateEstimate(x);
  y_Kalman = filter.updateEstimate(y);
  z_Kalman = filter.updateEstimate(z);
  if ((unsigned long)millis() - MLXTime > 50) {
    mlx.enterSleepMode(false);
    Wire.beginTransmission(0x5A);
    objectTemp = mlx.getObjectTempCelsius();
    Wire.endTransmission();
    mlx.enterSleepMode();
    MLXTime = millis();
  }

  // Check if the temperature is below the safe threshold
  if (objectTemp < safeThreshold || objectTemp > safeThreshold2) {
    lowTempCount++;
  }
  totalCount++;
  unsigned long elapsedTime = millis() - startTime;
  // Check if 10s have passed
  if (elapsedTime >= lowTempDuration) {
    float lowTempPercentage = (float)lowTempCount / totalCount;

    if (lowTempPercentage > lowTempThreshold) {
      Serial.println("NGUY HIỂM - TESTING");
      bot.sendMessage(CHAT_ID, "Nhiệt độ đạt ngưỡng nguy hiểm!! BÒ CHẾT BÂY GIỜ", "");
    }

    // Reset variables
    startTime = millis();
    lowTempCount = 0;
    totalCount = 0;
  }

  if ((unsigned long)millis() - thingSpeakTimer > 15000) {
    // Data send to thingspeak
    ThingSpeak.setField(1, objectTemp);
    ThingSpeak.setField(3, x_Kalman);
    ThingSpeak.setField(4, y_Kalman);
    ThingSpeak.setField(5, z_Kalman);
    int httpCode = ThingSpeak.writeFields(channelID, apiKey);

    if (httpCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully.");
    } else {
      Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(httpCode));
    }
    thingSpeakTimer = millis();
  }

  Serial.print(objectTemp);
  Serial.print("    ");
  Serial.print(x_Kalman);
  Serial.print("    ");
  Serial.print(y_Kalman);
  Serial.print("    ");
  Serial.print(z_Kalman);
  Serial.println();

  // delay(50);
}
