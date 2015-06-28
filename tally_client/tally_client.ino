//#define DEBUG
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#ifdef DEBUG
  #include "printf.h"
#endif

#define LED_RED   2
#define LED_GREEN 3
#define ADDR_1    4
#define ADDR_2    5
#define ADDR_3    6
#define ADDR_4    7

#define MAX_NODES  6
#define DELAY_SHORT 200
#define DELAY_LONG  400

RF24 radio(9,10);

byte address;
byte blinkState = LOW;
long lastTimeDataReceived;
long blinkMillis;
const uint64_t listening_pipes[6] = { 0x3A3A3A3AF0LL, 0x3A3A3A3AE1LL,
                                      0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL,
                                      0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL };
byte ackValue;

void sendAckBatteryValue() {
  // analog reference is 1.1V / voltage divider consists of 18K + 5K6
  // so analog * 0.045 gives us 43 for 4.3V and 30 for 3.0V
  ackValue = analogRead(A0) * 0.045;
#ifdef DEBUG
  printf("sending ack %d\n", ackValue);
#endif
  radio.writeAckPayload(1, &ackValue, 1);
}

void setup() {
#ifdef DEBUG
  Serial.begin(57600);
  printf_begin();
#endif

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(ADDR_1, INPUT_PULLUP);
  pinMode(ADDR_2, INPUT_PULLUP);
  pinMode(ADDR_3, INPUT_PULLUP);
  pinMode(ADDR_4, INPUT_PULLUP);

  // set reference to 1.1V
  analogReference(INTERNAL);
  
  // read in the address
  if(digitalRead(ADDR_1) == LOW) address = 1;
  if(digitalRead(ADDR_2) == LOW) address |= 2;
  if(digitalRead(ADDR_3) == LOW) address |= 4;
  if(digitalRead(ADDR_4) == LOW) address |= 8;
  
  if(address > MAX_NODES-1) {
    address = MAX_NODES-1;
  }

#ifdef DEBUG
  printf("address: %d\n", address);
#endif  
  
  // display the address as blinks
  digitalWrite(LED_GREEN, HIGH);
  delay(DELAY_LONG);
  
  for(int i = 0; i< address; i++) {
    delay(DELAY_SHORT);
    digitalWrite(LED_RED, HIGH);
    delay(DELAY_SHORT);
    digitalWrite(LED_RED, LOW);
  }
  
  delay(DELAY_SHORT);
  digitalWrite(LED_GREEN, LOW);
  delay(DELAY_LONG);
  
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
#ifdef DEBUG
    printf("got data %u\n",receivedByte);
#endif

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


