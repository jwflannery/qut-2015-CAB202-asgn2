#
# 	CAB202 Tutorial 9: Debouncing, timers, & interrupts
#	Template for compiling with floating point printf
#
#	B.Talbot, September 2015
#	Queensland University of Technology
#

# Modify these
SRC=faces
CAB202_LIB_DIR=../cab202_teensy

# The rest should be all good as is
FLAGS=-mmcu=atmega32u4 -Os -DF_CPU=8000000UL -std=gnu99 -Wall -Werror
LIBS=-Wl,-u,vfprintf -lprintf_flt -lcab202_teensy -lm

all:
	avr-gcc $(SRC).c usb_serial.c $(FLAGS) -I$(CAB202_LIB_DIR) -L$(CAB202_LIB_DIR) $(LIBS) -o $(SRC).o
	avr-objcopy -O ihex $(SRC).o $(SRC).hex
