CXX = g++
CXXFLAGS = -O2 -std=c++20 -Wall

SRCS = $(wildcard *.cpp)
TARGET = code

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
