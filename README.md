# Life Lane – AI & IoT Traffic Signal System (Demo Kit)

Save lives by **prioritizing ambulances** at intersections using ESP32s, Firebase RTDB, and a lightweight ML predictor.

## 📁 Project Structure
```
Life_Lane_Demo/
├── ESP32_Code/
│   ├── ambulance_gps/
│   │   └── ambulance_gps.ino
│   └── traffic_signal/
│       └── traffic_signal.ino
├── Python_Backend/
│   ├── traffic_predictor.py
│   ├── ambulance_tracker.py
│   ├── firebase_config.py
│   └── requirements.txt
├── Web_Dashboard/
│   ├── index.html
│   ├── style.css
│   └── script.js   (ES module, Firebase v9)
├── Connections/
│   └── wiring_diagram.md
└── README.md
```

## ✅ What Works in This Demo
- **Ambulance ESP32** pushes GPS (or simulated) coordinates to `/ambulance/location`.
- **Python tracker** flags `ambulance/emergency_active=true` when within **500 m** of a signal.
- **Traffic Signal ESP32** reads emergency flag → forces **GREEN**.
- **Web Dashboard** shows status, lights, logs, and congestion bar.

---

## 🔧 Setup

### 1) Firebase (Realtime Database)
1. Create project (e.g., `life-lane-demo`).
2. Enable **Realtime Database**, set temporary rules for demo:
   ```json
   {
     "rules": { ".read": true, ".write": true }
   }
   ```
3. Get **database URL** and **RTDB secret** (for Arduino demo).
4. Create a **service account key** (JSON) for Python backend.

### 2) ESP32 Firmware
- Install Arduino IDE + ESP32 board package.
- Libraries: `FirebaseESP32`, `TinyGPS++`.
- Open:
  - `ESP32_Code/ambulance_gps/ambulance_gps.ino`
  - `ESP32_Code/traffic_signal/traffic_signal.ino`
- Replace WiFi + Firebase placeholders.
- **Option:** Use `simulateGPS()` indoors (commented in code).

### 3) Python Backend
```bash
cd Python_Backend
pip install -r requirements.txt
# Either export service account path:
export GOOGLE_APPLICATION_CREDENTIALS=/path/to/serviceAccount.json
# Or edit firebase_config.py placeholders
python ambulance_tracker.py
```
- Script watches `/ambulance/location` and toggles `emergency_active`.

### 4) Web Dashboard
- Open `Web_Dashboard/index.html` (served locally or file:// is fine).
- Edit `script.js` Firebase config placeholders.

---

## 🚀 Demo Flow
1. **Normal**: Traffic ESP32 cycles green → yellow → red.
2. **Ambulance Active**: GPS (or simulated) updates every ~2–3 s.
3. **Emergency**: Within 500 m of any signal → `emergency_active=true` → traffic turns **GREEN**.
4. **Resume**: Once clear → flag resets → normal cycle resumes.

---

## 🛠 Troubleshooting
- **ESP32 WiFi**: print IP, check SSID/password, 2.4 GHz only.
- **Firebase errors**: verify `FIREBASE_HOST` ends with `firebaseio.com`, RTDB secret correct.
- **GPS**: move outdoors; check satellites; or switch to `simulateGPS()`.
- **Dashboard** not updating: ensure `script.js` has valid Firebase config and your rules allow reads.

---

## 🔒 Security
This demo uses placeholders. For production:
- Use **Firebase-ESP-Client** with OAuth tokens (not raw RTDB secret).
- Lock database rules to **authenticated** users.
- Don’t commit service account keys; use env vars/secret managers.

---

## 📈 Next Steps
- Multiple ambulances (`/ambulances/{id}`) & per-signal routing.
- Map view (Google Maps) on the dashboard.
- Swap linear model with time-series (Prophet/LSTM) using real data.
- Edge failover if cloud disconnected.

---

**Made for fast, offline-ready demos.** Plug, simulate, and show the impact.
