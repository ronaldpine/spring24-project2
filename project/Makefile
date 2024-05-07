CXX=g++
CXXFLAGS=-I. -lcrypto -Wno-deprecated-declarations
DEPS=crypto-helpers.cpp
USERID=123456789

HELPER_SRCS=$(wildcard helpers/*.cpp)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $(CXXFLAGS)

all: server client
server: server.o $(HELPER_SRCS:.cpp=.o)
	$(CXX) -o $@ $^ $(CXXFLAGS)

client: client.o $(HELPER_SRCS:.cpp=.o)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf *.o helpers/*.o server client *.tar.gz

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .