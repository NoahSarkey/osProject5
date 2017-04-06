#!/bin/sh

for numframes in 1 2 3 4 5 6 7 8
do
	echo "Numpages: 8 Numframes: $numframes"
	echo "RAND"
	./virtmem 8 $numframes rand sort
	echo "CUSTOM"
	./virtmem 8 $numframes custom sort
	echo "FIFO"
	./virtmem 8 $numframes fifo sort
	echo " "
	echo " "
	echo " "
done