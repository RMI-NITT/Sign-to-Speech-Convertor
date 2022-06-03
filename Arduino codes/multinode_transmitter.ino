//Multnode transmitter program
// nrf24L01+ interfaced to Arduino

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 9
#define CSN_PIN 10
 
const uint64_t pipe = 0xE8E8F0F0E1LL;
 
RF24 radio(CE_PIN, CSN_PIN);
int data[2]; // depending on the number of sensors you want to use
 
void setup()
{
 Serial.begin(115200);
 radio.begin();
 radio.openWritingPipe(pipe);
}
 
void loop()
{
  data[0] = 500;  // code to identify the transmitter. 
  data[1] = 20; 
  radio.write( data, sizeof(data) );
  Serial.print(data[0]);
  Serial.print(" ");
  Serial.println(data[1]);
  delay(100);
 }
