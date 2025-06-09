/*
  Water Flow Sensor Node for ESP Mesh Network
  Modified from RandomNerdTutorials ESP Mesh example
  
  This code is for ESP32 nodes with water flow sensors (YF-S201 or similar)
  Connect water flow sensor signal pin to GPIO 4
*/

#include "painlessMesh.h"
#include <Arduino_JSON.h>

// MESH Details
#define   MESH_PREFIX     "WATERFLOWMESH"
#define   MESH_PASSWORD   "MESHpassword"
#define   MESH_PORT       5555

// Water Flow Sensor Pin
#define FLOW_SENSOR_PIN 4

// Node Configuration
int nodeNumber = 1; // Change this to 2 for second sensor node

// Flow sensor variables
volatile int flowPulseCount = 0;
float flowRate = 0.0;
float totalVolume = 0.0;
unsigned long oldTime = 0;
const float calibrationFactor = 7.5; // Pulses per liter per minute for YF-S201

// Mesh objects
Scheduler userScheduler;
painlessMesh mesh;

// Function prototypes
void sendMessage();
String getFlowReadings();
void IRAM_ATTR flowPulseCounter();

// Task to send readings every 1 second for real-time monitoring
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

// Interrupt service routine for flow sensor
void IRAM_ATTR flowPulseCounter() {
  flowPulseCount++;
}

void setup() {
  Serial.begin(115200);
  
  // Initialize flow sensor
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Start the task
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
  
  Serial.println("Water Flow Sensor Node Started");
  Serial.printf("Node Number: %d\n", nodeNumber);
}

void loop() {
  mesh.update();
  calculateFlowRate();
}

void calculateFlowRate() {
  if ((millis() - oldTime) > 1000) { // Calculate every second
    // Disable interrupts temporarily
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
    
    // Calculate flow rate in L/min
    flowRate = ((1000.0 / (millis() - oldTime)) * flowPulseCount) / calibrationFactor;
    
    // Add to total volume (in liters)
    totalVolume += (flowRate / 60); // Convert to L/s then add
    
    // Reset for next calculation
    flowPulseCount = 0;
    oldTime = millis();
    
    // Re-enable interrupts
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
    
    // Print readings to serial
    Serial.printf("Flow Rate: %.2f L/min, Total Volume: %.2f L\n", flowRate, totalVolume);
  }
}

String getFlowReadings() {
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["flowRate"] = flowRate;
  jsonReadings["totalVolume"] = totalVolume;
  jsonReadings["timestamp"] = mesh.getNodeTime();
  
  return JSON.stringify(jsonReadings);
}

void sendMessage() {
  String msg = getFlowReadings();
  mesh.sendBroadcast(msg);
  Serial.printf("Sending: %s\n", msg.c_str());
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  JSONVar myObject = JSON.parse(msg.c_str());
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
  
  int node = myObject["node"];
  double flowRate = myObject["flowRate"];
  double totalVolume = myObject["totalVolume"];
  
  Serial.printf("Node %d - Flow Rate: %.2f L/min, Total: %.2f L\n", 
                node, flowRate, totalVolume);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
