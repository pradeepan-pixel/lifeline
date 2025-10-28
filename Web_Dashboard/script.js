// File: Web_Dashboard/script.js
// Firebase v9 modular with ES modules via CDN imports

import { initializeApp } from 'https://www.gstatic.com/firebasejs/9.23.0/firebase-app.js';
import { getDatabase, ref, onValue } from 'https://www.gstatic.com/firebasejs/9.23.0/firebase-database.js';

// IMPORTANT: Replace these with your Firebase Web App config (not Admin SDK)
// Use the same RTDB URL as in the .ino files
// Import the functions you need from the SDKs you need
// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyCCYaEui8ptVXwilygEbmJIqM2T4xUa3ZY",
  authDomain: "life-lane-traffic.firebaseapp.com",
  databaseURL: "https://life-lane-traffic-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "life-lane-traffic",
  storageBucket: "life-lane-traffic.appspot.com",
  messagingSenderId: "389449007392",
  appId: "1:389449007392:web:b71aa2b49e6279e54e1745"
};



// Data layout used by ESP32 code
// Ambulance node: /ambulances/AMB001/{latitude, longitude, speed, altitude, satellites, gpsValid, status, driverName, timestamp}
// Intersections: /intersections/<intersection_id>/emergency_active

const AMB_ID = 'AMB001';

const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

