#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define SW1_PIN 3
#define SW2_PIN 18
#define SW3_PIN 19

#define PULSE_PIN 7
#define TRIG_PIN 8
#define ECHO_PIN 9

#define TEMP_PIN A0
#define CDS_PIN A1

#define TIME_DELAY 250

typedef enum {
  LcdStateInit = -2,        // Digital Pulse \n Number: A001
  LcdStateStart,            // Mode Button \n Push Mode SW1_PIN!
  LcdStateHome1,            // 1.Clock Mode \n Push Select!
  LcdStateHome2,            // 2. Sensor Mode \n Push Select!
  LcdStateMode1,            // [ DigitalClock ] \n[AM]11:59:50:56
  LcdStateMode2             // [AM]11:59:50:56 \nCDS:xxxx T:xx.xx
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);
SimpleTimer TIMER;

_LcdState LcdState, PreLcdState;
int TimerId, CdsId, UltraId, cnt;
int Cds;
int Hour = 11;
int Minute = 59;
int Sec = 50;
float Temp;



void timer_Start() {
  if (cnt++ == 3) LcdState == LcdStateInit;
}

void press_SW1() {
  cnt = 0;

  TIMER.enable(TimerId);
  while (digitalRead(SW1_PIN) == LOW) {
    if (LcdState == LcdStateInit) break;
  }
  TIMER.disable(TimerId);

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
  switch (LcdState) {
    case LcdStateStart:
      Cds = EEPROM.read(1);
      Temp = EEPROM.read(2);
      Hour = EEPROM.read(3);
      Minute = EEPROM.read(4);
      Sec = EEPROM.read(5);
      LcdState = LcdStateMode2;
      break;
    case LcdStateMode2:
      EEPROM.write(1, Cds);
      EEPROM.write(2, Temp);
      EEPROM.write(3, Hour);
      EEPROM.write(4, Minute);
      EEPROM.write(5, Sec);
      break;
  }
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

  // 타이머 초기설정
  TimerId = TIMER.setInterval(1000, timer_Start);    // 1초에 한번씩 timer_Start 함수 실행
  TemprId = TIMER.setInterval(300, sensing_Temp);    // 0.3초에 한번씩 sensing_Temp 함수 실행
  CdsId = TIMER.setInterval(200, sensing_Cds);       // 0.2초에 한번씩 sensing_Cds 함수 실행
  UltraId = TIMER.setInterval(100, sensing_Ultra);   // 0.1초에 한번씩 sensing_Ultra 함수 실행
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

void sensing_Pulse(int index) {
  int msec = 0;

  while (LcdState == LcdStateMode1) {
    msec = msec + pulseIn(PULSE_PIN, HIGH) + pulseIn(PULSE_PIN, LOW);
    if (msec >= 100) {
      msec = 0;
      sec++;
      if (sec >= 60) {
        sec = 0;
        minute++;
        if (minute >= 60) {
          minute = 0;
          hour++;
          LCD.setCursor(1, index);
          if (hour < 12) {
            LCD.print("PM");
          }
          else {
            hour %= 24;
            LCD.print("AM");
          }
          LCD.setCursor(4, index);
          LCD.print(hour);
        }
        LCD.setCursor(7, index);
        LCD.print(minute);
      }
      LCD.setCursor(10, index);
      LCD.print(sec);
      LCD.setCursor(13, index);
    }
    LCD.print(msec);
  }
}

void timer_Enable() {
  TIMER.enable(TempId);
  TIMER.enable(CdsId);
  TIMER.enable(UltraId);
}

void timer_Disable() {
  TIMER.disable(TempId);
  TIMER.disable(CdsId);
  TIMER.disable(UltraId);
}

void sensing_Temp() {
  Temp = analogRead(TEMP_PIN) * 0.48828125;
  // LM35 온도 공식 (100 * 5 / 1024)

  LCD.setCursor(11, 1);
  LCD.print(Temp);
}

void sensing_Cds() {
  Cds = analogRead(CDS_PIN);
  Serial.println(Cds);
  // 플로터 모니터 출력

  LCD.setCursor(4, 1);
  LCD.print(Cds);
}

void sensing_Ultra() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int distance = pulseIn(ECHO_PIN, HIGH) / 58;

  if (distance < 15) LcdState = LcdStateHome1;
}


void loop() {
  if (LcdState != PreLcdState) {
    PreLcdState = LcdState;
    timer_Disable();
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
        sensing_Pulse(1);
        break;
      case LcdStateMode2:
        lcd_Print("[AM]11:59:50:56", "CDS:xxxx T:xx.xx");
        timer_Enable();
        sensing_Pulse(0);
        break;
      default:
        break;
    }
  }
  TIMER.run();
}
