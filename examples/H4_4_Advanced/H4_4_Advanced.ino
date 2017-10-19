#include <Ticker.h>
#include <memory>
#include <deque>
#include <functional>
#include <algorithm>
#include "mutex.h"

char* genres[]={
  "disco",
  "punk",
  "jazz",
  "folk",
  "blues",
  "hip-hop",
  "house",
  "garage",
  "indie"
};

using namespace std;
typedef function<void()>  stdFn;

mutex_t       tqMutex;
    
class smartTicker: public Ticker {    // debug only - remove for production + global replace SmartTicker with Ticker
    static    uint32_t nextUid;
  public:
    uint32_t  ms,Rmax;
    uint32_t  rq=0;
    stdFn     fn;
    stdFn     chain=nullptr;
    uint32_t  uid;
    
    void smartOnce(uint32_t _ms,stdFn _fn, uint32_t _rq=0,uint32_t _Rmax=0,stdFn chain=nullptr,uint32_t uid=0);
    
    ~smartTicker(){
//      Serial.printf("Ticker %08x DTOR\n",this);
    }
};

uint32_t  smartTicker::nextUid=1;

typedef smartTicker*                      pSTick_t;
typedef unique_ptr<smartTicker>           upSTick_t;

deque<pSTick_t>               jobQ;
deque<pSTick_t>                           tickers;

void queueFn(pSTick_t pt){
  waitMutex();
//  Serial.printf("qfn: pt=%08x fn=%08x\n",pt,&pt->fn);
//  pt->fn();
//  Serial.println("still valid");
  jobQ.push_back(pt);
  ReleaseMutex(&tqMutex);   
}

void smartTicker::smartOnce(uint32_t _ms,stdFn _fn, uint32_t _rq,uint32_t _Rmax,stdFn _chain,uint32_t _uid){
//  Serial.printf("smartTicker: ms=%d @fn=%08x rq=%d Rmax=%d\n",_ms,&_fn,_rq,_Rmax);
//  _fn();
//  Serial.printf("SO still valid\n");
  fn=move(_fn);
  ms=_ms;
  rq=_rq;
  Rmax=_Rmax;
  chain=_chain;
  uid=_uid ? _uid:nextUid++;
  _attach_ms(Rmax > ms ? random(ms,Rmax):ms,false,reinterpret_cast<callback_with_arg_t>(queueFn),reinterpret_cast<uint32_t>(this));
//  Serial.printf("smartTicker: ms=%d @fn=%08x rq=%d Rmax=%d\n",_ms,&_fn,_rq,_Rmax);
}   

void waitMutex(){
  while(!GetMutex(&tqMutex)){
    ESP.wdtFeed(); // shouldn't need this: one or two ms should be enough, but just in case...
    delay(0);
  } 
}

uint32_t timer(uint32_t msec,stdFn fn,uint32_t rq=0,uint32_t Rmax=0,stdFn chain=nullptr,uint32_t uid=0){
  pSTick_t pt=new smartTicker();
  pt->smartOnce(msec,move(fn),rq,Rmax,chain,uid);
//  Serial.printf("timer: %d pt=%08x uid=%d\n",msec,pt,pt->uid);
  tickers.push_front(pt);
  return pt->uid;
}

void _killTicker(uint32_t uid,bool again=false){
//  Serial.printf("killTicker uid=%08x again=%d\n",uid,again);
  waitMutex();
  auto x=std::find_if(tickers.begin(), tickers.end(),[&](const pSTick_t& tp) {
//    Serial.printf("KT: tick=%08x uid=%d\n",tp,tp->uid);
      return uid==tp->uid;
    });
  if(x!=tickers.end()){
//    Serial.printf("KT: found ticker %08x %d\n",(*x),(*x)->uid);
//
    if(again){
      pSTick_t pt=(*x);
      uint32_t ms=pt->ms;
      uint32_t rq=pt->rq;
      uint32_t Rmax=pt->Rmax;
      uint32_t uid=pt->uid;
      stdFn    chain=move(pt->chain);
//      Serial.printf("RQ: requeue %08x %d\n",(*x),(*x)->uid);
      if((!rq) || --rq) timer(ms,move(pt->fn),rq,Rmax,move(chain),uid);
      else if(chain) chain();
    }
//
//    Serial.printf("KT: about to delete %08x %d\n",(*x),(*x)->uid);
    delete (*x);
//   Serial.printf("KT: b4 qcount=%d\n",tickers.size());
   tickers.erase(x);
//   Serial.printf("KT: b5 qcount=%d\n",tickers.size());

  }
  else{
//    Serial.println("WOT, KT NO TICKER? ????????????");
  }    
  ReleaseMutex(&tqMutex);  
}