class LifeLaneDashboard {
  constructor() {
    // UI elements
    this.el = {
      ambId: document.getElementById('amb-id'),
      ambDriver: document.getElementById('amb-driver'),
      ambMode: document.getElementById('amb-mode'),
      ambGPS: document.getElementById('amb-gps'),
      ambSats: document.getElementById('amb-sats'),
      ambSpeed: document.getElementById('amb-speed'),
      ambAlt: document.getElementById('amb-alt'),
      ambLoc: document.getElementById('amb-location'),
      ambTime: document.getElementById('amb-time'),
      ambFB: document.getElementById('amb-fb'),
      ambWiFi: document.getElementById('amb-wifi'),
      intersectionsBody: document.getElementById('intersections-body'),
      logs: document.getElementById('logs')
    };

    // Map
    this.map = L.map('map').setView([12.9716, 77.5946], 12);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 19,
      attribution: '&copy; OpenStreetMap contributors'
    }).addTo(this.map);

    this.ambMarker = L.marker([12.9716,77.5946], { title: 'Ambulance' }).addTo(this.map);

    // Intersections config copied from .ino (update here too if changed there)
    this.intersections = [
      { id: 'intersection_001', name: 'Silk Board Junction', lat: 12.9700, lng: 77.5900 },
      { id: 'intersection_002', name: 'BTM Layout Signal', lat: 12.9750, lng: 77.5950 },
      { id: 'intersection_003', name: 'Jayadeva Hospital Junction', lat: 12.9800, lng: 77.6000 },
      { id: 'intersection_004', name: 'HSR Layout Signal', lat: 12.9650, lng: 77.5850 },
      { id: 'intersection_005', name: 'Forum Mall Junction', lat: 12.9850, lng: 77.6050 }
    ];

    this.intersectionMarkers = {};
    this.intersections.forEach(intx => {
      const marker = L.circleMarker([intx.lat, intx.lng], {
        radius: 8,
        color: '#2c3e50',
        weight: 2,
        fillColor: '#2ecc71',
        fillOpacity: 0.8
      }).addTo(this.map).bindPopup(`${intx.name}`);
      this.intersectionMarkers[intx.id] = marker;
    });

    // Speed chart
    this.speedData = [];
    const ctx = document.getElementById('speedChart').getContext('2d');
    this.speedChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'Speed',
          data: [],
          borderColor: '#27ae60',
          backgroundColor: 'rgba(39, 174, 96, 0.2)',
          tension: 0.25,
          pointRadius: 0
        }]
      },
      options: {
        responsive: true,
        animation: false,
        plugins: { legend: { display: false } },
        scales: { y: { beginAtZero: true, suggestedMax: 100 } }
      }
    });

    this.lastLat = 12.9716;
    this.lastLng = 77.5946;

    this.init();
  }

  init() {
    this.wireFirebase();
    this.addLog('Dashboard ready');
  }

  wireFirebase() {
    // Ambulance listener
    const ambRef = ref(db, `/ambulances/${AMB_ID}`);
    onValue(ambRef, (snap) => {
      const v = snap.val();
      if (!v) return;
      this.updateAmbulance(v);
    }, (err) => this.addLog(`Ambulance read error: ${err?.message||err}`));

    // Intersections branch listener (single subscription)
    const intxRef = ref(db, '/intersections');
    onValue(intxRef, (snap) => {
      const data = snap.val() || {};
      Object.keys(data).forEach(id => {
        const isEmergency = !!data[id]?.emergency_active;
        this.updateIntersection(id, isEmergency);
      });
      // Refresh table using last known ambulance position
      this.refreshIntersectionTable(this.lastLat, this.lastLng);
    }, (err) => this.addLog(`Intersections read error: ${err?.message||err}`));

    // Firebase status indicator
    this.el.ambFB.textContent = 'Connected';
    this.el.ambFB.classList.add('active');
  }

  updateAmbulance(v) {
    const lat = Number(v.latitude||0);
    const lng = Number(v.longitude||0);
    const speed = Number(v.speed||0);
    const sats = Number(v.satellites||0);
    const alt = Number(v.altitude||0);
    const gpsValid = !!v.gpsValid;
    const status = String(v.status||'').toUpperCase();
    const driver = String(v.driverName||'--');

    this.lastLat = lat;
    this.lastLng = lng;

    // Update UI
    this.el.ambId.textContent = AMB_ID;
    this.el.ambDriver.textContent = driver;
    this.el.ambMode.textContent = status;
    this.el.ambGPS.textContent = gpsValid ? 'Valid Fix' : 'Searching';
    this.el.ambSats.textContent = sats;
    this.el.ambSpeed.textContent = `${speed.toFixed(1)} km/h`;
    this.el.ambAlt.textContent = `${alt.toFixed(0)} m`;
    this.el.ambLoc.textContent = `${lat.toFixed(6)}, ${lng.toFixed(6)}`;
    this.el.ambTime.textContent = new Date().toLocaleTimeString();

    // Map
    const latLng = [lat, lng];
    this.ambMarker.setLatLng(latLng).bindPopup(`Ambulance ${AMB_ID}<br/>${lat.toFixed(6)}, ${lng.toFixed(6)}<br/>${speed.toFixed(1)} km/h`).openPopup();
    this.map.setView(latLng, this.map.getZoom(), { animate: true });

    // Chart
    const ts = new Date();
    this.speedData.push({ t: ts, v: speed });
    if (this.speedData.length > 60) this.speedData.shift();
    this.speedChart.data.labels = this.speedData.map(x => x.t.toLocaleTimeString());
    this.speedChart.data.datasets[0].data = this.speedData.map(x => x.v);
    this.speedChart.update('none');

    this.addLog(`Ambulance update: ${lat.toFixed(5)}, ${lng.toFixed(5)} | ${speed.toFixed(1)} km/h`);

    // Update intersections table distances
    this.refreshIntersectionTable(lat, lng);
  }

  updateIntersection(id, isEmergency) {
    const marker = this.intersectionMarkers[id];
    if (!marker) return;
    marker.setStyle({ fillColor: isEmergency ? '#e74c3c' : '#2ecc71' });

    // Update table row if exists
    const row = document.querySelector(`tr[data-id="${id}"]`);
    if (row) {
      const statusCell = row.querySelector('.status');
      statusCell.textContent = isEmergency ? 'EMERGENCY' : 'Normal';
      statusCell.className = `status ${isEmergency ? 'danger' : 'ok'}`;
    }
  }

  refreshIntersectionTable(lat, lng) {
    this.el.intersectionsBody.innerHTML = '';
    const rows = this.intersections.map(intx => {
      const marker = this.intersectionMarkers[intx.id];
      const isEmergency = marker && marker.options.fillColor === '#e74c3c';
      const dist = this.haversine(lat, lng, intx.lat, intx.lng);
      return {
        id: intx.id,
        name: intx.name,
        status: isEmergency ? 'EMERGENCY' : 'Normal',
        cls: isEmergency ? 'danger' : 'ok',
        distance: `${Math.round(dist)} m`
      };
    }).sort((a,b) => parseInt(a.distance) - parseInt(b.distance));

    rows.forEach(r => {
      const tr = document.createElement('tr');
      tr.dataset.id = r.id;
      tr.innerHTML = `
        <td>${r.id}</td>
        <td>${r.name}</td>
        <td class="status ${r.cls}">${r.status}</td>
        <td>${r.distance}</td>
      `;
      this.el.intersectionsBody.appendChild(tr);
    });
  }

  haversine(lat1, lon1, lat2, lon2) {
    const R = 6371000; // m
    const toRad = d => d * Math.PI / 180;
    const dLat = toRad(lat2 - lat1);
    const dLon = toRad(lon2 - lon1);
    const a = Math.sin(dLat/2)**2 + Math.cos(toRad(lat1))*Math.cos(toRad(lat2))*Math.sin(dLon/2)**2;
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    return R * c;
  }

  addLog(msg) {
    const p = document.createElement('p');
    p.textContent = `${new Date().toLocaleTimeString()} - ${msg}`;
    this.el.logs.appendChild(p);
    this.el.logs.scrollTop = this.el.logs.scrollHeight;
    while (this.el.logs.children.length > 200) this.el.logs.removeChild(this.el.logs.firstChild);
  }
}

window.addEventListener('DOMContentLoaded', () => new LifeLaneDashboard());
