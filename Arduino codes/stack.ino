
#include <StackArray.h>

StackArray <int> stackA;
StackArray <int> stackB;

const int a = 10;
const int b= 20;
const int c = 30;
const int d = 40;
int d1;
int d2;
void setup() 
{
  
  stackB.push(b);
  stackB.push(d);
  stackA.push(a);
  stackA.push(c);
  Serial.begin(115200);
}

void loop() 
{
  
  while(!stackA.isEmpty())
  {
    d1 = stackA.pop();
    d2 = stackB.pop();
    Serial.println("STACK A");
    Serial.println(d1);
    Serial.println("STACK B");
    Serial.println(d2);
  }
//  while(!stackB.isEmpty())
//  {
//    Serial.println("STACK B");
//    d2 = stackB.pop();
//    Serial.println(d2);
//  }
  
//  for(int i=0;i<stackB.count();i++)
//  {
//    Serial.println("STACK B");
//    d1 = stackB.pop();
//    for(int j=0;j<3;j++)
//    Serial.println(d1[j]);
//  }
}
