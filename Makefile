CC = g++
CPPFLAGS += -Wall -Wextra -D_FILE_OFFSET_BITS=64 -std=c++11 -O3
LDFLAGS += -lc

DESTDIR ?= /usr/local/bin/

all: multitsccc

multitsccc: multitsccc.cpp
	$(CC) -o multitsccc multitsccc.cpp $(CPPFLAGS) $(LDFLAGS)

install: all
	install -m 755 multitsccc multitsccc_hls $(DESTDIR)
