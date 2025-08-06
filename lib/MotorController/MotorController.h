#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

#include <TMCStepper.h>

// Pin definitions
#define EN_PIN      1
#define STEP_PIN_1  41
#define DIR_PIN_1   42
#define STEP_PIN_2  39
#define DIR_PIN_2   40
#define STEP_PIN_3  37
#define DIR_PIN_3   38
#define STEP_PIN_4  35
#define DIR_PIN_4   36
#define STEP_PIN_5  48
#define DIR_PIN_5   45

// Motor definitions
#define NUM_MOTORS          5
#define MAX_STEPS           281
#define MOTOR_RUN_CURRENT   120
#define MOTOR_HOLD_CURRENT  60
#define MICROSTEPS          16

class MotorController 
{
    private:
        // Private members
        TMC2209Stepper driver;
        int STEP_DELAY;
        int stepPins[NUM_MOTORS];
        int dirPins[NUM_MOTORS];
        int positions[NUM_MOTORS];
        int targets[NUM_MOTORS];
        unsigned long lastMovements[NUM_MOTORS];
        
        // Private methods
        bool isValidMotorIndex(int motorIndex);

    public:
        // Constructor
        MotorController();
        
        // Public methods
        void begin();
        void update();
        void setTarget(int motorIndex, int targetPosition);
        void printPosition(int motorIndex);
        void printPositions();
};

#endif