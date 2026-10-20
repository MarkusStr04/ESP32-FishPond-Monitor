
# 🐟 Fish Pond Aerator GSM Monitoring System

An automated, fault-tolerant monitoring system built with **ESP32** and **SIM800L** to track the operational status of a aerator motor for my father's fish pond. 

If the aerator stops running for **longer than 35 minutes**, the system triggers an **emergency phone call** to alert the owner before oxygen depletion harms the hundreds of fish.

## 🌟 Key Features

* **AC Current Sensing:** Tracks aerator activity non-invasively using an **SCT-013-030** current transformer.
* **Emergency Voice Call Alert:** Places a direct phone call (30s duration) if the motor remains stopped for 35 minutes.
* **False-Alarm Prevention:** Short power interruptions or transient motor stops (< 35 min) automatically reset the internal timer without disturbing you.
* **Daily Heartbeat SMS:** Sends a daily confirmation SMS every 24 hours to verify that the system is powered and online.
* **Zero-Spam Mechanism:** Ensures only one call is placed per failure event. Normal operation resets the alarm state automatically.
* **Self-Healing GSM Stack:** Periodically checks GSM network registration every 30 minutes and triggers a hardware pin reset on the SIM800L if connection is lost.
* **ADC Protection:** Made a voltage divider circuit to shift the AC signal of the sensor into the 0–3.3V range of the ESP32 ADC.

---

## 🛠️ Hardware Requirements

| Component | Model / Specifications | Purpose |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 WROOM-32 | Core logic & signal processing |
| **Current Sensor** | **SCT-013-030** (30A / 1V output) | Non-invasive AC current transformer |
| **GSM Module** | **SIM800L V2** (Quad-band) | SMS notifications & Voice calls |
| **DC Bias Circuit** | 2x 10kΩ Resistors + 10µF Capacitor | Shifts AC signal(~1.65V DC offset) |
| **Power Supply** | External 5.0V / 2A | Current supply for SIM800L and ESP32 in case of blackout |
| **Filtering Cap** | 1000µF Electrolitic Capacitor | Prevents SIM800L brownout resets during Calls |

---

## 🔌 Circuit Wiring & Pinout

### 1. SCT-013-030 Sensor & DC Bias Interface
Because the SCT-013-030 outputs an AC voltage wave (-1V to +1V peak around 0V) and the ESP32 ADC cannot handle negative voltages, a simple voltage divider is used to center the signal at **1.65V DC**

### 2. Pin Mapping Table

| ESP32 Pin | Connected Component | Description |
| :--- | :--- | :--- |
| **GPIO 34** | SCT-013-030 Signal Pin | Analog Input (ADC1) |
| **3.3V** | Voltage Divider R1 | 3.3V Supply Rail for DC Offset |
| **GPIO 16** | SIM800L **TX** | Hardware Serial2 RX |
| **GPIO 17** | SIM800L **RX** | Hardware Serial2 TX |
| **GPIO 18** | SIM800L **RST** | Hardware Reset Control Pin |
| **GND** | System Common Ground | **Must bridge ESP32 GND & SIM800L Power GND** |

> ⚠️ **Important Power Note:** Never power the SIM800L directly from the ESP32 3.3V or 5V pins. The module demands up to 2A power bursts during call initiation. Use an external 5.0V supply and connect all Ground (GND) lines together.

---
