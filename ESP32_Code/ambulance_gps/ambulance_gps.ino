/*
 * ESP32 Ambulance GPS Tracker -> Firebase RTDB with Proximity Detection
 * Fully Optimized Version: v4.2 (Batched Firebase Writes)
 * 
 * Features:
 * - Live GPS tracking or simulation mode
 * - Batched Firebase writes
 * - Proximity detection for intersections
 * - Serial command interface: sim, status, reset, help
 */

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <TinyGPS++.h>
#include <FirebaseJson.h>

// WiFi Credentials
#define WIFI_SSID "lifelane" 
#define WIFI_PASSWORD "1234567890" 

// Firebase Configuration  
#define Web_API_KEY "AIzaSyCNZG5HECuNE3vSFOV66GQrFOzRZHyaKWg" 
#define DATABASE_URL "https://lifeline-27737-default-rtdb.asia-southeast1.firebasedatabase.app/" 
#define USER_EMAIL "admin@lifeline.com" 
#define USER_PASS "lifeline@123"

// GPS Configuration
#define GPS_BAUD 9600
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// System Configuration
#define LED_PIN 2
#define UPDATE_INTERVAL 5000
#define GPS_TIMEOUT 15000
#define PROXIMITY_RADIUS 300
#define PROXIMITY_UPDATE_INTERVAL 3000

// ====== INTERSECTION DEFINITIONS ======
struct Intersection {
  String id;
  double lat;
  double lng;
  String name;
  bool lastEmergencyState;
};

Intersection intersections[] = {
  {"intersection_001", 12.9700, 77.5900, "Silk Board Junction", false},
  {"intersection_002", 12.9750, 77.5950, "BTM Layout Signal", false},
  {"intersection_003", 12.9800, 77.6000, "Jayadeva Hospital Junction", false},
  {"intersection_004", 12.9650, 77.5850, "HSR Layout Signal", false},
  {"intersection_005", 12.9850, 77.6050, "Forum Mall Junction", false}
};
const int NUM_INTERSECTIONS = 5;

// ====== FIREBASE SETUP ======
void processFirebaseData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// ====== GPS SETUP ======
TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

// ====== AMBULANCE DATA STRUCTURE ======
struct AmbulanceData {
  double latitude = 0.0;
  double longitude = 0.0;
  double speed = 0.0;
  double altitude = 0.0;
  int satellites = 0;
  bool gpsValid = false;
  bool emergency = true;
  String ambulanceId = "AMB001";
  String driverName = "Emergency Driver";
  String status = "ACTIVE";
  unsigned long lastUpdate = 0;
} ambulanceData;

// ====== STATUS VARIABLES ======
bool wifiConnected = false;
bool firebaseReady = false;
unsigned long lastGPSUpdate = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastProximityUpdate = 0;
unsigned long lastWiFiCheck = 0;
bool simulationMode = false;
int pendingFirebaseTasks = 0;

// ====== UTILITY FUNCTIONS ======
double calculateDistance(double lat1, double lng1, double lat2, double lng2) {
  double R = 6371000;
  double dLat = radians(lat2 - lat1);
  double dLng = radians(lng2 - lng1);
  double a = sin(dLat/2) * sin(dLat/2) + 
             cos(radians(lat1)) * cos(radians(lat2)) * 
             sin(dLng/2) * sin(dLng/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

void blinkLED(int times, int delayMs = 200) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

String createSeparator(int length, char character = '=') {
  String sep = "";
  for (int i = 0; i < length; i++) sep += character;
  return sep;
}

void printStartupBanner() {
  Serial.println("\n" + createSeparator(65));
  Serial.println("🚑 AMBULANCE GPS TRACKER v4.2 - FULLY OPTIMIZED");
  Serial.println("   Batched Firebase Writes & Smart Proximity Updates");
  Serial.println(createSeparator(65));
}

// ====== WIFI FUNCTIONS ======
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("\n📡 Connecting to WiFi: %s", WIFI_SSID);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\n✅ WiFi Connected | IP: %s\n", WiFi.localIP().toString().c_str());
    blinkLED(3, 100);
  } else {
    Serial.println("\n❌ WiFi Failed!");
  }
}

void checkWiFiConnection() {
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      WiFi.reconnect();
    } else {
      wifiConnected = true;
    }
  }
}

