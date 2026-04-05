# ESP_NOW Gateway for MQTT + Home Assistant + Telegram

ESP-NOW gateway library for ESP8266  that collects data from battery-powered sensors and forwards it to MQTT, Home Assistant, and Telegram.

It supports automatic device discovery, sensor registration, and multiple sensor types.

---

# 🚀 Features

- 📡 ESP-NOW communication with battery sensors
- 🌐 MQTT bridge to Home Assistant
- 🏠 Automatic Home Assistant discovery (MQTT Discovery)
- 📲 Telegram notifications
- 🔔 Door open events with instant alerts
- 📅 Daily first-door-open prediction message (via Telegram)
- 💻 Wake-on-LAN (WOL) via Telegram command
- 🧠 Automatic sensor pairing (first handshake)
- 🗂 Persistent device registry in gateway memory
- ⚡ Works with mixed sensor types

---

# 📦 Supported Sensors

- Door_Open_Sensor  
- Door_Ring  
- Mail  
- Temperature_Sensor  
- Humidity_Sensor  
- Voltmetr  
- SoilMoistureSensor  
- Water_pump  
- Atmospheric_pressure  
- Illuminance  
- Meteostation  
- ESP_NOW_Gateway  
- Counter_Gas  

---

# 🧠 How it works

## 1. First connection (pairing)
When a new sensor powers on:

- It performs a **handshake with gateway**
- Gateway registers device MAC + type
- Gateway stores device in memory
- Gateway sends **MQTT Discovery message** to Home Assistant

➡️ Sensor automatically appears in Home Assistant

---

## 2. Normal operation

After pairing:

- Sensor sends ESP-NOW packets
- Gateway receives data
- Data is:
  - forwarded to MQTT
  - processed for events
  - optionally sent to Telegram

---

## 📡 MQTT Integration

Each sensor publishes data to MQTT topics:


## Data / Web Interface Upload
Tools → ESP8266/ESP32 Sketch Data Upload
Upload folder:
examples/SLAVE2MQTT/data

## please install libraries
PubSubClient
ArduinoJson
FastBot2
WakeOnLan
