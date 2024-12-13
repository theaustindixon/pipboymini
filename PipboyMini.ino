// Austin Dixon's ESP32-S2 Pipboy Mini script

#include <Arduino.h>
#include "Adafruit_MAX1704X.h"
#include <Adafruit_NeoPixel.h>
#include "Adafruit_TestBed.h"
#include <Adafruit_BME280.h>
#include <Adafruit_ST7789.h> 
#include <Fonts/FreeSans12pt7b.h>
#include <TimeLib.h>
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include "http_parser.h"
#include <Arduino_JSON.h>
#include <Adafruit_GFX.h>    // Core graphics library 
#include <SPI.h>
#include <iostream>
#include <ctime>

Adafruit_BME280 bme; // I2C
bool bmefound = false;
extern Adafruit_TestBed TB;

Adafruit_MAX17048 lipo;
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

GFXcanvas16 canvas(240, 135);

const int tiltPin = 5;		// tilt sensor pin is connected to pin 5 (change to whichever pin you use for signal wire)

String json_array;
String weather;
String fullWeather;
String color;
JSONVar weath;
JSONVar temperature;
JSONVar temp_min;
JSONVar temp_max;
JSONVar humidity;
JSONVar wind;

////////////////////////////////////////
// WIFI
const char* ssid       = ""; // your wifi name here
const char* password   = ""; // your wifi password here
////////////////////////////////////////
// TIME SERVER
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -21600; //Central Standard Time (change to your timezone)
const int   daylightOffset_sec = 3600;
////////////////////////////////////////
// GET DATE
void printLocalDate()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain Date");
    return;
  }
  canvas.println(&timeinfo, "%m/%d/%Y");
}
////////////////////////////////////////
// GET TIME
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  canvas.println(&timeinfo, "%I:%M:%S");
}

////////////////////////////////////////
// GET WEATHER
String openWeatherMapApiKey = "";
String city = "";
String countryCode = "US";
String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;

