#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>

#define SW1_PIN 3
#define SW2_PIN 18
#define SW3_PIN 19

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
  LcdStateMode2_Cds,        // 2.Sensor Mode \n [CDS] xxx
  LcdStateMode2_Ultra,      // 2.Sensor Mode \n [Ultra] 013cm
  LcdStateMode2_Temp,       // 2.Sensor Mode \n [Temp] xx.xxC
} _LcdState;

// 전역 변수
LiquidCrystal_I2C LCD(0x27, 16, 2);
SimpleTimer TIMER;

_LcdState LcdState, PreLcdState;
int TempId, UltraId, CdsId;

void press_SW1() {
  int cnt = 0;
  while (digitalRead(SW1_PIN) == LOW) {
    delayMicroseconds(3000);
    if (++cnt >= 1000) {
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
      LcdState = LcdStateMode2_Cds;
      break;
  }
}

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
  UltraId = TIMER.setInterval(100, sensing_Ultra); // 초음파(ultrasonic wave)타이머
  TempId = TIMER.setInterval(500, sensing_Temp);   // 온도(Temperature) 타이머
  CdsId = TIMER.setInterval(500, sensing_Cds);     // 조도(illuminance) 타이머
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

void lcd_Pulse() {

}


void disable_Timer() {
  TIMER.disable(TempId);
  TIMER.disable(CdsId);
  TIMER.disable(UltraId);
}

void sensing_Temp() {
  float temp = analogRead(TEMP_PIN) * 0.48828125;
  // LM35 온도 공식 (100 * 5 / 1024)

  LCD.setCursor(8, 1);
  LCD.print(temp);

}

void sensing_Cds() {
  float cds = analogRead(CDS_PIN) * 0.0048828125;
  // 센서 전압 : 기준 전압(=5v) = 디지털 값(=analogRead(pin_number)) : 분해능(=1024)

  LCD.setCursor(7, 1);
  if (cds >= 2.5) {
    LCD.print(" On");
  }
  else {
    LCD.print("Off");
  }
}

void sensing_Ultra() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int distance = pulseIn(ECHO_PIN, HIGH) / 58;

  if (distance > 1000) return;
  Serial.println(distance);

  LCD.setCursor(9, 1);
  LCD.print("       ");
  LCD.setCursor(9, 1);
  LCD.print(distance);
  LCD.print("cm");

  /*
     Equation 1. d = v * t
     Equation 2. Distance = (Speed/170.15m) * (Meters/100cm) * (1e6us/170.15m) * (58.772us/cm)
     Equation 3. Distance = time/58 = (us)/(us/cm)=cm
  */
}


void loop() {
  if (LcdState != PreLcdState) {
    PreLcdState = LcdState;
    disable_Timer();
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
        lcd_Print("[ DigitalClock ]", "[AM]xx:xx:xx:xx");
        break;
      case LcdStateMode2_Cds:
        lcd_Print("2.Sensor Mode", " [CDS] xxx");
        TIMER.enable(CdsId);
        break;
      case LcdStateMode2_Ultra:
        lcd_Print("2.Sensor Mode", " [Ultra] 013cm");
        TIMER.enable(UltraId);
        break;
      case LcdStateMode2_Temp:
        lcd_Print("2.Sensor Mode", " [Temp] xx.xxC");
        TIMER.enable(TempId);
        break;
      default:
        break;
    }
  }
  TIMER.run();
}
