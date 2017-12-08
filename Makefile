
TCLINC=-I/usr/include/tcl8.6

CPP=g++
CPPFLAGS=-fpic -O3 $(TCLINC)

all: vecexpr.so

vecexpr.so: vecexpr.o
	$(CPP) $(CPPFLAGS) -shared vecexpr.o -o vecexpr.so

vecexpr.tar.gz: vecexpr.cpp Makefile
	tar czf vecexpr.tar.gz vecexpr.cpp Makefile

clean:
	rm vecexpr.o vecexpr.so vecexpr.tar.gz
