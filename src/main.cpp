#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ================= 1. CẤU HÌNH CHÂN (PICO W) =================
// Cảm biến Laser I2C
#define SDA_PIN 0
#define SCL_PIN 1
#define XSHUT_F1 2
#define XSHUT_F2 3
#define XSHUT_SL 4
#define XSHUT_SR 5

// Encoder Động cơ
#define ENC_L_A 6
#define ENC_L_B 7
#define ENC_R_A 8
#define ENC_R_B 9

// Động cơ (TB6612FNG)
#define MOT_L_IN1 10
#define MOT_L_IN2 11
#define MOT_L_PWM 12
#define MOT_R_IN1 13
#define MOT_R_IN2 14
#define MOT_R_PWM 15

// Cảm biến dò Line (Digital) - Dành cho cấu hình Sumo
#define LINE_FL 16 // Trước Trái
#define LINE_FR 17 // Trước Phải
#define LINE_BL 18 // Sau Trái
#define LINE_BR 19 // Sau Phải

// Nút nhấn chuyển trạng thái
#define BUTTON_PIN 20 

// ================= 2. THÔNG SỐ CẦN TINH CHỈNH =================
#define TICKS_PER_90_DEG 215    // Số xung để xe xoay 90 độ
#define TICKS_ALIGN_CENTER 150  // Số xung tiến vào giữa ngã 3
#define TICKS_PER_CELL 1103      // Số xung đi thẳng 1 ô mê cung

// NGƯỠNG NHẬN DIỆN THOÁT MÊ CUNG (mm)
// Nếu khoảng cách lớn hơn mức này -> Không có tường
#define OUT_OF_MAZE 300 

float Kp = 1.5;
float Kd = 0.5;
float last_error = 0;
int base_speed = 150;

// ================= 3. BIẾN TOÀN CỤC & TRẠNG THÁI =================
Adafruit_VL53L0X lox_F1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox_F2 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox_SL = Adafruit_VL53L0X();
Adafruit_VL53L0X lox_SR = Adafruit_VL53L0X();

volatile long encoderLeftCount = 0;
volatile long encoderRightCount = 0;

enum RobotState {
  STATE_WAITING = 0,
  STATE_EXPLORING = 1,
  STATE_FINISHED = 2,
  STATE_SPEEDRUN = 3
};

RobotState currentState = STATE_WAITING;

char path[200]; 
int pathLength = 0;

// ================= 4. HÀM NGẮT ENCODER =================
void countLeftEncoder() {
  if (digitalRead(ENC_L_B) == HIGH) encoderLeftCount++;
  else encoderLeftCount--;
}

void countRightEncoder() {
  if (digitalRead(ENC_R_B) == HIGH) encoderRightCount++;
  else encoderRightCount--;
}

// ================= 5. HÀM ĐIỀU KHIỂN ĐỘNG CƠ =================
void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed > 0) {
    digitalWrite(MOT_L_IN1, HIGH); digitalWrite(MOT_L_IN2, LOW); analogWrite(MOT_L_PWM, leftSpeed);
  } else if (leftSpeed < 0) {
    digitalWrite(MOT_L_IN1, LOW); digitalWrite(MOT_L_IN2, HIGH); analogWrite(MOT_L_PWM, -leftSpeed);
  } else {
    digitalWrite(MOT_L_IN1, LOW); digitalWrite(MOT_L_IN2, LOW); analogWrite(MOT_L_PWM, 0);
  }

  if (rightSpeed > 0) {
    digitalWrite(MOT_R_IN1, HIGH); digitalWrite(MOT_R_IN2, LOW); analogWrite(MOT_R_PWM, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(MOT_R_IN1, LOW); digitalWrite(MOT_R_IN2, HIGH); analogWrite(MOT_R_PWM, -rightSpeed);
  } else {
    digitalWrite(MOT_R_IN1, LOW); digitalWrite(MOT_R_IN2, LOW); analogWrite(MOT_R_PWM, 0);
  }
}

