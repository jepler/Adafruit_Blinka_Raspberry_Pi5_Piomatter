protodemo: protodemo.c piolib/*.c include/piomatter/*.h include/piomatter/protomatter.pio.h
	g++ -std=c++20 -Og -ggdb -x c++ -Iinclude -Ipiolib/include -o $@ $(filter %.c, $^) -Wno-narrowing

matrixmap.h:

include/piomatter/protomatter.pio.h: protomatter.pio assemble.py
	python assemble.py $< $@