void never(uint32_t uid){
//  Serial.printf("NEVER uid=%08x\n",uid);
  _killTicker(uid);
}
/*
void cleanJob(pSTick_t pt){
//  Serial.printf("cleanJob %08x\n",pt);
}
*/
void runJobs(){
  if(GetMutex(&tqMutex)){
    auto temp=move(jobQ);
    ReleaseMutex(&tqMutex);
    while(!temp.empty()){
      auto job=(temp.front());
      job->fn();
      _killTicker(job->uid,true);
      temp.pop_front();
    }
  }
}
//
void never(){
//  Serial.println("NEVER()");
  waitMutex();
  auto temp=tickers;
  ReleaseMutex(&tqMutex);
  while(!temp.empty()){
    _killTicker(temp.front()->uid);
    temp.pop_front();
  }
//  Serial.println("Should be clean now");
}

void once(uint32_t msec,stdFn fn,stdFn chain=nullptr){
  timer(msec,fn,1,0,chain);
}
uint32_t every(uint32_t msec,stdFn fn){
  return timer(msec,fn);
}
void nTimes(uint32_t n,uint32_t msec,stdFn fn,stdFn chain=nullptr){
  timer(msec,fn,n,0,chain);
}
void nTimesRandom(uint32_t n,uint32_t msec,uint32_t Rmax,stdFn fn,stdFn chain=nullptr){
  timer(msec,fn,n,Rmax,chain);
}
void onceRandom(uint32_t Rmin,uint32_t Rmax,stdFn fn,stdFn chain=nullptr){
  timer(Rmin,fn,1,Rmax,chain);  
}
uint32_t everyRandom(uint32_t Rmin,uint32_t Rmax,stdFn fn){
  return timer(Rmin,fn,0,Rmax);  
}
void runNow(stdFn fn){
  timer(0,fn,1);
}
//
void togglebuiltin(){
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}

void everyMinute(){
  Serial.printf("T=%d 1 min tick\n",millis()/1000);
}

void argue(const char* a,const char* b){
  Serial.printf("T=%d %s was %s\n",millis()/1000,a,b);
}
void pulsePin(uint8_t pin,unsigned int ms,bool active){
  digitalWrite(pin,active);
  once(ms,bind([](uint32_t pin,uint8_t hilo){ digitalWrite(pin,hilo); },pin,active ? LOW:HIGH));
}

#define DIT 80
#define DAH 200
#define GAP DIT*2.5
#define PAUSE 3*DAH

void SOS(){
  every(5000,[](){
    nTimes( 3, GAP, bind(pulsePin,LED_BUILTIN,DIT,LOW),
//    onComplete...
      [](){
        once(PAUSE,[](){
          nTimes( 3, GAP*2, bind(pulsePin,LED_BUILTIN,DAH,LOW),
  //      onComplete...
          [](){
            once(PAUSE,[](){
              nTimes( 3, GAP, bind(pulsePin,LED_BUILTIN,DIT,LOW));
            });
          });
        });
      });
    });
}

class demo {
  public:
    void print(const char* s){
      Serial.printf("T=%d %s [class method called by Ticker!]\n",millis()/1000,s);
  }
};

demo  demoClass;

