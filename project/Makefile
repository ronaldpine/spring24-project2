CC=g++
CFLAGS=-I. -lcrypto
DEPS=crypto-helpers.cpp
USERID=123456789

HELPER_SRCS=$(wildcard helpers/*.cpp)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: server client
server: server.o $(HELPER_SRCS:.cpp=.o)
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o $(HELPER_SRCS:.cpp=.o)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o helpers/*.o server client *.tar.gz

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .