#include <VanBus.h>

// Pin definitions for ESP32-S3 with SN65HVD230
#define VAN_RX_PIN 4
#define VAN_TX_PIN 5

// VAN packet identifiers for Peugeot 307 (based on PSAVanCanBridge research)
#define ENGINE_STATUS_IDEN 0x824     // Engine RPM, temperature, etc.
#define DASHBOARD_IDEN 0x8A4         // Dashboard display data
#define SPEED_IDEN 0x4D4             // Vehicle speed
#define FUEL_LEVEL_IDEN 0x554        // Fuel level and range
#define DOOR_STATUS_IDEN 0x4FC       // Door status, lights, indicators
#define OUTSIDE_TEMP_IDEN 0x8C4      // Outside temperature
#define TIME_IDEN 0x564              // Time and date
#define TRIP_COMPUTER_IDEN 0x524     // Trip computer data
#define WARNING_LIGHTS_IDEN 0x664    // Warning lights and indicators
#define AC_STATUS_IDEN 0x744         // Air conditioning status

// Function declarations
void processVanPacket(const TVanPacketRxDesc& packet);
void displayDashboardData();
void processSerialCommands();
void requestTripComputerData();
void resetTripComputer();
void simulateDashboardButton(uint8_t buttonCode);
void setClockTime(uint8_t hours, uint8_t minutes);
void flashHazardLights(bool enable);
void requestDiagnosticData(uint16_t diagnosticID);
void sendCustomVanPacket(uint16_t iden, uint8_t cmdFlags, const uint8_t* data, size_t dataLen);
void debugPacket(const TVanPacketRxDesc& packet);

// Structure to hold all dashboard data
struct DashboardData {
  uint16_t rpm = 0;
  uint8_t speed = 0;
  uint8_t engineTemp = 0;
  uint8_t fuelLevel = 0;
  uint16_t fuelRange = 0;
  int8_t outsideTemp = -128;  // Invalid temperature
  uint8_t hour = 0;
  uint8_t minute = 0;
  
  // Trip computer
  uint16_t avgConsumption = 0;
  uint16_t instantConsumption = 0;
  uint16_t tripDistance = 0;
  
  // Status flags
  bool leftIndicator = false;
  bool rightIndicator = false;
  bool hazardLights = false;
  bool headlights = false;
  bool parkingLights = false;
  bool engineWarning = false;
  bool oilWarning = false;
  bool batteryWarning = false;
  bool handbrake = false;
  
  // Door status
  bool frontLeftDoor = false;
  bool frontRightDoor = false;
  bool rearLeftDoor = false;
  bool rearRightDoor = false;
  bool tailgate = false;
  bool bonnet = false;
};

// Global variables
DashboardData dashboard;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTripRequest = 0;
const unsigned long DISPLAY_INTERVAL = 1000; // Update display every second
const unsigned long TRIP_REQUEST_INTERVAL = 5000; // Request trip data every 5 seconds
bool hazardState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Peugeot 307 VAN Dashboard Data Reader ===");
  Serial.println("Reading all dashboard data from VAN bus...");
  Serial.println("Commands available via Serial:");
  Serial.println("  'r' - Reset trip computer");
  Serial.println("  't' - Request trip computer data");
  Serial.println("  'h' - Toggle hazard lights");
  Serial.println("  's HH MM' - Set clock (e.g., 's 14 30')");
  Serial.println("  'd XXXX' - Send diagnostic request (hex)");
  Serial.println("  'b XX' - Simulate dashboard button (hex)");
  
  // Initialize VAN bus
  TVanBus::Setup(VAN_RX_PIN, VAN_TX_PIN);
  
  Serial.println("VAN Bus initialized. Monitoring dashboard data...");
  Serial.println("===============================================");
}