// ====== FIREBASE FUNCTIONS ======
void initFirebase() {
  ssl_client.setInsecure();
  initializeApp(aClient, app, getAuth(user_auth), processFirebaseData, "AuthTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void processFirebaseData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;
  if (aResult.isEvent() && aResult.eventLog().code() == 10) firebaseReady = true;
  if (aResult.isResult() && pendingFirebaseTasks > 0) pendingFirebaseTasks--;
}

// ====== SEND LOCATION ======
void sendLocationToFirebase() {
  if (!wifiConnected || !firebaseReady || pendingFirebaseTasks > 3) return;
  String id = ambulanceData.ambulanceId;
  String basePath = "/ambulances/" + id;

  Database.set<double>(aClient, basePath + "/latitude",  ambulanceData.latitude,  processFirebaseData, "Latitude"); pendingFirebaseTasks++;
  Database.set<double>(aClient, basePath + "/longitude", ambulanceData.longitude, processFirebaseData, "Longitude"); pendingFirebaseTasks++;
  Database.set<double>(aClient, basePath + "/speed",     ambulanceData.speed,     processFirebaseData, "Speed"); pendingFirebaseTasks++;
  Database.set<double>(aClient, basePath + "/altitude",  ambulanceData.altitude,  processFirebaseData, "Altitude"); pendingFirebaseTasks++;
  Database.set<int>(aClient,    basePath + "/satellites", ambulanceData.satellites, processFirebaseData, "Satellites"); pendingFirebaseTasks++;
  Database.set<bool>(aClient,   basePath + "/gpsValid",   ambulanceData.gpsValid,   processFirebaseData, "GpsValid"); pendingFirebaseTasks++;
  Database.set<bool>(aClient,   basePath + "/emergency",  ambulanceData.emergency,  processFirebaseData, "Emergency"); pendingFirebaseTasks++;
  Database.set<String>(aClient, basePath + "/status",     ambulanceData.status,     processFirebaseData, "Status"); pendingFirebaseTasks++;
  Database.set<String>(aClient, basePath + "/driverName", ambulanceData.driverName, processFirebaseData, "DriverName"); pendingFirebaseTasks++;
  Database.set<unsigned long>(aClient, basePath + "/timestamp", millis(), processFirebaseData, "Timestamp"); pendingFirebaseTasks++;

  Serial.printf("📡 Ambulance data sent | %s | Lat: %.6f | Lng: %.6f | Speed: %.1f km/h\n",
                id.c_str(), ambulanceData.latitude, ambulanceData.longitude, ambulanceData.speed);
  lastFirebaseUpdate = millis();
}

// ====== PROXIMITY UPDATES ======
void updateProximityData() {
  if (!wifiConnected || !firebaseReady || pendingFirebaseTasks > 3) return;
  if (millis() - lastProximityUpdate < PROXIMITY_UPDATE_INTERVAL) return;

  FirebaseJson proximityJson;
  bool hasChanges = false;

  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    double distance = calculateDistance(ambulanceData.latitude, ambulanceData.longitude, intersections[i].lat, intersections[i].lng);
    bool nearIntersection = (distance <= PROXIMITY_RADIUS);

    if (nearIntersection != intersections[i].lastEmergencyState) {
      proximityJson.set(intersections[i].id + "/emergency_active", nearIntersection);
      proximityJson.set(intersections[i].id + "/ambulance_id", ambulanceData.ambulanceId);
      proximityJson.set(intersections[i].id + "/distance_meters", (int)distance);
      proximityJson.set(intersections[i].id + "/last_update", (int)millis());

      intersections[i].lastEmergencyState = nearIntersection;
      hasChanges = true;

      Serial.printf("🚦 %s: %s (%.0fm)\n", intersections[i].name.c_str(), nearIntersection ? "EMERGENCY ACTIVE" : "Normal", distance);
    }
  }

  if (hasChanges) {
    String jsonString;
    proximityJson.toString(jsonString);
    Database.set<String>(aClient, "/intersections", jsonString, processFirebaseData, "ProximityBatch");
    pendingFirebaseTasks++;
    Serial.println("📡 Batched proximity updates sent");
  }

  lastProximityUpdate = millis();
}

// ====== GPS FUNCTIONS ======
void initGPS() {
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("🛰 GPS initialized");
  delay(1000);
  Serial.println("📍 Waiting for GPS signal...");
}

