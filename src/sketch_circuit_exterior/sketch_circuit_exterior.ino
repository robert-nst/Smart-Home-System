#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Servo.h>
#include <SPI.h>

// Ultrasonic Sensor pins
#define TRIG_PIN 5
#define ECHO_PIN 6

// Passive Buzzer pins
#define BUZZER_PIN 7

// LED pin
#define LED_PIN 4

// Servomotor pin
#define SERVO_PIN 12

// LCD pins
#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8

// declare display object
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// declare servomotor
Servo myservo;

// variables for Ultrasonic Sensor
long duration;
int distance;

// variable for Bluetooth communication
char switchstate;

void setup() {
  // init the serial communication, baud rate = 9600 bits/second
  Serial.begin(9600);
  
  // initialize pins for Ultrasonic Sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // init the Passive Buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // init the LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // attach the servomotor to the pin
  myservo.attach(SERVO_PIN);
  myservo.write(0); // Initial position

  // init the LCD
  // resolution
  tft.init(240, 320);
  // clear screen with black color
  tft.fillScreen(ST77XX_BLACK);

  // initial display messages
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Security System");

  tft.setCursor(10, 50);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Status: Waiting...");

  tft.setCursor(10, 100);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Distance: -- cm");
}

void loop() {
  // check for Bluetooth input
  if (Serial.available() > 0) {
    switchstate = Serial.read();
    Serial.print(switchstate);
    Serial.print("\n");
    delay(15);

    if (switchstate == '1') {
      // turn on the LED
      digitalWrite(LED_PIN, HIGH);
    } else if (switchstate == '0') {
      // turn off the LED
      digitalWrite(LED_PIN, LOW);
    } else if (switchstate == '2') {
      // rotate servo to 90 degrees
      myservo.write(90);
    } else if (switchstate == '3') {
      // rotate servo back to 0 degrees
      myservo.write(0);
    }
  }

  // clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // sets the trigPin on HIGH state for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);

  // calculate the distance
  distance = duration * 0.034 / 2;

  // clear the previous distance display
  tft.fillRect(10, 100, 220, 40, ST77XX_BLACK);

  // display the measured distance
  tft.setCursor(10, 100);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("Distance: ");
  tft.print(distance);
  tft.println(" cm");

  // check if the distance is less than 20cm and update the security status
  if (distance < 20) {
    // Set buzzer to 1000 Hz
    tone(BUZZER_PIN, 1000);
    tft.fillRect(10, 50, 220, 40, ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Status: ALERT!");
  } else {
    // otherwise, turn off the buzzer
    noTone(BUZZER_PIN);
    tft.fillRect(10, 50, 220, 40, ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.println("Status: Secure");
  }

  // delay for a short period before measuring again
  delay(500);
}
