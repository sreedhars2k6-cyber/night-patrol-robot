# Night Patrolling Robot 🤖🌙

**Academic Project — September 2025**

An autonomous mobile robotic vehicle for night-time surveillance and security patrolling within a designated area. The robot combines real-time motion detection, IR night vision, autonomous obstacle-avoidance navigation, and GSM-based SMS alerts to support security personnel and law enforcement.

## Features

- **Autonomous navigation** with ultrasonic-sensor-based obstacle avoidance
- **Motion detection** via PIR sensor (hardware) + camera frame-differencing (software), for redundancy
- **IR night vision** camera for reliable operation in low-visibility conditions
- **Live telemetry**: snapshots and motion events sent to a monitoring endpoint in real time
- **GSM SMS alerts** (SIM800L) automatically notify security personnel when unauthorized movement is detected

## System Architecture

```
 ┌───────────────────┐        Serial (USB)       ┌──────────────────────────┐
 │     Arduino        │ <-----------------------> │      Raspberry Pi         │
 │  - PIR sensor       │   MOTION_DETECTED events  │  - IR/NoIR camera          │
 │  - Ultrasonic sensor│                            │  - Motion detection (CV)  │
 │  - Motor driver      │                            │  - Live telemetry sender  │
 │  - SIM800L GSM module│                            │                            │
 └───────────────────┘                            └──────────────────────────┘
```

The Arduino handles real-time, low-level tasks (sensors, motors, SMS alerts), while the Raspberry Pi handles vision processing and telemetry — a common, robust split for robotics projects.

## Repository Structure

```
night-patrol-robot/
├── arduino/
│   └── night_patrol_robot.ino   # Motor control, PIR, ultrasonic, GSM SMS
├── raspberry_pi/
│   ├── motion_camera_alert.py   # Camera, night-vision motion detection, telemetry
│   └── requirements.txt
└── README.md
```

## Hardware Used

| Component | Purpose |
|---|---|
| Arduino Uno/Mega | Main motor & sensor controller |
| Raspberry Pi | Camera processing & telemetry |
| PIR Motion Sensor | Detect movement |
| HC-SR04 Ultrasonic Sensor | Obstacle avoidance |
| IR/NoIR Camera Module | Night vision live feed |
| SIM800L GSM Module | SMS alerts to security personnel |
| L298N Motor Driver + DC Motors | Robot locomotion |

## Setup

### Arduino
1. Open `arduino/night_patrol_robot.ino` in the Arduino IDE.
2. Update `ALERT_PHONE_NUMBER` with the guard's number.
3. Wire components per the pin map in the code comments.
4. Upload to your board.

### Raspberry Pi
```bash
cd raspberry_pi
pip install -r requirements.txt
python3 motion_camera_alert.py
```
Update `SERIAL_PORT` and `TELEMETRY_ENDPOINT` in `motion_camera_alert.py` to match your setup.

## Notes

- Pin assignments, sensor models, and the GSM module in this code are placeholders based on common builds — adjust them to match your exact hardware/wiring.
- Replace the `TELEMETRY_ENDPOINT` with your own dashboard/server, or remove the telemetry call if not needed.

## License

MIT — feel free to use/adapt for academic purposes.
