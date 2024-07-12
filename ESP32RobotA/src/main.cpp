//#include <WiFi.h>
#include <esp_wpa2.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
//Custom files
#include <wifiHandler.h>
#include <motor.h>

// Function declarations
void loadConfig();
void steer(int steer);

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

const int TCRTPin1 = 35;
const int TCRTPin2 = 34;

int default_speed = 70;

int TCRTVal1 = 0;
int TCRTVal2 = 0;

const int wifiLEDPin = 16;

//-----------------Tape Following--------------
//Offsets
int direct_error = 0;
int integral_error = 0;
int derivative_error = 0;
int total_error = 0;

double A = 0.0005;
double B = 0.0002;
double C = 0.0000;

int offset1 = 0;
int offset2 = 80;

Motor motorA (pwmPinA1, pwmPinA2, pwmChannelA1, pwmChannelA2);
Motor motorB (pwmPinB1, pwmPinB2, pwmChannelB1, pwmChannelB2);

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    //-----------Pins--------
    pinMode(wifiLEDPin, OUTPUT);
    digitalWrite(wifiLEDPin, LOW);

    pinMode(TCRTPin1, INPUT);
    pinMode(TCRTPin2, INPUT);

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
              //motorA.setSpeed(newSpeed);
              default_speed = newSpeed;
              request->send(200, "text/plain", "MotorA Speed updated");
            }
            else{
              request->send(400, "text/plain", "Invalid speed value. Must be between -255 and 255.");
            }
        }
        else {
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });

    server.on("/offset1", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            offset1 = message.toInt();
            request->send(200, "text/plain", "Threshold1 updated");
        } else {
            request->send(400, "text/plain", "No threshold1 sent");
        }
    });

    server.on("/offset2", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            offset2 = message.toInt();
            request->send(200, "text/plain", "Threshold2 updated");
        } else {
            request->send(400, "text/plain", "No threshold2 sent");
        }
    });


    server.on("/A", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            A = message.toDouble();
            request->send(200, "text/plain", "A updated");
        } else {
            request->send(400, "text/plain", "No A sent");
        }
    });

    server.on("/B", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            B = message.toDouble();
            request->send(200, "text/plain", "B updated");
        } else {
            request->send(400, "text/plain", "No B sent");
        }
    });

    server.on("/C", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            C = message.toDouble();
            request->send(200, "text/plain", "C updated");
        } else {
            request->send(400, "text/plain", "No C sent");
        }
    });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        motorA.setSpeed(0);
        motorB.setSpeed(0);
        total_error = 0;
        integral_error = 0;
        request->send(200, "text/plain", "Motors stopped");
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

    server.on("/getTCRT", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "TCRT1: " + String(analogRead(TCRTPin1)) + ", TCRT2: " + String(analogRead(TCRTPin2)) + ", Total Error: " + String(total_error) +  " MotorA: " + String(motorA.getSpeed()) + ", MotorB: " + String(motorB.getSpeed()));
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

  TCRTVal1 = analogRead(TCRTPin1) - offset1;
  TCRTVal2 = analogRead(TCRTPin2) - offset2;

  
  derivative_error =  direct_error - (TCRTVal1 - TCRTVal2); //direct error has not been updated yet
  direct_error = TCRTVal1 - TCRTVal2; //direct error is now updated
  integral_error += direct_error;

  total_error = A * direct_error + B * integral_error + C * derivative_error;
  total_error = constrain(total_error, -100, 100);

  if (TCRTVal1 > 1000 && TCRTVal2 > 1500){
    motorA.setSpeed(default_speed);
    motorB.setSpeed(default_speed);
    total_error = 0;
    integral_error = 0;
  }
  
  steer(-round(total_error));

  Serial.print("TCRTVal1: ");
  Serial.print(TCRTVal1);
  Serial.print(", TCRTVal2: ");
  Serial.print(TCRTVal2);

  Serial.print(",   Offset1: ");
  Serial.print(offset1);
  Serial.print(", Offset2: ");
  Serial.print(offset2);

  Serial.print(",   SpeedA: ");
  Serial.print(motorA.getSpeed());
  Serial.print(",   SpeedB: ");
  Serial.print(motorB.getSpeed());

  Serial.print(",   Error:");
  Serial.println(total_error);

  delay(50);

}

//Positive steer means motorA goes faster than motorB
void steer(int steer){
  //int speedA = motorA.getSpeed();
  //int speedB = motorA.getSpeed();
  motorA.setSpeed(default_speed + steer + 5);
  motorB.setSpeed(default_speed - steer); 
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

