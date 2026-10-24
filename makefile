CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS  := -lz

SRC := $(wildcard src/*.cpp) $(wildcard src/commands/*.cpp)
OBJ := $(SRC:.cpp=.o)

mygit: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) mygit

.PHONY: clean
