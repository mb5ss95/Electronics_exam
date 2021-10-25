#include <LiquidCrystal_I2C.h>

#define SW1_PIN 3
#define SW2_PIN 18
#define SW3_PIN 19

#define PULSE_PIN 7
#define TRIG_PIN 8
#define ECHO_PIN 9

#define TEMP_PIN A0
#define CDS_PIN A1

#define TIME_DELAY 250


uint8_t zero[8]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t one[8]  = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
uint8_t two[8]  = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
uint8_t three[8] = {0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};
uint8_t four[8] = {0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e};
uint8_t five[8]  = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};

typedef enum {
  LcdStateInit = -2,   // Digital Pulse \n Number: A001
  LcdStateStart,       // Mode Button \n Push Mode SW1_PIN!
  LcdStateHome1,       // 1. Pulse Mode \n Push Select!
  LcdStateHome2,       // 2. Sensor Mode \n Push Select!
  LcdStateMode1,       // [Pulse Mode] \n Freq: xxxHz
  LcdStateMode2        // T:xx.xxC CDS:DAY \n U:■■■■
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);

_LcdState LcdState, PreLcdState;


void press_SW1() {
  int cnt = 0;

  while (digitalRead(SW1_PIN) == LOW) {
    delayMicroseconds(15000);
    if (++cnt >= 200) {
      LcdState = LcdStateInit;
      break;
    }
  }

  switch (LcdState) {
    case LcdStateInit:
      break;
    case LcdStateHome1:
      LcdState = LcdStateHome2;
      break;
    default:
      LcdState = LcdStateHome1;
      break;
  }
}

void press_SW2() {
  switch (LcdState) {
    case LcdStateHome1:
      LcdState = LcdStateMode1;
      break;
    case LcdStateHome2:
      LcdState = LcdStateMode2;
      break;
  }
}

void press_SW3() {
  LcdState = LcdStateHome1;
}

void setup() {
  // 핀 설정
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);

  // 스위치 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(SW1_PIN), press_SW1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SW2_PIN), press_SW2, RISING);
  attachInterrupt(digitalPinToInterrupt(SW3_PIN), press_SW3, RISING);

  // 시리얼 모니터 초기설정
  Serial.begin(9600);

  // LCD 초기설정
  LCD.begin();
  LCD.backlight();
  LCD.createChar(0, zero);
  LCD.createChar(1, one);
  LCD.createChar(2, two);
  LCD.createChar(3, three);
  LCD.createChar(4, four);
  LCD.createChar(5, five);

  // LcdState 초기설정
  LcdState = LcdStateInit;
}

void lcd_Print(char *s1, char *s2) {
  LCD.clear();
  LCD.home();
  LCD.print(s1);
  LCD.setCursor(0, 1);
  LCD.print(s2);
}

void lcd_Blink(int num) {
  while (LcdState == LcdStateInit) {
    delay(TIME_DELAY);
    LCD.noDisplay();
    delay(TIME_DELAY);
    LCD.display();
    if (--num == 0) {
      LcdState = LcdStateStart;
      break;
    }
  }
}

void sensing_Pulse() {
  int PreFrequency;

  while (LcdState == LcdStateMode1) {
    int duration = pulseIn(PULSE_PIN, HIGH) + pulseIn(PULSE_PIN, LOW);
    int frequency = 1 / duration;
    // 주파수 = 1 / 진동주기

    if (PreFrequency == frequency) continue;

    LCD.setCursor(6, 1);
    LCD.print(frequency);
    LCD.print("Hz");

    PreFrequency = frequency;
  }
}

void sensing_Temp_Cds_Ultra() {
  float PreTemp, PreCds;
  int PreDistance;

  while (LcdState == LcdStateMode2 ) {
    sensing_Temp(PreTemp);
    sensing_Cds(PreCds);
    sensing_Ultra(PreDistance);
  }
}

void sensing_Temp(float &PreTemp) {
  float temp = analogRead(TEMP_PIN) * 0.48828125;
  // LM35 온도 공식 (100 * 5 / 1024)

  if (PreTemp == temp) return;

  LCD.setCursor(2, 0);
  LCD.print(temp);
  PreTemp = temp;
}

void sensing_Cds(float &PreCds) {
  float cds = analogRead(CDS_PIN) * 0.0048828125;
  // 센서 전압 : 기준 전압(=5v) = 디지털 값(=analogRead(pin_number)) : 분해능(=1024)

  if (cds == PreCds) return;

  LCD.setCursor(13, 0);
  if (cds < 2.5) {
    LCD.print("DAY");
  }
  else {
    LCD.print("NIG");
  }

  PreCds = cds;
}

void sensing_Ultra(int &PreDistance) {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int distance = pulseIn(ECHO_PIN, HIGH) / 58;
  // 거리 = 속력 * 시간

  int tens = distance / 5;  // 몫
  int units = distance % 5; // 나머지

  if (tens > 14 || PreDistance == distance) return;
  //LCD가 16칸이므로 초과시 return

  LCD.setCursor(2 + tens, 1);
  for (int i = tens; i < 14; i++) {
    LCD.write(0);
  }
  //(2,1)부터 초기화

  LCD.setCursor(2, 1);
  for (int i = 0; i < tens; i++) {
    LCD.write(5);
  }
  LCD.write(units);
  //(2,1)부터 거리 갱신

  PreDistance = distance;
}


void loop() {
  if (LcdState != PreLcdState) {
    PreLcdState = LcdState;
    switch (LcdState) {
      case LcdStateInit:
        lcd_Print("Digital Pulse", "Number:     A001");
        lcd_Blink(5);
        break;
      case LcdStateStart:
        lcd_Print(" Mode Button", " Push Mode SW1!");
        break;
      case LcdStateHome1:
        lcd_Print("1.Pulse Mode", " Push Select!");
        break;
      case LcdStateHome2:
        lcd_Print("2.Sensor Mode", " Push Select!");
        break;
      case LcdStateMode1:
        lcd_Print("[Pulse Mode]", "Freq: ");
        sensing_Pulse();
        break;
      case LcdStateMode2:
        lcd_Print("T:     C CDS:", "U:");
        sensing_Temp_Cds_Ultra();
        break;
      default:
        break;
    }
  }
}
