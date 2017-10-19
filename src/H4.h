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
#ifndef H4_H
#define H4_H
/*
		Extends the Arduino/ESP8266 Ticker Library
		Added Functionality:
			* Callback to std::function (as long as all arguments bound)
			* Callback to arbitrary Class methods (using std::bind e.g: bind(&classX::methodX,X_ptr,arg1,arg2... )
			* Callback on random time interval
			* Chain callbacks on one-off _timers: e.g. run a second function on completion of _timer
			  (nesting is allowed, providing facility for complex sequences)
			* All callbacks are serialised into a queue to prevent resource clashes / races / timing errors
	
	N.B.	ALL TIME VALUES ARE IN MILLISECONDS (ms) !!!   1s = 1000ms 1m = 60000ms 1h=3600000ms
	============================================================================================
	
	Historical note / Origin of nomenclature:
	-----------------------------------------
	
	H4 is named after John Harrison's "First Sea Watch", built in the 1700s to claim to "longitude prize"
	offered by the British gornement to the first person to be able to construct a timer that would be accurate
	enough to allow precise calulation of longitude in navigation.
	
	Harrison's work was fundamental to the success of the British Navy and merchant marine, read the fascinating
	history here: https://en.wikipedia.org/wiki/John_Harrison
			
 */
#include "changelog.h"
#include <Arduino.h>
#include <Ticker.h>
#include <deque>
#include <functional>
#include <algorithm>
#include <mutex.h>

using namespace std;

typedef function<void()>  		H4_STD_FN;	// saves a lot of typing

class smartTicker: public Ticker {
    static    uint32_t nextUid;			// Global UID counter
  public:
    uint32_t  	ms=0;					// ticker milliseconds
	uint32_t  	Rmax=0;					// Random Max value (ms doubles as Rmin) - ticker is Random if Rmax > ms
    uint32_t  	rq=0;					// Requeue: 0=free running; any other value = countdown counter
    H4_STD_FN   fn=nullptr;				// primary std::function to call on Tick
    H4_STD_FN   chain=nullptr;			// "onComplete" std::function to call on non free-running expiry
    uint32_t  	uid=0;					// unique ID for each timer created
    
    void smartAttach(uint32_t _ms,H4_STD_FN _fn, uint32_t _rq=0,uint32_t _Rmax=0,H4_STD_FN chain=nullptr,uint32_t uid=0);
	void onlyAttach();
   
    ~smartTicker(){}
};

typedef smartTicker*                    pSTick_t;		// for ticker container (deque)
typedef deque<pSTick_t>::iterator		pDQ_t;			// ptr to contained Ticker

typedef uint32_t						H4_TIMER;

class H4 {
		friend	class	smartTicker;
		
		static	mutex_t       			jqMutex;		// mutual locking between loop() / _queueFn()

		static	deque<pSTick_t>         jobQ;			// main job Queue
		static	deque<pSTick_t>			tickers;		// list of all active Tickers
		
				pDQ_t		_getTicker(uint32_t uid);				
				void 		_killTicker(uint32_t uid);
		static	void 		_queueFn(pSTick_t pt);
				void 		_removeTicker(pDQ_t t);				
				void		_rqTicker(uint32_t uid);
				uint32_t 	_timer(uint32_t msec,H4_STD_FN fn,uint32_t rq=0,uint32_t Rmax=0,H4_STD_FN chain=nullptr,uint32_t uid=0);
		static	void 		_waitMutex(mutex_t*);
	
	public:
		H4(){ CreateMutex(&jqMutex); }
		~H4(){}
		
				H4_TIMER	every(uint32_t msec,H4_STD_FN fn);
				H4_TIMER	everyRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn);
				void 		loop();
				void 		never();
				void 		never(H4_TIMER t);
				H4_TIMER 	nTimes(uint32_t n,uint32_t msec,H4_STD_FN fn,H4_STD_FN chain=nullptr);
				H4_TIMER 	nTimesRandom(uint32_t n,uint32_t msec,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN chain=nullptr);
				H4_TIMER 	once(uint32_t msec,H4_STD_FN fn,H4_STD_FN chain=nullptr);
				H4_TIMER 	onceRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN chain=nullptr);
				void	 	runNow(H4_STD_FN fn);
};

extern H4 h4;
#endif
