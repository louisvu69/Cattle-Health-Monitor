#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DFRobot_MLX90614.h>
// Telegram libraries
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
//----------------------------
#define BOT_TOKEN "YOUR TOKKEN YOU GOT FROM BOTFATHER"  // Enter the bottoken you got from botfather
#define CHAT_ID "YOUR ID" // Enter the bottoken you got from IDBot
Adafruit_MPU6050 mpu;
DFRobot_MLX90614_I2C mlx;  // instantiate an object to drive our sensor
//---------------------------
const char *server = "api.thingspeak.com";
const char *ssid = "ssid";  // replace with your WiFi SSID and WPA2 key
const char *pass = "pass";
const char *apiKey = "YOUR API KEY";  // Enter your Write API key from ThingSpeak
unsigned long channelID = 123123;        // Replace with your ThingSpeak channel ID
const char *host = "Your IPV4 address";
int port = 5000;

//---------------------------
float x, y, z, objectTemp;
const float safeThreshold = 31; // set temperature threhold
const float safeThreshold2 = 38.5; // set temperature threhold
const unsigned long dangerTempDuration = 20 * 1000;  // 5 minutes
const float dangerTempThrehold = 0.75;               // 75%
unsigned int dangerTempCount = 0;
unsigned int totalCount = 0;
WiFiClient client;
unsigned long mlxStartTime, MLXTime, MPUTime, thingSpeakTimer;
//--------------------------
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void setup(void) {
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
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  ThingSpeak.begin(client);  // Initialize ThingSpeak library

  while (NO_ERR != mlx.begin()) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }

  Serial.println("Begin ok!");
  while (!Serial)
    delay(10);

  Serial.println("Adafruit MPU6050 test!");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");
  Serial.println("Begin Okay!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.println("");
  mlxStartTime = millis();
  delay(100);
}


void checkTempCondition(const float &objectTemp) {
  // Check if the temperature is below the safe threshold
  if (objectTemp < safeThreshold || objectTemp > safeThreshold2) {
    dangerTempCount++;
  }
  totalCount++;
  unsigned long elapsedTime = millis() - mlxStartTime;
  // Check if 10s have passed
  if (elapsedTime >= dangerTempDuration) {
    float dangerTempPercentage = (float)dangerTempCount / totalCount;

    if (dangerTempPercentage > dangerTempThrehold) {
      Serial.println("NGUY HIỂM - TESTING");
      bot.sendMessage(CHAT_ID, "Nhiệt độ đạt ngưỡng nguy hiểm!! TIẾN HÀNH KIỂM TRA BÒ NGAY LẬP TỨC", "");
    }

    // Reset variables
    mlxStartTime = millis();
    dangerTempCount = 0;
    totalCount = 0;
  }
  MPUTime = MLXTime = thingSpeakTimer = millis();
}

void sendMessageToThingspeak(const float &objectTemp, const float &x, const float &y, const float &z) {
  ThingSpeak.setField(1, objectTemp);
  ThingSpeak.setField(3, x);
  ThingSpeak.setField(4, y);
  ThingSpeak.setField(5, z);
  int httpCode = ThingSpeak.writeFields(channelID, apiKey);
  if (httpCode == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  }
}

void checkTemperatureAndAccelermeter() {
  // Thời gian bắt đầu đếm
  unsigned long startTime = millis();
  objectTemp = mlx.getObjectTempCelsius();
  // Biến đếm thời gian thực
  unsigned long realTimeCount = 0;

  // Trạng thái
  int state = 0;

  // Lấy giá trị gia tốc ban đầu
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  x = a.acceleration.x;
  y = a.acceleration.y;
  z = a.acceleration.z;
  // Đếm thời gian và kiểm tra điều kiện

  while (realTimeCount < 30 * 1000) {
    if ((unsigned long)millis() - MPUTime > 20) {
      Wire.beginTransmission(0x68);
      x = a.acceleration.x;
      y = a.acceleration.y;
      z = a.acceleration.z;
      Wire.endTransmission();
      MPUTime = millis();
    }
    if ((unsigned long)millis() - MLXTime > 50) {
      mlx.enterSleepMode(false);
      Wire.beginTransmission(0x5A);
      objectTemp = mlx.getObjectTempCelsius();
      Wire.endTransmission();
      mlx.enterSleepMode();
      MLXTime = millis();
    }
    sendMessageToClient(objectTemp, x, y, z);
    if ((unsigned long)millis() - thingSpeakTimer > 15000) {
      sendMessageToThingspeak(objectTemp, x, y, z);
      thingSpeakTimer = millis();
    }
    checkTempCondition(objectTemp);

    // Đọc giá trị gia tốc mới
    mpu.getEvent(&a, &g, &temp);
    // In ra các giá trị gia tốc
    Serial.print(objectTemp);
    Serial.print("   ");
    Serial.print("X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(a.acceleration.z);
    Serial.println("");
    delay(20);

    // Kiểm tra sự thay đổi
    if (abs(a.acceleration.x - x) >= 4.0 || abs(a.acceleration.y - y) >= 4.0 || abs(a.acceleration.z - z) >= 4.0) {
      // Reset biến đếm thời gian và thời gian bắt đầu
      realTimeCount = 0;
      startTime = millis();
    }

    // Tính thời gian thực
    realTimeCount = millis() - startTime;

    // Nếu thời gian thực vượt quá 30 giây
    if (realTimeCount >= 15 * 1000) {
      // Nếu các giá trị không thay đổi
      if (state == 0) {
        Serial.println("BÒ ĐANG LƯỜI VẬN ĐỘNG");
        bot.sendMessage(CHAT_ID, "BÒ ĐANG LƯỜI VẬN ĐỘNG", "");
        // // Reset toàn bộ các biến về giá trị ban đầu
        x = a.acceleration.x;
        y = a.acceleration.y;
        z = a.acceleration.z;
        realTimeCount = 0;
        startTime = millis();
        state = 0;
      } else {
        // Đặt trạng thái là 1
        state = 1;
      }
    }
  }
}


void sendMessageToClient(const float &objectTemp, const float &x, const float &y, const float &z) {
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
    return;
  }

  client.print(objectTemp);
  client.print("    ");
  client.print(x);
  client.print("    ");
  client.print(y);
  client.print("    ");
  client.println(z);
}
void loop() {
  checkTemperatureAndAccelermeter();
}
