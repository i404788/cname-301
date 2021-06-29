DOMAIN ?= r.devd.pw
DEFINES ?= -DDOMAIN="$(DOMAIN)"
CXX ?= clang++
CXXFLAGS ?= --std=c++17 -O2 $(DEFINES)#-Werror -Wall
LIBS ?= -lcttpd -luriparser -lfmt -lpthread

MAIN_SOURCES=$(wildcard ./*.cpp)
MAIN_HEADERS=$(wildcard ./*.h) $(wildcard ./*.hpp)
MAIN_OBJECTS=$(MAIN_SOURCES:.cpp=.o)

all: main

main: $(MAIN_OBJECTS) $(MAIN_SOURCES)
	$(CXX) $(CXXFLAGS) $(MAIN_OBJECTS) $(LIBS) -o main

%.o: %.cpp $(MAIN_HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f ./main
	@rm -rf ./*.o
