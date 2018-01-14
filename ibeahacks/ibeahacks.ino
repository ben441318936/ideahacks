#include <Servo.h>

//sensor
#include <Adafruit_HTU21DF.h>
#include <Wire.h>

#include <Time.h>
#include <TimeLib.h>

#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>

//LCD
//#include <LiquidCrystal.h>
//LiquidCrystal lcd(12,11,5,4,3,2);

Servo servo;

#define JSON_BUFF_DIMENSION 2500
//#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "AndroidAP";        // your network SSID (name)
char pass[] = "Ben441318936";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

String apiKey = "8e930b59cb64efd628888e8fd438cef0";

String text;
int endResponse = 0;
boolean startJson = false;

boolean timeToWater = false;
boolean willBeRain = false;

int timeStart = millis();
int timeElapsed = 0;
int humAvg = 0;
int dataPoints = 1;
boolean isHumLow = false;

int humSum = 0;

int status = WL_IDLE_STATUS;

const char server[] = "api.openweathermap.org";    // name address for openweathermap (using DNS)

WiFiClient client;

/////////////////////////////////////////////////


unsigned long lastConnectionTime = 10 * 60 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 2 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)
String countryCode = "5368361"; // LA:5368361 London:2643741
const float MEASURE_INT = 0.5; // number of minutes between humidity readings
const int NUM_DATA = 2; // number of data points to collect



////////////////////////////////////////////////

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
float sensTemp = 0;
float sensHum = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial);
  //lcd.begin(16, 2);
  //lcd.clear();
  Serial.println("Serial initialised");
  //lcd.print("LCD initialised");
  delay(2000);
  //lcd.clear();

  servo.attach(9);
  servo.write(90);

  text.reserve(JSON_BUFF_DIMENSION);
  Serial.println("json buff loaded");

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();

  if (!htu.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
}

boolean stopplz = false;
boolean getSensor = false;

void loop() {

  // if ten minutes have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    getSensor = false;
    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println("Start http request");
    httpRequest();
  }

  /*if (!client.available() && !stopplz){
    Serial.println("No incoming data");
    }
    while (client.available()) {
    stopplz = true;
    char c = client.read();
    Serial.write(c);
    }*/

  char c = 0;
  if (client.available()) {
    stopplz = true;
    c = client.read();
    // json contains equal number of open and close curly brackets, therefore by counting
    // the open and close occurences, we can determine when a json is completely received

    if (c == '{') {
      startJson = true;         // set startJson true to indicate json message has started
      endResponse++;
      //Serial.println("Open curly");
    }
    if (c == '}') {
      endResponse--;
      //Serial.println("Close curly");
    }
    if (startJson == true) {
      text += c;
    }
    //Serial.println(endResponse);
  }
  else {
    if (!stopplz) {
      Serial.println("No incoming data");
      delay(500);
    }
  }
  // endResponse == 0 means equal number of open and close curly brackets reached
  if (endResponse == 0 && startJson == true) {
    //Serial.println("Done counting braces, parsing now...");
    parseJson(text.c_str());  // parse c string text in parseJson function
    text = "";                // clear text string for the next time
    startJson = false;        // set startJson to false to indicate that a new message has not yet started
    getSensor = true;
  }

  if (getSensor) {
    timeElapsed = millis() - timeStart;
    if (timeElapsed >= MEASURE_INT * 60 * 1000) {
      Serial.println("Num data points: " + String(dataPoints));
      sensHum = htu.readHumidity();
      humSum += sensHum;
      if (dataPoints >= NUM_DATA) {
        Serial.println("Time to move the servo...");
        dataPoints = 1;
        humSum /= NUM_DATA + 1;
        Serial.println("Avg humidity: " + String(humSum));
        if (!willBeRain && humSum < 70) {
          servo.write(0); // open valve
          delay(140);
          servo.write(90);
          delay(-1000 / 7 * humSum + 15000);
          servo.write(180);
          // delay(150);
          // servo.write(90);
          for (int pos = 0; pos <= 90; pos++) { 
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(2);                       // waits 15ms for the servo to reach the position
          }
        }
        humSum = 0;
        timeElapsed = 0;
      }
      else {
        dataPoints++;
        Serial.println("Got a data point: " + String(sensHum));
      }
      timeStart = millis();
    }

    /*lcd.setCursor(0,0);
      lcd.print(String(sensTemp));
      lcd.setCursor(0,1);
      lcd.print(String(sensHum));*/
  }

  /*if (timeToWater) {
    // code for servo
    //lcd.print("Watering...");
    digitalWrite(8, HIGH);
    timeToWater = false;
    }*/
}

