N=$1
A=a.wav
B=b.wav
C=c.wav
D=d.wav
T=t.wav

sox $A $T reverse
sox $T $C silence 2 1 5%
sox $C $T fade q 5
sox $T $C reverse

sox $B $T reverse
sox $T $D silence 2 1 5%
sox $D $T fade q 5
mv $T $D

sox b.wav -e stat -v
#sox --norm $C $D $T
sox $C $D $T
sox $T -r44100 $N.wav resample

#ATTACK=0.3
#DECAY=0.8
#FLOOR=-90
#DELAY=0.2
#sox $N.wav $T mcompand \
#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY" 1600
#	"$ATTACK,$DECAY $FLOOR 12 -90 $DELAY" 4700
#	"$ATTACK,$DECAY $FLOOR -3 -90 $DELAY" 8900
#	"$ATTACK,$DECAY $FLOOR 3 -90 $DELAY" 11050
#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY" 15700
#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY"
#mv $T $N.wav

flac $N.wav
