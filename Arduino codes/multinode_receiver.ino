// Multinode communication program
// nrf24L01 module interfaced to Arduino board

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 8
 
 const uint64_t pipe = 0xE8E8F0F0E1LL;
 
RF24 radio(CE_PIN, CSN_PIN);
int data[2];

void setup()
 {
 Serial.begin(115200);
 radio.begin();
 radio.openReadingPipe(1,pipe);
 radio.startListening();
 Serial.println("Nrf24L01 Receiver Starting");
 }
 
void loop()
 {
  if ( radio.available() )
   {
    radio.read(data, sizeof(data));
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.println(data[1]);
    delay(100);
  }
}

 
