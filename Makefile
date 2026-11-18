# Makefile for the Warehouse Robot Path and Task Planner (GNU Make 3.81 safe).
#
#   make            build planner
#   make test       build and run all tests
#   make clean      remove build products
#   make clean && make test SAN=1    sanitizers (needs WSL/Linux, not MinGW GCC 6)

CC     = gcc
CSTD   = -std=c11
WARN   = -Wall -Wextra
OPT    = -O2
CFLAGS = $(CSTD) $(WARN) -g $(OPT) -Isrc -Itests
LDFLAGS =

# Sanitizer build: make SAN=1
ifdef SAN
CFLAGS  = $(CSTD) $(WARN) -g -O0 -fsanitize=address,undefined -Isrc -Itests
LDFLAGS = -fsanitize=address,undefined
endif

# gcc on Windows adds .exe to the output name
ifeq ($(OS),Windows_NT)
EXE = .exe
endif

# Algorithm/support modules shared by the planner and the tests.
# Add each new module here as we build it (grid.c, heap.c, dijkstra.c, ...).
CORE_SRC = src/metrics.c src/problem.c src/io.c
CORE_OBJ = $(CORE_SRC:.c=.o)

MAIN_OBJ = src/main.o
TEST_OBJ = tests/run_tests.o tests/harness.o tests/test_metrics.o tests/test_parser.o

HEADERS = $(wildcard src/*.h) $(wildcard tests/*.h)

all: planner$(EXE)

planner$(EXE): $(MAIN_OBJ) $(CORE_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

run_tests$(EXE): $(TEST_OBJ) $(CORE_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

test: run_tests$(EXE)
	./run_tests$(EXE)

# Any .o is built from the .c with the same name; rebuild if a header changes.
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o tests/*.o planner$(EXE) run_tests$(EXE)

.PHONY: all test clean
