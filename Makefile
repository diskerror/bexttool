
#	Compiler
CP = clang++ -std=c++20

#	Boost version
BV = 1.81

CX = $(CP) -Wall -Winvalid-pch \
	-I/opt/local/libexec/boost/$(BV)/include \
	-L/usr/local/lib \
	-L/opt/local/libexec/boost/$(BV)/lib \
	-O2 -o $@

.PHONY: all clean

all: bexttool

bexttool: main.cp Wave.h Makefile
	$(CX) main.cp

clean:
	rm -f bexttool
