#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <Adafruit_TLC5947.h>

// Pin definitions
#define DATA_PIN  23
#define CLK_PIN   18
#define LAT_PIN   5
#define EN_PIN    -1

class LedController 
{
    private:
        // Private members
        Adafruit_TLC5947 tlc;

    public:
        // Constructor
        LedController();
        
        // Public methods
        void begin();
        void setBrightness(int pin, int percentage);
};

#endif