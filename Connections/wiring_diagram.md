# Connections / Wiring Diagram

> **Boards:** 2 × ESP32 DevKit  
> **Power:** USB for both boards (or 5V supply)

## ESP32 #1 — Ambulance GPS Tracker
**GPS Module → ESP32 (3.3V logic!)**
- VCC → **3.3V**
- GND → **GND**
- TX  → **GPIO16 (RX2)**
- RX  → **GPIO17 (TX2)**

> Use `HardwareSerial(2)` at 9600 baud. Keep GPS antenna near window.

## ESP32 #2 — Traffic Signal Controller
**LEDs (with 220 Ω resistors) → ESP32**
- Red LED (anode)   → **GPIO2** (through 220Ω)
- Yellow LED (anode)→ **GPIO4** (through 220Ω)
- Green LED (anode) → **GPIO5** (through 220Ω)
- All cathodes → **GND**

> If LEDs are too dim, try 100–220 Ω. Ensure common GND.

## Network
- Both ESP32s on same WiFi SSID
- Internet access required for Firebase RTDB

## Notes
- Double-check board selected in Arduino IDE: **ESP32 Dev Module**
- If GPS shows no data, check satellites, move outdoors, or use **simulateGPS()**.
