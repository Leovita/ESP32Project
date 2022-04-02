#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <iostream>
#include <cmath>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const char *ssid = "iPhone di Leonardo";
const char *password = "leo123456";
const char *mqttServer = "mqtt.ssh.edu.it";
const int mqttPort = 1883;
const char *mqttUser = "";
const char *mqttPassword = "";
const char *deviceName = "ESP32";
const char *sensorLocation = "Napoleon III Apartments";
const char *topic = "museum/params";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme;

String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup()
{
  Serial.begin(9600);
  bme.begin(0x76);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(23, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);

  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client", mqttUser, mqttPassword))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }

    timeClient.begin();
    timeClient.setTimeOffset(7200);
  }
}

void loop()
{
  timeClient.update();
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

  StaticJsonDocument<200> esp32JSON;
  char buffer[256];
  bool fireAlert = false;
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  String formattedTime = timeClient.getFormattedTime();
  String weekDay = String(ptm->tm_mday);
  String currentYear = String(ptm->tm_year + 1900);
  String currentMonthName = months[ptm->tm_mon];
  String currentDate = String(currentYear) + "-" + String(currentMonthName) + "-" + String(weekDay);

  String tempValue = String(round(bme.readTemperature())) + "C";
  String humValue = String(round(bme.readHumidity())) + "%";

  bool highTemp = (bme.readTemperature() >= 25) ? true : false;
  bool highHum = (bme.readHumidity() >= 45) ? true : false;

  if (bme.readTemperature() >= 26)
  {
    fireAlert = true;
    digitalWrite(23, HIGH);
  }
  // else
  // {
  //   delay(180000) // 3 min?
  // }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(sensorLocation);
  display.display();

  esp32JSON["deviceName"] = deviceName;
  esp32JSON["dateInfo"] = currentDate;
  esp32JSON["timeInfo"] = formattedTime;
  esp32JSON["sensorLocation"] = sensorLocation;
  esp32JSON["tempValue"] = tempValue;
  esp32JSON["humValue"] = humValue;
  esp32JSON["tempAlert"] = highTemp;
  esp32JSON["humAlert"] = highHum;
  esp32JSON["fireAlert"] = fireAlert;

  serializeJson(esp32JSON, buffer);

  if (client.publish("museum/params", buffer))
  {
    Serial.println("Museum parameters got successfully sended!");
  }
  else
  {
    Serial.println("Error sending message");
  }
  delay(10000);
}