# Smart Trash Can — Cleaning Gotham

An IoT-based smart waste management system . The system monitors trash bin states (Empty, Full, Tipped) in real time using Arduino sensors, transmits data over WiFi to a FastAPI backend, and exposes an API consumed by an AR smartphone application.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Project Structure](#project-structure)
- [Hardware Components](#hardware-components)
- [Wiring / Pin Map](#wiring--pin-map)
- [Software Components](#software-components)
  - [Arduino Firmware](#arduino-firmware)
  - [FastAPI Backend](#fastapi-backend)
  - [AR Application](#ar-application)
- [Data Flow](#data-flow)
- [API Reference](#api-reference)
- [Setup & Running](#setup--running)
- [Bin States](#bin-states)
- [Known Limitations](#known-limitations)

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────┐
│                        PHYSICAL LAYER                                │
│                                                                      │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────────────────┐   │
│  │  Ultrasonic │   │    Tilt     │   │   LDR Light  │  Water   │   │
│  │   Sensor    │   │   Switch    │   │    Sensor    │  Sensor  │   │
│  │ (HC-SR04)   │   │   (D12)     │   │    (A0)      │  (A1)    │   │
│  │  D10 / D11  │   └──────┬──────┘   └──────┬───────┴────┬─────┘   │
│  └──────┬───────┘          │                 │            │         │
│         └──────────────────┴─────────────────┴────────────┘         │
│                                    │                                 │
│                            ┌───────▼────────┐                       │
│                            │    ARDUINO     │                       │
│                            │  (Uno / Mega)  │                       │
│                            │                │                       │
│                            │  LCD  16x2 ◄───┤ D4-D9                │
│                            │  LED       ◄───┤ D13                  │
│                            │  Buzzer    ◄───┤ A2                   │
│                            └───────┬────────┘                       │
│                                    │ SoftwareSerial (D2/D3)         │
│                            ┌───────▼────────┐                       │
│                            │   ESP8266      │                       │
│                            │  WiFi Module   │                       │
│                            └───────┬────────┘                       │
└───────────────────────────────────┼──────────────────────────────── ┘
                                    │ HTTP GET over TCP/IP
                    ┌───────────────▼────────────────┐
                    │      FastAPI Backend            │
                    │      (Python / Uvicorn)         │
                    │      Port 8000                  │
                    │                                 │
                    │  GET /update  ← Arduino push    │
                    │  GET /latest  → AR App poll     │
                    │  GET /        → Health check    │
                    └───────────────┬────────────────┘
                                    │ JSON (HTTP)
                    ┌───────────────▼────────────────┐
                    │       AR Application            │
                    │   (Unity / Smartphone)          │
                    │                                 │
                    │  • 3D trash can overlays        │
                    │  • Real-life bin state in AR    │
                    │  • Garbage truck 3D model       │
                    │  • Animated truck route         │
                    └────────────────────────────────┘
```

---

## Project Structure

```
Smart-Trash-Can/
├── README.md                   # This file
├── AR/
│   └── main.py                 # FastAPI backend server
└── Ardino/
    └── sketch_mar29a.ino       # Arduino firmware (ESP8266 + sensors)
```

---

## Hardware Components

| Component | Part | Purpose |
|---|---|---|
| Microcontroller | Arduino Uno / Mega | Central logic and sensor reading |
| WiFi Module | ESP8266 | HTTP data transmission over WiFi |
| Distance Sensor | HC-SR04 (Ultrasonic) | Measures fill level (distance to trash) |
| Tilt Sensor | Tilt Switch | Detects if bin is tipped over |
| Light Sensor | LDR (Photoresistor) | Detects lid open / ambient light |
| Moisture Sensor | Water sensor module | Detects liquid in bin |
| Display | 16x2 LCD (LiquidCrystal) | Shows bin status and distance |
| Indicator | LED | Visual alert when bin is full |
| Indicator | Buzzer | Audio alert when bin is full |

---

## Wiring / Pin Map

```
Arduino Pin  │ Connected To
─────────────┼──────────────────────────────
D2 (RX)      │ ESP8266 TX
D3 (TX)      │ ESP8266 RX  (via voltage divider — 5V→3.3V)
D4            │ LCD RS
D5            │ LCD EN
D6            │ LCD D4
D7            │ LCD D5
D8            │ LCD D6
D9            │ LCD D7
D10           │ Ultrasonic TRIG
D11           │ Ultrasonic ECHO
D12           │ Tilt switch (INPUT_PULLUP)
D13           │ LED
A0            │ LDR light sensor
A1            │ Water sensor
A2            │ Buzzer
```

---

## Software Components

### Arduino Firmware

**File:** [Ardino/sketch_mar29a.ino](Ardino/sketch_mar29a.ino)

The firmware runs a continuous loop every 10 seconds:

1. Reads all sensors (distance, tilt, light, water).
2. Applies bin state logic (see [Bin States](#bin-states)).
3. Updates the LCD display and triggers LED / buzzer if needed.
4. Sends all sensor values to the FastAPI server via HTTP GET over TCP.

**WiFi communication** uses AT commands sent to the ESP8266 over `SoftwareSerial`. Each data push:
- Opens a TCP connection to the server.
- Sends an HTTP/1.1 GET request.
- Waits for the server response.
- Closes the connection.

**Configuration**:

```cpp
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASS = "YourPassword";
const char* SERVER_IP = "192.168.x.x";   // machine running FastAPI
const int   SERVER_PORT = 8000;
```

---

### FastAPI Backend

**File:** [AR/main.py](AR/main.py)

A lightweight Python API server with three endpoints.

**Run it:**

```bash
cd AR
pip install fastapi uvicorn
uvicorn main:app --host 0.0.0.0 --port 8000
```

The server stores the most recent sensor reading in memory. Every new push from the Arduino overwrites the previous value.

---

### AR Application

Built in Unity for a smartphone. It:

- Polls `GET /latest` on the FastAPI server to read current bin states.
- Renders 3D trash can models over a city map using image/object tracking.
- Reflects real-life bin state (Empty / Full / Tipped) visually in AR.
- Animates a 3D garbage truck along a pre-defined route when all bins are in a non-default state.

---

## Data Flow

```
[Arduino sensors]
      │  every 10 s
      ▼
Compute state (Full / Tipped / Empty)
      │
      ▼
HTTP GET /update?distance=X&tilt=Y&light=Z&water=W
      │
      ▼
[FastAPI] stores in latest_data{}
      │
      ▼
[AR App] polls GET /latest
      │
      ▼
Renders bin state + triggers garbage truck animation
```

---

## API Reference

Base URL: `http://<server-ip>:8000`

### `GET /`
Health check.

**Response:**
```json
{ "message": "Trash can server is running" }
```

---

### `GET /update`
Called by the Arduino to push sensor readings.

**Query Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `distance` | int | Distance in cm from sensor to trash surface |
| `tilt` | int | `1` = tipped, `0` = upright |
| `light` | int | Raw ADC value from LDR (0–1023) |
| `water` | int | Raw ADC value from water sensor (0–1023) |

**Example:**
```
GET /update?distance=5&tilt=0&light=512&water=0
```

**Response:**
```json
{
  "status": "ok",
  "received": {
    "distance": 5,
    "tilt": 0,
    "light": 512,
    "water": 0
  }
}
```

---

### `GET /latest`
Called by the AR app to retrieve the most recent sensor data.

**Response:**
```json
{
  "distance": 5,
  "tilt": 0,
  "light": 512,
  "water": 0
}
```

Returns `null` for each field until the first Arduino push is received.

---

## Setup & Running

### 1. FastAPI Server

```bash
# Install dependencies
pip install fastapi uvicorn

# Run the server (replace 0.0.0.0 to bind to all interfaces)
cd AR
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

Find your machine's local IP (`ip addr` / `ipconfig`) and set it as `SERVER_IP` in the Arduino sketch.

### 2. Arduino Firmware

1. Open `Ardino/sketch_mar29a.ino` in the Arduino IDE.
2. Update the WiFi credentials and server IP at lines 27–30.
3. Connect the hardware according to the [pin map](#wiring--pin-map) above.
4. Flash the sketch to the board.
5. Open the Serial Monitor at 9600 baud to watch connection logs.

### 3. AR Application

Build and deploy the Unity project to your smartphone. Ensure the phone and the FastAPI server are on the same network (or the server is publicly accessible).

---

## Bin States

The Arduino determines the bin state from sensor readings:

| State | Condition | Action |
|---|---|---|
| **Empty** (default) | `distance >= 7 cm` | LCD shows "BIN OK", LED and buzzer off |
| **Full** | `distance < 7 cm` | LCD shows "FULL! EMPTY IT", LED on, buzzer on |
| **Tipped** | Tilt switch held LOW for 3+ seconds | Clears the Full flag (`waitingForEmpty = false`) |

The project spec requires all three states to be tracked. When all bins are in a non-default state the AR app signals the garbage truck to move.

---

## Known Limitations

- **In-memory storage:** The FastAPI server loses all data on restart. No database is used.
- **Single bin firmware:** The project requirement is 3 physical trash cans, and we are indeed using 3 bins. However, the current firmware and API are written for a single bin , there is no `bin_id` or per bin tracking. All three bins currently share one set of sensor readings on the `/latest` endpoint, so the API cannot distinguish which bin is Full or Tipped.
- **No authentication:** The `/update` endpoint accepts data from any source without verification.
- **HTTP only:** The ESP8266 used here does not support HTTPS; all data is transmitted in plain text.
- **Hardcoded credentials:** WiFi SSID and password are embedded in the Arduino sketch.
