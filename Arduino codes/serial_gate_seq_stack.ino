// Arduino Code for gateway microcontroller
// ATmega328p configured to use internal 8MHz oscillator
// Exchanges data between SPI (nRF24L01+) and UART (RX-TX spresense)
// Receives and sends packets in pairs, sequential data in order from 2 transmitter nodes
// Uses stacks to push incoming data and pop periodic data into UART buffer

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <StackArray.h>

#define CE_PIN 7  
#define CSN_PIN 8

const uint64_t pipe = 0xE8E8F0F0E1LL;
 
RF24 radio(CE_PIN, CSN_PIN);
int data[8];
int *d1;
int *d2;

StackArray <int> stackA[8];
StackArray <int> stackB[8];

void setup()
{
 Serial.begin(115200);
 radio.begin();
 radio.openReadingPipe(1,pipe);
 radio.startListening();
}

void loop()
 {
  if ( radio.available())
   {
    radio.read(data, sizeof(data));
    if(data[0]==0)
      {
        for(int i=0;i<8;i++)
          stackA[i].push(data[i]);
//        Serial.print(data[0]);
//        Serial.print("/");
//        Serial.print(data[1]);
//        Serial.print("/");
//        Serial.print(data[2]);
//        Serial.print("/");
//        d1 = stackA.pop();
//        Serial.print(d1[0]);
//        Serial.print("/");
//        Serial.print(d1[1]);
//        Serial.print("/");
//        Serial.print(d1[2]);
//        Serial.println("/");
      }
    else if(data[0]==1)
      {
        for(int i=0;i<8;i++)
          stackB[i].push(data[i]);
//        Serial.print(data[0]);
//        Serial.print("/");
//        Serial.print(data[1]);
//        Serial.print("/");
//        Serial.print(data[2]);
//        Serial.print("/");
//        d2 = stackB.pop();
//        Serial.print(d2[0]);
//        Serial.print("/");
//        Serial.print(d2[1]);
//        Serial.print("/");
//        Serial.print(d2[2]);
//        Serial.println("/");
      } 
   }
   if(!(stackA[0].isEmpty()||stackB[0].isEmpty()))
  {
    for(int i=0;i<8;i++)
    {
      Serial.print(stackA[i].pop());
      Serial.print("/");
    }
    Serial.println("");
    for(int i=0;i<8;i++)
    {
      Serial.print(stackB[i].pop());
      Serial.print("/");
    }
    Serial.println("");
    
//    d1 = stackA.pop();
//    for(int i=0;i<8;i++)
//    {
//      Serial.print(d1[i]);
//      Serial.print("/");
//    }
//    Serial.println("");
//    d2 = stackB.pop();
//    for(int j=0;j<8;j++)
//    {
//      Serial.print(d2[j]);
//      Serial.print("/");
//    }

//    Serial.println("");
//    Serial.println("------------------------");
  }
}

 
