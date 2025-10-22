.PHONY: all
all: SelectServer

SelectServer: Main.cc log.cpp
	g++ -o $@ $^ -std=c++11

.PHONY: clean
clean:
	rm -f SelectServer