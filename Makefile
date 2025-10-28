CXX = gcc
CXXFLAGS = -Wall -Wextra
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)
SDL_IMG_CFLAGS = -I/opt/homebrew/include/SDL2/SDL2_image
SDL_IMG_LDFLAGS = -lSDL2_image

TARGET = wolfC
SRC = src/main.c

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SDL_IMG_CFLAGS) $(SRC) -o $(TARGET) $(SDL_LDFLAGS) $(SDL_IMG_LDFLAGS)

clean:
	rm -f $(TARGET)
