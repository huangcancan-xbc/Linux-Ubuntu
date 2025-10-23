.PHONY: all
all: PollServer

PollServer: Main.cc log.cpp
	g++ -o $@ $^ -std=c++11

.PHONY: clean
clean:
	rm -f PollServer