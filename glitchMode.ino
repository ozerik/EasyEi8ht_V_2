byte glitchMode() {
  if (recPins == 0) {
    glitchStep = 0;
    glitchStepCount = 0;
    doubleTime = 0;
    cylon = 0;
    return (sequence[stepNum] & mute);
  } else {  // GLITCH MODE ENGAGED!!!

    if (glitchStep == 0) {  // this part stores the glitch step
      if (micros() - lastClock < (interval >> 1)) {
        glitchStep = stepNum;
      } else {
        short TGS = stepNum - 1;
        if (TGS == 0) TGS = seqLength;
        glitchStep = TGS;
      }
      doubleTime = 1;
    }

    if (bitRead(recPins, 0) == 1) {
      return (sequence[glitchStep] & mute);
    }


    if (bitRead(recPins, 1) == 1) {
      glitchStepCount = !glitchStepCount;
      if (glitchStepCount + glitchStep > seqLength) {
        return (sequence[1] & mute);
      } else return (sequence[glitchStep + glitchStepCount] & mute);
    }


    if (bitRead(recPins, 2) == 1) {
      glitchStepCount++;
      if (glitchStepCount > 2) glitchStepCount = 1;
      if (glitchStepCount + glitchStep - 1 > seqLength) {
        return (sequence[glitchStep + glitchStepCount - 1 - seqLength] & mute);
      } else return (sequence[glitchStep + glitchStepCount - 1] & mute);
    }


    if (bitRead(recPins, 3) == 1) {
      glitchStepCount++;
      if (glitchStepCount > 4) glitchStepCount = 1;
      if (glitchStepCount + glitchStep - 1 > seqLength) {
        return (sequence[glitchStep + glitchStepCount - 1 - seqLength] & mute);
      } else return (sequence[glitchStep + glitchStepCount - 1] & mute);
    }

    if (bitRead(recPins, 4) == 1) {
      glitchStepCount++;
      if (glitchStepCount > 6) glitchStepCount = 1;
      if (glitchStepCount + glitchStep - 1 > seqLength) {
        return (sequence[glitchStep + glitchStepCount - 1 - seqLength] & mute);
      } else return (sequence[glitchStep + glitchStepCount - 1] & mute);
    }

    if (bitRead(recPins, 5) == 1) {
      if (cylon == 0) {
        glitchStepCount++;
        if (glitchStepCount > 4) cylon = 1;
      } else {
        glitchStepCount--;
        if (glitchStepCount < 2) cylon = 0;
      }
      if (glitchStepCount + glitchStep - 1 > seqLength) {
        return (sequence[glitchStep + glitchStepCount - 1 - seqLength] & mute);
      } else return (sequence[glitchStep + glitchStepCount - 1] & mute);
    }


    if (bitRead(recPins, 6) == 1) {
      if (cylon == 0) {
        glitchStepCount++;
        if (glitchStepCount > 9) cylon = 1;
      } else {
        glitchStepCount--;
        if (glitchStepCount < 2) cylon = 0;
      }
      if (glitchStepCount + glitchStep - 1 > seqLength) {
        return (sequence[glitchStep + glitchStepCount - 1 - seqLength] & mute);
      } else return (sequence[glitchStep + glitchStepCount - 1] & mute);
    }


    if (bitRead(recPins, 7) == 1) {
      if (cylon == 0) {
        cylon = 1;
        for (int i = 1001; i < 1010; i++) {
          sequence[i] = sequence[random(1, seqLength)];
        }
      }
      glitchStepCount++;
      if (glitchStepCount > 8) glitchStepCount = 1;
      return (sequence[1000 + glitchStepCount] & mute);
    }
  }
}
