//#include <WiFi.h>
#include <esp_wpa2.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
//Custom files
#include <wifiHandler.h>
#include <motor.h>

// Function declarations
void loadConfig();
void update_line_following();

//---------------------Wifi Stuff (For Debugging)------------------
const char* SSID = "ubcsecure";
String USERNAME;
String PASSWORD;

const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

//-------------------Pin Definitions--------------
//-----Line Following----
const int pwmPinA1 = 23;
const int pwmChannelA1 = 0;
const int pwmPinA2 = 22;
const int pwmChannelA2 = 1;

const int pwmPinB1 = 19;
const int pwmChannelB1 = 2;
const int pwmPinB2 = 18;
const int pwmChannelB2 = 3;

const int TCRTPinR = 35;
const int TCRTPinL = 34;

const int wifiLEDPin = 16;

//----Claw------
const int servoPin = 0;
const int servoPwmChannel = 4;
const int sideLimitPinR = 14;
const int sideLimitPinL = 27;
const int heightLimitPinR = 26;
const int heightLimitPinL = 25;

//-----------------Steering--------------
//----Values-----
int TCRT_val_1 = 0;
int TCRT_val_2 = 0;

int default_speed = 20;
int speed_offset = 0;

//----Offsets----
int wheel_offset_A = 0;
int wheel_offset_B = 0;

int TCRT_offset_1 = 0;
int TCRT_offset_2 = 0;

//Error Values
int proportional_error = 0;
int total_error = 0;
int previous_error = 0;
int lost_correction = 50;
int lost_threshold = 50;

//PID coefficients
double A = 0.005; //porportional
int B = 5;

//------------


Motor motorA (pwmPinA1, pwmPinA2, pwmChannelA1, pwmChannelA2);
Motor motorB (pwmPinB1, pwmPinB2, pwmChannelB1, pwmChannelB2);

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    //-----------Pins--------
    pinMode(wifiLEDPin, OUTPUT);
    digitalWrite(wifiLEDPin, LOW);

    pinMode(TCRTPinR, INPUT);
    pinMode(TCRTPinL, INPUT);

    ledcSetup(servoPwmChannel, 1000, 8);
    ledcAttachPin(servoPin, servoPwmChannel);

    pinMode(sideLimitPinR, INPUT);
    pinMode(sideLimitPinL, INPUT);
    pinMode(heightLimitPinR, INPUT);
    pinMode(heightLimitPinL, INPUT);

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
   *    - /
   */
    server.on("/setfreqs", HTTP_POST, [](AsyncWebServerRequest *request) {
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

    server.on("/setspeedA", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            motorA.setSpeed(message.toInt());
            request->send(200, "text/plain", "MotorA Speed updated");
        }
        else {
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });

    server.on("/setspeedB", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            motorB.setSpeed(message.toInt());
            request->send(200, "text/plain", "MotorA Speed updated");
        }
        else {
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });

    server.on("/setdefaultspeed", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            default_speed = message.toInt();
            request->send(200, "text/plain", "Default speed updated");
        }
        else {
          Serial.println("No speed sent");
          request->send(400, "text/plain", "No speed sent");
        }
    });


    server.on("/setTCRToffset1", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            TCRT_offset_2 = message.toInt();
            request->send(200, "text/plain", "Threshold1 updated");
        } else {
            request->send(400, "text/plain", "No threshold1 sent");
        }
    });

    server.on("/setTCRToffset2", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            TCRT_offset_2 = message.toInt();
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

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        motorA.setSpeed(0);
        motorB.setSpeed(0);
        total_error = 0;
        request->send(200, "text/plain", "Motors stopped");
    });

    server.on("/lostthreshold", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            lost_threshold = message.toInt();
            request->send(200, "text/plain", "lost_threshold updated");
        } else {
            request->send(400, "text/plain", "No lost_threshold sent");
        }
    });

    server.on("/lostcorrection", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            lost_correction = message.toInt();
            request->send(200, "text/plain", "lost_correction updated");
        } else {
            request->send(400, "text/plain", "No lost_correction sent");
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

    server.on("/getTCRTs", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "TCRT1: " + String(analogRead(TCRTPinR)) + ", TCRT2: " + String(analogRead(TCRTPinL)) + " , Total Error: " + String(total_error));
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

  update_line_following();

  Serial.print("TCRTVal1: ");
  Serial.print(TCRT_val_1);
  Serial.print(", TCRTVal2: ");
  Serial.print(TCRT_val_2);

  Serial.print(",   Offset1: ");
  Serial.print(TCRT_offset_1);
  Serial.print(", Offset2: ");
  Serial.print(TCRT_offset_2);

  Serial.print(",   SpeedA: ");
  Serial.print(motorA.getSpeed());
  Serial.print(",   SpeedB: ");
  Serial.print(motorB.getSpeed());

  Serial.print(",   Error:");
  Serial.println(total_error);
  
  delay(0);

}

/*
*
*/
void update_line_following() {
    speed_offset = 0;
    TCRT_val_1 = analogRead(TCRTPinR) - TCRT_offset_1; 
    TCRT_val_2 = analogRead(TCRTPinL) - TCRT_offset_2;

    proportional_error = TCRT_val_1 - TCRT_val_2;

    if (abs(proportional_error) > B){
      total_error = constrain(A * proportional_error, -100, 100);

      // If both sensors are on the line, go straight
      if (TCRT_val_1 > 1000 && TCRT_val_2 > 1000) {
          motorA.setSpeed(default_speed);
          motorB.setSpeed(default_speed);
          total_error = 0;
      } 

      //If we are lost
      else if (TCRT_val_1 < lost_threshold && TCRT_val_2 < lost_threshold){
        if (default_speed > 40){
          speed_offset = default_speed/5; 
        }
        
        
        if (previous_error > 0){
          total_error = lost_correction;
        }
        else{
          total_error = -lost_correction;
        }
      }
      // Adjust motor speeds based on PID control
      motorA.setSpeed(default_speed - round(total_error) - speed_offset);
      motorB.setSpeed(default_speed + round(total_error) - speed_offset);
      
      previous_error = proportional_error;
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

