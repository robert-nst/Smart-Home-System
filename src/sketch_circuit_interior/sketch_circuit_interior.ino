#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <Servo.h>

// DHT11 PIN
#define DHTPIN 7

dht11 DHT;

// Initialize I2C LCD (address to 0x27 for a 16 chars and 2-line display)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// time to calibrate
int calibrationTime = 30;
// time of a low impluse
long unsigned int lowIn;
// the time the sensor has to be low before we assume all motion has stop
long unsigned int pause = 5000; // millis

// transition state variables
boolean lockLow = true;
boolean takeLowTime;
// flag system security (activated / deactivated)
boolean securityActive = false;
// flag gas sensor
boolean mq2Active = false;

// PIR Sensor pin
int pirPin = 3;
// LED pin
int ledPin = 13;
// Passive Buzzer pin
int buzzerPin = 8;
// Button pin
int buttonPin = 12;
// MQ-2 Gas Sensor pin
int mq2Pin = A0;
// Servomotors pins
int servo1Pin = 9;
int servo2Pin = 10;

// button debouncer variables
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int buttonState = HIGH;
int lastButtonState = HIGH;

// LCD information carousel
unsigned long displayTimer = 0;
int displayState = 0;

// declare servomotors
Servo servo1;
Servo servo2;

void setup() {
  // init serial communication, baud rate = 9600 bits/second
  Serial.begin(9600);

  // set pins as input or output
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(mq2Pin, INPUT);
  digitalWrite(pirPin, LOW);

  // attach the servos to their pins
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  // init position
  servo1.write(0);
  servo2.write(0);

  // init the LCD
  lcd.init();
  lcd.backlight();

  // give the PIR sensor some time to calibrate
  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" done");
  Serial.println("SENSOR ACTIVE");
  delay(50);

  // display initial message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Security system");
  lcd.setCursor(0, 1);
  lcd.print("deactivated");
}

void loop() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // if the switch changed:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the security system if the new button state is LOW
      if (buttonState == LOW) {
        // toggle security system
        securityActive = !securityActive;
        Serial.print("Security system ");
        Serial.println(securityActive ? "activated" : "deactivated");

        // update the LCD display
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Security system");
        lcd.setCursor(0, 1);
        lcd.print(securityActive ? "activated" : "deactivated");
      }
    }
  }

  // save the reading, next time in loop, it will be the lastButtonState:
  lastButtonState = reading;

  // read the MQ-2 Gas Sensor value
  int mq2Value = analogRead(mq2Pin);

  // check if the MQ-2 sensor value is above a specified value
  if (mq2Value > 700) {
    // activate servos only if they are not already active
    if (!mq2Active) {
      servo1.write(0);
      servo2.write(90);
      mq2Active = true;
    }
  } else {
    // deactivate servos only if they are active
    if (mq2Active) {
      servo1.write(90);
      servo2.write(0);
      mq2Active = false;
    }
  }

  // update the display every 3 seconds
  if (millis() - displayTimer >= 3000) {
    displayTimer = millis();

    DHT.read(DHTPIN);
    switch (displayState) {
      // display temperature
      case 0:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temperature:");
        lcd.setCursor(0, 1);
        lcd.print(DHT.temperature);
        lcd.print(" C");
        displayState = 1;
        break;

       // display humidity
      case 1:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Humidity:");
        lcd.setCursor(0, 1);
        lcd.print(DHT.humidity);
        lcd.print(" %");
        displayState = 2;
        break;

      // display MQ-2 Gas Sensor value
      case 2: 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Gas Measurement:");
        lcd.setCursor(0, 1);
        lcd.print(mq2Value);
        displayState = 3;
        break;

      // display security system state
      case 3:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Security system");
        lcd.setCursor(0, 1);
        lcd.print(securityActive ? "activated" : "deactivated");
        displayState = 0;
        break;
    }
  }

  if (securityActive) {
    // turn on the passive buzzer at 1000 Hz and the led when motion is detected
    if (digitalRead(pirPin) == HIGH) {
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 1000);

      // makes sure we wait for a transition to LOW before any further output is made:
      if (lockLow) {
        lockLow = false;
        Serial.println("---");
        Serial.print("motion detected at ");
        Serial.print(millis() / 1000);
        Serial.println(" sec");
        delay(50);
      }
      takeLowTime = true;
    }

    // turn off the passive buzzer and the led when motion is not detected anymore
    if (digitalRead(pirPin) == LOW) {
      digitalWrite(ledPin, LOW); 
      noTone(buzzerPin);

      if (takeLowTime) {
        // save the time of the transition from high to LOW
        lowIn = millis();
        // make sure this is only done at the start of a LOW phase
        takeLowTime = false;
      }
      // if the sensor is low for more than the given pause we assume that no more motion is going to happen
      if (!lockLow && millis() - lowIn > pause) {
        lockLow = true;
        Serial.print("motion ended at ");  // output
        Serial.print((millis() - pause) / 1000);
        Serial.println(" sec");
        delay(50);
      }
    }
  } else {
    // ensure the LED is off when the system is inactive
    digitalWrite(ledPin, LOW);
    // ensure the buzzer is off when the system is inactive
    noTone(buzzerPin);
  }
}
