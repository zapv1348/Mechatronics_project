CC=g++

CFLAGS = -O3 $(shell pkg-config --cflags opencv)
LIBS = $(shell pkg-config --libs opencv) -lrt

final : final.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) 

clean:
	rm -f *.o final
