#include <Arduino.h>
#include <Adafruit_TLC5947.h>

#define DATA_PIN  23
#define CLK_PIN   18
#define LAT_PIN   5
#define EN_PIN    -1

Adafruit_TLC5947 tlc = Adafruit_TLC5947(1, CLK_PIN, DATA_PIN, LAT_PIN);

void _setup() 
{
  Serial.begin(115200);

  Serial.println("TLC Test");

  tlc.begin();

  tlc.setPWM(0, 0);
  tlc.write();
}

int d = 1;

void _loop() 
{
  for (int i = 0; i < 4095; i++)
  {
    if (i % 5 == 0) 
    {
        Serial.println(i);
        tlc.setPWM(0, i);
        tlc.write();
    }
    
    delay(d);
  }
  
  for (int i = 0; i < 4095; i++)
  {
    if (i % 5 == 0) 
    {
      Serial.println(4095 - i);
      tlc.setPWM(0, 4095 - i);
      tlc.write();
    }
    delay(d);
  }
}