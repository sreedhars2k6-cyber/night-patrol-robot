"""
Night Patrolling Robot - Raspberry Pi Vision & Telemetry Module
-----------------------------------------------------------------
Academic Project (Sept 2025)

Responsibilities:
  1. Capture live video from a Pi/USB camera (IR-capable module for
     night vision in low-light conditions).
  2. Run frame-differencing based motion detection as a software-side
     confirmation/companion to the Arduino's PIR sensor.
  3. Stream/transmit live telemetry (snapshots + status) to a simple
     local server / cloud endpoint so security personnel can monitor
     activity in real time.
  4. Listen on Serial to the Arduino for "MOTION_DETECTED" events and
     save timestamped snapshots as evidence.

Run:
    pip install -r requirements.txt
    python3 motion_camera_alert.py

Hardware:
    Raspberry Pi 3/4 + Pi NoIR Camera (or USB IR camera)
    Connected to Arduino via USB Serial (e.g. /dev/ttyUSB0)
"""

import cv2
import serial
import time
import os
import requests
from datetime import datetime

# ---------- Configuration ----------
SERIAL_PORT = "/dev/ttyUSB0"      # change to match Arduino port (e.g. COM3 on Windows)
BAUD_RATE = 9600
SNAPSHOT_DIR = "snapshots"
TELEMETRY_ENDPOINT = "http://localhost:5000/telemetry"  # replace with your server / dashboard URL
MOTION_AREA_THRESHOLD = 900       # min contour area to count as motion

os.makedirs(SNAPSHOT_DIR, exist_ok=True)


def connect_arduino():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"[INFO] Connected to Arduino on {SERIAL_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"[WARN] Could not open serial port: {e}")
        return None


def send_telemetry(event_type, frame_path=None):
    """Transmit a live telemetry alert to the monitoring endpoint."""
    payload = {
        "event": event_type,
        "timestamp": datetime.now().isoformat(),
        "snapshot": frame_path,
    }
    try:
        requests.post(TELEMETRY_ENDPOINT, json=payload, timeout=2)
        print(f"[TELEMETRY] Sent: {payload}")
    except requests.RequestException as e:
        print(f"[WARN] Telemetry send failed (endpoint offline?): {e}")


def save_snapshot(frame):
    filename = datetime.now().strftime("%Y%m%d_%H%M%S") + ".jpg"
    path = os.path.join(SNAPSHOT_DIR, filename)
    cv2.imwrite(path, frame)
    print(f"[INFO] Snapshot saved: {path}")
    return path


def detect_motion(prev_gray, gray):
    """Simple frame-differencing motion detector (works with IR night frames too)."""
    delta = cv2.absdiff(prev_gray, gray)
    thresh = cv2.threshold(delta, 25, 255, cv2.THRESH_BINARY)[1]
    thresh = cv2.dilate(thresh, None, iterations=2)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    for c in contours:
        if cv2.contourArea(c) >= MOTION_AREA_THRESHOLD:
            return True
    return False


def main():
    arduino = connect_arduino()
    cap = cv2.VideoCapture(0)  # 0 = default camera; use IR/NoIR camera index if different

    if not cap.isOpened():
        print("[ERROR] Could not open camera.")
        return

    ret, prev_frame = cap.read()
    if not ret:
        print("[ERROR] Could not read initial frame.")
        return
    prev_gray = cv2.cvtColor(prev_frame, cv2.COLOR_BGR2GRAY)
    prev_gray = cv2.GaussianBlur(prev_gray, (21, 21), 0)

    print("[INFO] Night patrol monitoring started. Press 'q' to quit.")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        gray = cv2.GaussianBlur(gray, (21, 21), 0)

        software_motion = detect_motion(prev_gray, gray)
        prev_gray = gray

        # Check Arduino PIR-based event over serial
        hardware_motion = False
        if arduino and arduino.in_waiting:
            line = arduino.readline().decode(errors="ignore").strip()
            if line == "MOTION_DETECTED":
                hardware_motion = True
                print("[ALERT] PIR motion event received from Arduino.")

        if software_motion or hardware_motion:
            snapshot_path = save_snapshot(frame)
            send_telemetry("motion_detected", snapshot_path)
            cv2.putText(frame, "MOTION DETECTED", (20, 40),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

        cv2.imshow("Night Patrol Robot - Live Feed", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    if arduino:
        arduino.close()


if __name__ == "__main__":
    main()
