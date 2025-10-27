CXX = gcc
CXXFLAGS = -Wall -Wextra
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)
TARGET = wolfC
SRC = src/main.c

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SRC) -o $(TARGET) $(SDL_LDFLAGS)

clean:
	rm -f $(TARGET)
