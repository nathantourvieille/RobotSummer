//#include <WiFi.h>
#include <esp_wpa2.h>
#include <ESPAsyncWebServer.h>
//Custom files
#include <wifiHandler.h>
#include <motor.h>

// Function declarations
void checkIfSeenR(bool verbose);
void checkIfSeenL(bool verbose);
void clearSeenR();
void clearSeenL();

void loadConfig();
void updateLineFollowing();
void find_next_station();

//---------------------Wifi Stuff (For Debugging)------------------
const char* SSID = "nate1234";

const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

//-------------------Pin Definitions--------------
//-----Line Following----
const int pwmPinA1 = 14;
const int pwmChannelA1 = 0;
const int pwmPinA2 = 15;
const int pwmChannelA2 = 1;

const int pwmPinB1 = 16;
const int pwmChannelB1 = 2;
const int pwmPinB2 = 17;
const int pwmChannelB2 = 3;

//FRONT
const int TCRTPinFrontR = 1; 
const int TCRTPinFrontL = 2; 

//LEFT
const int TCRTPinLeftF = 3;
const int TCRTPinLeftB = 4;

//RIGHT
const int TCRTPinRightF = 5;
const int TCRTPinRightB = 6;


//BACK
const int TCRTPinBackR = 7;
const int TCRTPinBackL = 8;


//const int wifiLEDPin = 16;

//----Claw------
//const int servoPin = 0;
//const int servoPwmChannel = 4;
//const int sideLimitPinR = 14;
//const int sideLimitPinL = 27;
//const int heightLimitPinR = 26;
//const int heightLimitPinL = 25;

//-----------------Steering--------------
//----Values-----
int TCRT_val_FrontR = 0;
int TCRT_val_FrontL = 0;
int TCRT_val_BackR = 0;
int TCRT_val_BackL = 0;


int default_speed = 90;
int direction = 1;
int lost_correction = 10;
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


//PID coefficients
double A = 0.005; //porportional
int B = 10;
int C = 32;

//Thresholds

int lost_threshold = 250;
int line_threshold = 500;
//
bool seenRightF = false;
bool seenRightB = false;
bool seenLeftF = false;
bool seenLeftB = false;
bool seenWhite = false;

int stations_left = 0;
int slow_speed = 65;

unsigned long starting_time;
long max_duration;
//------------
Motor motorA (pwmPinA1, pwmPinA2, pwmChannelA1, pwmChannelA2);
Motor motorB (pwmPinB1, pwmPinB2, pwmChannelB1, pwmChannelB2);

//-------------State Machine-------------

enum RobotState {
    START,
    DEBUG_TCRTS,
    MOVE_TO_BOTTOM_BUN,
    ALIGN_WITH_BOTTOM_BUN,
    COLLECT_BOTTOM_BUN,
    MOVE_TO_CUTTING_BOARD,
    HAND_OVER_BUN,
    MOVE_TO_PATTIES,
    COLLECT_PATTIES,
    MOVE_TO_STOVE,
    COOK_PATTIES,
    WAIT_FOR_PATTIE_SIGNAL,
    COLLECT_PATTIE,
    MOVE_TO_LETTUCE,
    COLLECT_LETTUCE,
    WAIT_FOR_TOP_BUN_SIGNAL,
    COLLECT_TOP_BUN,
    MOVE_TO_PLATES,
    COLLECT_PLATE,
    PLACE_TOP_BUN_ON_BOARD,
    FINISHED
};

RobotState current_state = START;

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    //-----------Pins--------
    //pinMode(wifiLEDPin, OUTPUT);
    //digitalWrite(wifiLEDPin, LOW);
    pinMode(0, INPUT_PULLUP); //BOOT button

    pinMode(TCRTPinFrontR, INPUT);
    pinMode(TCRTPinFrontL, INPUT);
    pinMode(TCRTPinRightF, INPUT);
    pinMode(TCRTPinRightB, INPUT);

    //pinMode(TCRTPinBackR, INPUT);
    //pinMode(TCRTPinBackL, INPUT);

    //ledcSetup(servoPwmChannel, 1000, 8);
    //ledcAttachPin(servoPin, servoPwmChannel);
    //pinMode(sideLimitPinR, INPUT);
    //pinMode(sideLimitPinL, INPUT);
    //pinMode(heightLimitPinR, INPUT);
    //pinMode(heightLimitPinL, INPUT);

    Serial.println("Setting Up Wifi...");
    setupWiFi(SSID);
    //digitalWrite(wifiLEDPin, HIGH);

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

    server.on("/linethreshold", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            line_threshold = message.toInt();
            request->send(200, "text/plain", "line_threshold updated");
        } else {
            request->send(400, "text/plain", "No line_threshold sent");
        }
    });

    server.on("/setstate", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("state", true)) {
            String stateParam = request->getParam("state", true)->value();
            int stateInt = stateParam.toInt(); // Convert the string parameter to an integer
            current_state = static_cast<RobotState>(stateInt); // Set the current state using the integer value
            request->send(200, "text/plain", "State updated to " + stateParam);
        } else {
            request->send(400, "text/plain", "State parameter missing");
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
      request->send(200, "text/plain", "TCRTFrontR: " + String(analogRead(TCRTPinFrontR)) + ", TCRTFrontL: " + String(analogRead(TCRTPinFrontL)) 
                                    + ", TCRTRightF: " + String(analogRead(TCRTPinRightF)) + ", TCRTRightB: " + String(analogRead(TCRTPinRightB)) 
                                    + ", TCRTLeftF: " + String(analogRead(TCRTPinLeftF)) + ", TCRTLeftB: " + String(analogRead(TCRTPinLeftB)) 
                                    + ", TCRTBackR: " + String(analogRead(TCRTPinBackR)) + ", TCRTBackL: " + String(analogRead(TCRTPinBackL)) 
                                    );
    });


    server.begin();
    Serial.println("HTTP server started");

  Serial.println("START");
}

