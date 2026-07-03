/*
  Night Patrolling Robot - Arduino Firmware
  ------------------------------------------
  Academic Project (Sept 2025)

  Functions handled by this Arduino:
    1. PIR motion sensor  -> detect suspicious movement
    2. HC-SR04 ultrasonic -> obstacle avoidance during patrol
    3. L298N motor driver -> autonomous navigation (forward/reverse/turn)
    4. SIM800L GSM module -> send SMS alert to security personnel on detection

  Night-vision video/camera streaming is handled separately by a
  Raspberry Pi (see /raspberry_pi/motion_camera_alert.py), which also
  talks to this Arduino over Serial to know when motion was detected.

  Wiring (adjust pins as per your actual build):
    PIR sensor OUT      -> D2
    HC-SR04 TRIG        -> D3
    HC-SR04 ECHO        -> D4
    Motor Driver IN1    -> D5
    Motor Driver IN2    -> D6
    Motor Driver IN3    -> D7
    Motor Driver IN4    -> D8
    Motor Driver ENA    -> D9  (PWM)
    Motor Driver ENB    -> D10 (PWM)
    SIM800L TX          -> D11 (Arduino RX, via SoftwareSerial)
    SIM800L RX          -> D12 (Arduino TX, via SoftwareSerial)
    Buzzer / Alert LED  -> D13
*/

#include <SoftwareSerial.h>

// ---------- Pin Definitions ----------
const int PIR_PIN        = 2;
const int TRIG_PIN       = 3;
const int ECHO_PIN       = 4;

const int IN1 = 5, IN2 = 6, IN3 = 7, IN4 = 8;
const int ENA = 9, ENB = 10;

const int GSM_RX_PIN = 11;   // Arduino RX <- SIM800L TX
const int GSM_TX_PIN = 12;   // Arduino TX -> SIM800L RX
const int ALERT_PIN  = 13;

// ---------- Config ----------
const int OBSTACLE_DISTANCE_CM = 20;     // stop/turn threshold
const int MOTOR_SPEED           = 180;   // 0-255 PWM
const unsigned long SMS_COOLDOWN_MS = 60000UL; // avoid spamming SMS (1 min)
const char* ALERT_PHONE_NUMBER = "+91XXXXXXXXXX"; // <-- set guard's number

SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);

unsigned long lastSmsTime = 0;

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);      // Debug + link to Raspberry Pi
  gsmSerial.begin(9600);   // SIM800L

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ALERT_PIN, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  stopMotors();
  initGSM();

  Serial.println("Night Patrol Robot: system ready.");
}

// ---------- Main Loop ----------
void loop() {
  long distanceCm = readUltrasonicDistance();

  if (distanceCm > 0 && distanceCm < OBSTACLE_DISTANCE_CM) {
    avoidObstacle();
  } else {
    moveForward();
  }

  if (digitalRead(PIR_PIN) == HIGH) {
    handleMotionDetected();
  }

  delay(100);
}

// ---------- Obstacle Avoidance ----------
long readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return -1; // no echo
  long distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void avoidObstacle() {
  stopMotors();
  delay(150);
  moveBackward();
  delay(300);
  stopMotors();
  delay(100);
  turnRight();
  delay(400);
  stopMotors();
}

// ---------- Motion Detection + Alerting ----------
void handleMotionDetected() {
  digitalWrite(ALERT_PIN, HIGH);
  Serial.println("MOTION_DETECTED"); // Raspberry Pi listens for this to log/snapshot

  unsigned long now = millis();
  if (now - lastSmsTime > SMS_COOLDOWN_MS) {
    sendSMS("ALERT: Unauthorized movement detected by Night Patrol Robot!");
    lastSmsTime = now;
  }
  delay(500);
  digitalWrite(ALERT_PIN, LOW);
}

// ---------- GSM (SIM800L) ----------
void initGSM() {
  delay(2000);
  gsmSerial.println("AT");        delay(200);
  gsmSerial.println("AT+CMGF=1"); delay(200); // SMS text mode
}

void sendSMS(const char* message) {
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(ALERT_PHONE_NUMBER);
  gsmSerial.println("\"");
  delay(300);
  gsmSerial.println(message);
  delay(300);
  gsmSerial.write(26); // Ctrl+Z to send
  delay(2000);
  Serial.println("SMS alert sent.");
}

// ---------- Motor Control ----------
void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
