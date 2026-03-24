#include <TimerOne.h>
#include <EEPROM.h>


/* -----------MODULAR FOR THE MASSES EASY EI8HT---------------
  This is a performance trigger sequencer with eight channels, eight mechanical key switch buttons,
  designed from the ground up to be extremely easy and intuitive to use. I'm a self-taught coder,
  so this software will have room for improvement, especially with the variables. It's a large
  project for my brain to hold, so I lost control of it somewhat toward the end (glitch mode)
  so some variables may be redundant or unnecessary but hey, it fits just fine on an Arduion and
  I don't think more variables will slow the code down at all.

  I've documented almost every line of code, so hopefully the functionality of the program will be
  clear and legible. I eagerly welcome improvements in efficiency or function

  Some improvements that should be possible: 
    tap tempo features a TWEAK variable that was required to
  keep the tempo from drifting. I don't know why it's drifting... my theory is that the code takes
  time to execute and somehow that time is making the Timer1 timer interrupt get delayed? That still
  doesn't really make sense to me so that part could be better.
    Glitch mode (the freestyle mode #3) was supposed to do 32nd notes with the shift key pressed,
  but it doesn't. I ran out of time getting that to work. Also, glitch mode is ripe for fancy crazy
  pattern generation which I didn't explore very much.
    EEPROM recording nad reading: The E8 v1.0 code recorded a bunch of variables into the EEPROM
  but I haven't implemented that. Not sure I want to. Having a perfectly factory-fresh module ready 
  to go on every reset might be a good thing. The tempo will revert to 120BPM, who cares, tap in a
  new tempo, and if you've been button-mashing in configure mode and don't know how to get swing
  back to 0 and a reasonalbe sequence length, resetting the module will get you there.


  OKAY!!! Super brief manual

  When turned on, a clock will be going. Default is 120BPM with no external trigger connected.
  INPUTS:
  Clock In: accepts a trigger, which becomes the module's clock
  Reset: resets the module to the first spot in the pattern. Not sure how useful it is, since we
    don't know which setp is step 1, BUT can be a lot of fun to create complex interactive patterns
    when you have multiple E8 modules messing with each other.
  Mode switch: switches modes. More later! :P
  Clock Out: outputs 16th note triggers. Very useful for clocking the rest of your system if the
  EasyEi8ht is the primary clock in your rack. If you're using an external trigger, it just copies
    the clock from the Clock In jack.

  External triggering should conceptually be 16th notes. When an external trigger is stopped or
    disconnected, the module will wait for the middle button (shift key) to be pressed to restart.
    When restarted on the internal clock, it'll be going the same speed (about) as the external clock.
  Internal clock can be set by tapping the middle button shift key faster than once per second, when
    the mode switch is in the CFG spot
  THREE MODES
  R/P - record/play
    Tap one of the keys, and it will play instantly. That tap will get recorded into that spot in the
    pattern. The pattern is 32 beats by default. You can change it using CFG mode (explained later)
    Hold the shift key, and key presses will *only* play and be recorded into the pattern upon the
    clock event. This is a great way to get a channel full of 16th notes.
    Tap the shift key and it'll light up (with brief off-blinks in time with the clock). Tap any key
    with the shift key lit up, and it will mute that channel. Tap that channel while muted, and BANG
    that channel's pattern is deleted, and you're free to enter a new pattern.
    Tap the shift key, get it lit up, and tap a muted track? That channel gets unmuted, and you've
    got the previously entered pattern playing again.
  CFG - configure
    Tapping keys 1, 2, or 3, sets the freestyle mode.
    Shift + 1 selects a shorter pattern. Shift + 3 selects a longer pattern. Shift + 2 returns to the
    default 32 beat pattern
    Hit all three keys across the middle (keys 4, shift, and 5) and the module *IF SELF-CLOCKED* will
    stop running. Tapping keys 4 or 5 while the clock is stopped will scroll through the pattern.
    Hit all three keys in the middle column will instantly delete the entire pattern.
    Key 6 engages swing mode, slewing every other sixteenth note one direction. Key 8 slews the OTHER
    every other sixteenth note. One direction will sound wrong, because we have no way of knowing which
    beat in your performance is the downbeat, and which is the upbeat.
    Key 7 returns the swing to zero, it's back to not swing at all.
  FS - freestyle
    Freestyle mode doesn't alter the pattern that's recorded or playing. It's the GO CRAZY mode, nothing
    you do here will change your pattern.
    Mode one: key presses do 16th notes of that channel. Shift mutes all the channels, only performing
    the keys you press.
    Mode two: just like Mode One, but it does 32nd notes.
    Mode three: Glitch mode....... randomizes and loops sections of the pattern. Shift plus key doesn't
    actually do anything I'm pretty sure <--- room for improvement

  
*/