// ================= 6. HÀM DI CHUYỂN ODOMETRY =================
void moveForwardTicks(long ticks, int speed) {
  encoderLeftCount = 0; encoderRightCount = 0;
  setMotors(speed, speed);
  while ((abs(encoderLeftCount) + abs(encoderRightCount)) / 2 < ticks) {}
  setMotors(0, 0); delay(100);
}

void turnLeft90() {
  encoderLeftCount = 0; encoderRightCount = 0;
  setMotors(-150, 150); 
  while (abs(encoderRightCount) < TICKS_PER_90_DEG) { }
  setMotors(0, 0); delay(100);
}

void turnRight90() {
  encoderLeftCount = 0; encoderRightCount = 0;
  setMotors(150, -150);
  while (abs(encoderLeftCount) < TICKS_PER_90_DEG) { }
  setMotors(0, 0); delay(100);
}

void turnAround180() {
  encoderLeftCount = 0; encoderRightCount = 0;
  setMotors(150, -150);
  while (abs(encoderLeftCount) < (TICKS_PER_90_DEG * 2)) { }
  setMotors(0, 0); delay(100);
}

// ================= 7. HÀM LSRB =================
void recordAndSimplifyPath(char action) {
  path[pathLength] = action;
  pathLength++;

  if (pathLength >= 3 && path[pathLength - 2] == 'B') {
    char last = path[pathLength - 1];
    char preBack = path[pathLength - 3];
    char newAction = 'S';

    if (preBack == 'L' && last == 'L') newAction = 'S';
    else if (preBack == 'L' && last == 'R') newAction = 'B';
    else if (preBack == 'L' && last == 'S') newAction = 'R';
    else if (preBack == 'R' && last == 'L') newAction = 'B';
    else if (preBack == 'S' && last == 'L') newAction = 'R';
    else if (preBack == 'S' && last == 'S') newAction = 'B';

    pathLength -= 3;
    path[pathLength] = newAction;
    pathLength++;
  }
}

// ================= 8. KHỞI TẠO =================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(ENC_L_A, INPUT_PULLUP); pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP); pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), countLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), countRightEncoder, RISING);

  pinMode(MOT_L_IN1, OUTPUT); pinMode(MOT_L_IN2, OUTPUT); pinMode(MOT_L_PWM, OUTPUT);
  pinMode(MOT_R_IN1, OUTPUT); pinMode(MOT_R_IN2, OUTPUT); pinMode(MOT_R_PWM, OUTPUT);
  setMotors(0, 0);

  Wire.setSDA(SDA_PIN); Wire.setSCL(SCL_PIN); Wire.begin();

  pinMode(XSHUT_F1, OUTPUT); pinMode(XSHUT_F2, OUTPUT);
  pinMode(XSHUT_SL, OUTPUT); pinMode(XSHUT_SR, OUTPUT);
  
  digitalWrite(XSHUT_F1, LOW); digitalWrite(XSHUT_F2, LOW);
  digitalWrite(XSHUT_SL, LOW); digitalWrite(XSHUT_SR, LOW);
  delay(10);

  digitalWrite(XSHUT_F1, HIGH); delay(10); lox_F1.begin(0x30);
  digitalWrite(XSHUT_F2, HIGH); delay(10); lox_F2.begin(0x31);
  digitalWrite(XSHUT_SL, HIGH); delay(10); lox_SL.begin(0x32);
  digitalWrite(XSHUT_SR, HIGH); delay(10); lox_SR.begin(0x33);
  
  Serial.println("KHOI DONG XONG. Nhan nut de BAT DAU DO DUONG.");
}

