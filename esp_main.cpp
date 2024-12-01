#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6vDy3JGgi"
#define BLYNK_TEMPLATE_NAME "ESP32 Main"
#define BLYNK_AUTH_TOKEN "hWv5ybBN5tI8L9qmAuemgcieHK8yNsas"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <map>
String Web_App_URL = "https://script.google.com/macros/s/AKfycbyOwknxHK9e47ERILi6CA0gT0_ctp3e8lwaZAdBD6ht7DvBNV_qEiPRpvS-bJUZHquP/exec";
const char *ssid = "GalaxyWarich";
const char *password = "Warich2264";
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // Offset for Thailand (GMT+7)
const int maxRetries = 20;
#include <Arduino.h>
#include <BlynkSimpleEsp32.h>
#include <cmath>
double Humidity = 0.0;
double Temperature = 0.0;
double PM25 = 0.0;
double PM10 = 0.0;
double Light = 0.0;
double Sound = 0.0;
double Carbon = 0.0;
int Outlier = 0;

int last1 = 0;
int last1buf = 0;
int last2 = 0;
int last2buf = 0;

BlynkTimer timer;
// void serverTick();

int retryCount = 0;
void setup()
{
  Serial.begin(115200);
  // Blynk.begin(BLYNK_AUTH_TOKEN, "Rb-i", "frxsne5vz837k");
  // timer.setInterval(10000L, serverTick);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("Retry: ");
    lcd.print(++retryCount);
  }
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(1000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Restart device");
    while (true);
  }
  timeClient.begin();
  lcd.clear();
}

// void serverTick()
// {
//   Blynk.syncVirtual(V11, V12);
// }

BLYNK_WRITE(V11)
{
  Serial.println("New data from Node 1");
  double raw_data[5];
  for (int i = 0; i < 5; i++)
  {
    raw_data[i] = param[i].asDouble();
  }
  Serial.printf("%.2f %.2f %.2f %.2f %.2f", raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4]);
  Serial.println();

  Blynk.virtualWrite(V0, raw_data[0]);
  Blynk.virtualWrite(V1, raw_data[1]);
  Blynk.virtualWrite(V2, raw_data[2]);
  Blynk.virtualWrite(V3, raw_data[3]);
  Blynk.virtualWrite(V7, raw_data[4]);
  Blynk.virtualWrite(V9, last1 != int(raw_data[4]));

  Humidity = raw_data[0];
  Temperature = raw_data[1];
  PM25 = raw_data[2];
  PM10 = raw_data[3];

  last1 = last1buf;
  last1buf = raw_data[4];
}

BLYNK_WRITE(V12)
{
  Serial.println("New data from Node 2");
  double raw_data[4];
  for (int i = 0; i < 4; i++)
  {
    raw_data[i] = param[i].asDouble();
  }
  Serial.printf("%.2f %.2f %.2f %.2f", raw_data[0], raw_data[1], raw_data[2], raw_data[3]);
  Serial.println();

  Blynk.virtualWrite(V4, raw_data[0]);
  Blynk.virtualWrite(V5, raw_data[1]);
  Blynk.virtualWrite(V6, raw_data[2]);
  Blynk.virtualWrite(V8, raw_data[3]);
  Blynk.virtualWrite(V10, last2 != int(raw_data[3]));

  Light = raw_data[0];
  Sound = raw_data[1];
  Carbon = raw_data[2];

  last2 = last2buf;
  last2buf = raw_data[3];
}

double value = 0.0;

std::map<int, std::pair<String, String>> map_int_toString = {
    {0, {"Humidity", "%"}},
    {1, {"Temp", "C"}},
    {2, {"PM25", "ppm"}},
    {3, {"PM10", "ppm"}},
    {4, {"Light", "lx"}},
    {5, {"Sound", ""}},
    {6, {"Carbon", "ppm"}},
};

void rule_base_ai(double temp) {
    double mean = 32.67;
    double std = 14.71; 
    double n = 10.0;
    // Calculate z-score
    double z_score = (temp - mean) / (std / sqrt(n));
    if (abs(z_score) >= 2.576) {
        Outlier = 1; // If probability is below 0.01, it's considered an outlier
    } else {
        Outlier = 0; // Otherwise, it's not an outlier
    }
}


const char DEGREE_SYMBOL = (char)223; 
void updateLCDRow1(int map, double value)
{
  if (map_int_toString.find(map) != map_int_toString.end())
  {
    lcd.setCursor(0, 1); // Set cursor for the second row (line 1)
    lcd.print(map_int_toString[map].first);
    lcd.print(":");
    lcd.print(value);
    if (map_int_toString[map].first == "Temp")
    { 
      rule_base_ai(value);
      lcd.print(DEGREE_SYMBOL);
    }
    lcd.print(map_int_toString[map].second);
    if (Outlier == 1 & map_int_toString[map].first == "Temp") {
      lcd.print(" *");
    }
  }
}

unsigned long lastTimeUpdate = 0;              // Time when the time was last updated
unsigned long lastGoogleUpdate = 0;  
const unsigned long timeUpdateInterval = 1000; // Update time every 1 second
const unsigned long googleSheetInterval = 10000; // Update Google Sheets every 10 seconds
unsigned long currentMillis = 0;
int int_Value = 0;

void loop()
{
  // Blynk.run();
  // timer.run();
  currentMillis = millis();
  
  if (currentMillis - lastTimeUpdate >= timeUpdateInterval)
  {
    lcd.clear();
    // Update time
    timeClient.update();
    String currentTime = timeClient.getFormattedTime(); // Format: HH:MM:SS
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(currentTime);
    // Update sensor value on LCD Row1
    std::vector<double> sensorValues = {Humidity, Temperature, PM25, PM10, Light, Sound, Carbon};
    int_Value = (int_Value+1)%sensorValues.size();
    value = sensorValues[int_Value];
    updateLCDRow1(int_Value, value);
    lastTimeUpdate = currentMillis;
  }

  if (currentMillis - lastGoogleUpdate >= googleSheetInterval && WiFi.status() == WL_CONNECTED)
  {
    timeClient.update();
    String currentTime = timeClient.getFormattedTime(); // Format: HH:MM:SS
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(currentTime);
    lcd.print(" *");
    int_Value = 0;

    // Create a URL for sending or writing data to Google Sheets.
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL = Send_Data_URL + "&humd=" + String(Humidity);
    Send_Data_URL = Send_Data_URL + "&temp=" + String(Temperature);
    Send_Data_URL = Send_Data_URL + "&pm25=" + String(PM25);
    Send_Data_URL = Send_Data_URL + "&pm10=" + String(PM10);
    Send_Data_URL = Send_Data_URL + "&light=" + String(Light);
    Send_Data_URL = Send_Data_URL + "&sound=" + String(Sound);
    Send_Data_URL = Send_Data_URL + "&carbon=" + String(Carbon);
    Send_Data_URL = Send_Data_URL + "&outlier=" + String(Outlier);

    // Initialize HTTPClient as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET();
    http.end();
    lastGoogleUpdate = currentMillis;
  }
}