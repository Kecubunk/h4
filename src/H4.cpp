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
#include <H4.h>

deque<pSTick_t>               				H4::jobQ;				// The job Queue
deque<pSTick_t>             					H4::tickers;		// the active Ticker list

mutex_t																H4::jqMutex;		// the Queue serialisation lock

H4_TIMER															H4::nextUid=0;
//
//	smartTicker (extends Ticker)
//
void ICACHE_FLASH_ATTR smartTicker::onlyAttach(){
  _attach_ms(Rmax > ms ? random(ms,Rmax):ms,true,reinterpret_cast<callback_with_arg_t>(H4::_queueFn),reinterpret_cast<uint32_t>(this));
}
//
//	smartAttach
//
void ICACHE_FLASH_ATTR smartTicker::smartAttach(uint32_t _ms,H4_STD_FN _fn, H4_WHEN_FN _rq,uint32_t _Rmax,H4_STD_FN _chain,uint32_t _uid){
  fn=move(_fn);
  ms=_ms;
  rq=_rq;
  Rmax=_Rmax;
  chain=move(_chain);
  uid=_uid;
	onlyAttach();
}
//####################################################################################################################
//
//	H4 proper
//
//#####################################################################################################################
H4::H4(){
	CreateMutex(&jqMutex);
	every(1000,bind([](H4* me){
					me->load=me->nextUid-me->prevUid;
					me->prevUid=me->nextUid;
					},this));
}
//
//	_getTicker
//		- utility function, return queue ptr from given uid
//
pDQ_t inline H4::_getTicker(uint32_t uid){
	auto x=find_if(tickers.begin(), tickers.end(),[&](const pSTick_t& tp) {	return uid==tp->uid; 	});
	return x;
}
//
//	_killTicker
//		- internal core of public API never(...
//		- cleanly stop repetetive functions + associated ticker
//
void ICACHE_FLASH_ATTR H4::_killTicker(uint32_t uid){
	pDQ_t	t;
	t=_getTicker(uid);
	if(t!=tickers.end()) _removeTicker(t);
}
//
//	_queueFn
//		-	static single-point-of-call for all Tickers
//		- stuff job in queue
//
void H4::_queueFn(pSTick_t pt){
  _waitMutex(&jqMutex);
  jobQ.push_back(pt);
  ReleaseMutex(&jqMutex);   
}
//
//	_removeTicker
//		- clean jobQ of any not-yet-run instances of this job
//		- get rid and cleanup
//
void ICACHE_FLASH_ATTR H4::_removeTicker(pDQ_t t){
//		int is=jobQ.size();
		uint32_t uid=(*t)->uid;
		jobQ.erase( remove_if(jobQ.begin(), jobQ.end(),[&](pSTick_t pt) {	return uid==pt->uid; }),jobQ.end());
		delete (*t);
		tickers.erase(t);	
}
//
//	_rqTicker
//		- requeue random timer
//		- delete expiring (nTimes, once ) & schedule chain
//
void ICACHE_FLASH_ATTR H4::_rqTicker(uint32_t uid){
	pDQ_t	t;
	t=_getTicker(uid);
	if(t!=tickers.end()){
		pSTick_t pt=(*t);
		if(pt->rq){																			// expiry-type (not free-running) now, once..., nTimes...
			if(!(pt->rq())){															// just counted down to zero
				if(pt->chain) queueFunction(pt->chain);				// so schedule chain if it has one
				_removeTicker(t);															// cleanup expired timer
			}
		}
		else {																						// free running, ie every / everyRandom	
			if(pt->Rmax > pt-> ms) pt->onlyAttach();				// if Random, choose new rand time for next iteration
		}
	}
}
//
//	_timer
//		- create underlying ticker and add to ticker container
//
H4_TIMER ICACHE_FLASH_ATTR H4::_timer(uint32_t msec,H4_STD_FN fn,H4_WHEN_FN rq,uint32_t Rmax,H4_STD_FN chain){
  pSTick_t pt(new smartTicker());
  pt->smartAttach(msec,move(fn),rq,Rmax,move(chain),++nextUid);
	tickers.push_back(pt);
	return pt->uid;
}
//
//	_waitMutex
//
void H4::_waitMutex(mutex_t* mutex){
	while(!GetMutex(mutex)){
		ESP.wdtFeed(); // shouldn't need this: one or two ms should be enough, but just in case...
		delay(0);
  } 
}
//#####################################################################################################################
//
//		public functions API
//
//#####################################################################################################################

