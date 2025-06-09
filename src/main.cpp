#include <Arduino.h>
#include <SoftwareSerial.h>
#include "secrets.h"

// Pin definitions
#define trigPin 9
#define echoPin 8
#define espRx 2
#define espTx 3

SoftwareSerial esp8266(espRx, espTx);


// Function prototypes
void resetESP();
void connectWiFi();
float readDistanceCM();
void sendToThingSpeak(float distance);
void sendCommand(const char* cmd, int delayTime);
void flushESP();
bool waitForResponse(const char* expected, unsigned long timeout);
bool waitForMultipleResponses(String expected1, String expected2, unsigned long timeout);
void clearBuffer();

void setup() {
  Serial.begin(9600);
  esp8266.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println("System starting...");
  resetESP();
  connectWiFi();
}

void loop() {
  float distance = readDistanceCM();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // ALERT if distance exceeds threshold
  if (distance > 10.0) {
    Serial.println("⚠️ ALERT: Water level LOW (Distance > 10 cm)");
  }

  sendToThingSpeak(distance);
  delay(20000); // 20 seconds between updates
}


// === Utility Functions ===

float readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void resetESP() {
  clearBuffer();
  flushESP();
  sendCommand("AT+RST\r\n", 2000);
  if (waitForMultipleResponses("ready", "WIFI GOT IP", 7000)) {
    Serial.println("ESP8266 Reset successful.");
  } else {
    Serial.println("ESP8266 Reset failed or timed out.");
  }
  flushESP();
}

void connectWiFi() {
  clearBuffer();
  flushESP();
  sendCommand("AT+CWMODE=1\r\n", 1000);
  waitForResponse("OK", 3000);
  flushESP();

  String cmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + String(WIFI_PASSWORD) + "\"\r\n";
  sendCommand(cmd.c_str(), 6000);

  if (waitForMultipleResponses("WIFI GOT IP", "CONNECTED", 10000)) {
    Serial.println("WiFi Connected.");
  } else {
    Serial.println("WiFi connection failed.");
  }
  flushESP();
}

void sendToThingSpeak(float distance) {
  clearBuffer();

  String data = "GET /update?api_key=" + String(THINGSPEAK_API_KEY) +
              "&field1=" + String(distance) +
              " HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\n\r\n";

  sendCommand("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n", 2000);
  if (!waitForResponse("OK", 5000)) {
    Serial.println("Failed to start TCP connection");
    return;
  }

  String sendLength = "AT+CIPSEND=" + String(data.length()) + "\r\n";
  sendCommand(sendLength.c_str(), 1000);
  if (!waitForResponse(">", 3000)) {
    Serial.println("No prompt for data");
    return;
  }

  esp8266.print(data);
  delay(2000);

  flushESP();
  sendCommand("AT+CIPCLOSE\r\n", 1000);
  Serial.println("Data sent to ThingSpeak.");
}

void sendCommand(const char* cmd, int delayTime) {
  flushESP();
  esp8266.print(cmd);
  delay(delayTime);
}

void flushESP() {
  while (esp8266.available()) {
    Serial.write(esp8266.read());
  }
}

bool waitForResponse(const char* expected, unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (esp8266.available()) {
      char c = esp8266.read();
      response += c;
      if (response.indexOf(expected) != -1) {
        return true;
      }
    }
  }
  Serial.println("Full response: " + response);
  return false;
}

bool waitForMultipleResponses(String expected1, String expected2, unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (esp8266.available()) {
      char c = esp8266.read();
      response += c;
      if (response.indexOf(expected1) != -1 || response.indexOf(expected2) != -1) {
        return true;
      }
    }
  }
  Serial.println("Full response: " + response);
  return false;
}

void clearBuffer() {
  while (esp8266.available()) esp8266.read();
}
