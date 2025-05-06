#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>
#include <Scheduler.h>
#include "arduino_secrets.h" //wifi & api key

// === WiFi & API ===
const char* weatherHost = "api.openweathermap.org";
String city = "Brooklyn,US";
String apiKey = YOUR_API_KEY;  // 

// === pinout ===
const int coolWhitePin = 5;
const int warmWhitePin = 6;
const int potPin = A0;
const int buttonPin = 3; 
const int ledPin = 8;  // wifi connection indicator

// === variables ===
float coolRatio = 0.5;
float warmRatio = 0.5;
int currentKelvin = 3500;
bool lightOn = true;

// === button pullup ===
bool lastButtonState = HIGH;
bool buttonState = HIGH;
bool buttonPressed = false;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, weatherHost, 80);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(coolWhitePin, OUTPUT);
  pinMode(warmWhitePin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  connectToWiFi();

  Scheduler.startLoop(dimmerLoop);
  Scheduler.startLoop(checkButtonToggle);
}

void loop() {
  updateWeatherAndSetColor();
  delay(600000);  // update every 10 min
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, HIGH);  // indicator
}

void updateWeatherAndSetColor() {
  String requestURL = "/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric";
  Serial.println("Requesting: " + requestURL);
  client.get(requestURL);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode == 200) {
    JSONVar weatherData = JSON.parse(response);
    if (JSON.typeof(weatherData) == "undefined") {
      Serial.println("Parsing failed!");
      return;
    }

    long currentTime = weatherData["dt"];
    long sunrise = weatherData["sys"]["sunrise"];
    long sunset = weatherData["sys"]["sunset"];
    String weatherMain = (const char*) weatherData["weather"][0]["main"];

    Serial.print("Weather: ");
    Serial.println(weatherMain);

    if (currentTime < sunrise || currentTime > sunset) {
      setColorTemperature(2700);
    } else if (weatherMain == "Clear") {
      setColorTemperature(4500);
    } else if (weatherMain == "Rain" || weatherMain == "Drizzle" || weatherMain == "Thunderstorm") {
      setColorTemperature(2500);
    } else {
      setColorTemperature(3500);
    }

  } else {
    Serial.print("Weather request failed. Status: ");
    Serial.println(statusCode);
  }
}

void setColorTemperature(int kelvin) {
  currentKelvin = kelvin;

  if (kelvin >= 4500) {
    coolRatio = 0.4;
    warmRatio = 0.6;
  } else if (kelvin >= 3500) {
    coolRatio = 0.35;
    warmRatio = 0.65;
  } else if (kelvin >= 2700) {
    coolRatio = 0.2;
    warmRatio = 0.8;
  } else {
    coolRatio = 0.0;
    warmRatio = 1.0;
  }

  Serial.print("Color temp set to ");
  Serial.print(kelvin);
  Serial.print("K | coolRatio=");
  Serial.print(coolRatio, 2);
  Serial.print(", warmRatio=");
  Serial.println(warmRatio, 2);
}

void dimmerLoop() {
  while (true) {
    int potValue = analogRead(potPin);
    int maxPWM = map(potValue, 0, 1023, 0, 255);

    int coolPWM = coolRatio * maxPWM;
    int warmPWM = warmRatio * maxPWM;

    if (lightOn && maxPWM > 5) {
      analogWrite(coolWhitePin, coolPWM);
      analogWrite(warmWhitePin, warmPWM);
      Serial.print("coolWhite: ");
      Serial.println(coolPWM);
      Serial.print("warmWhite: ");
      Serial.println(warmPWM);
    } else {
      analogWrite(coolWhitePin, 0);
      analogWrite(warmWhitePin, 0);
      digitalWrite(coolWhitePin, LOW);
      digitalWrite(warmWhitePin, LOW);
    }

    delay(100);
  }
}

void checkButtonToggle() {
  while (true) {
    int reading = digitalRead(buttonPin);
  
  if (lastButtonState == LOW && reading == HIGH) {
      lightOn = !lightOn;
      Serial.print("Light toggled: ");
      Serial.println(lightOn ? "ON" : "OFF");
    }

    lastButtonState = reading;
    delay(10);  // prevent fast double press
  }
}
