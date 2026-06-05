#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// NRF24 CE=4, CSN=5
RF24 radio(4, 5);
const byte address[6] = "NODE1";

unsigned long lastReceive = 0;

// ===============================
// MOTOR BRUSH (TOGGLE)
// ===============================
#define BRUSH_RPWM 14    // Forward
#define BRUSH_LPWM 27    // Not used

int brushPWM = 200;

// ===============================
// PACKET STRUCT (MATCH TX)
// ===============================
struct Packet {
  uint8_t brushCmd;   // 0 = OFF, 1 = ON
  uint8_t joyDir;     // 0=CENTER, 1=UP, 2=DOWN, 3=LEFT, 4=RIGHT
  uint16_t joyX;      // ADC X
  uint16_t joyY;      // ADC Y
};

Packet pkt;

// ===============================
// DIRECTION NAME TABLE
// ===============================
const char* dirName[] = {
  "CENTER",
  "UP",
  "DOWN",
  "LEFT",
  "RIGHT"
};

// ===============================
// MOTOR CONTROL
// ===============================
void brushStop() {
  analogWrite(BRUSH_RPWM, 0);
  analogWrite(BRUSH_LPWM, 0);
}

void brushForward() {
  analogWrite(BRUSH_RPWM, brushPWM);
  analogWrite(BRUSH_LPWM, 0);
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== NRF24 RX (BRUSH ONLY + JOYSTICK DATA) ===");

  pinMode(BRUSH_RPWM, OUTPUT);
  pinMode(BRUSH_LPWM, OUTPUT);
  brushStop();

  SPI.begin(18, 19, 23, 5);

  if (!radio.begin()) {
    Serial.println("NRF24 INIT FAIL!");
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  Serial.println("RX READY\n");
}

// ===============================
// LOOP
// ===============================
void loop() {

  if (radio.available()) {
    radio.read(&pkt, sizeof(pkt));

    // ---------------------------
    // SERIAL DEBUG (TEXT VERSION)
    // ---------------------------
    Serial.print("BrushCmd=");
    Serial.print(pkt.brushCmd);

    Serial.print(" | Dir=");
    Serial.print(dirName[pkt.joyDir]);  // <<< TEXT OUTPUT

    Serial.print(" | X=");
    Serial.print(pkt.joyX);

    Serial.print(" | Y=");
    Serial.println(pkt.joyY);

    // ---------------------------
    // BRUSH MOTOR CONTROL
    // ---------------------------
    if (pkt.brushCmd == 1)
      brushForward();
    else
      brushStop();

    lastReceive = millis();
  }

  // No Signal Warning
  if (millis() - lastReceive > 1000) {
    Serial.println("NO SIGNAL");
    lastReceive = millis();
  }

  delay(30);
}
