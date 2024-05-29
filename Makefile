
TCLINC=-I/usr/include/tcl8.6

CPP=g++
CPPFLAGS=-fPIC -O3 $(TCLINC) -pedantic

all: vecexpr.so

vecexpr.so: vecexpr.o
	$(CPP) $(CPPFLAGS) $^ -shared -o $@

vecexpr.tar.gz: vecexpr.cpp Makefile
	tar czf vecexpr.tar.gz vecexpr.cpp Makefile

clean:
	rm vecexpr.o vecexpr.so vecexpr.tar.gz
