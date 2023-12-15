rem Make JUmpong for the 5200
ca65 mycartname.s
cl65 -t atari5200 -C atari5200_with_font.cfg -Or -m map.txt -o jumpongV3.bin jumpong.c mycartname.o
copy jumpongV3.bin C:\Users\dev7\Documents\GitHub\Jum52OO\driver_VS2019SDL