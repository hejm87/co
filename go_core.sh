#!/bin/sh
  
ulimit -c unlimited

echo "param count:$#\n"

if [ $# -lt 1 ]; then
    echo "usage[0] >> exec file\n"
    exit
fi

i = 0
while true; do
	rm -f core
	./$1
	if [ -f "./core" ]; then
		echo "hahaha"
		break
	fi
	i=$(($i+1))
done
echo "create core file"
echo $i
