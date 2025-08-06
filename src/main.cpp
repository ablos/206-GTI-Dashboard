#include <Arduino.h>
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

// TMC2209 UART configuration
#define DRIVER_ADDRESS 0b00
#define R_SENSE 0.11f

TMC2209Stepper driver(&Serial2, R_SENSE, DRIVER_ADDRESS);

// Motor configuration for PM20T-036
const int MOTOR_RUN_CURRENT = 120;    // mA RMS (100-150mA range)
const int MOTOR_HOLD_CURRENT = 60;    // mA RMS
int STEP_DELAY = 500;                 // microseconds between steps
const int MICROSTEPS = 16;            // 16 microsteps with interpolation

// Motor specifications
const float STEPS_PER_REV = 1080.0;   // PM20T-036 has 1080 steps/rev
const float DEGREES_PER_STEP = 360.0 / STEPS_PER_REV;

// Position tracking
long currentPosition = 0;             // in microsteps
float currentAngle = 0;              // in degrees

// Function declarations
void updateAngle();
void printPosition();
void performStep(bool dir);
void performMultipleSteps(bool dir, int count);
void configureTMC2209();
void moveToAngle(float targetAngle);
void printStatus();
void printHelp();

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== TMC2209 + PM20T-036 Manual Control ===");
    Serial.println("Initializing...");

    // Initialize UART for TMC2209
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    delay(100);
    
    // Initialize pins
    pinMode(EN_PIN, OUTPUT);
    
    pinMode(STEP_PIN_1, OUTPUT);
    pinMode(DIR_PIN_1, OUTPUT);
    pinMode(STEP_PIN_2, OUTPUT);
    pinMode(DIR_PIN_2, OUTPUT);
    pinMode(STEP_PIN_3, OUTPUT);
    pinMode(DIR_PIN_3, OUTPUT);
    pinMode(STEP_PIN_4, OUTPUT);
    pinMode(DIR_PIN_4, OUTPUT);
    pinMode(STEP_PIN_5, OUTPUT);
    pinMode(DIR_PIN_5, OUTPUT);

    // Disable motor initially
    digitalWrite(EN_PIN, HIGH);

    // Initialize and configure driver
    driver.begin();
    delay(100);
    
    configureTMC2209();
    
    // Enable motor
    digitalWrite(EN_PIN, LOW);
    delay(200);
    
    printStatus();
    printHelp();
}

void configureTMC2209()
{
    Serial.println("Configuring TMC2209...");
    
    // CRITICAL: Enable VSENSE for low current operation
    driver.vsense(1);  // High sensitivity (0.18V) for currents < 500mA
    
    // Set motor currents
    // For v0.7.3, use rms_current with hold multiplier
    driver.rms_current(MOTOR_RUN_CURRENT, MOTOR_HOLD_CURRENT / (float)MOTOR_RUN_CURRENT);
    
    // Configure microstepping
    driver.microsteps(MICROSTEPS);
    driver.intpol(true);  // Smooth 256 microstep interpolation
    
    // StealthChop for quiet operation
    driver.en_spreadCycle(false);  // false = StealthChop enabled
    driver.pwm_autoscale(true);
    driver.pwm_autograd(true);
    driver.pwm_freq(1);  // 35.1kHz
    driver.TPWMTHRS(0);  // StealthChop at all speeds
    
    // Chopper configuration for low current
    driver.toff(4);
    driver.hstrt(5);
    driver.hend(3);
    driver.blank_time(24);
    
    // Motor direction
    driver.shaft(true);  // Adjust based on your wiring
    
    // Standby current reduction
    driver.freewheel(1);
    
    Serial.println("Configuration complete!");
}

