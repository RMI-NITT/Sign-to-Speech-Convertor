// Program to read and dsiplay IMU sensor data
// Computes Yaw Pitch Roll

// Required header files
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <SoftwareSerial.h>

// MPU object
MPU6050 mpu;

// MPU control/status variables
bool dmpReady = false; // set true if DMP initialisation was successful
uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
uint8_t devStatus; // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount; // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high

// Orientation variables
Quaternion q; // [w, x, y, z] quaternion container
VectorFloat gravity; // [x, y, z] gravity vector
float ypr[3]; // yaw, pitch, roll values - yaw only required
double yaw,pitch,roll;


// function to trigger interrupt when DMP data is ready
void dmpDataReady()
{
  mpuInterrupt = true;
}

// function to input offsets for calibration
void mpu_offset_declare()
{
  mpu.setXGyroOffset(10);
  mpu.setYGyroOffset(-93);
  mpu.setZGyroOffset(-1);
  mpu.setXAccelOffset(-3412);
  mpu.setYAccelOffset(-1447);
  mpu.setZAccelOffset(3467);
}

// setup
void setup()
{
  // Start serial communication
  Serial.begin(115200);
  Serial.println("INITIALISING ...");
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
    attachInterrupt(0, dmpDataReady, RISING); // Attach interrupt for MPU data reading
    mpuIntStatus = mpu.getIntStatus(); // Get status of interrupt
    dmpReady = true; // enable DMP status
    packetSize = mpu.dmpGetFIFOPacketSize(); // read data packet size
  }
  else // failing to intialise MPU
  {
  Serial.print("DMP Initialization failed");
  if(devStatus==1)
    Serial.println("Initial memory load failed");
  else if(devStatus==2)
    Serial.println("DMP configuration update failed");
  }

}

// Loop
void loop()
{
  if (!dmpReady) // if DMP is not ready, exit from program
    return;
  while (!mpuInterrupt && fifoCount < packetSize) // when data is not sent
  {
      
  }
  mpuInterrupt = false; // set interrupt flag as 0
  mpuIntStatus = mpu.getIntStatus(); // get Get interrupt status
  fifoCount = mpu.getFIFOCount(); // Get FIFO buffer size
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) // FIFO overflow gas occured
  {
    mpu.resetFIFO(); //resetting MPU
    Serial.println("FIFO overflow!");
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
    yaw = ypr[0] * 180/M_PI; // Compute yaw from data obtained
    pitch = ypr[1] * 180/M_PI; // Compute pitch from data obtained
    roll = ypr[2] * 180/M_PI; // Compute roll from data obtained
    Serial.print("YPR: "); //debugging
    Serial.print(yaw); //debugging
    Serial.print(" | "); //debugging
    Serial.print(pitch); //debugging
    Serial.print(" | "); //debugging
    Serial.println(roll); //debugging
  }
}
