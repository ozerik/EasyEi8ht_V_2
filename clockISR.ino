void clockISR() {
  if (Vtriggering == false) {  // this means TRIGGER TIME (unless swinging, then maybe we'll delay a bit)
    if (swing != 0) {          // time to swing, I guess
      if (swing < 0) {
        if (subtractNextSwing == false) {   // okay so we'll delay the start of the 0-bit (even) step numbers by "swing" amount, and subtract the same value from the odd step number
          if (bitRead(VstepNum, 0) == 0) {  // looks at VstepNum to see if the last bit is zero, meaning it's an even number
            Timer1.setPeriod(abs(swing));   // if it's even, set the timer to delay by swing-amount of microseconds
            subtractNextSwing = true;       // and set the subtractEvenSwing to true, which will tell the interrupt routine to subtract the "swing" amount from the next (odd) step number
            return;                         // just bail on this section of the sketch.
          }
        }
      } else if (subtractNextSwing == false) {  // this must mean the swing value is greater than zero
        if (bitRead(VstepNum, 0) == 1) {        // checks for odd step numbers
          Timer1.setPeriod(abs(swing));         // if it's odd, delay the next clock event by this many microseconds
          subtractNextSwing = true;             // sets this variable to true, so it'll subtract the "swing" amount from the even step number
          return;                               // stop running this function. Forget it. Do NOT go any farther in the sketch
        }
      }
    }

    // if we're running through this after being delayed by the swing routine, do this!!!
    Vtriggering = true;                                                // tells the rest of the sketch that we're triggering stuff!!!
    VlastClock = micros();                                             // when was this, exactly??
    Timer1.setPeriod(TRIG_LNGTH);                                      // come back here in just a little while to turn all the outputs off!
    VstepNum++;                                                        // increments the stepNumber
    if (VstepNum > seqLength) VstepNum = 1;                            // starts sequence back over at 1
    bitWrite(PORTD, 1, 1);                                             // turns on the Clock Out output for external clocking. Will need to be SWUNG later on when I get to that
    byte playPins;                                                     // container for the current pattern that's gonna be performed
    playPins = sequence[VstepNum] & mute & solo;                       // if mute or solo are anything besides 0b11111111, that trigger output won't fire
    if (mode == 1 && shift == 1) playPins |= recPins;                  // plays currently pressed keys in mode 1 while shift is pressed
    if (mode == 3) {                                                   // here's the freestyle mode code
      if (FSMode == 1) {                                               // Mode one!
        if (shift == true) playPins = recPins;                         // unless you're holding down the shift key, then it plays JUST the keys you're pressing
        else playPins |= recPins;                                      // if you aren't holding shift, it adds whatever you're pressing to the pattern
      } else if (FSMode == 2) {                                        // freestyle mode two!
        if (recPins > 0) {                                             // if any keys are held down,
          doubleTime = true;                                           // this is a flag telling the loop to play recPins halfway between clock tics
          if (subtractNextSwing == true) {                             // if we're swinging.....
            VdoubleTimer = micros() + ((interval - abs(swing)) >> 1);  // do the math like this. To be clear, this creates a microsecond time that will be when we want the loop to play recPins
          } else VdoubleTimer = micros() + (interval >> 1);            // we're not swinging, so we can do the simpler math
          playPins |= recPins;                                         // if shift is NOT held down, play the keys held down plus the regular pattern
        }                                                              // and hopefully all the rest of the stuff will happen in loop()
        if (shift == true) {                                           // shift is held down
          playPins = recPins;                                          // when shift is held, play just the keys held down
        }                                                              //
      } else if (FSMode == 3) {                                        // freestyle mode 3, GLITCH MODE
        playPins = glitchMode();                                       // runs the glitchMode() function and returns a byte
      }
    }

    if (mode == 2) {
      PORTB |= playPins << 2 & 0b00011100;  // handles the three LEDs that might not be on high
      DDRB |= playPins << 2 & 0b00111100;   // these two lines make playPins pins be OUTPUTS
      DDRC |= playPins >> 4 & 0b00001111;   // these two lines make playPins pins be OUTPUTS, which will make them be "off" in the cfg()
    } else {
      PORTB |= playPins << 2 & 0b00111100;  // output turn on them all turn them on!!!!!!!!!!!
      PORTC |= playPins >> 4 & 0b00001111;  // output turn on them all turn them on!!!!!!!!!!!
    }
  } else {                  // this is the part that turns stuff OFF. What happens when we're not triggering
    Vtriggering = false;    // tells the rest of the sketch we're not triggering stuff anymore. The rest of the sketch can do live-performed trigger outputs, 32nd notes, whatever
    bitWrite(PORTD, 1, 0);  // turns off the Clock Out output
    if (mode == 2) {        // we're in the part that the LEDs are dimly lit
      DDRB &= 0b11000011;   // these two lines make pins be INPUTS
      DDRC &= 0b11110000;   // these two lines make pins be INPUTS, which will make them be "off" in the cfg()

    } else {
      PORTB &= 0b11000011;  // outputs turned off
      PORTC &= 0b11110000;  // outputs turned off
    }
    doubleGlitch = false;
    glitchOnce = false;
    if (subtractNextSwing == true) {            // this is what happens with the swing situation
      Timer1.setPeriod(interval - abs(swing));  // calculation saying when the next timer runthrough should be
      subtractNextSwing = false;                // don't do this next time!
    } else Timer1.setPeriod(interval);          // this is just the calculated 16th note value. If I wrote the code carefully enough, and it's fast enough we'll have to subtract the TRIG_LNGTH from this value to keep it from drifting?
    if (STOP == true) Timer1.stop();  // if the timer needs not to full-on run, because it's getting exernally clocked, stop the timer
  }
}
