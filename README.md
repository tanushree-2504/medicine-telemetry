# medicine-telemetry
# Embedded Medical Device Validation and Telemetry Platform  ## Project Overview A dual-microcontroller biomedical telemetry system that simulates, transmits,  processes, and visualizes real-time physiological data.  ## System Architecture
## Hardware Used
- Arduino UNO
- ESP32 DevKit V1
- 2 Jumper Wires

## Wiring
| Arduino UNO | ESP32 DevKit V1 |
|-------------|-----------------|
| TX (Pin 1)  | GPIO16 (RX2)    |
| GND         | GND             |

## Parameters Monitored
| Parameter         | Normal Range     | Alert Condition      |
|-------------------|------------------|----------------------|
| Heart Rate        | 60–100 BPM       | >100 Tachycardia     |
|                   |                  | <60 Bradycardia      |
| SpO2              | 95–100%          | <92% Hypoxia         |
| Temperature       | 36.5–37.5°C      | >38°C Fever          |
| Blood Pressure    | 110–130/70–90    | >140 Hypertension    |
| Respiratory Rate  | 12–20 br/min     | —                    |

## Features
- UART packet transmission with packet numbering
- Packet validation and corruption detection
- Packet loss detection
- Circular buffer storing last 100 readings
- Real-time anomaly detection (Tachycardia, Bradycardia, Fever, Hypoxia)
- Statistical analysis (Average, Max, Min HR, SpO2, Temperature)
- Live web dashboard with 5 tabs
- Historical graphs with threshold lines
- Medical alert log
- CSV data export

## Communication Protocol
Packet format:
