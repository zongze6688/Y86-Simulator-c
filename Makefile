CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

.PHONY: all test clean
all: cpu

cpu: cpu.c cpu_common.c cpu_common.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ cpu.c cpu_common.c $(LDLIBS)

test: cpu
	python3 test.py --bin ./cpu

clean:
	rm -f cpu
