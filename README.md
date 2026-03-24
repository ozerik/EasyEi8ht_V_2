Here's version 2 of the EasyEi8ht. (2026-March)

It's a performance oriented trigger sequencer, powered by an Arduino Nano. There's nine mechanical keyswitches, one for each of the 8 channels, and one shift button. There's LEDs under each keyswitch for visual feedback. There's eight outputs that correspond to the patterns tapped in to each keyswitch.

The module can clock itself, set by tap-tempo. The pattern can loop up to 256 ticks (16th notes, nominally), it can do swing, accept an external trigger, send a clock out to whererever, and do sorta freestyle beats while the pattern is running.

There are issues that keep popping up, and I'm NOT a trained coder, but I've tried to make the code readable with lots of comments, and, well, it makes sense (barely) to my brain.
