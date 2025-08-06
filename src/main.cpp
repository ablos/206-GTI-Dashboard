#include <Arduino.h>
#include <MotorController.h>

MotorController motorController;

void setup() 
{
    Serial.begin(115200);
    motorController.begin();

    motorController.setTarget(0, 50);
    motorController.printPositions();
}

void loop() 
{
    motorController.update();
}