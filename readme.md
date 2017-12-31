![H4 Logo](/assets/H4.png)
## ESP8266 Scheduler / timer library for class methods and std::functions
# Introduction

The Ticker class in the Arduino ESP8266 does an excellent job of allowing asynchronous programming, which is essential for writing robust apps / firmware on the ESP8266 platform. However, it is only able to use "normal" funtions as callbacks, i.e. it cannot easily call std::function objects nor class methods. H4 was designed to overcome these limitations and provide a simple interface for calling lambdas, std::functions and normal functions. It also extends the basic Ticker functionality by adding methods to call timers after random intervals or a specific number of times. Perhaps its most important feature is that it serialises all timer events into a queue which is protected by a mutex. This prevents any resource clashes, race conditions and other real-time programming "nasties" and allows it to form the core of a robust event-driven app.

Many thanks to **Richard A Burton** <richardaburton@gmail.com> for his mutex function, without which H4 (and all subsequent firmware based upon it) would not have been possible.
* https://github.com/raburton/esp8266/tree/master/mutex

# Historical note

H4 is named after John Harrison's "First Sea Watch", built in the 1700s to claim to "longitude prize" offered by the British government to the first person to be able to construct a timer that would be accurate enough to allow precise calulation of longitude in navigation.
	
Harrison's work was fundamental to the success of the British Navy and merchant marine, read the fascinating history here: https://en.wikipedia.org/wiki/John_Harrison

# What's new in 0.9.3

* when(H4_WHEN_FN,H4_STD_FN) function
* whenever(H4_WHEN_FN,H4_STD_FN) function

Think "IFTT" or debug "watch" statements. When H4_WHEN_FN returns zero, H4_STD_FN is run. This occurs in "global scope" so it's great for spotting bugs, e.g.:
```cpp
	when([](){ return !(globalvalue==666); },[](){ Serial.println("globalvalue has been corrupted"); }); // single-shot
	whenever([](){ return digitalRead(5); },[](){ Serial.println("PIN 5 LOW!!!"); }); // repetetive
```

NB the H4_WHEN_FN needs to be "short 'n' sweet" (preferably < 1ms) to prevent exponential Q growth leading to crash
Care must be taken with the "whenever" variant to reset the H4_WHEN_FN trigger, or a 1ms rapid loop will occur
Ideally they should be used a) by expert users b) rarely c) carefully and d) for debugging only! There is *always* a better way to do what you THINK you need them for

* randomTimes
* randomTimesRandom

Random version of their "nTimes" equivalents useful for simulation / testing / gaming etc

# Getting Started

## Prerequisites

**H4** requires no additional Arduino libraries to be installed.

Numerous tutorials exists explaing how to intall libraries into your Arduino IDE. Here are a couple of examples:

* https://www.baldengineer.com/installing-arduino-library-from-github.html
* http://skaarhoj.com/wiki/index.php/Steps_to_install_an_Arduino_Library_from_GitHub

# API reference

N.B. You must call the H4 `loop()` function from within the main loop of your program as often as possible

H4_STD_FN is shorthand for `std::function<void(void)>`
H4_WHEN_FN is shorthand for `std::function<uint32_t(void)>`

```c++
	H4_TIMER	every(uint32_t msec,H4_STD_FN fn);
	H4_TIMER	everyRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn);
	uint32_t	getLoad();
	void 		loop();
	void 		never();
	void 		never(H4_TIMER uid);
	H4_TIMER 	nTimes(uint32_t n,uint32_t msec,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	H4_TIMER 	nTimesRandom(uint32_t n,uint32_t msec,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	H4_TIMER 	once(uint32_t msec,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	H4_TIMER 	onceRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	void	 	queueFunction(H4_STD_FN fn);
	H4_TIMER 	randomTimes(uint32_t tmin,uint32_t tmax,uint32_t msec,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	H4_TIMER 	randomTimesRandom(uint32_t tmin,uint32_t tmax,uint32_t msec,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN chain=nullptr);
	void		when(H4_WHEN_FN,H4_STD_FN);
	void		whenever(H4_WHEN_FN,H4_STD_FN);	
```

