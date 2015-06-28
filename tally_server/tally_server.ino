#define USEATEM
//#define DEBUG
#include <SPI.h>
#ifdef DEBUG
  #include "printf.h"
#endif
#include "nRF24L01.h"
#include "RF24.h"
#include <Ethernet.h>
#ifdef USEATEM
  #include <ATEMbase.h>
  #include <ATEMstd.h>
#endif

#define MAX_NODES   6
#define STATUS_LED  2

long ledMillis;
byte blinkState = LOW;

byte leds[MAX_NODES] = { 0, 0, 0, 0, 0, 0 };
byte voltages[MAX_NODES] = { 0, 0, 0, 0, 0, 0 };
const uint64_t listening_pipes[MAX_NODES] = { 0x3A3A3A3AF0LL, 0x3A3A3A3AE1LL, 
                                              0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL,
                                              0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL };
byte mac[] = { 0x90, 0xA2, 0xDB, 0x1D, 0x6B, 0xB4 };
IPAddress clientIp(192, 168, 10, 99);
EthernetServer server(80);
#ifdef USEATEM
  ATEMstd AtemSwitcher;
  bool AtemOnline = false;
#endif
RF24 radio(7,8);

void setup() {
  
  pinMode(STATUS_LED, OUTPUT);
#ifdef DEBUG
  Serial.begin(57600);
  printf_begin();
#endif

  radio.begin();
  radio.setAutoAck(1);
  radio.enableAckPayload();
  radio.setRetries(0,10);
  radio.setPayloadSize(1);

  Ethernet.begin(mac, clientIp);

#ifdef USEATEM
  // Initialize a connection to the switcher:
  AtemSwitcher.begin(IPAddress(192, 168, 10, 240), 56417);
#ifdef DEBUG
  AtemSwitcher.serialOutput(0x80);
#endif
  AtemSwitcher.connect();
#endif

  for(int i = 0; i< 3; i++) {
    delay(200);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(STATUS_LED, LOW);
  }
}


void loop() {
 
  long currentMillis = millis();
  
  // simulate data
  if(currentMillis - ledMillis > 5000) {
    ledMillis = currentMillis; 
    blinkState++;
    leds[0] = blinkState;
    leds[1] = blinkState;
    leds[2] = blinkState;
    leds[3] = blinkState;
    leds[4] = blinkState;
    leds[5] = blinkState;
    if(blinkState == 3) blinkState = 0;
  }


#ifdef USEATEM
  loopAtem();
#endif

  loopNRF();
  
  loopHttp();
  
}

void loopHttp() {
  
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html");
          client.println("Connection: close\r\n");
//          client.println();
          client.println("<!DOCTYPE HTML><html>");
          // output the value of each nrf-client voltage
          for (byte i=0; i < MAX_NODES; i++) {
            client.print("node ");
            client.print(i);
            client.print(": ");
            client.print(voltages[i]/10);
            client.print(",");
            client.print(voltages[i]%10);
            client.println("V<br />");
          }
#ifdef USEATEM
          client.print("<br/>Atem connection: ");
          client.println(AtemOnline ? "Online":"Offline");
#endif
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}

void loopNRF() {
  byte i, payload;
  
  for(i = 0; i < MAX_NODES; i++) {
    radio.openWritingPipe(listening_pipes[i]);

    if(!radio.write( &leds[i], 1)) {
#ifdef DEBUG
      printf("write failed\n");
#endif     
    } else {
      if(radio.available()) {
        radio.read(&payload, 1);
#ifdef DEBUG
        printf("node %d = %d\n", i, payload);
#endif
        voltages[i] = payload;
      }
    }             
    radio.stopListening();
  }
}

#ifdef USEATEM
void loopAtem() {
  
  byte i;
   
  AtemSwitcher.runLoop();
  
  if (AtemSwitcher.hasInitialized())  {
    if (!AtemOnline)  {
      AtemOnline = true;
          
      // turn on the green status led to indicate a connection
      digitalWrite(STATUS_LED, HIGH);      
    }

    for(i = 0; i < MAX_NODES; i++) {    
      leds[i] = AtemSwitcher.getTallyByIndexTallyFlags(i) & 0x03;
      
      if(bitRead(leds[i], 0)) {
        leds[i] &= 0x01;
      }
      
    }
    
  } else {
    // at this point the ATEM is not connected and initialized anymore

    if (AtemOnline) {
      AtemOnline = false;
        
      // turn off the green connection idicator
      digitalWrite(STATUS_LED, LOW);
    }     
  }
}
#endif
