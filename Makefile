.PHONY: default help clean clean-all force
CC=cc
CXX=g++
CFLAGS=-Wall -c -g -std=c++11 -DLEVELDB_PLATFORM_POSIX -DHAVE_O_CLOEXEC
CC_OPTS=.


TARGETS=leveldb
default: $(TARGETS)

SOURCES=$(wildcard *.cc)
OBJECTS=$(patsubst %.cc,%.o,$(SOURCES))

LIBS=-lpthread

leveldb: $(OBJECTS)
	$(CXX) $(OBJECTS)  -o $(TARGETS)

%.o : %.cc
	$(CXX) $(CFLAGS) $(CC_OPTS)  $< -o $@
    
clean:
	rm -f *.o
	rm -f $(TARGETS)
