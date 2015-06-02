CC=$(shell which gcc)
CXX=$(shell which g++)
CFLAGS=-Wall -g -O -std=c99 -pedantic -fPIC
CXXFLAGS=-Wall -g -O -pedantic -fPIC
PKGCONFIG=$(shell which pkg-config)
GLEWINC=$(shell $(PKGCONFIG) --cflags glew)
GLEWLIB=$(shell $(PKGCONFIG) --libs glew)
INC=-I$(PWD) $(GLEWINC)
LDFLAGS=
GLLIBS=-lglut -lGL $(GLEWLIB)

default: teatime

clean:
	rm -f teatime *.o

.PHONY: default clean

teatime: teatime.o teapot.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(GLLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $^