**every** 		`H4_TIMER every(uint32_t msec,H4_STD_FN fn)`

		- Runs function fn every (msec) milliseconds.
		- Returns timer UID which can be used to subsequently cancel this timer [see never(uid) ]

**everyRandom** `H4_TIMER everyRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn)`

		- Runs function fn every random (Rmin < T < Rmax) milliseconds.
		- Returns timer UID which can be used to subsequently cancel this timer [see never(uid) ]
		
**getLoad** `uint32_t	getLoad()`

		- Returns a value indicating numbers of timers created in last second. Useful for detecting "rogue" processes creating multiple timers too rapidly

**loop** 		`void loop()`

		- Consume jobs in queue.
		- Must be called as frequently as possible and not be "starved".
		- Any calls to delay() from within any of your callback will interfere with the timing of all H4 functions.
		- In short, there is no longer any reason for you to call delay - use a timer/callback!
		
**never** 		`void never(H4_TIMER uid)`

		- Cancel a timer, given its UID. NO-OP If no timer found
		
				`void never()`

		- Cancel ALL timers (including any that call this, hence no subsequent chain is executed)
		- Use with care, never(uid) always preferred. Save UID of any function that may need cancellation
		
**nTimes**		`H4_TIMER nTimes(uint32_t n,uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete)`

		- Runs function fn (n) times, each after delay of msec milliseconds.
		- Allows optional "onComplete" function (chain) to be run after (n) iterations
		- nTimes(1,.. is equivalent to once(...

**nTimesRandom** `H4_TIMER nTimesRandom(uint32_t n,uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete)`

		- Runs function fn (n) times, each after delay of random (Rmin < T < Rmax) milliseconds.
		- Allows optional "onComplete" function (chain) to be run at function end
		- nTimesRandom(1,... is equivalent to onceRandom(...

**once**		`H4_TIMER once(uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete)`

		- Runs function fn after delay of msec milliseconds.
		- Allows optional "onComplete" function (chain) to be run at function end
		- equivalent to nTimes(1...

**onceRandom** 	`H4_TIMER onceRandom(uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete)`

		- Runs function fn after delay of random (Rmin < T < Rmax) milliseconds.
		- Allows optional "onComplete" function (chain) to be run at function end
		- equivalent to nTimesRandom(1...

**queueFunction**		`void queueFunction(H4_STD_FN fn)`

		- Schedules function fn for imminent execution, i.e. no initial delay as in once... functions
		- Use this in any asynchronous callback to "wrap" async functionality into a serialised task. This reduces delays and minimises resource clashes / timing issues

**randomTimes**			`H4_TIMER randomTimes(uint32_t tmin,uint32_t tmax,uint32_t msec,H4_STD_FN fn,H4_STD_FN onComplete)`

		- runs function fn random number of times where tmin < n < tmax, each after delay msec milliseconds.
		- allows "onComplete" function (chain) to be run at function end
		
**randomTimesRandom**	`H4_TIMER randomTimesRandom(uint32_t tmin,uint32_t tmax,uint32_t Rmin,uint32_t Rmax,H4_STD_FN fn,H4_STD_FN onComplete)`

		- runs function fn random number of times where tmin < n < tmax, each after random delay of between Rmin and Rmax milliseconds.
		- allows "onComplete" function (chain) to be run at function end

**when**				`void when(H4_WHEN_FN w,H4_STD_FN fn)`

		- function fn will execute once when function w returns ZERO

**whenever**			`void whenever(H4_WHEN_FN w,H4_STD_FN fn)`

		- function fn will execute while ever function w returns ZERO, thus:
		- care must be taken to reset w condition else rapid (1msec) loop of function fn will occur until w returns NON-ZERO

(C) 2017,2018 **Phil Bowles**
philbowles2012@gmail.com