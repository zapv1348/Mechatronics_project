CC=g++

CFLAGS = -O3 $(shell pkg-config --cflags opencv) -lpthread -lwiringPi
LIBS = $(shell pkg-config --libs opencv) 

capture : capture.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) 

clean:
	rm -f *.o capture
