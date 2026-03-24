void cfg() {
  {                              // mode change DO ONCE section
    if (oldMode != 2) {          // did the mode change?
      oldMode = mode;            // tracker for if the mode changes
      if (triggering == true) {  // leave the pins OUTPUT until triggering stops
        oldMode = 10;            // oldMode is is being abused here to track if the switch was thrown while triggering =='ed true
      } else {                   // looks like it's not triggering anymore
        PORTB |= 0b00100000;     // these two lines make pins ON
        PORTC |= 0b00001111;     // except the top row. That's for "freestyle mode"
        DDRB &= 0b11000011;      // these two lines make pins be INPUTS
        DDRC &= 0b11110000;      // these two lines make pins be INPUTS
      }
      if (bitRead(PIND, 0) == LOW) tapped = true;  // this takes care of the problem with tap tempo if you've held the shift key down while switching into this mode
      muting = 0;
    }
  }


  {                                                     // this part responds to button presses in this mode
    if (recPins > oldRecPins) {                         // This part only happens once per button press
      if (bitRead(recPins, 0) == 1 && shift == true) {  // SHIFT and KEY1 was pressed
        seqNum--;                                       // take away 1 from the variable which sets the sequence length down in the function below...
        if (seqNum < 1) seqNum = 1;                     // there is no zero or negative values in the list of sequence lengths
        SLselect();                                     // this runs through the sequence lengths and chooses the one you chose
      }
      if (bitRead(recPins, 2) == 1 && shift == true) {  // SHIFT and KEY 2 were pressed
        seqLength = 32;                                 // returns sequence length to the default 32 steps
      }
      if (bitRead(recPins, 2) == 1 && shift == true) {  // SHIFT and KEY3 was pressed
        seqNum++;                                       // add 1 to sequence length variable
        if (seqNum > 7) seqNum = 7;                     // don't make it larger than 7. You can choose, dear coder, to make this much larger, with sequence lengths up to 1111 steps, I guess
        SLselect();                                     // sequence length select routine
      }
      if (STOP == true && triggerKeys == false && external == false) {  // keys 4 and 5 will go back and forward through the sequence if the module is stopped
        triggerKeys = true;                                             // here's the tracker sorta debouncing this section and allowing the rest of the sketch to know what's going on
        triggerTimer = millis();                                        // sets the start point for how long these triggers are going to be OUTPUT and HIGH
        if (bitRead(recPins, 3) == 1) {                                 // somebody pushed key four on the keybad
          VstepNum--;                                                   // so take one away from the step number. VstepNum is the volatile step number
          if (VstepNum < 1) VstepNum = seqLength;                       // we gotta roll the VstepNum value over to the end if it gets past the beginning
          stepNum = VstepNum;                                           // these two variables have to stay the same. The clockISR has VstepNum while most of the rest of the sketch uses stepNum
          PORTB |= sequence[stepNum] << 2 & 0b00011100;                 // handles the three LEDs that might not be on high
          DDRB |= sequence[stepNum] << 2 & 0b00111100;                  // these two lines make playPins pins be OUTPUTS
          DDRC |= sequence[stepNum] >> 4 & 0b00001111;                  // these two lines make playPins pins be OUTPUTS, which will make them be "off" in the cfg()
        }
        if (bitRead(recPins, 4) == 1) {                  // This time button five was pushed
          VstepNum++;                                    // add one to step number
          if (VstepNum > seqLength) VstepNum = 1;        // roll step number over
          stepNum = VstepNum;                            // keep the variables the same
          PORTB |= sequence[stepNum] << 2 & 0b00011100;  // handles the three LEDs that might not be on high
          DDRB |= sequence[stepNum] << 2 & 0b00111100;   // these two lines make playPins pins be OUTPUTS
          DDRC |= sequence[stepNum] >> 4 & 0b00001111;   // these two lines make playPins pins be OUTPUTS, which will make them be "off" in the cfg()
        }
      }

      if (bitRead(recPins, 5) == 1) {
        noInterrupts();
        swing -= SWING_GRANULARITY;
        interrupts();
      }
      if (bitRead(recPins, 6) == 1) {
        noInterrupts();
        swing = 0;
        interrupts();
      }
      if (bitRead(recPins, 7) == 1) {
        noInterrupts();
        swing += SWING_GRANULARITY;
        interrupts();
      }
    }

    {                                                                            // trigger turner offer for the "scrolling through the pattern" section
      if (triggerKeys == true && triggerTimer + TRIG_LNGTH_MILLIS < millis()) {  // looks for the timer to be finished and the triggering tracker to be true
        triggerKeys = false;                                                     // make the triggering tracker be false, that's what this line does
        PORTB &= 0b11100011;                                                     // turns off top three things
        DDRB &= 0b11000011;                                                      // these two lines make pins be INPUTS
        DDRC &= 0b11110000;                                                      // these two lines make pins be INPUTS, which will make them be "off" in the cfg()
      }
    }
  }

  {                                              // STOPS the sequence clock
    if ((recPins == B00011000) && shift == 1) {  // this stops the clock while self-clocked (doesn't do anything when being clocked externally)
      STOP = true;                               // tracks if the module is stopped or not
      bitWrite(PORTC, 5, 0);                     // turn off the shift LED
      Timer1.stop();                             // stop the timer
      Vtriggering = false;                       // this will enter the timer cycle when it restarts all ready to go
      tapNum = -1;                               // get tap tempo mode back to ready for action later
      tapBucket = 0;                             // empty the tap tempo bucket variable
    }                                            // by the way, this is outside the "happens once per button press" part because what if the shift key is the last key pressed? It's not tracked by the "button press" section
  }

  {                                                   // whole sequence eraser
    if ((recPins == B01000010) && shift == 1) {       // this loooks for the middle column to all be pressed
      for (int i = 0; i < 256; i++) sequence[i] = 0;  // and runs throught he sequence writing 0s to everything
      FSMode = oldFSMode;                             // assumes you didn't take less than half a second to decide to delete the sequence
    }
  }

  {
    if (shift == false) {                                                // FSMode chooser
      if (bitRead(PIND, 2) == 0) {                                       //
        FSMode = 1;                                                      // these three if statements check the top 3 buttons
      }                                                                  // the oldFSMode stuff is so that when these buttons get pressed
      else if (bitRead(PIND, 3) == 0 && cfgDebounce + 300 < millis()) {  // for other reasons (deleting sequence)
        oldFSMode = FSMode;                                              //
        FSMode = 2;                                                      // or for selecting the sequence length,
        cfgDebounce = millis();                                          // and revert it back to the original FSMode
      } else if (bitRead(PIND, 4) == 0) {                                // last one
        FSMode = 3;                                                      //
      }
    }
    if (Vtriggering == false && triggerKeys == false) {  // checks this variable to see if the module is currently outputting triggers
      byte tempMode = 0;                                 // creates a variable to OR with the top three keys to show which mode is chosen
      bitWrite(tempMode, FSMode + 1, 1);                 // FSMode plus one moves the bitWrite pointer to the correct bit in tempMode
      PORTB &= 0b11100011;                               // blanks out the top three key LEDs
      PORTB |= tempMode;                                 // puts the tempMode bit where it needs to go
    }
  }

  {
    if (STOP == 0) {                                                 // this is the TAP TEMPO section!
      if (bitRead(PIND, 0) == LOW) {                                 // this is the shift key getting pressed down
        if (tapped == false && tapDebounce + 5 < millis()) {         // there's a 5 millisecond debounce period here
          tapped = true;                                             // tracks if the tap happened? Did it happen? Then this is true
          tapDebounce = millis();                                    // here's the timer variable for debouncing the tap
          if (tapNum == -1) {                                        // first tap! Fun!
            tapTime = micros();                                      // all that's done on the first time through is record the microsecond that the first tap happened
            tapNum = 0;                                              // first tap! Which is to say zeroth tap???
          } else {                                                   // WELL!!! I guess there was already a first tap
            unsigned long now = micros();                            // what time is it? Now is what time it is
            unsigned long increment = now - tapTime;                 // how long ago was the last tap? This is the calculation for that
            tapTime = now;                                           // This is the microsecond that this tap happened
            tapBucket += increment;                                  // this takes the new elapsed amount of time and adds it to this variable. 2nd tap is the first time we have a non-zero value
            tapNum++;                                                // and this will be the number of intervals we've measured so far
            if (tapNum > 4) {                                        // four taps in, that's when this starts to happen
              quarterNote = tapBucket / tapNum;                      // Here's the averaging function that divides the total elapsed time since the first tap with number of taps that have ocurred
              interval = ((quarterNote >> 2) - TRIG_LNGTH + TWEAK);  // hopefully this works without being inside noInterrupts(); and interrupts();
            }
            if (tapNum == 9) {          // on the ninth tap, we synchronize the flashy light... (losing up to in the sequence, not sure that's ideal) 3 steps
              bitWrite(stepNum, 0, 1);  // we're changing the stepNum variable to have two 0s in the least significant bit places, so the light will be on next time around?
              bitWrite(stepNum, 1, 1);  // we're changing the stepNum variable to have two 0s in the least significant bit places, so the light will be on next time around?
              triggering = true;        // we're gonna run the clockISR routine next, and we want it to trigger stuff
              Timer1.setPeriod(1);      // one microsecond from now we're gonna execute the timer
            }
          }
        }
      } else if (tapped > 0 && tapDebounce + 10 < millis()) {  // if tapped variable is a positive value and the debounce is done...
        tapped = 0;                                            // make the tapped variable zero
        tapDebounce = millis();                                // set the debounce timer
      }
      if (tapDebounce + 1000 < millis()) {  // do this if we've kinda finished tapping
        tapNum = -1;                        // the first tap will make this ZERO later on
        tapBucket = 0;                      // here's where the giant variable gets zeroed out
      }
    } /*  end tap tempo detect code   */
  }
}

void SLselect() {
  switch (seqNum) {  // selects the sequence length
    case 1:
      seqLength = 16;
      break;
    case 2:
      seqLength = 24;
      break;
    case 3:
      seqLength = 32;
      break;
    case 4:
      seqLength = 48;
      break;
    case 5:
      seqLength = 64;
      break;
    case 6:
      seqLength = 128;
      break;
    case 7:
      seqLength = 256;
  }
}
