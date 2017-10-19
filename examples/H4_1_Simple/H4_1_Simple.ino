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
 *    Demonstrates usage of the H4 timer libarary for common functions
 *    By "common" we mean neither a Lambda function nor a C++ Standard library std::function object
 *    
 *    While H4 can be used to good effect with "common" functions, its flexibility only really becomes
 *    apparent when Lambda functions and std::function objects are used. You are strongly urged to
 *    learn these features of C++ to get the best of out H4
 *    
 *    This example flashes the built-in LED "in the background" continuously  while simulating other  
 *    work in the main loop.
 *    
 *    It also creates a simple function (whose output will be interleaved with the main loop's output)
 *    and also sets up a one-time future randomly timed task to cancel the simple function
 *    
 */
#include <H4.h>

H4  h4;

uint32_t simple;                                                       // we need this global variable because later we are going to cancel the simple task
/*
    NB this is NOT ideal programming "style" - ideally a Lambda function would be a lot cleaner (and simpler to understand)
    This better method is shown in a later example
     
*/
void simpleFunction(){
  Serial.printf("T=%d It's easy\n",millis());
}

void toggleBuiltin(){
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}

void cancelSimple(){
  Serial.printf("T=%d Cancelling simple function\n",millis());
  h4.never(simple);                                                     // ...a lot like life....
}
void setup() {
  Serial.begin(74880);
  pinMode(LED_BUILTIN,OUTPUT);
  h4.every(250,toggleBuiltin);                                          // flash LED rapidly on/off every 250 ms (4x per second)
  simple=h4.every(1000,simpleFunction);                                 // run simpleFunction every second and hold onto its "UID" so we can cancel it later
  h4.onceRandom(10000,15000,cancelSimple);                              // after between 10 and 15 seconds, cancel the simpleFunction
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
