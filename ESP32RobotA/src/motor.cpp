#include "motor.h"

// Constructor
Motor::Motor(int initSpeed, int initDirection) 
    : speed(initSpeed), direction(initDirection) {}

// Setter methods
void Motor::setSpeed(int newSpeed) {
    speed = newSpeed;
}

void Motor::setDirection(int newDirection) {
    direction = newDirection;
}

// Getter methods
int Motor::getSpeed() const {
    return speed;
}

int Motor::getDirection() const {
    return direction;
}
