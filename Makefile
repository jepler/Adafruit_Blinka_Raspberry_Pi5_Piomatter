protodemo: protodemo.c piolib/*.c | protomatter.pio.h
	g++ -Og -ggdb -x c++ -Ipiolib/include -o $@ $^ -Wno-narrowing

protomatter.pio.h: protomatter.pio assemble.py
	python assemble.py $< $@
