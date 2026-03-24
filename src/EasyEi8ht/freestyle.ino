void freestyle() {
  if (oldMode != 3) {
    oldMode = 3;                // tracker for if the mode changes
    if (triggering == false) {  // only turn the outputs off if triggering isn't happening
      PORTB &= ~0b00111100;     // these two lines make pins OFF
      PORTC &= ~0b00001111;     // these two lines make pins OFF
    } else oldMode = 10;        // oldMode is abused here to force this section to keep being run until triggering == true
    DDRB |= 0b00111100;         // these two lines make pins be OUTPUTS
    DDRC |= 0b00001111;         // these two lines make pins be OUTPUTS
  }
  if (STOP == true && shift == true && external == false) {  // okay this starts the clock back up when pressed
    STOP = false;                                            // says NOT STOPPED to the rest of the sketch
    Vtriggering = false;                                     // falses this variable
    clockISR();                                              // runs the clockISR routine, starting up the whole thing!
  }
  if (FSMode == 3) {                                 // glitch mode section of code is called in the clockISR function. This is what looks for half-beat timing to run 32nd notes when SHIFT is pressed
    if (shift == true) {                             // if the shift key is pressed...
      if ((lastClock + interval >> 1) < micros()) {  // look for the timer to say "hey, half of an interval has passed! time to play glitch again!
        if (doubleGlitch == false) {                 // but please only do it once
          doubleGlitch = true;                       // only go once! Until this variable is turned false again
          glitchOnce = true;                         // keeps it happening once once once SIGH
          byte glitchPins = glitchMode();            // fill new variable glitchPins with the value returned by glitchPins()
          PORTB |= glitchPins << 2 & 0b00111100;     // these lines play whatever glitchPins() returned
          PORTC |= glitchPins >> 4 & 0b00001111;     // hope it's cool
          doubleTimer = micros();                    // setting the doubleTimer timer so all the pins can get turned off
        }
      }
    }
    if (glitchOnce == true && doubleGlitch == true && doubleTimer + TRIG_LNGTH < micros()) {
      glitchOnce = false;
      // doubleGlitch = false;
      PORTB &= 0b11000011;  // these lines TURN OFF outputs
      PORTC &= 0b11110000;  // these lines TURN OFF outputs
    }
  }
}