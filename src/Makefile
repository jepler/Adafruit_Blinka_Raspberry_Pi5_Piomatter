protodemo: protodemo.c piolib/*.c include/piomatter/*.h include/piomatter/protomatter.pio.h Makefile
	g++ -std=c++20 -O3 -ggdb -x c++ -Iinclude -Ipiolib/include -o $@ $(filter %.c, $^) -Wno-narrowing

matrixmap.h:

include/piomatter/protomatter.pio.h: protomatter.pio assemble.py
	python assemble.py $< $@
