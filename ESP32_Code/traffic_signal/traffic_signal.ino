/*
 * ESP32 Traffic Signal Controller -> Firebase RTDB
 * Reads emergency status from ambulance tracker and controls traffic lights
 * Uses FirebaseClient library (same as ambulance code)
 * 
 * Features:
 * - Normal traffic light cycle (Green -> Yellow -> Red)
 * - Emergency mode (Force Green when ambulance nearby)
 * - Firebase RTDB integration with proper authentication
 * - Status LED indicators and comprehensive logging
 * 
 * Hardware Connections:
 * - Red LED: GPIO2
 * - Yellow LED: GPIO4  
 * - Green LED: GPIO5
 * - Built-in LED: GPIO2 (status indicator)
 * 
 * Author: Traffic Management System
 * Version: 1.0
 */

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// WiFi Credentials (SAME AS AMBULANCE)
#define WIFI_SSID "lifeline" 
#define WIFI_PASSWORD "9876543210" 

// Firebase Configuration (SAME AS AMBULANCE)
#define Web_API_KEY "AIzaSyCCYaEui8ptVXwilygEbmJIqM2T4xUa3ZY" 
#define DATABASE_URL "https://life-lane-traffic-default-rtdb.asia-southeast1.firebasedatabase.app/" 
#define USER_EMAIL "admin@lifeline.com" 
#define USER_PASS "lifeline@123"

// Traffic Light Pins
#define RED_LED    2
#define YELLOW_LED 4
#define GREEN_LED  5
#define LED_BUILTIN 2


// Add after your defines
#define INTERSECTION_ID "intersection_001"  // Change this for each traffic signal
#define PROXIMITY_TIMEOUT 45000  // Clear emergency after 45 seconds

// Timing Configuration
#define NORMAL_GREEN_MS  10000  // 10 seconds
#define NORMAL_YELLOW_MS 3000   // 3 seconds  
#define NORMAL_RED_MS    10000  // 10 seconds
#define CHECK_INTERVAL   2000   // Check Firebase every 2 seconds

// Traffic States
enum TrafficState {
  NORMAL_GREEN,
  NORMAL_YELLOW, 
  NORMAL_RED,
  EMERGENCY_GREEN
};

// ====== FIREBASE SETUP ======
void processFirebaseData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// ====== TRAFFIC CONTROLLER VARIABLES ======
TrafficState currentState = NORMAL_GREEN;
unsigned long lastStateChange = 0;
unsigned long lastFirebaseCheck = 0;
bool emergencyActive = false;
bool wifiConnected = false;
bool firebaseReady = false;
unsigned long emergencyStartTime = 0;
bool emergencyTimedOut = false;

// ====== UTILITY FUNCTIONS ======

void blinkBuiltinLED(int times, int delayMs = 200) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
  }
}

void setTrafficLights(bool red, bool yellow, bool green) {
  digitalWrite(RED_LED, red ? HIGH : LOW);
  digitalWrite(YELLOW_LED, yellow ? HIGH : LOW);
  digitalWrite(GREEN_LED, green ? HIGH : LOW);
  
  // Debug output
  Serial.printf("Traffic Lights: R=%s Y=%s G=%s\n", 
                red ? "ON" : "OFF", 
                yellow ? "ON" : "OFF", 
                green ? "ON" : "OFF");
}

String getStateName(TrafficState state) {
  switch(state) {
    case NORMAL_GREEN: return "NORMAL_GREEN";
    case NORMAL_YELLOW: return "NORMAL_YELLOW"; 
    case NORMAL_RED: return "NORMAL_RED";
    case EMERGENCY_GREEN: return "EMERGENCY_GREEN";
    default: return "UNKNOWN";
  }
}

// ====== WIFI FUNCTIONS ======

void initWiFi() {
  Serial.println("\n========================= WiFi Setup =========================");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nWiFi Connected Successfully!");
    Serial.printf("\nIP Address: %s", WiFi.localIP().toString().c_str());
    Serial.printf("\nSignal Strength: %d dBm\n", WiFi.RSSI());
    blinkBuiltinLED(3, 100);
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi Connection Failed!");
    blinkBuiltinLED(5, 100);
  }
}

// ====== FIREBASE FUNCTIONS ======

void initFirebase() {
  Serial.println("\n======================= Firebase Setup =======================");
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(10000);
  ssl_client.setHandshakeTimeout(10);
  
  // Initialize Firebase App
  Serial.println("Initializing Firebase authentication...");
  initializeApp(aClient, app, getAuth(user_auth), processFirebaseData, "TrafficAuthTask");
  
  // Initialize Realtime Database
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  
  Serial.println("Firebase initialization started");
  Serial.println("Waiting for authentication...");
  
  blinkBuiltinLED(2, 300);
}

void processFirebaseData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()) {
    Firebase.printf("Event: %s, msg: %s, code: %d\n", 
                   aResult.uid().c_str(), 
                   aResult.eventLog().message().c_str(), 
                   aResult.eventLog().code());
    
    // Check if authentication is successful
    if (aResult.eventLog().code() == 10) {  // Code 10 means ready
      firebaseReady = true;
      Serial.println("Firebase authentication successful!");
    }
  }

  if (aResult.isDebug()) {
    Firebase.printf("Debug: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Firebase.printf("Error: %s, msg: %s, code: %d\n", 
                   aResult.uid().c_str(), 
                   aResult.error().message().c_str(), 
                   aResult.error().code());
  }

  if (aResult.available()) {
    Firebase.printf("Data: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    
    // Handle emergency status response
    // Replace the emergency handling section in processFirebaseData with:
if (String(aResult.uid().c_str()).indexOf("CheckEmergency") >= 0) {
  String payload = String(aResult.c_str());
  
  if (payload.indexOf("true") >= 0) {
    if (!emergencyActive) {
      emergencyActive = true;
      emergencyStartTime = millis();
      emergencyTimedOut = false;
      currentState = EMERGENCY_GREEN;
      Serial.println("EMERGENCY MODE ACTIVATED!");
      blinkBuiltinLED(5, 100);
    }
  } else {
    if (emergencyActive && !emergencyTimedOut) {
      emergencyActive = false;
      Serial.println("Emergency cleared by ambulance movement");
    }
  }
}
}
}