void loop() 
{
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch(cmd) {
            // === SINGLE STEP CONTROLS ===
            case 'l':  // Single step left
                performStep(false);
                currentPosition--;
                updateAngle();
                Serial.print("Left -> ");
                printPosition();
                break;
                
            case 'r':  // Single step right
                performStep(true);
                currentPosition++;
                updateAngle();
                Serial.print("Right -> ");
                printPosition();
                break;
            
            // === MULTI-STEP CONTROLS ===    
            case 'L':  // 10 steps left
                performMultipleSteps(false, 10);
                currentPosition -= 10;
                updateAngle();
                Serial.print("10 steps left -> ");
                printPosition();
                break;
                
            case 'R':  // 10 steps right
                performMultipleSteps(true, 10);
                currentPosition += 10;
                updateAngle();
                Serial.print("10 steps right -> ");
                printPosition();
                break;
                
            case '<':  // 100 steps left
                performMultipleSteps(false, 100);
                currentPosition -= 100;
                updateAngle();
                Serial.print("100 steps left -> ");
                printPosition();
                break;
                
            case '>':  // 100 steps right
                performMultipleSteps(true, 100);
                currentPosition += 100;
                updateAngle();
                Serial.print("100 steps right -> ");
                printPosition();
                break;
            
            // === ANGLE CONTROLS ===    
            case '0':  // Move to 0°
                moveToAngle(0);
                break;
                
            case '1':  // Move to 30°
                moveToAngle(30);
                break;
                
            case '2':  // Move to 60°
                moveToAngle(60);
                break;
                
            case '3':  // Move to 90°
                moveToAngle(90);
                break;
                
            case '4':  // Move to 120°
                moveToAngle(120);
                break;
                
            case '5':  // Move to 150°
                moveToAngle(150);
                break;
                
            case '6':  // Move to 180°
                moveToAngle(180);
                break;
                
            case '7':  // Move to 210°
                moveToAngle(210);
                break;
                
            case '8':  // Move to 240°
                moveToAngle(240);
                break;
                
            case '9':  // Move to 270°
                moveToAngle(270);
                break;
            
            // === SPEED CONTROLS ===    
            case '+':  // Increase step delay (slower)
                STEP_DELAY += 100;
                Serial.print("Speed slower: ");
                Serial.print(STEP_DELAY);
                Serial.println(" μs/step");
                break;
                
            case '-':  // Decrease step delay (faster)
                STEP_DELAY = max(200, STEP_DELAY - 100);
                Serial.print("Speed faster: ");
                Serial.print(STEP_DELAY);
                Serial.println(" μs/step");
                break;
            
            // === POSITION CONTROLS ===    
            case 'z':  // Zero position
                currentPosition = 0;
                currentAngle = 0;
                Serial.println("Position ZEROED!");
                printPosition();
                break;
                
            case 'p':  // Print position
                printPosition();
                break;
            
            // === MOTOR CONTROLS ===    
            case 'e':  // Enable motor
                digitalWrite(EN_PIN, LOW);
                Serial.println("Motor ENABLED");
                break;
                
            case 'd':  // Disable motor
                digitalWrite(EN_PIN, HIGH);
                Serial.println("Motor DISABLED");
                break;
            
            // === INFO COMMANDS ===    
            case 's':  // Status
                printStatus();
                break;
                
            case 'h':  // Help
            case '?':
                printHelp();
                break;
                
            case '\n':  // Ignore newlines
            case '\r':
                break;
                
            default:
                Serial.print("Unknown command: '");
                Serial.print(cmd);
                Serial.println("' (press 'h' for help)");
        }
    }
}

void performStep(bool dir) 
{
    digitalWrite(DIR_PIN_1, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_2, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_3, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_4, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_5, dir ? HIGH : LOW);
    
    delayMicroseconds(10);  // Direction setup time
    
    digitalWrite(STEP_PIN_1, HIGH);
    digitalWrite(STEP_PIN_2, HIGH);
    digitalWrite(STEP_PIN_3, HIGH);
    digitalWrite(STEP_PIN_4, HIGH);
    digitalWrite(STEP_PIN_5, HIGH);
    
    delayMicroseconds(STEP_DELAY);
    digitalWrite(STEP_PIN_1, LOW);
    digitalWrite(STEP_PIN_2, LOW);
    digitalWrite(STEP_PIN_3, LOW);
    digitalWrite(STEP_PIN_4, LOW);
    digitalWrite(STEP_PIN_5, LOW);
    
    delayMicroseconds(STEP_DELAY);
}

void performMultipleSteps(bool dir, int count) 
{
    digitalWrite(DIR_PIN_1, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_2, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_3, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_4, dir ? HIGH : LOW);
    digitalWrite(DIR_PIN_5, dir ? HIGH : LOW);
    
    delayMicroseconds(10);
    
    for (int i = 0; i < count; i++) {
        digitalWrite(STEP_PIN_1, HIGH);
        digitalWrite(STEP_PIN_2, HIGH);
        digitalWrite(STEP_PIN_3, HIGH);
        digitalWrite(STEP_PIN_4, HIGH);
        digitalWrite(STEP_PIN_5, HIGH);
        
        delayMicroseconds(STEP_DELAY);
        digitalWrite(STEP_PIN_1, LOW);
        digitalWrite(STEP_PIN_2, LOW);
        digitalWrite(STEP_PIN_3, LOW);
        digitalWrite(STEP_PIN_4, LOW);
        digitalWrite(STEP_PIN_5, LOW);
        
        delayMicroseconds(STEP_DELAY);
    }
}

