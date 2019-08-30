#!/bin/bash

# 只有一个参数
if [ $# != 1 ]; then
	echo "error, pls input param, 0-none boot, 1-app1, 2-app2"
	echo "eg: ./build.sh 1"
	exit
fi

if [ $1 == 0 ]; then
	make COMPILE=gcc BOOT=none APP=0 SPI_SPEED=40 SPI_MODE=DIO SPI_SIZE_MAP=2 EXTRA_CCFLAGS="-DGLOBAL_DEBUG"
elif [ $1 == 1 ]; then
	make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=40 SPI_MODE=DIO SPI_SIZE_MAP=2 EXTRA_CCFLAGS="-DGLOBAL_DEBUG -DGLOBAL_DEBUG_ON"
elif [ $1 == 2 ]; then
	make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=40 SPI_MODE=DIO SPI_SIZE_MAP=2 EXTRA_CCFLAGS="-DGLOBAL_DEBUG -DGLOBAL_DEBUG_ON"
fi