void loop() {
  TVanPacketRxDesc packet;
  
  // Process incoming VAN packets
  if (VanBus.Receive(packet)) {
    processVanPacket(packet);
  }
  
  // Handle serial commands
  processSerialCommands();
  
  // Periodically request trip computer data (some data needs requesting)
  if (millis() - lastTripRequest >= TRIP_REQUEST_INTERVAL) {
    requestTripComputerData();
    lastTripRequest = millis();
  }
  
  // Display dashboard data periodically
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    displayDashboardData();
    lastDisplayUpdate = millis();
  }
  
  delay(1);
}

void processVanPacket(const TVanPacketRxDesc& packet) {
  uint16_t iden = packet.Iden();
  const uint8_t* data = packet.Data();
  int dataLen = packet.DataLen();
  
  switch (iden) {
    case ENGINE_STATUS_IDEN: // 0x824 - Engine data
      if (dataLen >= 8) {
        // RPM in bytes 2-3
        dashboard.rpm = ((data[2] << 8) | data[3]) / 8;
        // Engine temperature in byte 4 (offset by 40°C)
        dashboard.engineTemp = data[4] - 40;
      }
      break;
      
    case SPEED_IDEN: // 0x4D4 - Vehicle speed
      if (dataLen >= 2) {
        dashboard.speed = data[1]; // Speed in km/h
      }
      break;
      
    case FUEL_LEVEL_IDEN: // 0x554 - Fuel level
      if (dataLen >= 7) {
        dashboard.fuelLevel = data[5]; // Fuel level percentage
        dashboard.fuelRange = (data[6] << 8) | data[7]; // Range in km
      }
      break;
      
    case DOOR_STATUS_IDEN: // 0x4FC - Doors and lights
      if (dataLen >= 3) {
        // Parse door status from byte 0
        dashboard.frontLeftDoor = (data[0] & 0x01) != 0;
        dashboard.frontRightDoor = (data[0] & 0x02) != 0;
        dashboard.rearLeftDoor = (data[0] & 0x04) != 0;
        dashboard.rearRightDoor = (data[0] & 0x08) != 0;
        dashboard.tailgate = (data[0] & 0x10) != 0;
        dashboard.bonnet = (data[0] & 0x20) != 0;
        
        // Parse lights from byte 1
        dashboard.leftIndicator = (data[1] & 0x01) != 0;
        dashboard.rightIndicator = (data[1] & 0x02) != 0;
        dashboard.hazardLights = (data[1] & 0x04) != 0;
        dashboard.parkingLights = (data[1] & 0x08) != 0;
        dashboard.headlights = (data[1] & 0x10) != 0;
        dashboard.handbrake = (data[1] & 0x20) != 0;
      }
      break;
      
    case OUTSIDE_TEMP_IDEN: // 0x8C4 - Outside temperature
      if (dataLen >= 2) {
        dashboard.outsideTemp = data[1] - 40; // Temperature with -40°C offset
      }
      break;
      
    case TIME_IDEN: // 0x564 - Time
      if (dataLen >= 3) {
        dashboard.hour = data[1];
        dashboard.minute = data[2];
      }
      break;
      
    case TRIP_COMPUTER_IDEN: // 0x524 - Trip computer
      if (dataLen >= 8) {
        dashboard.avgConsumption = (data[2] << 8) | data[3];
        dashboard.instantConsumption = (data[4] << 8) | data[5];
        dashboard.tripDistance = (data[6] << 8) | data[7];
      }
      break;
      
    case WARNING_LIGHTS_IDEN: // 0x664 - Warning lights
      if (dataLen >= 2) {
        dashboard.engineWarning = (data[0] & 0x01) != 0;
        dashboard.oilWarning = (data[0] & 0x02) != 0;
        dashboard.batteryWarning = (data[0] & 0x04) != 0;
      }
      break;
  }
}