void moveToAngle(float targetAngle)
{
    // Calculate target position in microsteps
    long targetPosition = (long)((targetAngle / 360.0) * STEPS_PER_REV * MICROSTEPS);
    long stepsToMove = targetPosition - currentPosition;
    
    Serial.print("Moving to ");
    Serial.print(targetAngle, 1);
    Serial.print("° (");
    Serial.print(abs(stepsToMove));
    Serial.print(" microsteps)... ");
    
    // Determine direction
    bool dir = (stepsToMove > 0);
    stepsToMove = abs(stepsToMove);
    
    // Move to target
    performMultipleSteps(dir, stepsToMove);
    
    // Update position
    currentPosition = targetPosition;
    updateAngle();
    
    Serial.print("Done! ");
    printPosition();
}

void updateAngle()
{
    currentAngle = (float)currentPosition / (MICROSTEPS * STEPS_PER_REV) * 360.0;
}

void printPosition()
{
    Serial.print("Pos: ");
    Serial.print(currentAngle, 2);
    Serial.print("° (step ");
    Serial.print(currentPosition);
    Serial.println(")");
}

void printStatus()
{
    Serial.println("\n=== TMC2209 Status ===");
    
    // // Driver status register
    // uint32_t drv_status = driver.DRV_STATUS();
    // Serial.print("DRV_STATUS: 0x");
    // Serial.println(drv_status, HEX);
    
    // // Parse status bits
    // Serial.print("Standstill: ");
    // Serial.println((drv_status >> 31) & 0x01 ? "Yes" : "No");
    
    // Serial.print("Overtemp warning: ");
    // Serial.println(driver.otpw() ? "YES!" : "No");
    
    // Serial.print("Overtemp shutdown: ");
    // Serial.println(driver.ot() ? "YES!" : "No");
    
    // Serial.print("Short to GND A: ");
    // Serial.println(driver.s2ga() ? "YES!" : "No");
    
    // Serial.print("Short to GND B: ");
    // Serial.println(driver.s2gb() ? "YES!" : "No");
    
    // // Current settings
    // Serial.print("\nRMS Current: ");
    // Serial.print(driver.rms_current());
    // Serial.println(" mA");
    
    // Serial.print("VSense: ");
    // Serial.println(driver.vsense() ? "High (0.18V)" : "Low (0.32V)");
    
    // Serial.print("Microsteps: ");
    // Serial.println(driver.microsteps());
    
    // Serial.print("StealthChop: ");
    // Serial.println(driver.en_spreadCycle() ? "Disabled" : "Enabled");
    
    // // Motor load
    // Serial.print("Motor Load: ");
    // Serial.println(driver.cs_actual());
    
    // Position
    Serial.print("\nCurrent Position: ");
    Serial.print(currentAngle, 2);
    Serial.print("° (microstep ");
    Serial.print(currentPosition);
    Serial.println(")");
    
    Serial.print("Step delay: ");
    Serial.print(STEP_DELAY);
    Serial.println(" μs");
    
    Serial.println("==================\n");
}

void printHelp()
{
    Serial.println("\n=== COMMAND HELP ===");
    Serial.println("MOVEMENT:");
    Serial.println("  l/r     - Single step left/right");
    Serial.println("  L/R     - 10 steps left/right");
    Serial.println("  </>     - 100 steps left/right");
    Serial.println("  0-9     - Move to angle (0°, 30°, 60°...270°)");
    
    Serial.println("\nSPEED:");
    Serial.println("  +/-     - Slower/Faster");
    
    Serial.println("\nPOSITION:");
    Serial.println("  z       - Zero current position");
    Serial.println("  p       - Print current position");
    
    Serial.println("\nMOTOR:");
    Serial.println("  e/d     - Enable/Disable motor");
    
    Serial.println("\nINFO:");
    Serial.println("  s       - Show status");
    Serial.println("  h/?     - Show this help");
    
    Serial.println("\nTIP: Open Serial Plotter to see position graph!");
    Serial.println("==================\n");
}