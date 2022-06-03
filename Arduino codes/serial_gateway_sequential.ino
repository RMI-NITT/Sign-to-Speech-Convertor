// Arduino Code for gateway microcontroller
// ATmega328p configured to use internal 8MHz oscillator
// Exchanges data between SPI (nRF24L01+) and UART (RX-TX spresense)
// Receives and sends packets in pairs, sequential data in order from 2 transmitter nodes

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 8
 
const uint64_t pipe = 0xE8E8F0F0E1LL;
 
RF24 radio(CE_PIN, CSN_PIN);
int data[8];

int glove_id = 0;
int counter =  0;
void setup()
 {
 Serial.begin(115200);
 radio.begin();
 radio.openReadingPipe(1,pipe);
 radio.startListening();
 }
 
void loop()
 {
  if ( radio.available() )
   {
    radio.read(data, sizeof(data));
    if(glove_id == data[0])
    {
      for(int i=0;i<8;i++)
      {
          Serial.print(data[i]);
          Serial.print("/");   
      }
      Serial.println("");
      if(glove_id==1)
        {
          glove_id = 0;
//          counter++;
//          Serial.println(counter);
        }
      else
        glove_id = 1;
    }
  }
}

 
