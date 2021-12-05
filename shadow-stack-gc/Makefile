# This expects clang and llc to be available in PATH

all: gc
	./gc

gc: gc.s intr.cpp
	clang gc.s intr.cpp -o gc

gc.s: gc.ll
	llc gc.ll
