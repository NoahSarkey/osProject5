#!/bin/sh

for numframes in 48 55 65 75 85 95 100
do
	#num1 = numframes/numpages;
	#if (( $(echo "$num1 > $num2" |bc -l) )); then
	./virtmem 100 $numframes custom sort
	#fi
done