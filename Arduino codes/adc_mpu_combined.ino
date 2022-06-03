// 5 x ADC + MPU YPR data acquitition
// Data stored in data variable separated using /
// Uses UART Serial to trasnmit data read and computed

// Implements Moving window median filter for ADC value smoothening

// Required header files
#include<avr/io.h>
#include<stdlib.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include<String.h>

int adc_val[5];
char adc_str[5][10];

#define filterSamples   81              
int sensSmoothArray [5][filterSamples];    
  
int adc_smooth[5]; 

// MPU object
MPU6050 mpu;

// MPU control/status variables
bool dmpReady = false; // set true if DMP initialisation was successful
uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
uint8_t devStatus; // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount; // count of all bytes currently in FIFO
uint8_t fifoBuffer[128]; // FIFO storage buffer
volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high

// Orientation variables
Quaternion q; // [w, x, y, z] quaternion container
VectorFloat gravity; // [x, y, z] gravity vector
float ypr[3]; // yaw, pitch, roll values - yaw only required
int ypr_val[3];
char ypr_str[3][10];

char data[30];

int j, k, temp, top, bottom;
long total;
int w;
// static int raw[filterSamples];
static int sorted[filterSamples];
boolean done;

// Function for ADC conversion
int ADC_convert()
{
  ADCSRA|=(1<<ADSC);
  while(ADCSRA&(1<<ADSC));
  return ADC;
}

// function to trigger interrupt when DMP data is ready
void dmpDataReady()
{
  mpuInterrupt = true;
}

// function to input offsets for calibration
void mpu_offset_declare()
{
  mpu.setXGyroOffset(8);
  mpu.setYGyroOffset(-96);
  mpu.setZGyroOffset(-1);
  mpu.setXAccelOffset(-3384);
  mpu.setYAccelOffset( -1333);
  mpu.setZAccelOffset(3700);
}

// 5 ADC value computation
void adc_conversion()
{
    ADMUX=(1<<REFS0);
    adc_val[0] = ADC_convert();

    ADMUX=(1<<REFS0)|(1<<MUX0);
    adc_val[1] = ADC_convert();
    
    ADMUX=(1<<REFS0)|(1<<MUX1);
    adc_val[2] = ADC_convert();

    ADMUX=(1<<REFS0)|(1<<MUX0)|(1<<MUX1);
    adc_val[3] = ADC_convert();

    ADMUX=(1<<REFS0)|(1<<MUX2)|(1<<MUX1);
    adc_val[4] = ADC_convert();

      
}

int digitalSmooth(int rawIn, int *sensSmoothArray)
{     


  w = (w + 1) % filterSamples;    
  sensSmoothArray[w] = rawIn;                 

  // Serial.print("raw = ");

  for (j=0; j<filterSamples; j++)
  {     
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;                           
  while(done != 1){       
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++){
      if (sorted[j] > sorted[j + 1]){     
        temp = sorted[j + 1];
        sorted [j+1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }
 
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1)); 
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  
    k++; 
  }
  return total / k;    
}

//setup
void setup()
{
  Serial.begin(9600);
//  Serial.println("Initialising . . .");
    
  // Setting up ADC
//  ADMUX=(1<<REFS0);               // Internal Vref, ADC1
//  ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);  // Prescaler 128
  
  // Start I2C communication
  Wire.begin();
  // Initialise MPU
  mpu.initialize();
  // Get status of MPU
  devStatus = mpu.dmpInitialize();
  // Feed the offset values for calibration of MPU
  mpu_offset_declare();

  if (devStatus == 0) // If MPU has been initialised
  {
    mpu.setDMPEnabled(true); // Initalise DMP
//    attachInterrupt(0, dmpDataReady, RISING); // Attach interrupt for MPU data reading
    mpuIntStatus = mpu.getIntStatus(); // Get status of interrupt
    dmpReady = true; // enable DMP status
    packetSize = mpu.dmpGetFIFOPacketSize(); // read data packet size
  }
  else // failing to intialise MPU
  {
//    Serial.print("DMP Initialization failed");
//    if(devStatus==1)
//      Serial.print("Initial memory load failed");
//    else if(devStatus==2)
//      Serial.print("DMP configuration update failed");
  }

//  Serial.println("Initialised . . ."); 
}

// loop
void loop()
{
  mpu.resetFIFO();
  if (!dmpReady) // if DMP is not ready, exit from program
    return;
    
    adc_conversion();
    
    for(int i=0;i<5;i++)
    {
      adc_smooth[i] = digitalSmooth(adc_val[i], sensSmoothArray[i]);
    }
 
    strcpy(data,"\0");
    strcat(data,"0/");
    for(int i=0;i<3;i++)
    {
      itoa(ypr_val[i],ypr_str[i],10);
      strcat(data,ypr_str[i]);
      strcat(data,"/");
    }
    for(int i=0;i<5;i++)
    {
      itoa(adc_smooth[i],adc_str[i],10);
      strcat(data,adc_str[i]);
      strcat(data,"/");
    }
    Serial.println(data);

  mpuIntStatus = mpu.getIntStatus(); // get Get interrupt status
  fifoCount = mpu.getFIFOCount(); // Get FIFO buffer size
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) // FIFO overflow gas occured
  {
    mpu.resetFIFO(); //resetting MPU
//    Serial.print("FIFO overflow!");
  }
  else if (mpuIntStatus & 0x02) // data is sent from MPU
  {
    while (fifoCount < packetSize) // wait till FIFO buffer is complete
      fifoCount = mpu.getFIFOCount();
    mpu.getFIFOBytes(fifoBuffer, packetSize); // get the data from MPU
    fifoCount -= packetSize; // check if multiple data packets exist
    mpu.dmpGetQuaternion(&q, fifoBuffer); // compute quaternion from FIFO raw values
    mpu.dmpGetGravity(&gravity, &q); // Calculate gravity from quaternion values
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity); // compute YPR from gravity and quaternion
    ypr_val[0] = int(ypr[0] * 180/M_PI) + 180; // Compute yaw from data obtained
    ypr_val[1] = int(ypr[1] * 180/M_PI); + 180;// Compute pitch from data obtained
    ypr_val[2] = int(ypr[2] * 180/M_PI); + 180;// Compute roll from data obtained
  }
}
