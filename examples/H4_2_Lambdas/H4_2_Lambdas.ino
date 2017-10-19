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
 *      - Arbitrary Class methods
 *      - Lambda functions
 *      - C++ Standard library std::function objects 
 *      
 *      NB all std::function calls must resolve to <std:function<void()>> which they wiil
 *      if std::bind is used to bind all parameters at call time. a helper macro is 
 *      defined H4_STD_FN as <std:function<void()>> to assist
 *    
 */
#include <H4.h>
#include<functional>                                                   // don't omit this!

H4  h4;

class simpleClass{
  public:
    void print(const char* x){
      Serial.printf("simpleClass: %s\n",x);  
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

  h4.runNow(bind(&simpleClass::print,sC,"Yes, it CAN be done!"));       // will run once with no delay as soon as youe exit setup
  h4.everyRandom(10000,30000,
    bind(&simpleClass::print,sC,"Yes, it CAN be done!")                 // and (rather annoyingly) again every 10 - 30 seconds
    ); 

/*  
  could also have done:

  H4_STD_FN fn=bind(&simpleClass::print,sC,"Yes, it CAN be done!");
  h4.runNow(fn);
  h4.everyRandom(10000,30000,fn);
*/

  simple=h4.every(1000,bind(simpleFunction,&counter));                  // run simpleFunction every second and hold onto its "UID" so we can cancel it later
  h4.onceRandom(10000,15000,[](){                                       // after between 10 and 15 seconds, cancel the simpleFunction
    Serial.printf("T=%d Cancelling simple function\n",millis());        // by running this lambda
    h4.never(simple);                                                   // ...a lot like life....
    });                              
}
/*
  h4.loop must be called as often as possible, all your timings will be "out" if you don't
  do not do anything in loop that causes any long delays or waits for external resources.
  use Async libraries, register a callback and use "runNow" in the callback.
  If you do this, you will NEVER need to call delay() - which is a GOOD THING

*/

void loop() {
  h4.loop();                                                           // you MUST call this as often as possible
  if(!(millis()%5000)) {
    Serial.printf("T=%d doing some stuff in main loop\n",millis());
  }
}
