void recPlay() {
  {
    if (oldMode != 1) {
      if (triggering == false) {
        PORTB &= ~0b00111100;  // these two lines make pins OFF
        PORTC &= ~0b00001111;  // these two lines make pins OFF
      } else oldMode = 10;     // oldMode is abused here to force this section to keep being run until triggering == true
      oldMode = 1;             // tracker for if the mode changes
      DDRB |= 0b00111100;      // these two lines make pins be OUTPUTS
      DDRC |= 0b00001111;      // these two lines make pins be OUTPUTS
      muting = 0;              // just in case the shift key was held down earlier
    }
  }

  {                                                    // button player turn onner
    if (recPins > oldRecPins && !triggerKeys) {        // runs every time a new keypress is detected but only once
      triggerKeys = true;                              // tracker to only turn off keys when theyre's actually one that got played
      triggerTimer = millis();                         // start the trigger timer
      newKeys = recPins ^ oldRecPins;                  // don't want held-down keys to play on EVERY keypress
      if (((newKeys ^ mute) > mute) && muting == 0) {  // okay, the newKeys bit makes the mute variable a larger value so it's flipping from 0 to 1, meaning that track is muted
        for (int i = 0; i <= seqLength; i++) {         // this for loop runs through the sequence, replacing the newKeys bit in each trigger value with a 0
          sequence[i] &= ~newKeys;                     // the ~newKeys will be all 1s and one 0, and that 0 is replacing whatever's in that spot in each step
        }                                              // good job
        mute = mute ^ newKeys;                         // resets the mute variable so the just-muted track can be used
        muting = 0;                                    // resets the muting variable. It's like "my job here is done"
      }
      if (muting == 1) {        // mute the newly pressed track!? Or ERASE THE WHOLE THING muahahaha
        mute = mute ^ newKeys;  // this flips the bit where "newKeys" is a 1
        oldMute = mute;         // next time maybe mute the track IDK
        muting = 0;             //reset the mute to nope
      } else {
        if (shift == false) {                  // only plays instant triggers if the shift key isn't held
          PORTB |= newKeys << 2 & 0b00111100;  // these two lines play whatever
          PORTC |= newKeys >> 4 & 0b00001111;  // was pressed into the keys right away
        }
        if (STOP == false || external == true) {                             // if the clock is stopped, don't record key presses
          if (lastClock + (interval >> 1) > micros()) {  // checks the microsecond time since the last clock trigger event
            sequence[stepNum] |= newKeys;                // records new button presses into the current step of the sequence *IF* they were entered closer to this step
          } else {                                       //
            laterKeys |= newKeys;                        // dumps the key presses into the variable that'll be written after the next clock fires
          }
        }
      }
    }
  }

  {                                    // records laterKeys into the sequence AFTER the stepNum is updated
    if (stepNum != oldStepNum) {       // keeps track, only fires first time stepNum is incremented
      oldStepNum = stepNum;            // now this if statement won't run because they'll be the same
      sequence[stepNum] |= laterKeys;  // laterKeys value gets recorded into the just-outputted sequence step
      laterKeys = 0;                   // clears out the laterKeys variable once it's recorded into the pattern
    }
  }

  {                                                                  // does the mute thing
    if (oldShift != shift && shiftDebounce + DEBOUNCE < millis()) {  // the state of the shift key was changed!!!
      shiftDebounce = millis();                                      // shift key debouncer
      if (oldShift < shift) {                                        // shift button pressed, this'll execute ONCE
        oldShift = shift;                                            // the if statement won't run until the key status changes
        if (muting == 1) muting = 2;                                 // hey! on NEXT release, turn muting mode off
        if (STOP == true && external == false) {                     // is the STOP variable set to true? Also, is there external triggering?????
          clockISR();                                                // is it is, that means this button press means "GO TIME!"
          STOP = false;                                              // we're no longer stopped
          doNotMute = true;                                          // also, we don't want this shift-key-press to count as a mute press.
        }
      } else {                        // shift button released!?
        oldShift = shift;             // makes the whole statemnt execute just once
        if (muting == 0) muting = 1;  // sets the mute variable to true ATTENTION!!! muting needs to be turned off somehow when MUTE TRACK was chosen maybe with oldShift = shift... or maybe needs to be a byte not bool
        else muting = 0;              // ope, looks like you decided not to mute a track
        if (doNotMute == true) {      // must have done 16th notes into pattern or used the shift key as a START key
          muting = 0;                 // makes shift key release NOT start mute process
          doNotMute = false;          // resets doNotMute flag
        }
      }
    }
  }


  {  // button player turn offer
    if (triggerKeys && triggering == false && triggerTimer + TRIG_LNGTH_MILLIS < millis()) {
      triggerKeys = false;  // do I really need triggerKeys?
      PORTB &= 0b11000011;  // these lines TURN OFF outputs
      PORTC &= 0b11110000;  // these lines TURN OFF outputs
    }
  }
}
