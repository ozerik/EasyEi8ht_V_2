void extTempo() {                             // this just runs once every time the external clock gets a trigger
  if (tapNum == -1) {                         // first tap! Fun!
    tapTime = micros();                       // all that's done on the first time through is record the microsecond that the first tap happened
    tapNum = 0;                               // first tap! Which is to say zeroth tap???
  } else {                                    // WELL!!! I guess there was already a first tap
    unsigned long now = micros();             // what microsecond is it now??
    unsigned long increment = now - tapTime;  // how long ago was the last tap? This is the calculation for that
    tapTime = now;                            // This is the microsecond that this tap happened
    tapBucket += increment;                   // this takes the new elapsed amount of time and adds it to this variable. 2nd tap is the first time we have a non-zero value
    tapNum++;                                 // and this will be the number of intervals we've measured so far
    if (tapNum > 4) {                         // four taps in, that's when this happens
      interval = (tapBucket / tapNum);        // sets the interval variable as the time between external clock events
      quarterNote = interval << 2;            // telling the rest of the sketch what a quarter note is. Basically, the external clock's 16th note interval times 4
    }
  }
}