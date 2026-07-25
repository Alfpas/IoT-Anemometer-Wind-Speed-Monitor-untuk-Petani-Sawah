# 🌬️ IoT Anemometer — Wind Speed Monitor untuk Petani Sawah

Sistem monitoring kecepatan angin berbasis IoT yang dirancang untuk membantu petani sawah memantau kondisi angin di lahan mereka secara real-time. Data dikirim dari sensor di lapangan ke dashboard web melalui MQTT.

![Deployment di Sawah](render2.png)

## 📋 Fitur

- Pengukuran kecepatan angin real-time menggunakan sensor cup-type anemometer (HC-020K)
- Pengiriman data via MQTT (TLS/secure) ke EMQX Cloud
- Dashboard web real-time menggunakan Socket.io
- Indikator status LED pada perangkat (connecting, standby, sending, error)
- Desain enclosure tahan lapangan dengan panel surya

## 🏗️ Arsitektur Sistem

```
[ Anemometer + ESP32 ]
         |
         | MQTT (TLS, port 8883)
         v
[ EMQX Cloud Broker ]
         |
         | Subscribe topic
         v
[ Backend Node.js (Express + Socket.io) ]
         |
         | WebSocket
         v
[ Dashboard Web (Browser) ]
```

## 🔧 Hardware

| Komponen | Keterangan |
|---|---|
| Mikrokontroler | ESP32 |
| Sensor kecepatan | HC-020K (speed sensor / slotted disc) |
| Konektivitas | WiFi |
| Catu daya | Panel surya + baterai |
| Enclosure | Tiang kayu + housing elektronik |

![CAD Render Anemometer](render.png)

## 📂 Struktur Folder

```
anemometer-iot/
├── app/          # Backend web (Node.js + Express + Socket.io)
│   ├── server.js
│   └── public/
├── stl/          # File 3D print (STL) untuk enclosure & cup anemometer
├── firmware/      # Kode ESP32 (.ino)
└── img/          # Dokumentasi visual (render CAD, foto deployment)
```

## ⚙️ Konfigurasi

### 1. Firmware (ESP32)

Edit bagian berikut di kode `.ino` sebelum upload:

```cpp
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";

const char* mqtt_server = "xxxxxxx.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883;

const char* mqtt_user = "anemometer_esp1";
const char* mqtt_pass = "your_password";

const char* topic = "weather/anemometer";
```

Pastikan root CA certificate dari EMQX Cloud sudah dimasukkan ke variabel `root_ca` di kode.

**Library yang dibutuhkan (Arduino IDE):**
- `WiFi.h`
- `WiFiClientSecure.h`
- `PubSubClient`
- `ArduinoJson`

### 2. Backend Web

```bash
cd app
npm install express socket.io mqtt
node server.js
```

Buka `http://localhost:3000` di browser untuk melihat dashboard real-time.

## 📡 Format Data (MQTT Payload)

Data dikirim dalam format JSON tiap 1 detik ke topic `weather/anemometer`:

```json
{
  "pulse": 24,
  "rpm": 144.0,
  "speed_ms": 3.98,
  "speed_kmh": 14.33
}
```

## 💡 Indikator LED Status

| Pola LED | Arti |
|---|---|
| Kedip cepat | Sedang menghubungkan (WiFi/MQTT) |
| Kedip pelan | WiFi tersambung, MQTT belum |
| Mati | Sistem normal, standby |
| Kedip singkat sesekali | Data berhasil dikirim |
| Tiga kedip cepat | Pengiriman data gagal |
| Menyala terus | Gangguan koneksi (gagal berkali-kali) |

## 🚀 Status Project

✅ Firmware ESP32 berjalan dan berhasil mengirim data ke EMQX Cloud
✅ Backend web menerima data real-time via MQTT
🔲 Penambahan sensor tambahan (suhu, kelembapan, kelembapan tanah) — direncanakan
🔲 Deployment jangka panjang di lahan sawah

## 📄 Lisensi

MIT License
