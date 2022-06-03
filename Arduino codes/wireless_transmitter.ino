// Basic two point transmitter code
// nrf24L01 interfaced to Arduino

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9,10); // CE, CSN

const byte address[6] = "00001";
const char text[] = "Hello World";

void setup() 
{
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void loop() 
{
  radio.write(&text, sizeof(text));
  delay(1000);
}
