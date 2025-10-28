# Life Lane – AI & IoT Traffic Signal System (Demo Kit)

A compact demo that prioritizes ambulances at intersections using ESP32 nodes, Firebase Realtime Database, and a rules-based controller. This repository includes the Arduino sketches and a lightweight web dashboard.

<!-- Badges (optional) -->
<!-- Example: -->
<!-- ![Arduino](https://img.shields.io/badge/Arduino-ESP32-00979D?logo=arduino&logoColor=white) -->
<!-- ![Firebase](https://img.shields.io/badge/Firebase-RTDB-FFCA28?logo=firebase&logoColor=black) -->

<!-- Hero Image Placeholder -->
<!-- Add a system overview image or demo photo here -->
<!-- ![Life Lane Demo](docs/images/hero.jpg) -->

## Overview
- Ambulance ESP32 publishes GPS (or simulated) coordinates to Firebase.
- Traffic Signal ESP32 listens for an emergency flag and forces GREEN when active.
- Web Dashboard presents live status and basic logs.

## Table of Contents
- Features
- Project Structure
- Hardware & Connections
- Setup
- Usage
- Troubleshooting
- Security
- Roadmap
- License

## Features
- Real-time ambulance location publishing via RTDB
- Preemption of traffic signal to GREEN during emergency
- Simple dashboard to visualize state and logs
- Indoor testing support with simulated GPS

## Project Structure
```
lifeline/
├── ESP32_Code/
│   ├── ambulance_gps/
│   │   └── ambulance_gps.ino
│   └── traffic_signal/
│       └── traffic_signal.ino
├── Web_Dashboard/
│   ├── index.html
│   ├── style.css
│   └── script.js
├── Connections/
│   └── wiring_diagram.md
├── CP210x_Windows_Drivers/
└── README.md
```

<!-- Architecture Diagram Placeholder -->
<!-- ![Architecture](docs/images/architecture.png) -->

## Hardware & Connections
- Boards: 2x ESP32 DevKit
- Peripherals: LEDs (R/Y/G), resistors, optional GPS module for ambulance node
- USB-UART Driver: CP210x (Windows drivers included in CP210x_Windows_Drivers)
- Wiring: Refer to Connections/wiring_diagram.md

<!-- Wiring Image Placeholder -->
<!-- ![Wiring](docs/images/wiring.png) -->

## Setup

### 1) Firebase (Realtime Database)
1. Create a Firebase project (e.g., life-lane-demo).
2. Enable Realtime Database and set permissive rules for a quick demo only:
   ```json
   { "rules": { ".read": true, ".write": true } }
   ```
3. Note your Database URL (e.g., https://<project-id>-default-rtdb.firebaseio.com).

### 2) ESP32 Firmware (Arduino IDE)
- Install Arduino IDE and the ESP32 board package.
- Install libraries used by the sketches (e.g., Firebase-ESP-Client/FirebaseESP32, TinyGPS++).
- Open:
  - ESP32_Code/ambulance_gps/ambulance_gps.ino
  - ESP32_Code/traffic_signal/traffic_signal.ino
- Fill in Wi-Fi credentials and Firebase configuration placeholders.
- Optionally enable simulated GPS for indoor testing.

### 3) Web Dashboard
- Open Web_Dashboard/index.html directly in a browser (file://) or serve via a static server.
- Update Web_Dashboard/script.js with your Firebase web config.

<!-- Dashboard Screenshot Placeholder -->
<!-- ![Dashboard](docs/images/dashboard.png) -->

## Usage
1. Power both ESP32 devices and connect to Wi‑Fi.
2. Start the ambulance sketch to publish GPS or simulated coordinates.
3. Observe the traffic signal sketch: normal cycle → preempt to GREEN when emergency flag is set → resume after clear.
4. Open the web dashboard to monitor state and logs.

## Troubleshooting
- ESP32 Wi‑Fi: verify SSID/password and 2.4 GHz network.
- Firebase: ensure the database URL is correct and credentials are valid.
- GPS: test outdoors or switch to simulated coordinates.
- Dashboard: confirm Firebase web config in script.js and database rules allow reads.

## Security
For demo only. For production:
- Use OAuth-based auth with Firebase-ESP-Client; avoid raw RTDB secrets.
- Restrict database rules to authenticated users.
- Keep secrets out of source control; use environment variables/secret managers.

## Roadmap
- Multiple ambulances under /ambulances/{id} with per-signal routing
- Map visualization on the dashboard
- Improved time-series predictions based on real data
- Offline/edge failover if cloud is unavailable

## License
Add your chosen license here (e.g., MIT, Apache-2.0) and include a LICENSE file.

---

Placeholders for images and badges are included as comments in this README. Replace them with actual image links/files when ready.
