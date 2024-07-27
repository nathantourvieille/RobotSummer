//#include <WiFi.h>
#include <ESP32Servo.h>
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

void updateLineFollowing();
void find_next_station();
void scoot();
void slowWrite(Servo &servo, int targetAngle);
void setServoPosition(String servoName, int angle);

//---------------------Wifi Stuff (For Debugging)------------------
const char* SSID = "nate1234";

const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

//-------------------Pin Definitions--------------
//-----Line Following----
const int pwmPinA1 = 38;
const int pwmChannelA1 = 0;
const int pwmPinA2 = 37;
const int pwmChannelA2 = 1;

const int pwmPinB1 = 36;
const int pwmChannelB1 = 2;
const int pwmPinB2 = 35;
const int pwmChannelB2 = 3;

//BACK
const int TCRTPinBackR = 1;
const int TCRTPinBackL = 2;

//RIGHT
const int TCRTPinRightF = 3;
const int TCRTPinRightB = 4;

//LEFT
const int TCRTPinLeftF = 5;
const int TCRTPinLeftB = 6;

//FRONT
const int TCRTPinFrontR = 7; 
const int TCRTPinFrontL = 8; 

//const int wifiLEDPin = 16;

//----Claw------
const int centerServoPin = 9;
Servo centerServo;

const int clawArmServoPin = 10;
Servo clawArmServo;
const int clawArmOut = 105;
const int clawArmUp = 55;
const int clawArmNeutral = 85;

const int clawServoPin = 11;
Servo clawServo;
const int clawOpen = 80;
const int clawClosed = 55;

const int chuteArmServoPin = 12;
Servo chuteArmServo;
const int chuteArmIn = 0;
const int chuteArmOut = 120;

const int chuteServoPin = 13;
Servo chuteServo;
const int chuteOpen = 70;
const int chuteClosed = 120;

//-----------------Steering--------------
//----Values-----
int TCRT_val_R = 0;
int TCRT_val_L = 0;

int default_speed = 185;
int direction = 1;
int lost_correction = 30; //How much duty cycle to add/subtract from wheels when steering for line following
int speed_offset = 0; //

//----Offsets----
//Error Values
int proportional_error = 0;
int total_error = 0;
int previous_error = 0;


//PID coefficients
double A = 0.005; //porportional
int minimum_error = 10;

//Thresholds

int lost_threshold = 500;
int line_threshold = 2000;
//
bool seenRightF = false;
bool seenRightB = false;
bool seenLeftF = false;
bool seenLeftB = false;
bool seenWhite = false;

int stations_left = 0;
int slow_speed = 155;

unsigned long starting_time;
long max_duration;
int servo_delay = 2;
//------------
Motor motorA (pwmPinA1, pwmPinA2, pwmChannelA1, pwmChannelA2);
Motor motorB (pwmPinB1, pwmPinB2, pwmChannelB1, pwmChannelB2);

//-------------State Machine-------------

enum RobotState {
    START,
    MOVE_TO_LETTUCE,
    ALIGN_WITH_LETTUCE,
    COLLECT_LETTUCE,
    MOVE_TO_PLATE,
    COLLECT_PLATE,
    MOVE_TO_SERVING_AREA,
    SERVE,
    DEBUG_TCRTS
};