void setup() {
  static uint32_t edwin;
  static uint32_t marvin;
  static uint32_t heartbeat;
  
  static uint32_t j=10;
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  
  CreateMutex(&tqMutex);
  Serial.begin(74880);
  delay(1000);
  Serial.println("Let's get it on: https://www.youtube.com/watch?v=x6QZn9xiuOE");
  once(500,bind(&demo::print,demoClass,"WOW!"));
  heartbeat=every(1000,bind(pulsePin,LED_BUILTIN,100,LOW));
  edwin=every(1000,bind(argue,"Edwin Starr","the best"));
  onceRandom(25000,15000,[](){
    Serial.printf("T=%d '25 Miles' is the best ever: https://www.youtube.com/watch?v=hFredbE3goM\n",millis()/1000);
    Serial.printf("T=%d Not forgetting: 'Stop her on Sight (S.O.S)'! https://www.youtube.com/watch?v=RwjLGqQYBBk\n",millis()/1000);
    never(edwin);
    never(heartbeat); // stop the heartbeat flashing
    SOS(); // and flash S-O-S
    Serial.printf("T=%d Check the LED it's flashing S-O-S in morse code every 5 seconds...\n",millis()/1000);
  });
  
    everyRandom(5000,10000,[](){Serial.printf("T=%d (annoying %s music every 5-10 seconds)\n",millis()/1000,genres[random(0,8)]); }); 
    once(10000,[](){
        Serial.printf("T=%d Stop arguing! They were both great, but Edwin WAS better...\n",millis()/1000);
        never(marvin);
        },
      [](){ 
        Serial.printf("T=%d I still say Marvin had the better voice...\n",millis()/1000);
        Serial.printf("T=%d ..listen to 'Heard it through the grapevine' https://www.youtube.com/watch?v=hajBdDM2qdg\n",millis()/1000);
        } // "chain" function - called when "once" completes
      );
    // What "one second ticker" ? we haven't created one yet!
    // It's important to realise that none of the called functions run NOW
    runNow([](){ Serial.println("...unless we do THIS..."); });
    // so the order in which you create the timers is largely irrelevant...
    // ...but the order in which the timers "fire" IS important!
    // try it yourself - rearrange all "the top-level" calls in any order you
    // like and the sketch output will still be the same
    every(60000,everyMinute);
    // THAT "one second ticker" that will get cancelled by the "10 second one-shot" a few lines ago...   
    marvin=every(1000,bind(argue,"Marvin Gaye","better"));
    // Stop the 1-second timer after 10 secs      
    // at 57 to 59 seconds after T0, we will start some other timers...
    // (no reason why a timer routine can't "chain in" other timers!)
    // 
    // Let's call this "New T0" - we can't predict when it will actually occur...
    // It will start another timer which will run 5 times with a second in between each, it wil run from New T0 -> New T0+5
    // ...somewhere round the third or fourth "tick" - it will get mixed in with the 1 minute timer we started earlier
    // It will also start another timer which will begin somewhere in the middle of the previous one
    // i.e. the outputs from the two will be mixed up (with the minute ticker too as mentioned)
    // ...not forgetting that at any time that annoying ping could also pop up! 
    onceRandom(57000,59000,[](){
      Serial.printf("T=%d between 57 and 59 seconds after startup...New T0\n",millis()/1000);
      nTimes(3,1000,[](){
        static int count=1;
        Serial.printf("T=%d Motown Chartbusters Volume %d\n",millis()/1000,count++);
        },
        [](){ // "chain functions - runs when nTimes(3... finishes
        Serial.printf("T=%d ...or maybe 'Chain of Fools' by Aretha Franklin...https://www.youtube.com/watch?v=hrcUNChhOP0\n",millis()/1000);
        // and can run other functions that can chain in other functions that can...and so on...
        onceRandom(2000,3000,[](){ Serial.printf("T=%d Was that an album or just a single?\n",millis()/1000); },
          bind(argue,"Now, Aretha...SHE","the greatest!")
          );
        }
      );
      
      Serial.printf("T=%d 2-3 sec after New T0 (i.e. at 59 < T < 62), we discuss the greatest album of all time\n",millis()/1000);
      Serial.printf("T=%d while Edwin Starr sings '25 miles'\n",millis()/1000);
      onceRandom(2000,3000,[](){
        nTimes(10,1000,bind(
          [](uint32_t n) {// 10x, 1sec apart
            uint32_t& remaining=(*reinterpret_cast<uint32_t*>(n));
            Serial.printf("T=%d %d! (%d More miles...)\n",millis()/1000,remaining,remaining--);
          }
          ,(uint32_t) &j));      
      });
    });
    // That final "count from 0 to 9" timer will start between 59 and 62 seconds and run for another 10 secs
    // it should finish then between 69 and 72 seconds
    // ...except that it won't get that far because of what we are about to do next...
    // between 65 and 70 seconds (right in the middle of counting from 0 to 9...we stop ALL timers
    // ..including the one flashing the builtin in led
    onceRandom(65000,70000,[](){
      Serial.println("No more, please!");
      never();
      runNow([](){ Serial.println("Silence is golden - until the next time!"); });  //...nTimes(1,0 = ...once(0 -  time of 0 makes it run immediately
    // and FINALLY(?) after a couple of minutes silence, let's run this whole thing again....
      j=10; // or Edwin Starr will be walking backwards!
      once(90000,setup);
  });
}

void loop() {
  int ifh,cfh;
  ifh=ESP.getFreeHeap();
  runJobs();
  cfh=ESP.getFreeHeap();
//  if(cfh!=ifh) Serial.printf("HEAPCHANGE: i=%d c=%d diff=%+d\n",ifh,cfh,cfh-ifh);
}
