CC = cc
CFLAGS = -std=gnu11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test clean
all: sttyl

sttyl: sttyl.c

test: sttyl
	python3 -m unittest discover -s tests -v

clean:
	rm -f sttyl
