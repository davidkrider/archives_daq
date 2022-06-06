This is a collection of programs to automate the digitazation of cassette tapes, per this writeup: https://davidkrider.com/tape-digitizer/

`digitize4.c` will start a (mono) tape duplicator, though a digital output (by way of a powered relay), wait for it to double check the tape is rewound through a digital input, then sample the signals as it "duplicates" the tape to an empty bay, write a WAV file for each side of the tape (side B will be reversed of course), and then stop.

`process.sh` will call `sox` to trim the leader, fade it out, flip it, fade it out again, then flip the side B file, and paste it to the end of side A, then normalize the volume and resample it for an MP3, and then finally convert it to FLAC.
 
