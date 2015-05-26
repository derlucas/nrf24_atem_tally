#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define MAX_NODES   6

RF24 radio(7,8);

long ledMillis;
int leds = 0;

const uint64_t listening_pipes[MAX_NODES] = { 0x3A3A3A3AF0LL, 0x3A3A3A3AE1LL, 0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL, 0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL };


void setup() {
  Serial.begin(57600);
  printf_begin();

  radio.begin();
  radio.setAutoAck(1);
  radio.enableAckPayload();
  radio.setRetries(0,5);
  radio.setPayloadSize(1);
}



void loop() {
  long currentMillis = millis();
  uint8_t i;
  uint8_t payload;
  
  for(i = 0; i < MAX_NODES; i++) {
    radio.openWritingPipe(listening_pipes[i]);

    if(!radio.write( &leds, 1)) {
//      printf("write failed\n");
    } else {
      if(!radio.available()) {
//        printf("blank ack received\n");
      } else {
        radio.read(&payload, 1);
//        printf("node %d = %d\n", i, payload);
      }
      
      delay(50);
      
    }             
    radio.stopListening();
  }


  // currently we only let the LEDs blink. Here is the right place for ethernet/atem stuff
  if(currentMillis - ledMillis > 500) {
    leds = leds+1;
    if(leds > 3) leds = 0;    
    ledMillis = currentMillis;
  }
  
  
}
