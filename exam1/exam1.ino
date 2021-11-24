#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>

// 스위치 핀
#define SW1_PIN 3
#define SW2_PIN 18
#define SW3_PIN 19

// 펄스 발생기 핀
#define PULSE_PIN 7

// 초음파 센서 핀
#define TRIG_PIN 8
#define ECHO_PIN 9

// 온도 센서 핀, 조도센서 핀
#define TEMP_PIN A0
#define CDS_PIN A1

// 딜레이
#define TIME_DELAY 250

// 상태 변수를 만들어서 LCD화면의 상태에 따라 화면창이 바뀌고, 동작이 바뀜.
typedef enum {
  LcdStateInit = -2,   // Digital Pulse \n Number: A001
  LcdStateStart,       // Mode Button \n Push Mode SW1_PIN!
  LcdStateHome1,       // 1. Pulse Mode \n Push Select!
  LcdStateHome2,       // 2. Sensor Mode \n Push Select!
  LcdStateMode1,       // [Pulse Mode] \n Freq: xxxHz
  LcdStateMode2        // T:xx.xxC CDS:DAY \n U:■■■■
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);   // LCD 객체
SimpleTimer TIMER;                    // Timer 객체
_LcdState LcdState, PreLcdState;      // 상태 변수



// SW1을 눌렀을 때 수행하는 함수
void press_SW1() {
  int cnt = 0;

  // 채터링 방지
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);

  // 버튼을 길게 누를때 0.15초 간격으로 200번 카운터하면 3초가 됨.
  while (digitalRead(SW1_PIN) == LOW) {
    delayMicroseconds(15000); // 이 함수는 16,384값을 초과하면 아니됨.
    if (++cnt >= 197) {
      LcdState = LcdStateInit;
      break;
    }
  }

  // 버튼을 떼었을 때 상태 전환
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

// SW2를 눌렀을 때 수행하는 함수
void press_SW2() {
  // 채터링 방지
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);

  switch (LcdState) {
    case LcdStateHome1:
      LcdState = LcdStateMode1;
      break;
    case LcdStateHome2:
      LcdState = LcdStateMode2;
      break;
  }
}

// SW3을 눌렀을 때 수행하는 함수
void press_SW3() {
  // 채터링 방지
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);

  LcdState = LcdStateHome1;
}


// LCD 첫번째 줄에 s1을, 두번째 줄에 s2를 표시함
void lcd_Print(char *s1, char *s2) {
  LCD.clear();
  LCD.home();
  LCD.print(s1);
  LCD.setCursor(0, 1);
  LCD.print(s2);
}

// LCD를 num번 수행함
void lcd_Blink(int num) {
  // SW3을 누르면 어떠한 경우에서도 동작구성 2)-가)로 가야됨.
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

// Mode 2. Sensor Mode 동작
void sensing_Temp_Cds_Ultra() {
  // 타이머 아이디
  int a, b, c;

  // 온도, 조도, 초음파 센서 타이머 설정
  a = TIMER.setInterval(333, sensing_Temp);
  b = TIMER.setInterval(377, sensing_Cds);
  c = TIMER.setInterval(399, sensing_Ultra);

  while (LcdState == LcdStateMode2) {
    TIMER.run();
  }

  // 타이머 종료
  TIMER.deleteTimer(a);
  TIMER.deleteTimer(b);
  TIMER.deleteTimer(c);
}

// Mode 2. LM35 Sensor Mode 동작
void sensing_Temp() {
  // LM35 온도 공식 (100 * 5 / 1024)
  float temp = analogRead(TEMP_PIN) * 0.48828125;

  LCD.setCursor(2, 0);
  LCD.print(temp);
  LCD.print("C");
}

// Mode 2. CDS Sensor Mode 동작
void sensing_Cds() {
  // 센서 전압 : 기준 전압(=5v) = 디지털 값(=analogRead(pin_number)) : 분해능(=1024)
  float cds = analogRead(CDS_PIN) * 0.0048828125;

  LCD.setCursor(13, 0);
  if (cds < 2.5) {
    LCD.print("DAY");
  }
  else {
    LCD.print("NIG");
  }
}

// Mode 2. 초음파 Sensor Mode 동작
void sensing_Ultra() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 거리 = 속력 * 시간
  int distance = pulseIn(ECHO_PIN, HIGH) / 58;

  int tens = distance / 5;  // 몫
  int units = distance % 5; // 나머지

  //LCD가 16칸이므로 초과시 return
  if (tens > 14) return;

  //(2 + tens, 1)부터 초기화
  LCD.setCursor(2 + tens, 1);
  for (int i = tens; i < 14; i++) {
    LCD.print(" ");
  }

  //(2, 1)부터 거리 갱신
  LCD.setCursor(2, 1);
  for (int i = 0; i < tens; i++) {
    LCD.write(5);
  }
  LCD.write(units);
}

// Mode 1. Pulse Mode 동작
void sensing_Pulse() {
  int duration, frequency, PreFrequency;

  while (LcdState == LcdStateMode1) {
    // pulseIn함수의 반환값이 micro단위임.
    duration = pulseIn(PULSE_PIN, HIGH) + pulseIn(PULSE_PIN, LOW);

    // 주파수 = 1 / 진동주기
    frequency = round(1000000 / duration);

    // 이전 주파수와 현재 주파수가 같을 시 다시 실행
    if (PreFrequency == frequency) continue;

    LCD.setCursor(6, 1);
    LCD.print(frequency);
    LCD.print("Hz    ");

    PreFrequency = frequency;
  }
}

void setup() {
  // 초음파 센서가 측정한 거리에 따라 세로로 한 칸당 1cm를 표현함.
  uint8_t one[8]  = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
  uint8_t two[8]  = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
  uint8_t three[8] = {0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};
  uint8_t four[8] = {0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e};
  uint8_t five[8]  = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};

  // 핀 설정
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  pinMode(SW3_PIN, INPUT);
  pinMode(PULSE_PIN, INPUT);
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

  // 초음파 센서 거리에 따른 사용자 정의 문자 지정
  LCD.createChar(1, one);
  LCD.createChar(2, two);
  LCD.createChar(3, three);
  LCD.createChar(4, four);
  LCD.createChar(5, five);

  // LcdState 초기설정
  LcdState = LcdStateInit;
}

void loop() {
  // LcdState이 이전 상태와 같으면 실행안됨. 이게 없으면 LCD가 계속 꺼졌다 켜졌다를 반복함.
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
        lcd_Print("[Pulse Mode]", "Freq:       ");
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
