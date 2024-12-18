protodemo: protodemo.c piolib/*.c protomatter.pio.h matrixmap.h include/*.h
	g++ -Og -ggdb -x c++ -Iinclude -Ipiolib/include -o $@ $(filter %.c, $^) -Wno-narrowing

matrixmap.h:

protomatter.pio.h: protomatter.pio assemble.py
	python assemble.py $< $@
