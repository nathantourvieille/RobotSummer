//#include <WiFi.h>
#include <esp_wpa2.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
//Custom files
#include <wifiHandler.h>
#include <motor.h>

// Function declarations
void loadConfig();

//---------------------Wifi Stuff (For Debugging)------------------
const char* SSID = "ubcsecure";
String USERNAME;
String PASSWORD;

const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

//-------------------Pin Definitions--------------
const int pwmPinA1 = 23;
const int pwmChannelA1 = 0;
const int pwmPinA2 = 22;
const int pwmChannelA2 = 1;

const int pwmPinB1 = 19;
const int pwmChannelB1 = 3;
const int pwmPinB2 = 18;
const int pwmChannelB2 = 4;

const int wifiLEDPin = 16;

Motor motorA (pwmPinA1, pwmPinA2, pwmChannelA1, pwmChannelA2);
Motor motorB (pwmPinB1, pwmPinB2, pwmChannelB1, pwmChannelB2);

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    //-----------Pins--------
    pinMode(wifiLEDPin, OUTPUT);
    digitalWrite(wifiLEDPin, LOW);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }
    loadConfig();

    setupWiFi(SSID, USERNAME.c_str(), PASSWORD.c_str());
    digitalWrite(wifiLEDPin, HIGH);

    // ------------------Define Endpoints (for debugging)-----------------

  /*  Existing Commands
   *    - /freq
   *    - /speedA
   *    - /speedB
   *    - /wifistatus
   *    - /getspeeds
   *    - /getfreqs
   */

    server.on("/freq", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            int newFreq = message.toInt();
            if (newFreq > 0) { // Ensure the frequency is a positive value
                motorA.setFreq(newFreq);
                motorB.setFreq(newFreq);
                request->send(200, "text/plain", "PWM frequency updated");
            } else {
                request->send(400, "text/plain", "Invalid frequency value. Must be a positive integer.");
            }
        } 
        else {
            message = "No PWM sent";
            Serial.println("No PWM sent");
            request->send(400, "text/plain", "No PWM sent");
        }
    });

    server.on("/speedA", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            int newSpeed = message.toInt();
            if (newSpeed>= -255 && newSpeed<=255){
              motorA.setSpeed(newSpeed);
              request->send(200, "text/plain", "MotorA Speed updated");
            }
            else{
              request->send(400, "text/plain", "Invalid speed value. Must be between -255 and 255.");
            }
        }
        else {
          message = "No speed sent";
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });

    server.on("/speedB", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            int newSpeed = message.toInt();
            if (newSpeed>= -255 && newSpeed<=255){
              motorB.setSpeed(newSpeed);
              request->send(200, "text/plain", "MotorB Speed updated");
            }
            else{
              request->send(400, "text/plain", "Invalid speed value. Must be between -255 and 255.");
            }
        }
        else {
          message = "No speed sent";
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });

    server.on("/wifistatus", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", getWiFiStatus());
    });

    server.on("/getspeeds", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "MotorA: " + String(motorA.getSpeed()) + ", MotorB: " + String(motorB.getSpeed()));
    });
    
    server.on("/getfreqs", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "MotorA: " + String(motorA.getFreq()) + ", MotorB: " + String(motorB.getFreq()));
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting to reconnect...");
        digitalWrite(wifiLEDPin, LOW);
        connectToWiFi(SSID);
        digitalWrite(wifiLEDPin, HIGH);
    }
}

//Used to load wifi username and password
void loadConfig() {
    File file = SPIFFS.open("/config.txt", "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        int separatorIdx = line.indexOf('=');
        if (separatorIdx == -1) {
            continue;
        }

        String key = line.substring(0, separatorIdx);
        String value = line.substring(separatorIdx + 1);

        if (key == "USERNAME") {
            USERNAME = value;
        } else if (key == "PASSWORD") {
            PASSWORD = value;
        }
    }
    file.close();
}
