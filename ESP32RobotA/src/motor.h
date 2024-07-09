#ifndef MOTOR_H
#define MOTOR_H

class Motor {
private:
    int speed;      // Speed of the motor
    int direction;  // Direction of the motor (1 or 0)

public:
    // Constructor
    Motor(int initSpeed = 0, int initDirection = 0);

    // Setter methods
    void setSpeed(int newSpeed);
    void setDirection(int newDirection);

    // Getter methods
    int getSpeed() const;
    int getDirection() const;
};

#endif // MOTOR_H