// pin reading stuff
byte recPins;      // all the "keys" (microprocessor pins) that are being "pressed" (brought low by the switch)
byte oldRecPins;   // stores previous received pins
byte playPins;     // pins that are gonna be PLAYED
bool shift;        // is shift key held or pressed? This becomes true
bool oldShift;     // lets me keep track of when the shift key was released or whatever
byte newKeys;      // temp received pins, compares oldRecPins with recPins
bool triggerKeys;  // tracker to just turn off outputs if the keys were JUST PRESSED in record/play or freestyle mode
byte laterKeys;    // holder for keypresses (that already were performed) to be added to the *next* sequence after it plays


// sequence stuff
unsigned long quarterNote = 500000;  // here's the quarter note value. Tap tempo mode will adjust this value, and an external trigger will help calculate this as well?
byte sequence[1111];                 // this is the sequence. 1,111 possible steps is ludicrously long, but at least there's room for us not to accidentally WRITE A VALUE INTO AN ARRAY PAST THE DECLARED SIZE and cause gravity to flip or whatever
short seqNum = 3;                    // low long is sequence? Used to set the length among the choices
byte stepNum;                        // what step number is the sequence in?
byte oldStepNum;                     // lets me record the keypresses into the sequence AFTER the step number was changed
volatile byte VstepNum;              // okay a volatile version of the stepNum variable
short seqLength = 32;                // memory this, how many steps are in the current sequence
byte solo = 0b11111111;              // variable that gets AND'ed with the to-be-outputted sequence value. For when one track only is gonna play
byte mute = 0b11111111;              // variable that gets AND'ed with the to-be-outputted sequence value. Mutes one or more tracks
byte oldMute;                        // keeps track of which tracks you've got muted, and unmutes it if you didn't delete it
bool doubleTime;                     // means we're in doubletime mode, module will play 32nd notes!


// timing stuff
unsigned long triggerTimer;                          // A timer variable for when to turn off keypress triggers (and maybe 32nd note triggers?)
#define TRIG_LNGTH 8000                              // how long will the module keep the triggers high? In microseconds.
#define TRIG_LNGTH_MILLIS 8                          // this needs to be "TRIG_LNGTH" divided by 1,000 :P you know, for milliseconds
#define EXT_TRIG_PAD 100000                          // when externally clocked, this value is added so that the next external trigger isn't stepping on the internal clock's toes does that make sense?
#define DEBOUNCE 20                                  // debounce value for shift mute function
#define TWEAK 10630                                   // adds or subtracts to the timer interval to nudge it into keeping time
unsigned long shiftDebounce;                         // timer for shift debouncing
volatile unsigned long VlastClock;                   // this is the micros() time the last time the clockISR was run
unsigned long lastClock;                             // copy of VlastClock... used to calculate if the keypress was before or after the midpoint in the pattern
volatile unsigned long interval = quarterNote >> 2;  // interval is the calculated period of the module's internal clock. Gets set by tap tempo or something? Volatile because it's 4 bytes long and read by ISR
unsigned long cfgDebounce;                           // for keeping track of if you didn't mean to change the FSMode when pushing key 2... maybe you just wanted to erase the sequence?
byte oldFSMode;                                      // stores the old freestyle mode
long swing;                                          // swing value, can go plus or minus
bool subtractNextSwing;                              // tracks if to shorten next interval by "swing" amount
volatile unsigned long VdoubleTimer;                 // timer for freestyle mode 2, 32nd notes
volatile unsigned long doubleTimer;                  // timer for freestyle mode, non volatile
unsigned long clockDebounce;                         // debounce timer just for the external trigger!
short extClockTrack;                                 // a variable to help me keep the external trigger well-behaved
 


