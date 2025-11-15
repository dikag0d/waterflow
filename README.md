# ESP32 Waterflow Monitor – Sistem Pendeteksi Kebocoran Air
Menggunakan Mesh Topology (painlessMesh) + MQTT (HiveMQ) + Dashboard Web

## 📌 Deskripsi Proyek
Proyek ini adalah sistem monitoring aliran air berbasis **ESP32** yang mampu mendeteksi indikasi **kebocoran pipa** menggunakan sensor water flow. Data dikirim melalui **mesh network**, diteruskan oleh gateway ke **MQTT HiveMQ**, dan ditampilkan pada **dashboard web** secara realtime.

Sistem terdiri dari 2 kode utama:
- **meshsensor.ino** → Node sensor pengukur aliran air.
- **meshgateway.ino** → Gateway penerima data mesh dan publisher MQTT.

---

## 📡 Arsitektur Sistem
[ESP32 Node Sensor] -- Mesh --> [ESP32 Gateway] -- MQTT --> [HiveMQ Broker] --> [Dashboard Web]

### Node Sensor (`meshsensor.ino`)
- Membaca water flow sensor (pulse).
- Menghitung flow rate & total liter.
- Mengirim JSON via mesh ke gateway.
- Mengirim status "kebocoran" jika selisih volume melebihi ambang.

### Gateway (`meshgateway.ino`)
- Menerima semua pesan dari mesh network.
- Parsing data JSON dari node sensor.
- Publish data ke MQTT HiveMQ.
- Meneruskan status kebocoran ke dashboard.

---

## ✨ Fitur Utama
- Komunikasi **mesh topology** antar ESP32.
- Integrasi **MQTT (HiveMQ)**.
- Deteksi kebocoran otomatis berdasarkan selisih volume.
- Dashboard web realtime berbasis MQTT WebSocket.
- Format data JSON rapi dan ringan.

---

## 🧩 Struktur Folder

/project
│
├── meshgateway.ino
├── meshsensor.ino
└── README.md

yaml
Copy code

---

## 📁 Format Data JSON

### Data normal:
```json
{
  "node": "sensor01",
  "flow": 2.5,
  "liter": 15.2,
  "leak": false
}

2 liter / menit
Ambang batas bisa kamu setting di meshsensor.ino