RobotState current_state = START;

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    //-----------Pins--------
    pinMode(0, INPUT_PULLUP); //BOOT button

    pinMode(TCRTPinFrontR, INPUT);
    pinMode(TCRTPinFrontL, INPUT);

    pinMode(TCRTPinRightF, INPUT);
    pinMode(TCRTPinRightB, INPUT);

    pinMode(TCRTPinLeftF, INPUT);
    pinMode(TCRTPinLeftB, INPUT);

    pinMode(TCRTPinBackR, INPUT);
    pinMode(TCRTPinBackL, INPUT);


    //ledcSetup(centerPwmChannel, 300, 8);
    //ledcAttachPin(centerServoPin, centerPwmChannel);

    centerServo.attach(centerServoPin);
    clawArmServo.attach(clawArmServoPin);
    clawServo.attach(clawServoPin);
    chuteArmServo.attach(chuteArmServoPin);
    chuteServo.attach(chuteServoPin);


    setupWiFi(SSID);

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

    server.on("/defaultspeed", HTTP_POST, [](AsyncWebServerRequest *request) {
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

    server.on("/minimumerror", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);
            minimum_error = message.toDouble();
            request->send(200, "text/plain", "Minimum error updated");
        } else {
            request->send(400, "text/plain", "Minimum error sent");
        }
    });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        motorA.setSpeed(0);
        motorB.setSpeed(0);
        total_error = 0;
        request->send(200, "text/plain", "Motors stopped");
    });

    server.on("/servodelay", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("message", true)) {
          String message = request->getParam("message", true)->value();
          servo_delay = message.toInt();
          }
        request->send(200, "text/plain", "Servo Delay Updated?");
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

    server.on("/servo", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("message", true)) {
            String message = request->getParam("message", true)->value();
            int spaceIndex = message.indexOf(' ');
            if (spaceIndex != -1) {
                String servoName = message.substring(0, spaceIndex);
                int angle = message.substring(spaceIndex + 1).toInt();

                // Check if the angle is within the expected range
                if (angle >= 0 && angle <= 180) {
                    setServoPosition(servoName, angle);
                    request->send(200, "text/plain", "Servo " + servoName + " set to " + String(angle) + " degrees");
                } else {
                    request->send(400, "text/plain", "Invalid angle value");
                }
            } else {
                request->send(400, "text/plain", "Invalid message format");
            }
        } else {
            request->send(400, "text/plain", "Message parameter missing");
        }
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
    
  //slowWrite(clawArmServo, clawArmUp);
  //slowWrite(clawServo, clawClosed);
  //slowWrite(chuteArmServo, chuteArmIn);
  //slowWrite(chuteServo, chuteClosed);
  //slowWrite(centerServo, 180);
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

      delay(500);
      break;
      
    case START:
      //Do nothing
      //Transition
      if (digitalRead(0) == LOW){
        current_state = MOVE_TO_LETTUCE;
        stations_left = 2;
      }
    break;

    case MOVE_TO_LETTUCE:
      Serial.println("Moving to LETTUCE");
      //actions
      updateLineFollowing();
      checkIfSeenL(true);

      Serial.print("Stations Left: ");
      Serial.println(stations_left);

      if (seenLeftF && seenLeftB){
        Serial.println("Seen Both!");
        clearSeenL();
        stations_left--;
        while (analogRead(TCRTPinLeftF) > 800 || analogRead(TCRTPinLeftB) > 800){
          updateLineFollowing();
        }
      }

      //Transition
      if (stations_left == 0){
        Serial.println("Transitioning to ALIGN_WITH_LETTUCE");
        scoot();
        starting_time = millis();
        current_state = ALIGN_WITH_LETTUCE;
       }
      break;

      
    case ALIGN_WITH_LETTUCE:
      Serial.println("Align with LETTUCE");
      checkIfSeenL(true);
      if (seenLeftF && seenLeftB || millis() - starting_time > 1000){
        motorA.setSpeed(0);
        motorB.setSpeed(0);
        clearSeenL();
        direction = direction * -1;
        current_state = START; //change this to collect lettuce
      }
      break;
  
    case COLLECT_LETTUCE:
      Serial.println("Collect lettuce");
      //plz turn this into a function
      delay(1000);
      slowWrite(clawArmServo, clawArmNeutral);
      delay(1000);
      slowWrite(clawServo, clawOpen);
      delay(1000);
      slowWrite(clawArmServo, clawArmOut);
      delay(1000);
      slowWrite(clawServo, clawClosed);
      delay(1000);
      slowWrite(clawArmServo, clawArmUp);
      delay(1000);

      //Transition
      current_state = MOVE_TO_PLATE;
    break;

    case MOVE_TO_PLATE:
    Serial.println("Move to plate");
    slowWrite(centerServo, 0);
    current_state = COLLECT_PLATE;

    case COLLECT_PLATE:
      //plz turn this into a function
      Serial.println("Collect plate");
      delay(1000);
      slowWrite(clawArmServo, clawArmNeutral);
      delay(1000);
      slowWrite(clawServo, clawOpen);
      delay(1000);
      slowWrite(clawArmServo, clawArmOut);
      delay(1000);
      slowWrite(clawServo, clawClosed);
      delay(1000);
      slowWrite(clawArmServo, clawArmNeutral);
      starting_time = millis();
      current_state = MOVE_TO_SERVING_AREA;
    break;

    case MOVE_TO_SERVING_AREA:
      Serial.println("Moving to Serving area");
      //actions
      //updateLineFollowing();
      break;
    }  
  }

