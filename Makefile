CXX = g++
CXXFLAGS = -std=c++11 -Wall

SRCS = main.cpp bmp.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = image_processor

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXEC)