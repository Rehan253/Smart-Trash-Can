#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
 
// LCD
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);
 
// ESP8266 on D2, D3
SoftwareSerial espSerial(2, 3);
// D2 = Arduino RX  <- ESP TX
// D3 = Arduino TX  -> ESP RX (through voltage divider)
 
// Sensors
#define TRIG 10
#define ECHO 11
#define TILT 12
#define LDR A0
#define WATER A1
 
// Alerts
#define LED 13
#define BUZZER A2
 
bool waitingForEmpty = false;
unsigned long tiltStartTime = 0;
 
// CHANGE THESE
const char* WIFI_SSID = "AsawiriPhone";
const char* WIFI_PASS = "password123";
const char* SERVER_IP = "10.42.246.94";
const int SERVER_PORT = 8000;
 
long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
 
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
 
  long duration = pulseIn(ECHO, HIGH, 30000);
 
  if (duration == 0) {
    return 999;
  }
 
  long distance = duration * 0.034 / 2;
  return distance;
}
 
void sendCommand(String cmd, int waitTime) {
  espSerial.println(cmd);
  Serial.print("Sending command: ");
  Serial.println(cmd);
  delay(100);
  long start = millis();
 
  while (millis() - start < waitTime) {
    while (espSerial.available()) {
      Serial.write(espSerial.read());
    }
  }
}
 
void scanWiFiNetworks() {
  Serial.println("\n--- Scanning for WiFi Networks ---");
  espSerial.println("AT+CWLAP");
 
  String resp = "";
  long start = millis();
  while (millis() - start < 15000) {
    while (espSerial.available()) {
      char c = espSerial.read();
      resp += c;
      Serial.write(c);
    }
    if (resp.indexOf("OK") != -1) break;
  }
 
  if (resp.indexOf("CWLAP") == -1) {
    Serial.println("No networks found or scan failed.");
  }
  Serial.println("--- Scan Complete ---\n");
}
 
bool connectWiFi() {
  sendCommand("AT", 2000);
  sendCommand("AT+CWMODE=1", 2000);
  sendCommand("AT+CWQAP", 2000);
 
  scanWiFiNetworks();
 
  delay(1000);
 
  String joinCmd = "AT+CWJAP=\"";
  joinCmd += WIFI_SSID;
  joinCmd += "\",\"";
  joinCmd += WIFI_PASS;
  joinCmd += "\"";
 
  espSerial.println(joinCmd);
  Serial.println("Joining WiFi...");
  Serial.println(joinCmd);
 
  String resp = "";
  long start = millis();
  while (millis() - start < 20000) {
    while (espSerial.available()) {
      char c = espSerial.read();
      resp += c;
      Serial.write(c);
    }
    if (resp.indexOf("GOT IP") != -1) {
      Serial.println("\nWiFi connected!");
      sendCommand("AT+CIPMUX=0", 2000);
      sendCommand("AT+CIFSR", 3000);
      return true;
    }
    if (resp.indexOf("FAIL") != -1) {
      Serial.println("\nWiFi FAILED - check SSID/password");
      return false;
    }
  }
 
  Serial.println("\nWiFi timeout - no response");
  return false;
}
 
void sendDataToServer(long distance, int tilt, int lightValue, int waterValue) {
  String url = "/update?distance=" + String(distance) +
               "&tilt=" + String(tilt) +
               "&light=" + String(lightValue) +
               "&water=" + String(waterValue);
 
  String httpRequest = "GET " + url + " HTTP/1.1\r\n";
  httpRequest += "Host: " + String(SERVER_IP) + ":" + String(SERVER_PORT) + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
 
  // Force-close any existing connection first
  sendCommand("AT+CIPCLOSE", 1000);
  delay(500);
 
  // Start TCP connection
  String cipStart = "AT+CIPSTART=\"TCP\",\"";
  cipStart += SERVER_IP;
  cipStart += "\",";
  cipStart += SERVER_PORT;
 
  espSerial.println(cipStart);
  Serial.println("Sending: " + cipStart);
 
  // Wait for "CONNECT" — bail out if ERROR
  String resp = "";
  long start = millis();
  while (millis() - start < 5000) {
    while (espSerial.available()) {
      char c = espSerial.read();
      resp += c;
      Serial.write(c);
    }
    if (resp.indexOf("CONNECT") != -1) break;
    if (resp.indexOf("ERROR") != -1 || resp.indexOf("ALREADY") != -1) {
      Serial.println("TCP connect failed, aborting.");
      return;
    }
  }
 
  delay(200);
 
  // Send length
  sendCommand("AT+CIPSEND=" + String(httpRequest.length()), 2000);
 
  delay(500); // Allow ESP8266 to settle before waiting for ">"
 
  // Wait for ">" prompt
  resp = "";
  start = millis();
  while (millis() - start < 3000) {
    while (espSerial.available()) {
      char c = espSerial.read();
      resp += c;
      Serial.write(c);
    }
    if (resp.indexOf(">") != -1) break;
  }
 
  if (resp.indexOf(">") == -1) {
    Serial.println("No prompt received, aborting.");
    sendCommand("AT+CIPCLOSE", 1000);
    return;
  }
 
  // Send HTTP request
  espSerial.print(httpRequest);
  Serial.println("Sent: " + httpRequest);
 
  // Wait for server response
  start = millis();
  while (millis() - start < 5000) {
    while (espSerial.available()) {
      Serial.write(espSerial.read());
    }
  }
 
  sendCommand("AT+CIPCLOSE", 2000);
}
 
void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(TILT, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
 
  lcd.begin(16, 2);
  Serial.begin(9600);
  espSerial.begin(9600);
 
  lcd.print("SMART BIN SYS");
  delay(2000);
  lcd.clear();
 
  lcd.print("Connecting...");
  Serial.println("Connecting WiFi...");
 
  while (!connectWiFi()) {
    Serial.println("Retrying in 5s...");
    lcd.clear();
    lcd.print("WiFi Failed...");
    delay(5000);
  }
 
  lcd.clear();
  lcd.print("WiFi OK!");
  delay(1000);
}
 
void loop() {
  long distance = readDistance();
  bool tiltState = (digitalRead(TILT) == LOW);
  int lightValue = analogRead(LDR);
  int waterValue = analogRead(WATER);
 
  if (distance < 7 && waitingForEmpty == false) {
    waitingForEmpty = true;
  }
 
  if (waitingForEmpty) {
    if (tiltState) {
      if (tiltStartTime == 0) {
        tiltStartTime = millis();
      }
      if (millis() - tiltStartTime >= 3000) {
        waitingForEmpty = false;
        tiltStartTime = 0;
      }
    } else {
      tiltStartTime = 0;
    }
  }
 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dist:");
  lcd.print(distance);
  lcd.print("cm");
 
  lcd.setCursor(0, 1);
  if (waitingForEmpty) {
    lcd.print("FULL! EMPTY IT");
    digitalWrite(LED, HIGH);
    digitalWrite(BUZZER, HIGH);
  } else {
    lcd.print("BIN OK");
    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);
  }
 
  Serial.println("Sending data to server...");
  sendDataToServer(distance, tiltState ? 1 : 0, lightValue, waterValue);
 
  delay(10000);
}