// ================= 9. VÒNG LẶP CHÍNH =================
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200); 
    if (currentState == STATE_WAITING) {
      currentState = STATE_EXPLORING;
      Serial.println("--- BAT DAU DO DUONG ---");
    } 
    else if (currentState == STATE_FINISHED) {
      currentState = STATE_SPEEDRUN;
      Serial.println("--- BAT DAU CHAY SPEEDRUN ---");
    }
    while(digitalRead(BUTTON_PIN) == LOW); 
  }

  switch (currentState) {
    case STATE_WAITING: setMotors(0, 0); break;
    case STATE_EXPLORING: exploreMaze(); break;
    case STATE_FINISHED: setMotors(0, 0); break;
    case STATE_SPEEDRUN: speedRunMaze(); break;
  }
}

// ================= 10. DÒ ĐƯỜNG (LẦN 1) =================
void exploreMaze() {
  VL53L0X_RangingMeasurementData_t mF1, mF2, mSL, mSR;
  lox_F1.rangingTest(&mF1, false); lox_F2.rangingTest(&mF2, false);
  lox_SL.rangingTest(&mSL, false); lox_SR.rangingTest(&mSR, false);

  int dist_F = (mF1.RangeMilliMeter + mF2.RangeMilliMeter) / 2; 
  int dist_L = mSL.RangeMilliMeter;
  int dist_R = mSR.RangeMilliMeter;

  // KIỂM TRA ĐÍCH (THOÁT MÊ CUNG)
  // Nếu cả 3 hướng (Trước, Trái, Phải) đều không có vật cản (> 300mm)
  if (dist_F > OUT_OF_MAZE && dist_L > OUT_OF_MAZE && dist_R > OUT_OF_MAZE) {
    setMotors(0, 0);
    currentState = STATE_FINISHED;
    Serial.println("DA THOAT ME CUNG! Nhan nut de chay lan 2.");
    return;
  }

  if (dist_F < 100) { 
    setMotors(0, 0); delay(100); 
    if (dist_L > 150) { turnLeft90(); recordAndSimplifyPath('L'); } 
    else if (dist_R > 150) { turnRight90(); recordAndSimplifyPath('R'); } 
    else { turnAround180(); recordAndSimplifyPath('B'); }
  } 
  else {
    if (dist_L > 150) { 
      moveForwardTicks(TICKS_ALIGN_CENTER, 150); 
      turnLeft90();
      recordAndSimplifyPath('L');
    } 
    else {
      float error = dist_L - dist_R;
      if (dist_L > 150 || dist_R > 150) error = 0; 
      float output = (Kp * error) + (Kd * (error - last_error));
      last_error = error;
      setMotors(constrain(base_speed - output, 0, 255), constrain(base_speed + output, 0, 255));
    }
  }
}

// ================= 11. SPEEDRUN (LẦN 2) =================
void speedRunMaze() {
  for (int i = 0; i < pathLength; i++) {
    char action = path[i];

    if (action == 'S') { moveForwardTicks(TICKS_PER_CELL, 200); } 
    else if (action == 'L') { turnLeft90(); } 
    else if (action == 'R') { turnRight90(); }

    // Kiểm tra xem đã ra khỏi mê cung chưa sau mỗi bước
    VL53L0X_RangingMeasurementData_t mF1, mF2, mSL, mSR;
    lox_F1.rangingTest(&mF1, false); lox_F2.rangingTest(&mF2, false);
    lox_SL.rangingTest(&mSL, false); lox_SR.rangingTest(&mSR, false);
    
    int current_F = (mF1.RangeMilliMeter + mF2.RangeMilliMeter) / 2;
    int current_L = mSL.RangeMilliMeter;
    int current_R = mSR.RangeMilliMeter;

    if (current_F > OUT_OF_MAZE && current_L > OUT_OF_MAZE && current_R > OUT_OF_MAZE) {
      setMotors(0, 0);
      currentState = STATE_WAITING;
      Serial.println("HOAN THANH SPEEDRUN!");
      return;
    }
  }
  
  setMotors(0, 0);
  currentState = STATE_WAITING;
}
