#define H4_VERSION "0.9.3"
/*
	Authors:
							PMB	Phil Bowles		<philbowles2012@gmail.com>
	TODO:
				nothing (yet)

	Changelog:
	
		1/1/2018	0.9.3	new functions added:
							when / whenever + new example H4_5_Whenever
							randomTimes / randomTimesRandom + new example H4_6_Mayhem
							some code tidying, new classes H4Countown / H4randomCountdown added
		
		2/12/2017	0.9.2	BUGFIX: nextUID -> static ( else derived classes generate dup uid!!!)
							runNow removed
		
		3/11/2017	0.9.1	advanced example replaced (old one uploaded in error)
							code optimisation: nextUid moved to H4 from smartTicker
							getLoad() function added to show "busy-ness" of system
							runNow deprecated - remove at next release
							queueFunction added to replace runNow

 		19/10/2017 	0.9.0	PMB initial release

*/