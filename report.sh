#!/bin/bash

cd "$(dirname "$0")"

if [ ! -x report ]
then
	echo Application not found. Maybe you need to compile it using "make"? >&2
	exit 1
fi

while [ 1 ]
do
	./report 2>/dev/null | xargs wget -qO/dev/null
	sleep 15
done
