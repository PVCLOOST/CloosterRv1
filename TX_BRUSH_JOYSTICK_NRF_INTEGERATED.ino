#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// NRF24 CE = 4, CSN = 5
RF24 radio(4, 5);
const byte address[6] = "NODE1";

// -----------------------
// Toggle Switch (Brush)
// -----------------------
#define PIN_ON1 26   // Brush ON
#define PIN_ON2 25   // Brush OFF

uint8_t brushCmd = 0;

// -----------------------
// Joystick Pins
// -----------------------
#define VRX_PIN 34
#define VRY_PIN 35

// Thresholds
#define LEFT_HARD      900
#define RIGHT_HARD     2500
#define UP_HARD        1000
#define DOWN_HARD      3000

#define RIGHT_Y_LIMIT  1200
#define LEFT_Y_LIMIT   1200

// Direction encoding
#define DIR_CENTER 0
#define DIR_UP     1
#define DIR_DOWN   2
#define DIR_LEFT   3
#define DIR_RIGHT  4

// --> Direction name lookup table
const char* dirName[] = {
  "CENTER",
  "UP",
  "DOWN",
  "LEFT",
  "RIGHT"
};

// -----------------------
// Packet Structure
// -----------------------
struct Packet {
  uint8_t brushCmd;
  uint8_t joyDir;
  uint16_t joyX;
  uint16_t joyY;
};

Packet pkt;

// -----------------------
// Determine Joystick Direction
// -----------------------
uint8_t getJoystickDirection(int x, int y) {

  // PRIORITY: y == 0 → UP
  if (y == 0)
    return DIR_UP;

  // RIGHT REGION
  if (x > RIGHT_HARD) {

    if (y < RIGHT_Y_LIMIT)    return DIR_RIGHT;
    else if (y > DOWN_HARD)   return DIR_DOWN;
    else if (y < UP_HARD)     return DIR_UP;
    else                      return DIR_CENTER;
  }

  // LEFT REGION
  else if (x < LEFT_HARD) {

    if (y < LEFT_Y_LIMIT)     return DIR_LEFT;
    else if (y > DOWN_HARD)   return DIR_DOWN;
    else if (y < UP_HARD)     return DIR_UP;
    else                      return DIR_LEFT;
  }

  // CENTER REGION
  else {

    if (y < UP_HARD)          return DIR_UP;
    else if (y > DOWN_HARD)   return DIR_DOWN;
    else                      return DIR_CENTER;
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== NRF24 TX (BRUSH + JOYSTICK) ===");

  pinMode(PIN_ON1, INPUT_PULLDOWN);
  pinMode(PIN_ON2, INPUT_PULLDOWN);

  analogSetAttenuation(ADC_11db);

  SPI.begin(18, 19, 23, 5);

  if (!radio.begin()) {
    Serial.println("NRF24 INIT FAIL!");
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("TX READY!\n");
}

// ===============================
// LOOP
// ===============================
void loop() {

  // Brush Toggle
  if (digitalRead(PIN_ON1))       brushCmd = 1;
  else if (digitalRead(PIN_ON2))  brushCmd = 0;

  // Joystick
  int x = analogRead(VRX_PIN);
  int y = analogRead(VRY_PIN);

  uint8_t joyDir = getJoystickDirection(x, y);

  // Fill Packet
  pkt.brushCmd = brushCmd;
  pkt.joyDir   = joyDir;
  pkt.joyX     = x;
  pkt.joyY     = y;

  bool sent = radio.write(&pkt, sizeof(pkt));

  // ===============================
  // SERIAL DEBUG WITH TEXT OUTPUT
  // ===============================
  Serial.print("Brush=");
  Serial.print(pkt.brushCmd);

  Serial.print(" | Dir=");
  Serial.print(dirName[pkt.joyDir]);   // <<< DIRECTION AS TEXT

  Serial.print(" | X=");
  Serial.print(pkt.joyX);

  Serial.print(" | Y=");
  Serial.print(pkt.joyY);

  Serial.print(" | TX=");
  Serial.println(sent ? "OK" : "FAIL");

  delay(80);
}
