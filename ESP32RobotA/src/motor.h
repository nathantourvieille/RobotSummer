#ifndef MOTOR_H
#define MOTOR_H

class Motor {
private:
    int speed;      // Speed of the motor -255 to 255
    const int pwmPin1;
    const int pwmPin2;
    const int pwmChannel1;
    const int pwmChannel2;
    const int pwmResolution;
    const int pwmFreq;

public:
    // Constructor
    Motor(int pwmpin1, int pwmpin2, int pwmChannel1, int pwmChannel2, int pwmResolution = 8, int pwmFreq = 200, int initSpeed = 0);

    // Setter methods
    void setSpeed(int newSpeed);
    void setFreq(int newFreq);

    // Getter methods
    int getSpeed() const;
    int getFreq() const;
};

#endif // MOTOR_H