void collect_item();

void scoot(){
  motorA.setSpeed(slow_speed * direction);
  motorB.setSpeed(slow_speed * direction);
  delay(200);
  direction = direction * -1;
  motorA.setSpeed(slow_speed * direction);
  motorB.setSpeed(slow_speed * direction);
  direction = direction * -1;

}

void slowWrite(Servo &servo, int targetAngle) {
  int currentAngle = servo.read();
  if (currentAngle < targetAngle) {
      for (int angle = currentAngle; angle <= targetAngle; angle++) {
          servo.write(angle);
          delay(servo_delay);
      }
  } else {
      for (int angle = currentAngle; angle >= targetAngle; angle--) {
          servo.write(angle);
          delay(servo_delay);
      }
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


//Ideally update this to incorporate driving backwards, can use the direction parameter to toggle between looking at front vs back sensors
void updateLineFollowing() {
    speed_offset = 0;
    if (direction == 1){
      TCRT_val_R = analogRead(TCRTPinFrontR); 
      TCRT_val_L = analogRead(TCRTPinFrontL);
    }
    else{
      TCRT_val_R = analogRead(TCRTPinBackR); 
      TCRT_val_L = analogRead(TCRTPinBackL);
    }

    proportional_error = TCRT_val_R  - TCRT_val_L;

    //If error is too small, don't change anything
    if (abs(proportional_error) > minimum_error){
      total_error = constrain(A * proportional_error, -100, 100);

      // If both sensors are  on the line, go straight
      if (TCRT_val_R > line_threshold && TCRT_val_L > line_threshold && TCRT_val_R > line_threshold && TCRT_val_L > line_threshold) {
          motorA.setSpeed(default_speed * direction);
          motorB.setSpeed(default_speed * direction);
          total_error = 0;
      } 

      //If we are lost
      else if (TCRT_val_R < lost_threshold && TCRT_val_L < lost_threshold || (TCRT_val_R < lost_threshold && TCRT_val_L < lost_threshold)){
        //If speed is too high, slow it down: MAKE THIS MORE ROBUST
        if (default_speed > slow_speed + lost_correction){
          speed_offset = lost_correction; 
        }

        if (previous_error > 0){
          total_error = lost_correction;
        }
        else{
          total_error = -lost_correction;
        }
      }

      // Adjust motor speeds based on PID control
      motorA.setSpeed((default_speed - round(total_error) - speed_offset) * direction);
      motorB.setSpeed((default_speed + round(total_error) - speed_offset) * direction);
      
      previous_error = proportional_error;
    }
}

// Function to set the servo position
void setServoPosition(String servoName, int angle) {
    if (servoName == "centerServo") {
        centerServo.write(angle);
    } else if (servoName == "clawArmServo") {
        clawArmServo.write(angle);
    } else if (servoName == "clawServo") {
        clawServo.write(angle);
    } else if (servoName == "chuteArmServo") {
        chuteArmServo.write(angle);
    }else if (servoName == "chuteServo") {
         chuteServo.write(angle);
    }
}



