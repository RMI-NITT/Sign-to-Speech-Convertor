/****************************************************************************
 * examples/hello/hello_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <stdio.h>
#define TTYS_NAME "/dev/ttyS2"
#include <fcntl.h>

// #include <sdk/config.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

 struct gloveData
 {
   int roll;
   int pitch;
   int flex1;
   int flex2;
   int flex3;
   int flex4;
   int flex5;
 } leftGlove , rightGlove;
/*
 char* my_strcat(char* destination,  char source)
{
    // make `ptr` point to the end of the destination string
    char* ptr = destination + strlen(destination);
 
    // appends characters of the source to the destination string
    if (source != '\0') {
        *ptr++ = source;
    }
 
    // null terminate destination string
    *ptr = '\0';
 
    // the destination is returned by standard `strcat()`
    return destination;
}*/

 void getString(void)
 {
  int fd;                           //file locator
  char str[4];                      //read stream
  char bigstr[50] = ""; //store string
  int str_count = 0;     
  int retval;           //count of bigstr
  printf("Loopp\n");

  fd = open(TTYS_NAME, O_RDWR );

  while ( ( retval = read ( fd, &str, sizeof(str))) > 0) 
  {//read until end of file
  
  if(str[0] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[0];

  if(str[1] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[1];
  

  if(str[2] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[2];

  if(str[3] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[3];
  
 
  strcpy(str,"");  
  }

  bigstr[str_count++] = '\0';

  int i = 0;
  int j = 0;
  int k = 0;
  
  printf("LoopEnd\n");
  close(fd);
  char ab[10] = "";
  int arr[9];
  
  while(i<strlen(bigstr) & j < 8)
  {
      if(bigstr[i] == '/')
      {
          arr[j++] = atoi(ab);
          memset(ab, '\0', sizeof(ab));
          k=0;
      }
      else
      ab[k++] = bigstr[i];
       i++;
      
     
  }

  
  for(i=0;i<8;i++)
  printf("%d\n" , arr[i]);

  if(!arr[0])
  {
    leftGlove.roll = arr[1];
    leftGlove.pitch = arr[2];
    leftGlove.flex1 = arr[3];
    leftGlove.flex2 = arr[4];
    leftGlove.flex3 = arr[5];
    leftGlove.flex4 = arr[6];
    leftGlove.flex5 = arr[7];
  }

  else
  {

    rightGlove.roll = arr[1];
    rightGlove.pitch = arr[2];
    rightGlove.flex1 = arr[3];
    rightGlove.flex2 = arr[4];
    rightGlove.flex3 = arr[5];
    rightGlove.flex4 = arr[6];
    rightGlove.flex5 = arr[7];

  }
  
  
  


 }
 static int gpio_handler(int irq, FAR void *context, FAR void *arg)
{
  printf("\nINTERRUPT\n");
 board_gpio_write(PIN_PWM2, 1);
 return 1;
}


int main(int argc, FAR char *argv[])
{
  /*
  int fd;
  int retval;
  int count;
  char s[50] = "";
  char *str = s;
  
  printf("Hello0main\n");
while (true)
{
  getString();
  sleep(0.2);
 //printf("\n%d hey",leftGlove.flex5);
}
*/
board_gpio_write(PIN_PWM0, -1);

// ​board_gpio_config(PIN_SPI2_MISO, 0, true, false, PIN_FLOAT); //interrupt
// board_gpio_config(PIN_XXX, 0, true, false, PIN_DOWN);  //reset
// board_gpio_config(PIN_XXX, 0, false, true,  PIN_FLOAT);

//interrupt
board_gpio_config(PIN_PWM0,0,true,false, PIN_PULLUP);
board_gpio_intconfig(PIN_PWM0, INT_FALLING_EDGE, true, gpio_handler);
board_gpio_int(PIN_PWM0, true);

board_gpio_write(PIN_PWM1, -1);
board_gpio_config(PIN_PWM1, 0, 0, true, PIN_PULLDOWN);
board_gpio_write(PIN_PWM1, 0);

 while (true)
 {
   
 }
 
 
  printf("Hello0main\n");
  
  return 0;
}



