#include <MotorController.h>
#include <Arduino.h>

MotorController::MotorController() : driver(&Serial2, 0.11f, 0b00)
{
    STEP_DELAY = 500;

    stepPins[0] = STEP_PIN_1;
    stepPins[1] = STEP_PIN_2;
    stepPins[2] = STEP_PIN_3;
    stepPins[3] = STEP_PIN_4;
    stepPins[4] = STEP_PIN_5;

    dirPins[0] = DIR_PIN_1;
    dirPins[1] = DIR_PIN_2;
    dirPins[2] = DIR_PIN_3;
    dirPins[3] = DIR_PIN_4;
    dirPins[4] = DIR_PIN_5;
    
    for (int i = 0; i < NUM_MOTORS; i++)
    {
        positions[i] = 0;
        targets[i] = 0;
        lastMovements[i] = 0;
    }

    dirs[0] = DIR_1;
    dirs[1] = DIR_2;
    dirs[2] = DIR_3;
    dirs[3] = DIR_4;
    dirs[4] = DIR_5;
}

void MotorController::begin() 
{
    // Initialize UART for the TMC2209
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    delay(100);
    
    // Initialize pins
    pinMode(EN_PIN, OUTPUT);
    
    for (int i = 0; i < NUM_MOTORS; i++)
    {
        pinMode(stepPins[i], OUTPUT);
        pinMode(dirPins[i], OUTPUT);
    }
        
    // Disable motors for driver setup
    digitalWrite(EN_PIN, HIGH);
    
    // Initialize and configure driver
    driver.begin();
    delay(100);

    driver.vsense(1);
    driver.rms_current(MOTOR_RUN_CURRENT, MOTOR_HOLD_CURRENT / (float)MOTOR_RUN_CURRENT);
    driver.microsteps(MICROSTEPS);
    driver.intpol(true);
    driver.en_spreadCycle(false);
    driver.pwm_autoscale(true);
    driver.pwm_autograd(true);
    driver.pwm_freq(1);
    driver.TPWMTHRS(0);
    driver.toff(4);
    driver.hstrt(5);
    driver.hend(3);
    driver.blank_time(24);
    driver.shaft(true);
    driver.freewheel(1);
    
    // Re-enable motors
    digitalWrite(EN_PIN, LOW);
    delay(200);
}

void MotorController::update() 
{
    for (int i = 0; i < NUM_MOTORS; i++)
    {
        if (targets[i] != positions[i] && micros() - lastMovements[i] >= STEP_DELAY) 
        {
            if (targets[i] > positions[i]) 
            {
                digitalWrite(dirPins[i], dirs[i] ? HIGH : LOW);
                positions[i]++;
            }
            else 
            {
                digitalWrite(dirPins[i], dirs[i] ? LOW : HIGH);
                positions[i]--;
            }
            delayMicroseconds(1);

            digitalWrite(stepPins[i], HIGH);
            delayMicroseconds(2);
            digitalWrite(stepPins[i], LOW);
            lastMovements[i] = micros();
        }
    }
    
}

void MotorController::setTarget(int motorIndex, int targetPosition) 
{
    if (!isValidMotorIndex(motorIndex))
        return;
        
    targets[motorIndex] = constrain(targetPosition, 0, MAX_STEPS);
}

int MotorController::getTarget(int motorIndex) 
{
    if (!isValidMotorIndex(motorIndex))
        return 0;

    return targets[motorIndex];
}

int MotorController::getPosition(int motorIndex) 
{
    if (!isValidMotorIndex(motorIndex))
        return 0;

    return positions[motorIndex];
}

void MotorController::printPosition(int motorIndex) 
{
    if (!isValidMotorIndex(motorIndex))
        return;

    if (Serial) 
    {
        Serial.println("Motor " + String(motorIndex) + ": Position - " + String(positions[motorIndex]) + " Target - " + String(targets[motorIndex]));
    }
}

void MotorController::printPositions() 
{
    for (int i = 0; i < NUM_MOTORS; i++)
    {
        printPosition(i);
    }
}

bool MotorController::isValidMotorIndex(int motorIndex) 
{
    return motorIndex >= 0 && motorIndex < NUM_MOTORS;
}