void checkEmergencyStatus() {
  if (!wifiConnected || !firebaseReady) {
    return;
  }
  
  if (millis() - lastFirebaseCheck > CHECK_INTERVAL) {
    lastFirebaseCheck = millis();
    
    // Check emergency status for THIS specific intersection
    String emergencyPath = "/intersections/" + String(INTERSECTION_ID) + "/emergency_active";
    Database.get(aClient, emergencyPath, processFirebaseData, "CheckEmergency");
    
    // Handle emergency timeout
    if (emergencyActive && (millis() - emergencyStartTime > PROXIMITY_TIMEOUT)) {
      emergencyActive = false;
      emergencyTimedOut = true;
      Serial.println("Emergency timed out - resuming normal cycle");
    }
  }
}

// ====== TRAFFIC CONTROL FUNCTIONS ======

void handleEmergencyMode() {
  setTrafficLights(false, false, true); // Force GREEN
  
  // Status indicator
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Fast blink
  }
}

void handleNormalCycle() {
  unsigned long now = millis();
  unsigned long elapsed = now - lastStateChange;
  
  switch (currentState) {
    case NORMAL_GREEN:
      setTrafficLights(false, false, true); // GREEN
      if (elapsed >= NORMAL_GREEN_MS) {
        currentState = NORMAL_YELLOW;
        lastStateChange = now;
        Serial.println("Traffic: GREEN -> YELLOW");
      }
      break;
      
    case NORMAL_YELLOW:
      setTrafficLights(false, true, false); // YELLOW
      if (elapsed >= NORMAL_YELLOW_MS) {
        currentState = NORMAL_RED;
        lastStateChange = now;
        Serial.println("Traffic: YELLOW -> RED");
      }
      break;
      
    case NORMAL_RED:
      setTrafficLights(true, false, false); // RED
      if (elapsed >= NORMAL_RED_MS) {
        currentState = NORMAL_GREEN;
        lastStateChange = now;
        Serial.println("Traffic: RED -> GREEN");
      }
      break;
      
    case EMERGENCY_GREEN:
      // If emergency is cleared, resume normal cycle
      if (!emergencyActive) {
        currentState = NORMAL_GREEN;
        lastStateChange = now;
        Serial.println("Resuming normal cycle from GREEN");
      }
      break;
      
    default:
      currentState = NORMAL_GREEN;
      lastStateChange = now;
      break;
  }
  
  // Slow status blink for normal mode
  static unsigned long lastStatusBlink = 0;
  if (millis() - lastStatusBlink > 2000) {
    lastStatusBlink = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void printSystemStatus() {
  Serial.println("\n=================== TRAFFIC SYSTEM STATUS ===================");
  Serial.printf("WiFi: %s", wifiConnected ? "Connected" : "Disconnected");
  if (wifiConnected) {
    Serial.printf(" (%s)", WiFi.localIP().toString().c_str());
  }
  Serial.println();
  
  Serial.printf("Firebase: %s\n", firebaseReady ? "Ready" : "Not Ready");
  Serial.printf("Emergency Mode: %s\n", emergencyActive ? "ACTIVE" : "Normal");
  Serial.printf("Current State: %s\n", getStateName(currentState).c_str());
  Serial.printf("State Duration: %lu ms\n", millis() - lastStateChange);
  Serial.printf("Free Memory: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.println("=========================================================");
}

// ====== MAIN SETUP ======

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize pins
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Turn off all lights initially
  setTrafficLights(false, false, false);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.println("\n=============================================================");
  Serial.println("    ESP32 TRAFFIC SIGNAL CONTROLLER v1.0");
  Serial.println("    Emergency Vehicle Detection System");
  Serial.println("    Built with Firebase Client Library");
  Serial.println("=============================================================");
  
  // Initialize systems
  initWiFi();
  if (wifiConnected) {
    initFirebase();
  }
  
  // Start normal cycle
  currentState = NORMAL_GREEN;
  lastStateChange = millis();
  
  Serial.println("\nTraffic Signal Controller Ready!");
  Serial.println("Commands: 'S' for status, 'R' for restart");
  Serial.println("=============================================================\n");
  
  blinkBuiltinLED(1, 1000);
}

// ====== MAIN LOOP ======

void loop() {
  // Maintain Firebase connection
  app.loop();
  
  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    command.toUpperCase();
    
    if (command == "S") {
      printSystemStatus();
    } else if (command == "R") {
      Serial.println("Restarting system in 3 seconds...");
      delay(3000);
      ESP.restart();
    } else {
      Serial.println("Available commands: S (status), R (restart)");
    }
  }
  
  // Check for emergency status from Firebase
  checkEmergencyStatus();
  
  // Control traffic lights
  if (emergencyActive) {
    handleEmergencyMode();
  } else {
    handleNormalCycle();
  }
  
  // Small delay
  delay(100);
}