String GET_Request(const char* server) {
  HTTPClient http;    
  http.begin(server);
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return payload;
}
////////////////////////////////////////
// FULL WEATHER DISPLAY
void displayWeather(String color){
  if (color=="main"){
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setCursor(0, 17);
    canvas.setTextColor(ST77XX_RED);
    canvas.println("WEATHER TODAY");

    canvas.setTextColor(ST77XX_MAGENTA);
    canvas.print("Temperature: ");
    canvas.setTextColor(ST77XX_WHITE);
    int temp_f = (int(temperature) - 273) * 1.8 + 32;
    canvas.println(temp_f);

    canvas.setTextColor(ST77XX_MAGENTA);
    canvas.print("Temp Min: ");
    canvas.setTextColor(ST77XX_WHITE);
    int mintemp_f = (int(temp_min) - 273) * 1.8 + 32;
    canvas.println(mintemp_f);

    canvas.setTextColor(ST77XX_MAGENTA);
    canvas.print("Temp Max: ");
    canvas.setTextColor(ST77XX_WHITE);
    int maxtemp_f = (int(temp_max) - 273) * 1.8 + 32;
    canvas.println(maxtemp_f);

    canvas.setTextColor(ST77XX_MAGENTA);
    canvas.print("Wind: ");
    canvas.setTextColor(ST77XX_WHITE);
    canvas.print(wind);
    canvas.print(" mph");

    // finish it up and turn on backlight
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
  }
  else if (color=="green"){
    canvas.fillScreen(0x0200);
    canvas.setCursor(0, 17);
    canvas.setTextColor(0x07E0);
    canvas.println("WEATHER TODAY");

    canvas.print("Temperature: ");
    int temp_f = (int(temperature) - 273) * 1.8 + 32;
    canvas.println(temp_f);

    canvas.print("Temp Min: ");
    int mintemp_f = (int(temp_min) - 273) * 1.8 + 32;
    canvas.println(mintemp_f);

    canvas.print("Temp Max: ");
    int maxtemp_f = (int(temp_max) - 273) * 1.8 + 32;
    canvas.println(maxtemp_f);

    canvas.print("Wind: ");
    canvas.print(wind);
    canvas.print(" mph");

    // finish it up and turn on backlight
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);  
  }
}
////////////////////////////////////////
// Tilt Off Function
void tiltOFF(){
  canvas.fillScreen(ST77XX_BLACK);
  canvas.setTextColor(ST77XX_YELLOW);
  canvas.println("SLEEPING...");

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, LOW);
}
////////////////////////////////////////
// Main WatchFace
void mainWatch(String color){
  if (color=="main"){
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setCursor(0, 17);
    
    // HEADER TEXT
    canvas.setTextColor(ST77XX_RED);
    canvas.println("PIPBOY Mini");
    
    // DATE DISPLAY
    canvas.setTextColor(ST77XX_BLUE);
    canvas.print("Date: ");
    canvas.setTextColor(ST77XX_YELLOW);
    printLocalDate();

    // TIME DISPLAY
    canvas.setTextColor(ST77XX_BLUE);
    canvas.print("Time: ");
    canvas.setTextColor(ST77XX_WHITE);
    printLocalTime();
    
    // WEATHER DISPLAY
    canvas.setTextColor(ST77XX_MAGENTA);
    canvas.print("W: ");
    canvas.setTextColor(ST77XX_WHITE);
    canvas.println(weather);

    // BATTERY LEVEL DISPLAY
    canvas.setTextColor(ST77XX_GREEN); 
    canvas.print("Battery: ");
    canvas.setTextColor(ST77XX_WHITE);
    canvas.print(lipo.cellVoltage(), 1);
    canvas.print(" V  /  ");
    canvas.print(lipo.cellPercent(), 0);
    canvas.println("%");
  }
  else if (color=="green"){
    canvas.fillScreen(0x0200);
    canvas.setTextColor(0x07E0);
    canvas.setCursor(0, 17);
    
    // HEADER TEXT
    canvas.println("PIPBOY Mini");
    
    // DATE DISPLAY
    canvas.print("Date: ");
    printLocalDate();

    // TIME DISPLAY
    canvas.print("Time: ");
    printLocalTime();
    
    // WEATHER DISPLAY
    canvas.print("W: ");
    canvas.println(weather);

    // BATTERY LEVEL DISPLAY
    canvas.print("Battery: ");
    canvas.print(lipo.cellVoltage(), 1);
    canvas.print(" V  /  ");
    canvas.print(lipo.cellPercent(), 0);
    canvas.println("%");

    // finish it up and turn on backlight
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
  }
}
//////////////////////////////////////
// Binary Watchface
void binaryWatch(String color){
  
  if (color=="main"){
    canvas.fillScreen(ST77XX_BLACK);
  }
  else if (color=="green"){
    canvas.fillScreen(0x0200);
  }

  // finish it up and turn on backlight
  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  int hour = (timeinfo.tm_hour);
  int min = (timeinfo.tm_min);
  int sec = (timeinfo.tm_sec);

  if (hour >= 12){
    hour = hour - 12; // convert military time to standard time
  }

  tenhour(hour, color);
  onehour(hour, color);
  twohour(hour, color);
  fourhour(hour, color);
  eighthour(hour, color);
  
  tenmin(min, color);
  twentymin(min, color);
  fourtymin(min, color);

  // get first min digit
  if (min>=10){
    min = (min)%10;
  }

  onemin(min, color);
  twomin(min, color);
  fourmin(min, color);
  eightmin(min, color);

  tensec(sec, color);
  twentysec(sec, color);
  fourtysec(sec, color);

  // get first sec digit
  if (sec>=10){
    sec = (sec)%10;
  }

  onesec(sec, color);
  twosec(sec, color);
  foursec(sec, color);
  eightsec(sec, color);

  delay(1000);

}
////////////////////////////////////////
// Binary Watchface Functions
void tenhour(int hour, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_RED;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 15;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
  if(hour>=10){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void onehour(int hour, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_RED;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 45;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
    if((hour==1) || (hour==3) || (hour==7) || (hour==9) || (hour==11)){
      display.fillRoundRect(x, y, w, h, 5, blockcolor);
    } 
    else{
      display.drawRoundRect(x, y, w, h, 5, blockcolor);
    }
}
void twohour(int hour, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_RED;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 45;
  int y = display.height() - 60;
  int w = 25;
  int h = 25;
  if((hour==2) || (hour==3) || (hour==6) || (hour==7) || (hour==12)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void fourhour(int hour, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_RED;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 45;
  int y = display.height() - 90;
  int w = 25;
  int h = 25;
  if((hour==4) || (hour==5) || (hour==6) || (hour==7)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void eighthour(int hour, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_RED;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 45;
  int y = display.height() - 120;
  int w = 25;
  int h = 25;
  if((hour==8) || (hour==9)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void tenmin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 85;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
  if(((min>=10) && (min<20)) || ((min>=30) && (min<40)) || ((min>=50) && (min<60))){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void twentymin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 85;
  int y = display.height() - 60;
  int w = 25;
  int h = 25;
  if(((min>=20) && (min<40)) || ((min>=30) && (min<40))){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void fourtymin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 85;
  int y = display.height() - 90;
  int w = 25;
  int h = 25;
  if(min>=40){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void onemin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 115;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
  if((min==1) || (min==3) || (min==5) || (min==7) || (min==9)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void twomin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 115;
  int y = display.height() - 60;
  int w = 25;
  int h = 25;
  if((min==2) || (min==3) || (min==6) || (min==7)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void fourmin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 115;
  int y = display.height() - 90;
  int w = 25;
  int h = 25;
  if((min==4) || (min==5) || (min==6) || (min==7)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void eightmin(int min, String col) {
  uint16_t blockcolor;
  if (col=="main"){
    blockcolor = ST77XX_BLUE;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 115;
  int y = display.height() - 120;
  int w = 25;
  int h = 25;
  if((min==8) || (min==9)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void tensec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 155;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
  if(((sec>=10) && (sec<20)) || ((sec>=30) && (sec<40)) || ((sec>=50) && (sec<60))){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void twentysec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 155;
  int y = display.height() - 60;
  int w = 25;
  int h = 25;
  if(((sec>=20) && (sec<40)) || ((sec>=30) && (sec<40))){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void fourtysec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 155;
  int y = display.height() - 90;
  int w = 25;
  int h = 25;
  if(sec>=40){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void onesec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 185;
  int y = display.height() - 30;
  int w = 25;
  int h = 25;
  if((sec==1) || (sec==3) || (sec==5) || (sec==7) || (sec==9)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void twosec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 185;
  int y = display.height() - 60;
  int w = 25;
  int h = 25;
  if((sec==2) || (sec==3) || (sec==6) || (sec==7)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void foursec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 185;
  int y = display.height() - 90;
  int w = 25;
  int h = 25;
  if((sec==4) || (sec==5) || (sec==6) || (sec==7)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
void eightsec(int sec, String col) {
  uint16_t blockcolor;
  if (col=="main"){
  blockcolor = ST77XX_MAGENTA;
  }
  else if (col=="green"){
    blockcolor = ST77XX_GREEN;
  }
  int x = 185;
  int y = display.height() - 120;
  int w = 25;
  int h = 25;
  if((sec==8) || (sec==9)){
    display.fillRoundRect(x, y, w, h, 5, blockcolor);
  } 
  else{
    display.drawRoundRect(x, y, w, h, 5, blockcolor);
  }
}
////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  
  /////////////////////
  // Connect to Wi-Fi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //set the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //grab weather json and parse
  json_array = GET_Request(serverPath.c_str());
  delay(500);
  JSONVar myObject = JSON.parse(json_array);
  
  weath = myObject["weather"][0]["description"];
  weather = String(weath);
  temperature = myObject["main"]["temp"];
  temp_min = myObject["main"]["temp_min"];
  temp_max = myObject["main"]["temp_max"];
  humidity = myObject["main"]["humidity"];
  wind = myObject["wind"]["speed"];

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  /////////////////////
  
  delay(100);

  color = "main";
  watchface = "main";
  fullWeather = "off";

  pinMode(tiltPin, INPUT);		// set sensor pin as an INPUT pin
	digitalWrite(tiltPin, HIGH);	// turn on the built in pull-up resistor

  TB.neopixelPin = PIN_NEOPIXEL;
  TB.neopixelNum = 1; 
  TB.begin();
  TB.setColor(WHITE);

  display.init(135, 240);           // Init ST7789 240x135
  display.setRotation(1);
  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextColor(ST77XX_WHITE); 

  if (!lipo.begin()) {
    Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    while (1) delay(10);
  }
    
  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x")); 
  Serial.println(lipo.getChipID(), HEX);

  if (TB.scanI2CBus(0x77)) {
    Serial.println("BME280 address");

    unsigned status = bme.begin();  
    if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      return;
    }
    Serial.println("BME280 found OK");
    bmefound = true;
  }

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);
  
}

uint8_t j = 0;

void loop() {

  if (digitalRead(tiltPin)) { // check if the pin is high
		 Serial.println("**********************");

    TB.printI2CBusScan();

    if (j % 2 == 0) {
      if (fullWeather=="on"){
        displayWeather(color);
      }
      else if (watchface=="main"){
        mainWatch(color);
      }
      else if (watchface=="binary"){
        binaryWatch(color);
      }

/////////////////////////////////////////////////////////
///// BUTTONS
      Serial.println(digitalRead(0));
      Serial.println(digitalRead(1));
      Serial.println(digitalRead(2));
      
      //////////////////////
      // bottom right button
      if (!digitalRead(0)) { // full weather display
        if (fullWeather=="off"){
          fullWeather = "on";
        }
        else if (fullWeather=="on"){
          fullWeather = "off";
        }
        delay(2000);
      }
      //////////////////////
      // middle right button
      if (digitalRead(1)) { // select color scheme
        if (color=="main"){
          color = "green";// pipboy green watchface
        }
        else if (color=="green"){
          color = "main";
        }
        delay(2000);
      }
      //////////////////////
      // top right button
      if (digitalRead(2)) {
        if (watchface=="main"){
          watchface = "binary";
          fullWeather = "off";
        }
        else if (watchface=="binary"){
          watchface = "main";
          fullWeather = "off";
        }
        delay(2000);
      }
      ////////////////////// 
/////////////////////////////////////////////////////////

      // finish it up and turn on backlight
      display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
      pinMode(TFT_BACKLITE, OUTPUT);
      digitalWrite(TFT_BACKLITE, HIGH);
    }
    
    TB.setColor(TB.Wheel(j++));
    return;
    }

	else { // if tilt sensor is in "off" position, then we should turn off screen
    tiltOFF();
	}
}
