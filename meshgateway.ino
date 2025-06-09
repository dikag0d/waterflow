/*
  MQTT Gateway Node for Water Flow Mesh Network
  This ESP32 connects to WiFi and forwards mesh data to MQTT broker
  
  Install required libraries:
  - painlessMesh
  - PubSubClient
  - ArduinoJson
*/

#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char* mqtt_broker = "broker.hivemq.com"; // Free MQTT broker for testing
const int mqtt_port = 1883;
const char* mqtt_user = ""; // Leave empty for public brokers
const char* mqtt_password = "";
const char* mqtt_topic_base = "waterflow/sensor";

// MESH Details (same as sensor nodes)
#define MESH_PREFIX     "WATERFLOWMESH"
#define MESH_PASSWORD   "MESHpassword"
#define MESH_PORT       5555

// Node Configuration
int nodeNumber = 3; // Gateway node

// Objects
Scheduler userScheduler;
painlessMesh mesh;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Function prototypes
void connectToWiFi();
void connectToMQTT();
void publishToMQTT(String topic, String payload);
void checkConnections();

// Task to check connections every 10 seconds for better responsiveness
Task taskCheckConnections(TASK_SECOND * 10, TASK_FOREVER, &checkConnections);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqtt_broker, mqtt_port);
  connectToMQTT();
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Start connection check task
  userScheduler.addTask(taskCheckConnections);
  taskCheckConnections.enable();
  
  Serial.println("MQTT Gateway Node Started");
}

void loop() {
  mesh.update();
  mqttClient.loop();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.printf("Connected to WiFi: %s\n", ssid);
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    String client_id = "esp32-gateway-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s\n", client_id.c_str());
    
    if (mqttClient.connect(client_id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      
      // Subscribe to control topics if needed
      String controlTopic = String(mqtt_topic_base) + "/control";
      mqttClient.subscribe(controlTopic.c_str());
      
    } else {
      Serial.printf("Failed to connect to MQTT broker, rc=%d\n", mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void publishToMQTT(String topic, String payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.printf("Published to %s: %s\n", topic.c_str(), payload.c_str());
  } else {
    Serial.println("MQTT not connected, attempting reconnection...");
    connectToMQTT();
  }
}

void checkConnections() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectToWiFi();
  }
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectToMQTT();
  }
  
  // Send status update
  JSONVar statusMsg;
  statusMsg["gateway_node"] = nodeNumber;
  statusMsg["wifi_status"] = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
  statusMsg["mqtt_status"] = mqttClient.connected() ? "connected" : "disconnected";
  statusMsg["mesh_nodes"] = mesh.getNodeList().size() + 1; // +1 for self
  statusMsg["timestamp"] = mesh.getNodeTime();
  
  String statusTopic = String(mqtt_topic_base) + "/gateway/status";
  publishToMQTT(statusTopic, JSON.stringify(statusMsg));
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from mesh node %u: %s\n", from, msg.c_str());
  
  JSONVar myObject = JSON.parse(msg.c_str());
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing mesh message failed!");
    return;
  }
  
  // Extract data
  int node = myObject["node"];
  double flowRate = myObject["flowRate"];
  double totalVolume = myObject["totalVolume"];
  
  Serial.printf("Node %d - Flow Rate: %.2f L/min, Total: %.2f L\n", 
                node, flowRate, totalVolume);
  
  // Forward to MQTT
  String mqttTopic = String(mqtt_topic_base) + "/node" + String(node);
  
  // Create enhanced message with gateway info
  JSONVar mqttMsg;
  mqttMsg["sensor_node"] = node;
  mqttMsg["flow_rate_lpm"] = flowRate;
  mqttMsg["total_volume_l"] = totalVolume;
  mqttMsg["gateway_node"] = nodeNumber;
  mqttMsg["mesh_timestamp"] = myObject["timestamp"];
  mqttMsg["mqtt_timestamp"] = millis();
  
  publishToMQTT(mqttTopic, JSON.stringify(mqttMsg));
  
  // Also publish to a combined topic for easy monitoring
  String combinedTopic = String(mqtt_topic_base) + "/all";
  publishToMQTT(combinedTopic, JSON.stringify(mqttMsg));
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New mesh connection, nodeId = %u\n", nodeId);
  
  // Notify via MQTT
  JSONVar connectionMsg;
  connectionMsg["event"] = "new_connection";
  connectionMsg["node_id"] = nodeId;
  connectionMsg["gateway_node"] = nodeNumber;
  connectionMsg["timestamp"] = mesh.getNodeTime();
  
  String eventTopic = String(mqtt_topic_base) + "/events";
  publishToMQTT(eventTopic, JSON.stringify(connectionMsg));
}

void changedConnectionCallback() {
  Serial.println("Mesh connections changed");
  
  // Get current node list
  auto nodes = mesh.getNodeList();
  Serial.printf("Current mesh nodes: %d\n", nodes.size());
  
  // Notify via MQTT
  JSONVar connectionMsg;
  connectionMsg["event"] = "connections_changed";
  connectionMsg["active_nodes"] = nodes.size() + 1; // +1 for gateway
  connectionMsg["gateway_node"] = nodeNumber;
  connectionMsg["timestamp"] = mesh.getNodeTime();
  
  String eventTopic = String(mqtt_topic_base) + "/events";
  publishToMQTT(eventTopic, JSON.stringify(connectionMsg));
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
