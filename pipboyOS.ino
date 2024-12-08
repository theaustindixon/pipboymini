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


Adafruit_BME280 bme; // I2C
bool bmefound = false;
extern Adafruit_TestBed TB;

Adafruit_MAX17048 lipo;
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

GFXcanvas16 canvas(240, 135);

const int tiltPin = 5;		// tilt sensor pin is connected to pin 5 (change to whichever pin you use for signal wire)

String json_array;
String weather;
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
String openWeatherMapApiKey = ""; // your API key
String city = ""; // your city
String countryCode = ""; // your country
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

/////////////////////////////////////////////////////////
// BUTTONS
      Serial.println(digitalRead(0));
      Serial.println(digitalRead(1));
      Serial.println(digitalRead(2));
      
      if (!digitalRead(0)) {
        // FULL WEATHER DISPLAY
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

        TB.setColor(TB.Wheel(j++));
        delay(10000);
        return;

      }
      if (digitalRead(1)) {
        //canvas.print("D1, ");
      }
      if (digitalRead(2)) {
        //canvas.print("D2, ");
      }
/////////////////////////////////////////////////////////

      // finish it up and turn on backlight
      display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
      pinMode(TFT_BACKLITE, OUTPUT);
      digitalWrite(TFT_BACKLITE, HIGH);
    }
    
    TB.setColor(TB.Wheel(j++));
    delay(10);
    return;
    }

	else { // if tilt sensor is in "off" position, then we should turn off screen to save battery
    TB.printI2CBusScan();

		canvas.fillScreen(ST77XX_BLACK);
    canvas.setTextColor(ST77XX_YELLOW);
    canvas.println("SLEEPING...");

    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, LOW);

    TB.setColor(TB.Wheel(j++));
    delay(10);
    return;
	}
 
}
