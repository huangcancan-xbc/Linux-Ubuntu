.PHONY: all
all: EpollServer

EpollServer: Main.cc log.cpp
	g++ -o $@ $^ -std=c++11

.PHONY: clean
clean:
	rm -f EpollServer