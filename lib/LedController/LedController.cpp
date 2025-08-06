#include <LedController.h>
#include <Arduino.h>

LedController::LedController() : tlc(1, CLK_PIN, DATA_PIN, LAT_PIN) 
{
    // Constructor
}

void LedController::begin() 
{
    tlc.begin();
}

void LedController::setBrightness(int pin, int percentage) 
{
    percentage = constrain(percentage, 0, 100);
    int pwmValue = map(percentage, 0, 100, 0, 4095);

    tlc.setPWM(pin, pwmValue);
}