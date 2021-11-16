#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>


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


struct _Data {
  float Temp;
  int Cds;
  int Hour;
  int Minute1;
  int Minute2;
  int Sec1;
  int Sec2;
  int mSec1;
  int mSec2;
} Data;



// 상태 변수를 만들어서 LCD화면의 상태에 따라 화면창이 바뀌고, 동작이 바뀜.
typedef enum {
  LcdStateInit = -2,        // Digital Pulse \n Number: A001
  LcdStateStart,            // Mode Button \n Push Mode SW1_PIN!
  LcdStateHome1,            // 1.Clock Mode \n Push Select!
  LcdStateHome2,            // 2. Sensor Mode \n Push Select!
  LcdStateMode1,            // [ DigitalClock ] \n[AM]11:59:50:56
  LcdStateMode2             // [AM]11:59:50:56 \nCDS:xxxx T:xx.xx
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);   // LCD 객체
SimpleTimer TIMER;                    // Timer 객체
_LcdState LcdState, PreLcdState;      // 상태 변수

// SW1을 눌렀을 때 수행하는 함수
void press_SW1() {
  int cnt = 0;
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);

  // 버튼을 길게 누를때 0.15초 간격으로 200번 카운터하면 3초가 됨.
  while (digitalRead(SW1_PIN) == LOW) {
    delayMicroseconds(15000); // 이 함수는 16,384값을 초과하면 아니됨.
    if (++cnt >= 200) {
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
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);

  switch (LcdState) {
    case LcdStateStart:
      EEPROM.get(100, Data);
      LcdState = 10;
      break;
    case LcdStateMode2:
      EEPROM.put(100, Data);
      break;
  }
}

// EEPROM에서 가져온 값을 LCD에 표시
void lcd_Print() {
  LCD.clear();
  LCD.home();
  if (Data.Hour < 10) {
    LCD.print("[AM]0");
  }
  else if (Data.Hour < 12) {
    LCD.print("[AM]");
  }
  else {
    LCD.print("[PM]");
  }
  LCD.print(Data.Hour);
  LCD.print(":");
  LCD.print(Data.Minute1);
  LCD.print(Data.Minute2);
  LCD.print(":");
  LCD.print(Data.Sec1);
  LCD.print(Data.Sec2);
  LCD.print(":00");

  LCD.setCursor(0, 1);
  LCD.print("CDS:");
  LCD.print(Data.Cds);
  LCD.print(" ");
  LCD.setCursor(9, 1);
  LCD.print("T:");
  LCD.print(Data.Temp);
}

// LCD 첫번째 줄에 s1을, 두번째 줄에 s2를 표시함
void lcd_Print(char *s1, char *s2) {
  LCD.clear();
  LCD.home();
  LCD.print(s1);
  LCD.setCursor(0, 1);
  LCD.print(s2);
}

// LCD 깜빡이기를 num번 수행함
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

// Mode 2. LM35 Sensor Mode 동작
void sensing_Temp() {
  // LM35 온도 공식 (100 * 5 / 1024)
  Data.Temp = analogRead(TEMP_PIN) * 0.48828125;

  LCD.setCursor(11, 1);
  LCD.print(Data.Temp);
}

// Mode 2. CDS Sensor Mode 동작
void sensing_Cds() {
  Data.Cds = analogRead(CDS_PIN);

  // 플로터 모니터 출력
  Serial.println(Data.Cds);

  LCD.setCursor(4, 1);
  LCD.print(Data.Cds);
  LCD.print(" ");
}

// Mode 2. 초음파 Sensor Mode 동작
void sensing_Ultra() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 거리 = 속력 * 시간
  int distance = pulseIn(ECHO_PIN, HIGH) / 58;

  // 거리가 15cm 이내로 되면 동작구성 2)-가)로 돌아감
  if (distance < 15) LcdState = LcdStateHome1;
}

void count_Start() {
  Data.mSec2++;
}

// Mode 1. Clock Mode 동작, Mode 2. CDS, Temp Sensor Mode 동작
// index로 시계의 출력 위치를 조정
void sensing_Pulse(int index) {

  // 주기 = 1 / 주파수
  // pulseIn함수의 반환값이 micro단위임. 1000을 나누어 milli단위로 변환
  float duration = (pulseIn(PULSE_PIN, HIGH) + pulseIn(PULSE_PIN, LOW)) / 1000.0;

  // 100Hz의 펄스를 입력받아서 시계의 10 msec 단위의 기준으로 설정
  // 타이머 설정, (int)round(duration) 시간마다 count_Start 함수 실행
  int a, b, c, d;
  a = TIMER.setInterval((int)round(duration), count_Start);


  // LcdState = LcdStateMode2일때만
  // 온도, 조도, 초음파 센서 타이머 설정
  if (LcdState == LcdStateMode2) {
    b = TIMER.setInterval(333, sensing_Temp);
    c = TIMER.setInterval(377, sensing_Cds);
    d = TIMER.setInterval(399, sensing_Ultra);
  }

  while (LcdState == LcdStateMode1 || LcdState == LcdStateMode2) {
    if (Data.mSec2 >= 10) {
      Data.mSec2 = 0;
      Data.mSec1++;
      if (Data.mSec1 >= 10) {
        Data.mSec1 = 0;
        Data.Sec2++;
        if (Data.Sec2 >= 10) {
          Data.Sec2 = 0;
          Data.Sec1++;
          if (Data.Sec1 >= 6) {
            Data.Sec1 = 0;
            Data.Minute2++;
            if (Data.Minute2 >= 10) {
              Data.Minute2 = 0;
              Data.Minute1++;
              if (Data.Minute1 >= 6) {
                Data.Minute1 = 0;
                Data.Hour++;
                LCD.setCursor(1, index);
                Data.Hour %= 24;
                if (Data.Hour < 10) {
                  LCD.print("AM]0");
                  LCD.setCursor(5, index);
                }
                else if (Data.Hour < 12) {
                  LCD.print("AM");
                  LCD.setCursor(4, index);
                }
                else {
                  LCD.print("PM");
                  LCD.setCursor(4, index);
                }
                LCD.print(Data.Hour);
              }
              LCD.setCursor(7, index);
              LCD.print(Data.Minute1);
            }
            LCD.setCursor(8, index);
            LCD.print(Data.Minute2);
          }
          LCD.setCursor(10, index);
          LCD.print(Data.Sec1);
        }
        LCD.setCursor(11, index);
        LCD.print(Data.Sec2);
      }
      LCD.setCursor(13, index);
      LCD.print(Data.mSec1);
    }
    LCD.setCursor(14, index);
    LCD.print(Data.mSec2);
    TIMER.run();
  }
  TIMER.deleteTimer(a);
  TIMER.deleteTimer(b);
  TIMER.deleteTimer(c);
  TIMER.deleteTimer(d);
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
        lcd_Print("1.Clock Mode", " Push Select!");
        break;
      case LcdStateHome2:
        lcd_Print("2.Sensor Mode", " Push Select!");
        break;
      case LcdStateMode1:
        lcd_Print("[ DigitalClock ]", "[AM]11:59:50:56");
        Data = { 0.0, 0, 11, 5, 9, 5, 0, 5, 6};
        sensing_Pulse(1);
        break;
      case LcdStateMode2:
        lcd_Print("[AM]11:59:50:56", "CDS:xxx  T:xx.xx");
        Data = { 0.0, 0, 11, 5, 9, 5, 0, 5, 6};
        sensing_Pulse(0);
        break;
      default:
        lcd_Print();
        break;
    }
  }
}
