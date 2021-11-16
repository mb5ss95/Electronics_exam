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
  LcdStateInit = -2,        // Digital Pulse \n Number: A001
  LcdStateStart,            // Mode Button \n Push Mode SW1_PIN!
  LcdStateHome1,            // 1.Clock Mode \n Push Select!
  LcdStateHome2,            // 2. Sensor Mode \n Push Select!
  LcdStateMode1,            // [ DigitalClock ] \n[AM]11:59:50:56
  LcdStateMode2_Cds,        // 2.Sensor Mode \n [CDS] xxx
  LcdStateMode2_Ultra,      // 2.Sensor Mode \n [Ultra] 013cm
  LcdStateMode2_Temp,       // 2.Sensor Mode \n [Temp] xx.xxC
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);   // LCD 객체
SimpleTimer TIMER;                    // Timer 객체
_LcdState LcdState, PreLcdState;      // 상태 변수
int msec2;                            // 0.01초



// SW1을 눌렀을 때 수행하는 함수
void press_SW1() {
  int cnt = 0;

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
  switch (LcdState) {
    case LcdStateHome1:
      LcdState = LcdStateMode1;
      break;
    case LcdStateHome2:
      LcdState = LcdStateMode2_Cds;
      break;
  }
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);
}

// SW3을 눌렀을 때 수행하는 함수
void press_SW3() {
  switch (LcdState) {
    case LcdStateMode2_Cds:
      LcdState = LcdStateMode2_Ultra;
      break;
    case LcdStateMode2_Ultra:
      LcdState = LcdStateMode2_Temp;
      break;
    case LcdStateMode2_Temp:
      LcdState = LcdStateMode2_Cds;
      break;
  }
  delayMicroseconds(15000);
  delayMicroseconds(15000);
  delayMicroseconds(15000);
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
  float temp;

  while (LcdState == LcdStateMode2_Temp) {
    // LM35 온도 공식 (100 * 5 / 1024)
    temp = analogRead(TEMP_PIN) * 0.48828125;

    LCD.setCursor(8, 1);
    LCD.print(temp);
  }
}

// Mode 2. CDS Sensor Mode 동작
void sensing_Cds() {
  float cds;

  while (LcdState == LcdStateMode2_Cds) {
    // 센서 전압 : 기준 전압(=5v) = 디지털 값(=analogRead(pin_number)) : 분해능(=1024)
    cds = analogRead(CDS_PIN) * 0.0048828125;

    // 플로터 모니터 출력
    Serial.println(cds);

    LCD.setCursor(7, 1);
    if (cds >= 2.5) {
      LCD.print(" On");
    }
    else {
      LCD.print("Off");
    }
  }
}

// Mode 2. 초음파 Sensor Mode 동작
void sensing_Ultra() {
  int distance;

  while (LcdState == LcdStateMode2_Ultra) {
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 거리 = 속력 * 시간
    int distance = pulseIn(ECHO_PIN, HIGH) / 58;

    if (distance > 1000) continue;

    int tens = distance / 5;  // 몫
    int units = distance % 5; // 나머지

    LCD.setCursor(9, 1);
    LCD.print("       ");
    LCD.setCursor(9, 1);
    LCD.print(distance);
    LCD.print("cm  ");
  }
}

void count_Start() {
  msec2++;
}

// Mode 1. Clock Mode 동작
void sensing_Pulse() {
  // 11시 59분 50.56초
  int h = 11;
  int m1 = 5;
  int m2 = 9;
  int s1 = 5;
  int s2 = 0;
  int msec1 = 5;
  msec2 = 6;

  // 주기 = 1 / 주파수
  // pulseIn함수의 반환값이 micro단위임. 1000을 나누어 milli단위로 변환
  float duration = (pulseIn(PULSE_PIN, HIGH) + pulseIn(PULSE_PIN, LOW)) / 1000.0;

  // 100Hz의 펄스를 입력받아서 시계의 10 msec 단위의 기준으로 설정
  // 타이머 설정, (int)round(duration) 시간마다 count_Start 함수 실행
  int counter = TIMER.setInterval((int)round(duration), count_Start);


  while (LcdState == LcdStateMode1) {
    TIMER.run();
    if (msec2 >= 10) {
      msec2 = 0;
      msec1++;
      if (msec1 >= 10) {
        msec1 = 0;
        s2++;
        if (s2 >= 10) {
          s2 = 0;
          s1++;
          if (s1 >= 6) {
            s1 = 0;
            m2++;
            if (m2 >= 10) {
              m2 = 0;
              m1++;
              if (m1 >= 6) {
                m1 = 0;
                h++;
                LCD.setCursor(1, 1);
                h %= 24;
                if (h < 10) {
                  LCD.print("AM]0");
                  LCD.setCursor(5, 1);
                }
                else if (h < 12) {
                  LCD.print("AM");
                  LCD.setCursor(4, 1);
                }
                else {
                  LCD.print("PM");
                  LCD.setCursor(4, 1);
                }
                LCD.print(h);
              }
              LCD.setCursor(7, 1);
              LCD.print(m1);
            }
            LCD.setCursor(8, 1);
            LCD.print(m2);
          }
          LCD.setCursor(10, 1);
          LCD.print(s1);
        }
        LCD.setCursor(11, 1);
        LCD.print(s2);
      }
      LCD.setCursor(13, 1);
      LCD.print(msec1);
    }
    LCD.setCursor(14, 1);
    LCD.print(msec2);
  }
  TIMER.deleteTimer(counter);
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
        sensing_Pulse();
        break;
      case LcdStateMode2_Cds:
        lcd_Print("2.Sensor Mode", " [CDS] xxx");
        sensing_Cds();
        break;
      case LcdStateMode2_Ultra:
        lcd_Print("2.Sensor Mode", " [Ultra] 013cm");
        sensing_Ultra();
        break;
      case LcdStateMode2_Temp:
        lcd_Print("2.Sensor Mode", " [Temp] xx.xxC");
        sensing_Temp();
        break;
      default:
        break;
    }
  }
}
