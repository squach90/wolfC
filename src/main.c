#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>
#include <math.h>

typedef float    f32;
typedef double   f64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAP_WIDTH 15
#define MAP_HEIGHT 15

// static u8 MAPDATA[MAP_HEIGHT * MAP_WIDTH] = {
//     1, 3, 1, 3, 1, 3,
//     3, 0, 0, 0, 0, 1,
//     1, 0, 2, 2, 0, 3,
//     3, 0, 0, 0, 0, 1,
//     1, 0, 4, 4, 0, 3,
//     3, 0, 0, 4, 0, 1,
//     1, 3, 1, 3, 1, 3
// };

static u8 MAPDATA[MAP_HEIGHT * MAP_WIDTH] = {
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    1, 3, 2, 2, 2, 4, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1

};

struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
} state;

static void verline(int x, int y0, int y1, u32 color) {
    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;
    for (int y = y0; y <= y1; y++) {
        state.pixels[y * SCREEN_WIDTH + x] = color;
    }
}

int main(/*int argc, char** argv*/) {

    SDL_Init(SDL_INIT_VIDEO);

      state.window = SDL_CreateWindow("SDL2 Minimal",
          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          SCREEN_WIDTH, SCREEN_HEIGHT, 0);

      state.renderer = SDL_CreateRenderer(state.window, -1, SDL_RENDERER_ACCELERATED);
      state.texture = SDL_CreateTexture(state.renderer,
          SDL_PIXELFORMAT_ARGB8888,
          SDL_TEXTUREACCESS_STREAMING,
          SCREEN_WIDTH, SCREEN_HEIGHT);


    float petitPas = 0.01f;
    
    float playerX = 1.5f;
    float playerY = 1.5f;

    float playerRot = 0.0f;
    float playerFOV = 1.0472f;

    const f32 movespeed = 3.0f * 0.016f;
    const f32 rotspeed  = 1.5f * 0.016f;


    bool running = true;
    SDL_Event event;
    while (running) {
        const u8 *keystate = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Effacer le buffer pixels
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
            state.pixels[i] = 0xFF000000;

        if (keystate[SDL_SCANCODE_UP]) {
            float nx = playerX + cosf(playerRot) * movespeed;
            float ny = playerY + sinf(playerRot) * movespeed;
            if (MAPDATA[(int)ny * MAP_WIDTH + (int)nx] == 0) {
                playerX = nx;
                playerY = ny;
            }
        }

        if (keystate[SDL_SCANCODE_DOWN]) {
            float nx = playerX - cosf(playerRot) * movespeed;
            float ny = playerY - sinf(playerRot) * movespeed;
            if (MAPDATA[(int)ny * MAP_WIDTH + (int)nx] == 0) {
                playerX = nx;
                playerY = ny;
            }
        }
        
        if (keystate[SDL_SCANCODE_LEFT]) {
          playerRot -= rotspeed;
        } 
        if (keystate[SDL_SCANCODE_RIGHT]) {
          playerRot += rotspeed;
        } 


        for (int col = 0; col < SCREEN_WIDTH; col++) {
            float angleDuRayon = playerRot - playerFOV / 2 + ((float)col / SCREEN_WIDTH) * playerFOV;
            float dx = cosf(angleDuRayon);
            float dy = sinf(angleDuRayon);

            float x = playerX;
            float y = playerY;

            u8 val = 0;

            while (1) {
                x += dx * petitPas;
                y += dy * petitPas;

                int mapX = (int)x;
                int mapY = (int)y;
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT || MAPDATA[mapY * MAP_WIDTH + mapX] == 1)
                    break;
                val = MAPDATA[mapY * MAP_WIDTH + mapX];
                if (val != 0) break; // mur touch
            }

            u32 color = 0xFFFFFFFF;
            switch (val) {
                case 1: color = 0xFF0000FF; break; // bleu
                case 2: color = 0xFF00FF00; break; // vert
                case 3: color = 0xFFFF0000; break; // rouge
                case 4: color = 0xFFFF00FF; break; // viloet
            }

            float distance = sqrtf((x - playerX) * (x - playerX) + (y - playerY) * (y - playerY));
            // correct fish-eye
            distance *= cosf(angleDuRayon - playerRot);

            float factor = 0.65f; // + grand -> murs + loin
            float hauteurColonne = SCREEN_HEIGHT / (distance * factor);
            int y0 = SCREEN_HEIGHT / 2 - (int)(hauteurColonne / 2);
            int y1 = SCREEN_HEIGHT / 2 + (int)(hauteurColonne / 2);

            
            verline(col, y0, y1, color);
        }

        SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH * sizeof(u32));
        SDL_RenderClear(state.renderer);
        SDL_RenderCopy(state.renderer, state.texture, NULL, NULL);
        SDL_RenderPresent(state.renderer);
    }

    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();

    return 0;
}
