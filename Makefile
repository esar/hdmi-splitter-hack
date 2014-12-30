all:
	avr-gcc -g -O3 -mmcu=attiny9 -mno-short-calls main.c -o main
	avr-objcopy -O ihex main main.hex
	avrdude -p attiny9 -c avrispmkii -P usb -U flash:w:main.hex 
