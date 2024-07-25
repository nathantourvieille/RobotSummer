#include <motor.h>
#include <Arduino.h>

// Constructor
Motor::Motor(int pin1, int pin2, int pwmChannel1, int pwmChannel2, int pwmResolution, int pwmFreq, int initSpeed) 
    : pwmPin1(pin1), pwmPin2(pin2), pwmChannel1(pwmChannel1), pwmChannel2(pwmChannel2), pwmResolution(pwmResolution), pwmFreq(pwmFreq), speed(initSpeed){

    //Attatch motor pins
    ledcSetup(pwmChannel1, pwmFreq, pwmResolution);
    ledcAttachPin(pwmPin1, pwmChannel1);
    ledcSetup(pwmChannel2, pwmFreq, pwmResolution);
    ledcAttachPin(pwmPin2, pwmChannel2);    
    }

//-------------------Setter methods-------------------

//newSpeed: can be any int value as the function will automatically clamp the speed
void Motor::setSpeed(int newSpeed) {
    newSpeed = constrain(newSpeed, -255, 255);
    speed = newSpeed;
    if (speed>=0){
        //Serial.println(speed);
        ledcWrite(pwmChannel2, 0);
        ledcWrite(pwmChannel1, speed);
    }
    else{
        //Serial.println(-speed);
        ledcWrite(pwmChannel1, 0);
        ledcWrite(pwmChannel2, -speed);
    }
    //Serial.println("Motor Speed updated to: " + String(newSpeed));
}

//setFreq changes the PWM frequency of the motor pin
//newFreq: can be any int value as the function will automatically clamp the freq
void Motor::setFreq(int newFreq) {
    newFreq = constrain(newFreq, 0, 20000);
    // Reconfigure the PWM with the new frequency without attaching pins again
    if (ledcSetup(pwmChannel1, newFreq, pwmResolution) == 0) {
        Serial.println("Failed to set new PWM frequency on Channel 1. Check the frequency value and channel availability.");
        return;
    }

    if (ledcSetup(pwmChannel2, newFreq, pwmResolution) == 0) {
        Serial.println("Failed to set new PWM frequency on Channel 2. Check the frequency value and channel availability.");
        return;
    }

    Serial.println("PWM frequency updated to: " + String(newFreq));
}


// Getter methods
int Motor::getSpeed() const {
    return speed;
}

int Motor::getFreq() const {
    return pwmFreq;
}