// tracking stuff
bool Vtriggering;               // tracks if the module is currently doing a triggering event
bool triggering;                // copy of Vtriggering, is module currently doing a trigger
byte mode;                      // what position is the mode switch in?
byte oldMode;                   // did the mode switch just change? This will check!
byte FSMode = 1;                // three freestyle modes. 1 = shift-key mutes, keypresses do 16th notes. 2 = same but keypresses do 32nd notes! 3 = glitch crazy trigger storm!!!!
byte muting;                    // was the shift key tapped while in recPlay() mode? muting == 0 means not muting. muting == 1 means yes muting. muting == 3 means nevermind, going back to 0
bool doNotMute;                 // if you're in recPlay() mode and you've got the shift key down to record rolls into the pattern, you don't want the shift release to enter MUTE MODE right?
bool STOP;                      // tracks if the module is stopped or whatever
#define SWING_GRANULARITY 2000  // sets how much to change swing time by
bool external;                  // ANOTHER variable keeping track of if the module is being triggered by an external clock
bool doubleGlitch;              // I've lost track of stuff, I think I need this variable to run glitchMode on the 32nd notes
bool glitchOnce;                // I've seriously lost track.... this stops the glitch mode from just running at loop speed



// just tap timer stuff
bool tapped;                // tracks if the shiftkey was tapped
short tapNum;               // counts taps
unsigned long tapDebounce;  // timer variable to keep key presses from falsely triggering
unsigned long tapTime;      // this is the time between shift key presses
unsigned long tapBucket;    // this keeps getting tapTime added to it, and gets divided by the tapNum variable to set the interval variable


// glitch variables
bool glitchStep;
unsigned short glitchStepCount;
bool cylon;




void clockISR();
void recPlay();
void cfg();
void freestyle();
void extTempo();
byte glitchMode();


void setup() {
  pinMode(0, INPUT_PULLUP);  //PIND  B0000000x (breaks when Serial is used)
  pinMode(1, OUTPUT);        //PORTD B000000x0 (breaks when Serial is used)

  pinMode(2, INPUT_PULLUP);  //PIND B00000x00   button 1
  pinMode(3, INPUT_PULLUP);  //PIND B0000x000   button 2
  pinMode(4, INPUT_PULLUP);  //PIND B000x0000   button 3
  pinMode(5, INPUT_PULLUP);  //PIND B00x00000   button 4
  pinMode(6, INPUT_PULLUP);  //PIND B0x000000   button 5
  pinMode(7, INPUT_PULLUP);  //PIND Bx0000000   button 6
  pinMode(8, INPUT_PULLUP);  //PINB B0000000x   button 7
  pinMode(9, INPUT_PULLUP);  //PINB B000000x0   button 8


  pinMode(10, OUTPUT);  //PORTB B00000x00  output 1
  pinMode(11, OUTPUT);  //PORTB B0000x000  output 2
  pinMode(12, OUTPUT);  //PORTB B000x0000  output 3
  pinMode(13, OUTPUT);  //PORTB B00x00000  output 4     HEY LOOK HERE EVERYBODY!!!! This module kinda needs you to remove the pin 13 LED that's included on basically every Arduino. Or the resistor. Or cut the trace.
  pinMode(A0, OUTPUT);  //PORTC B0000000x  output 5
  pinMode(A1, OUTPUT);  //PORTC B000000x0  output 6
  pinMode(A2, OUTPUT);  //PORTC B00000x00  output 7
  pinMode(A3, OUTPUT);  //PORTC B0000x000  output 8

  pinMode(A4, INPUT_PULLUP);  //PINC  B000x0000   clock trigger in
  pinMode(A5, OUTPUT);        //PORTC B00x00000   flashy light


  Timer1.initialize(quarterNote >> 2);
  Timer1.attachInterrupt(clockISR);


  // Serial.begin(115200);
}
