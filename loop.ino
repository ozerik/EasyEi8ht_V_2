void loop() {

  // mode switch reader ALSO volatile variable copier ALSO reset watchdog
  {
    noInterrupts();                             // pauses interrupts
    mode = map(analogRead(A6), 50, 900, 1, 3);  // mode switch reader
    triggering = Vtriggering;                   // copies volatile variable into rest of sketch
    lastClock = VlastClock;                     // makes a copy of the last clock variable
    stepNum = VstepNum;                         // makes a copy of stepNum variable
    // if (analogRead(A7) < 80) stepNum = 255;     // makes step number 255 to signal the rest of the code that it's time to reset
    interrupts();  // restarts interrupts
    if (mode == 1) {
      recPlay();                                                                // sets mode to 1, record/play
      if (recPins > 0 && muting == 0 && shift == true && triggering == true) {  // looks for module being in mode 1, keys pressed, shift held down, AND the clockISR is in the triggering mode
        sequence[stepNum] |= recPins;                                           // fills pattern with rolls
        doNotMute = true;                                                       // don't go into mute mode when you release the shift key
      }
    } else if (mode == 2) cfg();  // sets mode to 2, configure
    else freestyle();             // sets mode to 3, freestyle
  }                               // end of mode switch reader


  // button press reader
  {                                         // detects button presses all the time. recPins means received input pins, which keys are pressed. 1 means pressed, 0 means not
    oldRecPins = recPins;                   // oldRecPins variable tracks changegs to keypresses, also doesn't let brand new keypresses play in the next clockISR event
    recPins = PIND >> 2;                    // grabs six of the keys
    recPins |= ((PINB << 6) & 0b11000000);  // grabs the other two
    recPins = ~recPins;                     // inverts bits
    if (bitRead(PIND, 0)) shift = false;    // shift key not held down (the pin is pulled up, taken LOW by the switch)
    else {                                  // shift key held down
      shift = true;                         // shift key IS held down
    }
  }  // end of button press detector

  // instant button player and turnoffer is in RecPlay()


  {                                                                     // here's the code that looks for an external trigger
    if (bitRead(PINC, 4) == 0) {                                        // loooks at pin A4 (external clock), waiting for it to go LOW
      if (clockDebounce + DEBOUNCE < millis() && extClockTrack == 0) {  // checks for debounce and that the tracking variable is 0
        external = true;                                                // WHOAH we're doing external triggers I guess
        extClockTrack = 1;                                              // sets the tracking variable to 1
        clockDebounce = millis();                                       // sets the debounce timer
        STOP = true;                                                    // tells the rest of the sketch that the internal clock isn't gonna need to run
        Vtriggering = false;                                            // this means the next runthrough of clockISR() (next line) will do triggers
        clockISR();                                                     // runs the ClockISR() routine, which sets off the whole trigger routine
        if (tapNum < 6) extTempo();                                     // runs the function to figure out how fast to set the tempo of the module (for 32nd notes mostly)
      }                                                                 //
    } else extClockTrack = 0;                                           // resets the tracking variable
    if (tapDebounce + 2000 < millis()) {                                // this returns TRUE every two seconds. That's often enough to set the interval of triggers coming in
      tapDebounce = millis();                                           // resets the two second timer variable
      tapNum = -1;                                                      // when it happens, make this variable -1 which will allow the extTempo() function to run and calculate the tempo
      tapBucket = 0;                                                    // the large number that holds the number to be averaged over in extTempo
    }                                                                   //
    if (external == true && clockDebounce + 500 < millis()) {           // one half second after the external trigger stops, the module is ready to use the internal clock again
      external = false;                                                 // ready for internal clocking!
    }
  }





  {                                                      // freestyle mode 2 code
    noInterrupts();                                      // pause interrupts
    doubleTimer = VdoubleTimer;                          // get the doubleTimer variable transferred
    interrupts();                                        // restart interrupts
    if (doubleTime == true && micros() > doubleTimer) {  // looks for when it's time to play between notes
      PORTB |= recPins << 2 & 0b00111100;                // these two lines play whatever
      PORTC |= recPins >> 4 & 0b00001111;                // keys are being held right now
    }
    if (doubleTime == true && doubleTimer + TRIG_LNGTH < micros()) {
      doubleTime = false;
      PORTB &= 0b11000011;  // these lines TURN OFF outputs
      PORTC &= 0b11110000;  // these lines TURN OFF outputs
    }
  }

  {                                           // shift key blinky light turn on and off-er
    byte blinker = stepNum << 6;              // puts the stepNum into a variable that'll be "true" if either of the last two bits are 1, and "false" if they're both 0
    if (muting == 1) {                        // reliant on if the shift key was pressed and released to enter mute mode
      bitWrite(PORTC, 5, 1);                  // turns LED ONNNNNN
      if (triggering) bitWrite(PORTC, 5, 0);  // turns LED off while the pattern is being output
    } else bitWrite(PORTC, 5, !blinker);      // the shift key LED will do the opposite of what the blinker variable says
  }
}
