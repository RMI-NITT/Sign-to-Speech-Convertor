// AVR code to read voltage levels using ADC and transmit through UART
// Atmega328p
// ADC input - ADC5 - PC5
// Reading and transmitting 5 ADC values

#include<avr/io.h>
#include<stdlib.h>

#define F_CPU 16000000
#define BAUD 9600
#define BAUDRATE (((F_CPU / (BAUD * 16UL))) - 1)  // Baud rate of 9600 programmed.

int i=0, adc=0;
float v_value=0;
char str[4] = "ADC:";
char adc_str[5];

void uart_tx(char* data)         // Function to transmit string
{
  for(i=0; data[i]!=0; i++)
  {
    while (!( UCSR0A & (1<<UDRE0)));  // Wait till buffer is empty
    UDR0 = data[i];                 // Load character to be transmitted
  }
  while (!( UCSR0A & (1<<UDRE0)));  // Wait till buffer is empty
  //UDR0 = '\n';                 // Load character to be transmitted
}

int ADC_convert()
{
  ADCSRA|=(1<<ADSC);
  while(ADCSRA&(1<<ADSC));
  return ADC;
}

int main()
{
  // Setting up ADC
  ADMUX=(1<<REFS0);               // Internal Vref, ADC1
  ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);  // Prescaler 128

  // Setting up UART
  UBRR0L = (BAUDRATE);     // Set the required baud rate, upper register
  UBRR0H = (BAUDRATE>>8);  // Set the required baud rate, lower register
  UCSR0B|= (1<<TXEN0);     // Enable TRANSMISSION
  uart_tx(str);
  while(1)
  {
    ADMUX=(1<<REFS0);
    adc = ADC_convert();
    itoa(adc,adc_str,10);
    uart_tx(adc_str);

    uart_tx(" | ");
    ADMUX=(1<<REFS0)|(1<<MUX0);
    adc = ADC_convert();
    itoa(adc,adc_str,10);
    uart_tx(adc_str);
    
    uart_tx(" | ");
    ADMUX=(1<<REFS0)|(1<<MUX1);
    adc = ADC_convert();
    itoa(adc,adc_str,10);
    uart_tx(adc_str);

    uart_tx(" | ");
    ADMUX=(1<<REFS0)|(1<<MUX0)|(1<<MUX1);
    adc = ADC_convert();
    itoa(adc,adc_str,10);
    uart_tx(adc_str);

    uart_tx(" | ");
    ADMUX=(1<<REFS0)|(1<<MUX2)|(1<<MUX1);
    adc = ADC_convert();
    itoa(adc,adc_str,10);
    uart_tx(adc_str);

    uart_tx("\n");
  }
  
  return 0;
}
