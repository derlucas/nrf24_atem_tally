#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define LED_RED   2
#define LED_GREEN 3
#define ADDR_1    4
#define ADDR_2    5
#define ADDR_3    6

#define MAX_NODES  6

RF24 radio(7,8);

int address;
int blinkState = LOW;
long lastTimeDataReceived;
long blinkMillis;
const uint64_t listening_pipes[6] = { 0x3A3A3A3AF0LL, 0x3A3A3A3AE1LL, 0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL, 0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL };

void sendAckBatteryValue() {
  uint8_t ackByte = (uint8_t)(analogRead(A0) >> 2);
//  printf("sending ack %d\n", ackByte);
  radio.writeAckPayload(1, &ackByte, 1);
}

void setup() {
  Serial.begin(57600);
  printf_begin();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(ADDR_1, INPUT_PULLUP);
  pinMode(ADDR_2, INPUT_PULLUP);
  pinMode(ADDR_3, INPUT_PULLUP);

  address = (digitalRead(ADDR_3) << 2) | (digitalRead(ADDR_2) << 1) | (digitalRead(ADDR_1) << 0);

  if(address > MAX_NODES-1) {
    address = MAX_NODES-1;
  }

  printf("address: %d\n", address);

  radio.begin();
  radio.setAutoAck(1);
  radio.enableAckPayload();
  radio.setRetries(0,10);
  radio.setPayloadSize(1);  
  radio.openReadingPipe(1,listening_pipes[address]);
  radio.startListening();
  sendAckBatteryValue();
}

void loop() {
  
  long currentMillis = millis();
  
  while(radio.available()) {
    uint8_t receivedByte;
    radio.read( &receivedByte, sizeof(uint8_t) );
//    printf("got data %u\n",receivedByte);

    sendAckBatteryValue();

    if(bitRead(receivedByte, 0)) {
      digitalWrite(LED_RED, HIGH);
    } else {
      digitalWrite(LED_RED, LOW);
    }
    if(bitRead(receivedByte, 1)) {
      digitalWrite(LED_GREEN, HIGH);
    } else {
      digitalWrite(LED_GREEN, LOW);
    }
  }
  
  if(!radio.available()) {
    
    // we have no data, so check for timeout
    if(currentMillis - lastTimeDataReceived > 5000) {
      
      if(currentMillis - blinkMillis > 100) {
        if(blinkState == LOW) {
          blinkState = HIGH;
        } else {
          blinkState = LOW;
        }
        digitalWrite(LED_RED, blinkState);
        digitalWrite(LED_GREEN, blinkState);
        blinkMillis = currentMillis;
      }    
    }
   
  } else {
    lastTimeDataReceived = currentMillis;
  }
  
}