void loop() {

  switch (current_state) {
    case DEBUG_TCRTS:
      Serial.print("Front R: ");
      Serial.print(analogRead(TCRTPinFrontR));
      Serial.print(" Front L: ");
      Serial.println(analogRead(TCRTPinFrontL));
      Serial.print(" Right F: ");
      Serial.print(analogRead(TCRTPinRightF));
      Serial.print(" Right B: ");
      Serial.println(analogRead(TCRTPinRightB));

      Serial.print(" Left F: ");
      Serial.print(analogRead(TCRTPinLeftF));
      Serial.print(" Left B: ");
      Serial.println(analogRead(TCRTPinLeftB));

      //Serial.print(" Back R: ");
      //Serial.print(analogRead(TCRTPinBackR));
      //Serial.print(" Back L: ");
      //Serial.println(analogRead(TCRTPinBackL));
      delay(500);
      break;
      
    case START:
      //Do nothing
      //Transition
      if (digitalRead(0) == LOW){
        current_state = MOVE_TO_BOTTOM_BUN;
        stations_left = 2;
      }
    break;

    case MOVE_TO_BOTTOM_BUN:
      Serial.println("Moving to Bottom Bun");
      //actions
      updateLineFollowing();
      checkIfSeenL(true);

      Serial.print("Stations Left: ");
      Serial.println(stations_left);

      if (seenLeftF && seenLeftB){
        Serial.println("Seen Both!");
        clearSeenL();
        stations_left--;
        while (analogRead(TCRTPinLeftF) > 300 || analogRead(TCRTPinLeftB) > 300){
          updateLineFollowing();
        }
      }

      //Transition
      if (stations_left == 0){
        Serial.println("Transitioning to ALIGN_WITH_BOTTOM_BUN");
        motorA.setSpeed(slow_speed * direction);
        motorB.setSpeed(slow_speed * direction);
        delay(200);
        direction = -1;
        motorA.setSpeed(slow_speed * direction);
        motorB.setSpeed(slow_speed * direction);
        starting_time = millis();
        current_state = ALIGN_WITH_BOTTOM_BUN;
       }
      break;
  
    case ALIGN_WITH_BOTTOM_BUN:
      Serial.println("Align with bottom");
      checkIfSeenL(true);
      if (seenLeftF && seenLeftB || millis() - starting_time > 1000){
        motorA.setSpeed(0);
        motorB.setSpeed(0);
        clearSeenL();
        current_state = START;
      }
      break;
    }  

  }

void checkIfSeenR(bool verbose = false){
  if (verbose){
    Serial.print("Right F: ");
    Serial.print(analogRead(TCRTPinRightF));
    Serial.print(" Right B: ");
    Serial.println(analogRead(TCRTPinRightB));
  }

  if (analogRead(TCRTPinRightF) > line_threshold){
    seenRightF = true;
  }
  if (analogRead(TCRTPinRightB) > line_threshold){
    seenRightB = true;
  }
}

void checkIfSeenL(bool verbose = false){
  if (verbose){
    Serial.print("Left F: ");
    Serial.print(analogRead(TCRTPinLeftF));
    Serial.print(" Left B: ");
    Serial.println(analogRead(TCRTPinLeftB));
  }
  if (analogRead(TCRTPinLeftF) > line_threshold){
    seenLeftF = true;
  }
  if (analogRead(TCRTPinLeftB) > line_threshold){
    seenLeftB = true;
  }
}

void clearSeenR(){
  seenRightF = false;
  seenRightB = false;
}

void clearSeenL(){
  seenLeftF = false;
  seenLeftB = false;
}


void updateLineFollowing() {
    speed_offset = 0;

    //ASK TA abt incorporating front and back/test it
    TCRT_val_FrontR = analogRead(TCRTPinFrontR) - TCRT_offset_1; 
    TCRT_val_FrontL = analogRead(TCRTPinFrontL) - TCRT_offset_2;
    //TCRT_val_BackR = analogRead(TCRTPinFrontR) - TCRT_offset_1; 
    //TCRT_val_BackL = analogRead(TCRTPinFrontL) - TCRT_offset_2;

    //proportional_error = TCRT_val_FrontR + TCRT_val_BackL - (TCRT_val_FrontL + TCRT_val_BackR);
    proportional_error = TCRT_val_FrontR  - TCRT_val_FrontL;

    //If error is too small, don't change anything
    if (abs(proportional_error) > B){
      total_error = constrain(A * proportional_error, -100, 100);

      // If both sensors are  on the line, go straight
      if (TCRT_val_FrontR > line_threshold && TCRT_val_FrontL > line_threshold && TCRT_val_BackR > line_threshold && TCRT_val_BackL > line_threshold) {
          motorA.setSpeed(default_speed);
          motorB.setSpeed(default_speed);
          total_error = 0;
      } 

      //If we are lost
      else if (TCRT_val_FrontR < lost_threshold && TCRT_val_FrontL < lost_threshold || (TCRT_val_BackR < lost_threshold && TCRT_val_BackL < lost_threshold)){
        //If speed is too high, slow it down: MAKE THIS MORE ROBUST
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