void readGPSData() {
  bool newData = false;
  unsigned long start = millis();
  while (millis() - start < 1000) {
    while (GPSSerial.available()) if (gps.encode(GPSSerial.read())) newData = true;
  }

  if (newData && gps.location.isValid()) {
    ambulanceData.latitude = gps.location.lat();
    ambulanceData.longitude = gps.location.lng();
    ambulanceData.speed = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
    ambulanceData.altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
    ambulanceData.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
    ambulanceData.gpsValid = true;
    ambulanceData.lastUpdate = millis();
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  } else if (millis() - ambulanceData.lastUpdate > GPS_TIMEOUT) ambulanceData.gpsValid = false;
}

// ====== SIMULATION ======
void simulateGPSData() {
  static double baseLat = 12.9716;
  static double baseLng = 77.5946;
  static unsigned long lastSimUpdate = 0;
  if (millis() - lastSimUpdate > 2000) {
    ambulanceData.latitude = baseLat + (sin(millis() / 20000.0) * 0.01);
    ambulanceData.longitude = baseLng + (cos(millis() / 25000.0) * 0.01);
    ambulanceData.speed = 30 + (sin(millis() / 10000.0) * 20);
    ambulanceData.altitude = 900 + random(-10,10);
    ambulanceData.satellites = 8 + random(-2,4);
    ambulanceData.gpsValid = true;
    ambulanceData.lastUpdate = millis();
    lastSimUpdate = millis();
    digitalWrite(LED_PIN,HIGH); delay(100); digitalWrite(LED_PIN,LOW);
  }
}

void toggleSimulationMode() {
  simulationMode = !simulationMode;
  ambulanceData.status = simulationMode ? "SIMULATION" : "ACTIVE";
  Serial.printf("🔄 Simulation Mode: %s\n", simulationMode ? "ENABLED" : "DISABLED");
  blinkLED(simulationMode ? 5 : 2,150);
  for (int i=0;i<NUM_INTERSECTIONS;i++) intersections[i].lastEmergencyState=false;
}

// ====== SERIAL COMMANDS ======
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); command.toLowerCase();
    if (command=="sim") toggleSimulationMode();
    else if (command=="status") printSystemStatus();
    else if (command=="reset") ESP.restart();
    else if (command=="help") printHelpMenu();
  }
}

void printSystemStatus() {
  Serial.println("\n" + createSeparator(50));
  Serial.println("📊 SYSTEM STATUS");
  Serial.println(createSeparator(50));
  Serial.printf("WiFi: %s | IP: %s\n", wifiConnected?"Connected":"Disconnected",WiFi.localIP().toString().c_str());
  Serial.printf("Firebase: %s | Pending Tasks: %d\n",firebaseReady?"Ready":"Not Ready",pendingFirebaseTasks);
  Serial.printf("GPS: %s | Mode: %s\n",ambulanceData.gpsValid?"Valid":"Invalid",simulationMode?"Simulation":"Live");
  Serial.printf("Location: %.6f, %.6f\n",ambulanceData.latitude,ambulanceData.longitude);
  Serial.printf("Speed: %.1f km/h | Altitude: %.1fm\n",ambulanceData.speed,ambulanceData.altitude);
  Serial.printf("Satellites: %d | Emergency: %s\n",ambulanceData.satellites,ambulanceData.emergency?"Active":"Inactive");
  Serial.println(createSeparator(50));
}

void printHelpMenu() {
  Serial.println("\n" + createSeparator(40));
  Serial.println("📋 AVAILABLE COMMANDS");
  Serial.println(createSeparator(40));
  Serial.println("sim    - Toggle simulation mode");
  Serial.println("status - Show system status");
  Serial.println("reset  - Restart ESP32");
  Serial.println("help   - Show this menu");
  Serial.println(createSeparator(40));
}

// ====== MAIN SETUP ======
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  printStartupBanner();
  blinkLED(2,300);
  initWiFi();
  if (wifiConnected) initFirebase();
  initGPS();
  ambulanceData.lastUpdate = millis();
  delay(2000);
  printHelpMenu();
  Serial.println("\n🚀 System Ready! Ambulance tracking started...");
}

// ====== MAIN LOOP ======
void loop() {
  app.loop();
  handleSerialCommands();
  checkWiFiConnection();
  if (simulationMode) simulateGPSData(); else readGPSData();
  if (app.ready() && (millis()-lastFirebaseUpdate>UPDATE_INTERVAL) && (ambulanceData.gpsValid||simulationMode)) sendLocationToFirebase();
  if (app.ready() && (ambulanceData.gpsValid||simulationMode)) updateProximityData();
  delay(100);
}