#!/bin/env ruby

SCRATCH = "/local/Scratch"
PRODUCTION = "/local/Production"

magazine = File.open('magazine.txt', 'r')

magazine.each_line do |tape|

	tape.chomp!
	p tape

	# Need to modify digitize3.c to take an argument to name the files,
	# and product a "name"-a.wav and -b.wav.
	#`digitize3 {tape}`

	# Spin off a second C program to stop the rewinding and run the robot,
	# or tack that onto the first program?

	done = "#{PRODUCTION}/#{tape}.wav"
	sideA = "#{SCRATCH}/#{tape}-a.wav"
	sideB = "#{SCRATCH}/#{tape}-b.wav"
	side1 = "#{PRODUCTION}/#{tape}-1.wav"
	side2 = "#{PRODUCTION}/#{tape}-2.wav"
	temp = "#{SCRATCH}/temp.wav"

	`sox "#{sideA}" "#{temp}" reverse`
	`sox "#{temp}" "#{side1}" silence 2 1 5%`
	`sox "#{side1}" "#{temp}" fade q 5`
	`sox "#{temp}" "#{side1}" reverse`

	`sox "#{sideB}" "#{temp}" reverse`
	`sox "#{temp}" "#{side2}" silence 2 1 5%`
	`sox "#{side2}" "#{temp}" fade q 5`
	`mv "#{temp}" "#{side2}"`

	# The stat function returns on STDERR. %x[] captures STDOUT.
	v1 = %x[sox "#{side1}" -e stat -v 2>&1]
	v2 = %x[sox "#{side2}" -e stat -v 2>&1]
	v1.chomp!; v2.chomp!
	v1 > v2 ? v = v2 : v = v1
	p "sox -v #{v} #{side1} -v #{v} #{side2} #{temp}"
	`sox -v "#{v}" "#{side1}" -v "#{v}" "#{side2}" "#{temp}"`
	`rm "#{side1}" "#{side2}"`

	`sox "#{temp}" -r44100 "#{done}" resample`

	#ATTACK=0.3
	#DECAY=0.8
	#FLOOR=-90
	#DELAY=0.2
	#`sox "#{done}" "#{temp}" mcompand \
	#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY" 1600
	#	"$ATTACK,$DECAY $FLOOR 12 -90 $DELAY" 4700
	#	"$ATTACK,$DECAY $FLOOR -3 -90 $DELAY" 8900
	#	"$ATTACK,$DECAY $FLOOR 3 -90 $DELAY" 11050
	#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY" 15700
	#	"$ATTACK,$DECAY $FLOOR 0 -90 $DELAY"`
	#`mv "#{temp}" "#{done}"`

	`flac "#{done}"`
	`rm "#{done}"`

end