void parseJson(const char * jsonString) {
  StaticJsonBuffer<4000> jsonBuffer;

  // FIND FIELDS IN JSON TREE
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonArray& list = root["list"];
  JsonObject& hour3 = list[0];
  JsonObject& hour6 = list[1];
  JsonObject& hour9 = list[2];
  JsonObject& hour12 = list[3];

  String city = root["city"]["name"];

  String timeHour3 = hour3["dt_txt"];
  float tempHour3 = hour3["main"]["temp"];
  float humidityHour3 = hour3["main"]["humidity"];
  String weatherHour3 = hour3["weather"][0]["main"];


  String timeHour6 = hour6["dt_txt"];
  float tempHour6 = hour6["main"]["temp"];
  float humidityHour6 = hour6["main"]["humidity"];
  String weatherHour6 = hour6["weather"][0]["main"];

  String timeHour9 = hour9["dt_txt"];
  float tempHour9 = hour9["main"]["temp"];
  float humidityHour9 = hour9["main"]["humidity"];
  String weatherHour9 = hour9["weather"][0]["main"];

  String timeHour12 = hour12["dt_txt"];
  float tempHour12 = hour12["main"]["temp"];
  float humidityHour12 = hour12["main"]["humidity"];
  String weatherHour12 = hour12["weather"][0]["main"];

  if (weatherHour3 == "Rain" || weatherHour6 == "Rain" || weatherHour9 == "Rain" || weatherHour12 == "Rain") {
    willBeRain = true;
  }

  /*
    // convert to PST
    struct tm ttmm;
    time_t epoch;
    char date[100];
    if(strptime(timeHour3, "%Y-%m-%d %H:%M:%S", &ttmm) != NULL) {
      epoch = mktime(&ttmm);
      epoch = (long)epoch - 8 * 3600; // UTC-8
      strftime(date, sizeof(date), "%Y-%m-%d", ttmm);
      Serial.println(epoch);
    }*/


  printWeather(timeHour3, tempHour3, humidityHour3, weatherHour3, "*C");
  printWeather(timeHour6, tempHour6, humidityHour6, weatherHour6, "*C");
  printWeather(timeHour9, tempHour9, humidityHour9, weatherHour9, "*C");
  printWeather(timeHour12, tempHour12, humidityHour12, weatherHour12, "*C");
  Serial.println("Will be rain: " + String(willBeRain));

  Serial.println();

}

void printWeather(String dt_txt, float temp, float humidity, String weather, String unit) {
  Serial.println(dt_txt);
  Serial.println("Temperature: " + String(temp) + unit);
  Serial.println("Humidity: " + String(humidity) + "%");
  Serial.println("Weather: " + weather);
  Serial.println("");
}

void isTimeToWater(float currHum, boolean willRain) {
  if (!willRain) {
    if (currHum < 50) {
      timeToWater = true;
    }
  }

  /*
     if(willRain){
       timeToWater = true;
     }
  */
}

void printTime(float dt) {
  dt = dt;
  int hr = hour(dt);
  int mi = minute(dt);
  int da = day(dt);
  int mon = month(dt);
  int yr = year(dt);

  String monString = "";

  switch (mon) {
    case 1: monString = "January";
    case 2: monString = "Febuary";
    case 3: monString = "March";
    case 4: monString = "April";
    case 5: monString = "May";
    case 6: monString = "June";
    case 7: monString = "July";
    case 8: monString = "August";
    case 9: monString = "September";
    case 10: monString = "October";
    case 11: monString = "November";
    case 12: monString = "December";
    default: monString = "January";
  }

  Serial.println(String(hr) + ":" + String(mi) + " " + monString + " " + String(da) + ", " + String(yr));
}

// this method makes a HTTP connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.print("GET /data/2.5/forecast?");
    client.print("id=" + countryCode);
    client.print("&appid=" + apiKey);
    client.print("&cnt=4"); // data is for every 3 hours, cnt=4 gives 12-hour look ahead
    client.println("&units=metric HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    //client.println("Connection: close");
    client.println();
  } else {
    Serial.println("unable to connect");
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