void displayDashboardData() {
  Serial.println("\n=== DASHBOARD DATA ===");
  
  // Engine data
  Serial.print("RPM: ");
  Serial.print(dashboard.rpm);
  Serial.print(" | Speed: ");
  Serial.print(dashboard.speed);
  Serial.print(" km/h | Engine Temp: ");
  Serial.print(dashboard.engineTemp);
  Serial.println("°C");
  
  // Fuel data
  Serial.print("Fuel: ");
  Serial.print(dashboard.fuelLevel);
  Serial.print("% | Range: ");
  Serial.print(dashboard.fuelRange);
  Serial.println(" km");
  
  // Temperature and time
  Serial.print("Outside Temp: ");
  if (dashboard.outsideTemp != -128) {
    Serial.print(dashboard.outsideTemp);
    Serial.print("°C");
  } else {
    Serial.print("--");
  }
  Serial.print(" | Time: ");
  if (dashboard.hour < 10) Serial.print("0");
  Serial.print(dashboard.hour);
  Serial.print(":");
  if (dashboard.minute < 10) Serial.print("0");
  Serial.println(dashboard.minute);
  
  // Trip computer
  Serial.print("Avg Consumption: ");
  Serial.print(dashboard.avgConsumption / 10.0, 1);
  Serial.print(" L/100km | Instant: ");
  Serial.print(dashboard.instantConsumption / 10.0, 1);
  Serial.print(" L/100km | Trip: ");
  Serial.print(dashboard.tripDistance / 10.0, 1);
  Serial.println(" km");
  
  // Indicators and lights
  Serial.print("Indicators: ");
  if (dashboard.leftIndicator) Serial.print("LEFT ");
  if (dashboard.rightIndicator) Serial.print("RIGHT ");
  if (dashboard.hazardLights) Serial.print("HAZARD ");
  if (dashboard.parkingLights) Serial.print("PARKING ");
  if (dashboard.headlights) Serial.print("HEADLIGHTS ");
  Serial.println();
  
  // Doors
  Serial.print("Doors: ");
  if (dashboard.frontLeftDoor) Serial.print("FL ");
  if (dashboard.frontRightDoor) Serial.print("FR ");
  if (dashboard.rearLeftDoor) Serial.print("RL ");
  if (dashboard.rearRightDoor) Serial.print("RR ");
  if (dashboard.tailgate) Serial.print("TAILGATE ");
  if (dashboard.bonnet) Serial.print("BONNET ");
  Serial.println();
  
  // Warnings
  Serial.print("Warnings: ");
  if (dashboard.engineWarning) Serial.print("ENGINE ");
  if (dashboard.oilWarning) Serial.print("OIL ");
  if (dashboard.batteryWarning) Serial.print("BATTERY ");
  if (dashboard.handbrake) Serial.print("HANDBRAKE ");
  Serial.println();
  
  Serial.println("======================");
}

void processSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "r") {
      resetTripComputer();
    }
    else if (command == "t") {
      requestTripComputerData();
    }
    else if (command == "h") {
      hazardState = !hazardState;
      flashHazardLights(hazardState);
    }
    else if (command.startsWith("s ")) {
      // Parse time setting: "s HH MM"
      int firstSpace = command.indexOf(' ');
      int secondSpace = command.indexOf(' ', firstSpace + 1);
      if (firstSpace > 0 && secondSpace > 0) {
        int hours = command.substring(firstSpace + 1, secondSpace).toInt();
        int minutes = command.substring(secondSpace + 1).toInt();
        setClockTime(hours, minutes);
      } else {
        Serial.println("Usage: s HH MM (e.g., s 14 30)");
      }
    }
    else if (command.startsWith("d ")) {
      // Parse diagnostic request: "d XXXX"
      String hexStr = command.substring(2);
      uint16_t diagID = (uint16_t)strtol(hexStr.c_str(), NULL, 16);
      requestDiagnosticData(diagID);
    }
    else if (command.startsWith("b ")) {
      // Parse button simulation: "b XX"
      String hexStr = command.substring(2);
      uint8_t buttonCode = (uint8_t)strtol(hexStr.c_str(), NULL, 16);
      simulateDashboardButton(buttonCode);
    }
    else if (command == "help") {
      Serial.println("\nAvailable commands:");
      Serial.println("  'r' - Reset trip computer");
      Serial.println("  't' - Request trip computer data");
      Serial.println("  'h' - Toggle hazard lights");
      Serial.println("  's HH MM' - Set clock (e.g., 's 14 30')");
      Serial.println("  'd XXXX' - Send diagnostic request (hex)");
      Serial.println("  'b XX' - Simulate dashboard button (hex)");
    }
    else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void requestTripComputerData() {
  uint8_t tripRequest[] = {0x00, 0x00, 0x00, 0x00};
  if (VanBus.SyncSendPacket(TRIP_COMPUTER_IDEN, 0x0E, tripRequest, sizeof(tripRequest))) {
    Serial.println("Requested trip computer data");
  }
}

