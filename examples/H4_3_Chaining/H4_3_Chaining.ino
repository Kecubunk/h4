/*
 MIT License

Copyright (c) 2017 Phil Bowles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 *    Demonstrates usage of the H4 timer library with: 
 *      - "chaining" of functions
 */
#include <H4.h>

H4  h4;

class simpleClass{
  public:
    void print(const char* x){
      Serial.printf("T=%d simpleClass: %s\n",millis(),x);  
    }
};

simpleClass sC;

void simpleFunction(uint32_t* x){
  Serial.printf("T=%d It's easy as %d\n",millis(),(*x)++);
}

void toggleBuiltin(){
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}

void setup() {
  static uint32_t simple;
  static uint32_t counter;
  
  Serial.begin(74880);
  pinMode(LED_BUILTIN,OUTPUT);
  h4.every(250,toggleBuiltin);                                          // flash LED rapidly on/off every 250 ms (4x per second)

  H4_STD_FN  jackson5=[](){
    Serial.print("The Jackson 5 sang: ");
    h4.nTimes(3,500,[]()
      {
      static char c=0x41;
      Serial.printf("%c ",c++);       // run 3 times...A B C
      },
      []()                            // and then... chain function
        {
        Serial.print("\nIt's easy as: ");
        h4.nTimes(3,500,[](){
          static int n=1;
          Serial.printf("%d ",n++);     // 1 2 3
          },                          // and then... chain function
          []()
            {
            Serial.print("\n...That's how easy it can be!\n");
            h4.once(5000,[](){ Serial.println("have a listen to the real thing: https://www.youtube.com/watch?v=ho7796-au8U"); });
            } // end 123 chain function
        ); // end 123 function
        } // end abc chain function
      ); // end "ABC" function
  }; // end fn declaration
//  sing it...some time in the future
  h4.onceRandom(5000,10000,jackson5);
}

void loop() {
  h4.loop();                                                           // you MUST call this as often as possible
  if(!(millis()%5000)) {
    Serial.printf("T=%d doing some stuff in main loop\n",millis());
  }
}
