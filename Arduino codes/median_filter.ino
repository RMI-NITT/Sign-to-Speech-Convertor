// Program to read ADC value and smoothen data
// Implements moving window median filter for smoothening

#define SensorPin0      0
#define SensorPin1      1
#define SensorPin2      2
#define SensorPin3      3
#define SensorPin4      6

#define filterSamples   81              
int sensSmoothArray1 [filterSamples];   
int sensSmoothArray2 [filterSamples];   

int rawData1, smoothData1;  
int rawData2, smoothData2;  

void setup(){
  Serial.begin(115200);
}

void loop(){       
  rawData1 = analogRead(SensorPin1);                        
  smoothData1 = digitalSmooth(rawData1, sensSmoothArray1);  

  Serial.println(smoothData1);
}

int digitalSmooth(int rawIn, int *sensSmoothArray){     
  int j, k, temp, top, bottom;
  long total;
  static int i;
 // static int raw[filterSamples];
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    
  sensSmoothArray[i] = rawIn;                 

  // Serial.print("raw = ");

  for (j=0; j<filterSamples; j++){     
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