void resetTripComputer() {
  uint8_t resetCommand[] = {0x40, 0x00, 0x00, 0x00};
  if (VanBus.SyncSendPacket(0x524, 0x08, resetCommand, sizeof(resetCommand))) {
    Serial.println("Trip computer reset sent successfully");
  } else {
    Serial.println("Failed to send trip computer reset");
  }
}

void simulateDashboardButton(uint8_t buttonCode) {
  uint8_t buttonPress[] = {buttonCode, 0x00, 0x00};
  if (VanBus.SyncSendPacket(0x8A4, 0x08, buttonPress, sizeof(buttonPress))) {
    Serial.print("Dashboard button 0x");
    Serial.print(buttonCode, HEX);
    Serial.println(" pressed");
  }
}

void setClockTime(uint8_t hours, uint8_t minutes) {
  if (hours > 23 || minutes > 59) {
    Serial.println("Invalid time format");
    return;
  }
  
  uint8_t timeData[] = {0x00, hours, minutes, 0x00};
  if (VanBus.SyncSendPacket(0x564, 0x08, timeData, sizeof(timeData))) {
    Serial.print("Clock set to ");
    if (hours < 10) Serial.print("0");
    Serial.print(hours);
    Serial.print(":");
    if (minutes < 10) Serial.print("0");
    Serial.println(minutes);
  } else {
    Serial.println("Failed to set clock");
  }
}

void flashHazardLights(bool enable) {
  uint8_t hazardCommand[] = {enable ? 0x01 : 0x00, 0x00, 0x00};
  if (VanBus.SyncSendPacket(0x4FC, 0x08, hazardCommand, sizeof(hazardCommand))) {
    Serial.print("Hazard lights ");
    Serial.println(enable ? "ON" : "OFF");
  }
}

void requestDiagnosticData(uint16_t diagnosticID) {
  uint8_t diagRequest[] = {0x03, (diagnosticID >> 8) & 0xFF, diagnosticID & 0xFF, 0x00};
  if (VanBus.SyncSendPacket(0x7CE, 0x0E, diagRequest, sizeof(diagRequest))) {
    Serial.print("Diagnostic request sent for ID: 0x");
    Serial.println(diagnosticID, HEX);
  }
}

void sendCustomVanPacket(uint16_t iden, uint8_t cmdFlags, const uint8_t* data, size_t dataLen) {
  if (VanBus.SyncSendPacket(iden, cmdFlags, data, dataLen)) {
    Serial.print("Custom packet sent - ID: 0x");
    Serial.print(iden, HEX);
    Serial.print(" Flags: 0x");
    Serial.print(cmdFlags, HEX);
    Serial.print(" Data: ");
    for (size_t i = 0; i < dataLen; i++) {
      if (data[i] < 0x10) Serial.print("0");
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Failed to send custom packet");
  }
}

void debugPacket(const TVanPacketRxDesc& packet) {
  Serial.print("ID: 0x");
  Serial.print(packet.Iden(), HEX);
  Serial.print(" Data: ");
  for (int i = 0; i < packet.DataLen(); i++) {
    if (packet.Data()[i] < 0x10) Serial.print("0");
    Serial.print(packet.Data()[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}