//
// every
//		- runs function fn every (msec) milliseconds.
//		- Returns timer UID which can be use to cancel [see never(uid) ]
//
H4_TIMER ICACHE_FLASH_ATTR H4::every(uint32_t msec,H4_STD_FN fn){
  return _timer(msec,fn);
}
//
//	everyRandom
//		- runs function fn every random (Rmin < T < Rmax) milliseconds.
//	  - Returns timer UID which can be use to cancel [see never(uid) ]
//
H4_TIMER ICACHE_FLASH_ATTR H4::everyRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn){
  return _timer(Rmin,fn,nullptr,Rmax);  
}
//
//	loop
//		- consume jobs in queue. Must be called as frequently as possible and not be "starved"
//
void H4::loop(){
		while(!jobQ.empty()){
			_waitMutex(&jqMutex);
      auto job=jobQ.front();
      jobQ.pop_front();
			ReleaseMutex(&jqMutex);
      job->fn();
			_rqTicker(job->uid);
			yield();
    }
}
//
//	never(uid)
//		- cancel a timer, given its UID. NO-OP If not found
//
void ICACHE_FLASH_ATTR H4::never(H4_TIMER uid){
		_killTicker(uid);
}
//
//	never
//		- cancel ALL timers (including the on that started it (if any), hence no subsequent chain is executed)
//		- use with care, never(uid) always preferred. Save UID of any function that may need cancellation
//
void ICACHE_FLASH_ATTR H4::never(){
  while(!tickers.empty()) _killTicker(tickers.front()->uid);
}
//
//	nTimes
//		- runs function fn (n) times, each after delay of msec milliseconds.
//		- allows "onComplete" function (chain) to be run after (n) iterations
//		- nTimes(1,.. is equivalent to once(...
//
H4_TIMER ICACHE_FLASH_ATTR H4::nTimes(uint32_t n,uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(msec,fn,H4Countdown(n),0,onComplete);
}
//
//	nTimesRandom
//		- runs function fn (n) times, each after delay of random (Rmin < T < Rmax) milliseconds.
//		- allows "onComplete" function (chain) to be run at function end
//		- nTimesRandom(1,... is equivalent to onceRandom(...
//
H4_TIMER ICACHE_FLASH_ATTR H4::nTimesRandom(uint32_t n,uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(Rmin,fn,H4Countdown(n),Rmax,onComplete);
}
//
//	once
//		- runs function fn after delay of msec milliseconds.
//		- allows "onComplete" function (chain) to be run at function end
//		- equivalent to nTimes(1...
//
H4_TIMER ICACHE_FLASH_ATTR H4::once(uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(msec,fn,H4Countdown(1),0,onComplete);
}
//
//	onceRandom
//		- runs function fn after delay of random (Rmin < T < Rmax) milliseconds.
//		- allows "onComplete" function (chain) to be run at function end
//		- equivalent to nTimesRandom(1...
//
H4_TIMER ICACHE_FLASH_ATTR H4::onceRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(Rmin,fn,H4Countdown(1),Rmax,onComplete);  
}
//
//	queueFunction - EXACTLY AS PER runNow but with less confusing name
//		- schedules function fn for imminent execution, i.e. no initial delay as in once... functions
//		- should really be called "runSoonish"
//
void ICACHE_FLASH_ATTR H4::queueFunction(H4_STD_FN fn){
  _timer(0,fn,H4Countdown(1));
}
//
//	randomTimes
//		- runs function fn random number of times where tmin < n < tmax, each after delay msec milliseconds.
//		- allows "onComplete" function (chain) to be run at function end
//
H4_TIMER 	ICACHE_FLASH_ATTR H4::randomTimes(uint32_t tmin,uint32_t tmax,uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(msec,fn,H4RandomCountdown(tmin,tmax),0,onComplete);
}
//
//	randomTimesRandom
//		- runs function fn random number of times where tmin < n < tmax, each after random delay of between Rmin and Rmax milliseconds.
//		- allows "onComplete" function (chain) to be run at function end
//
H4_TIMER 	ICACHE_FLASH_ATTR H4::randomTimesRandom(uint32_t tmin,uint32_t tmax,uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete){
  return _timer(Rmin,fn,H4RandomCountdown(tmin,tmax),Rmax,onComplete);
}
//
//	when
//		- function fn will execute when function w returns ZERO
//
void ICACHE_RAM_ATTR H4::when(H4_WHEN_FN w,H4_STD_FN fn){
  _timer(1,[]{},w,0,fn);
}
//
//	whenever
//		- function fn will execute when function w returns ZERO and then reschedule itself
//		- care must be taken to reset w condition else rapid (1msec) loop will occur
//
void ICACHE_RAM_ATTR H4::whenever(H4_WHEN_FN w,H4_STD_FN fn){
  when(w,bind([this](H4_WHEN_FN w,H4_STD_FN fn){
		fn();
		whenever(w,fn);
		},w,fn));
}