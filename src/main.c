#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>
#include <math.h>

typedef float    f32;
typedef double   f64;
typedef uint8_t  u8;
typedef uint32_t u32;

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 480
#define MAP_WIDTH 15
#define MAP_HEIGHT 15
#define MAX_TEXTURES 15
#define GUN_FRAMES 5
#define MAX_DOORS 10

// Map : 0 = empty, 1-6 blue wall, 11 = doors
static u8 MAPDATA[MAP_HEIGHT * MAP_WIDTH] = {
    1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,11,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,2,1,1,2,1,1,3,1,1,2,1,1,2,1
};

static bool COLLISION_MAP[MAP_HEIGHT * MAP_WIDTH];


// Textures Struct
typedef struct {
    u32* pixels;
    int width;
    int height;
} Texture;

// Engine States
struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    Texture textures[MAX_TEXTURES];
} state;


typedef struct {
    float x;
    float y;
    int textureId;
    bool visible;
    float distance;
    float angle;
} Sprite;

typedef struct {
    int x, y;
    int texNorth;
    int texSouth;
    int texWest;
    int texEast;
} SpecialBlock;

typedef struct {
    int x, y;
    bool isClosed;
} Door;


Door doors[MAX_DOORS];
int doorCount = 0;

// list of special block (with differents faces)
SpecialBlock specialBlocks[] = {
    {5, 8, 1, 10, 1, 1}, // x, y , textuID, textuID, textuID, textuID
    {5, 6, 10, 1, 1, 1},
    {9, 8, 1, 10, 1, 1},
    {9, 6, 10, 1, 1, 1},
    {5, 13, 1, 10, 1, 1},
    {5, 11, 10, 1, 1, 1},
    {9, 13, 1, 10, 1, 1},
    {9, 11, 10, 1, 1, 1},
};

int numSpecialBlocks = sizeof(specialBlocks) / sizeof(specialBlocks[0]);


SDL_Surface* gHelloWorld = NULL;
SDL_Texture* gHelloWorldTex = NULL;

SDL_Texture* gunFrames[GUN_FRAMES];


// Load PNG Textures
SDL_Surface* load_texture(const char* path) {
    SDL_Surface* surf = IMG_Load(path);
    if (!surf) {
        printf("Erreur chargement texture %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Surface* formatted = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(surf);
    return formatted;
}

bool loadGunTextures(SDL_Renderer* renderer) {
    for (int i=0; i<GUN_FRAMES; i++) {
        char path[64];
        sprintf(path, "assets/gun/sprite_%d.bmp", i);
        SDL_Surface* surf = SDL_LoadBMP(path);
        if (!surf) return false;

        SDL_SetColorKey(surf, SDL_TRUE, SDL_MapRGB(surf->format, 0x98,0x00,0x88));

        gunFrames[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!gunFrames[i]) return false;
    }
    return true;
}

bool load_texture_to_slot(const char* path, int slot) {
    if (slot >= MAX_TEXTURES) return false;
    SDL_Surface* surf = load_texture(path);
    if (!surf) return false;

    state.textures[slot].pixels = (u32*)malloc(surf->w * surf->h * sizeof(u32));
    if (state.textures[slot].pixels) {
        memcpy(state.textures[slot].pixels, surf->pixels, surf->w * surf->h * sizeof(u32));
        state.textures[slot].width = surf->w;
        state.textures[slot].height = surf->h;
    }
    SDL_FreeSurface(surf);
    return state.textures[slot].pixels != NULL;
}

bool loadMedia(SDL_Renderer* renderer) {
    gHelloWorld = SDL_LoadBMP("assets/gun/sprite_0.bmp");
    if (!gHelloWorld) {
        printf("Unable to load image! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    // Convert surface in textures for SDL_RenderCopy
    SDL_SetColorKey(gHelloWorld, SDL_TRUE, 0x980088);
    gHelloWorldTex = SDL_CreateTextureFromSurface(renderer, gHelloWorld);
    if (!gHelloWorldTex) {
        printf("Unable to create texture from BMP! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

// Draw a textured colonne with correct mapping
void verline_tex(int x, int y0, int y1, Texture* tex, int texX, f32 wallHeight, u32 fallbackColor, f32 lightFactor) {
    if (!tex || !tex->pixels) {
        if (y0 < 0) y0 = 0;
        if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;
        for (int y = y0; y <= y1; y++) {
            u32 col = fallbackColor;
            // apply shading
            u8 r = ((col >> 16) & 0xFF) * lightFactor;
            u8 g = ((col >> 8) & 0xFF) * lightFactor;
            u8 b = (col & 0xFF) * lightFactor;
            state.pixels[y * SCREEN_WIDTH + x] = (col & 0xFF000000) | (r << 16) | (g << 8) | b;
        }
        return;
    }

    f32 step = (f32)tex->height / wallHeight;
    f32 texPos = 0;
    if (y0 < 0) { texPos = -y0 * step; y0 = 0; }
    if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;

    for (int y = y0; y <= y1; y++) {
        int texY = (int)texPos % tex->height;
        if (texY < 0) texY += tex->height;

        u32 col = tex->pixels[texY * tex->width + texX];

        // apply shading
        u8 r = ((col >> 16) & 0xFF) * lightFactor;
        u8 g = ((col >> 8) & 0xFF) * lightFactor;
        u8 b = (col & 0xFF) * lightFactor;

        state.pixels[y * SCREEN_WIDTH + x] = (col & 0xFF000000) | (r << 16) | (g << 8) | b;

        texPos += step;
    }
}


// Checking collision
bool check_collision(f32 x, f32 y) {
    int mapX = (int)x;
    int mapY = (int)y;

    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
        return true; // coll if off map

    return COLLISION_MAP[(MAP_HEIGHT - 1 - mapY) * MAP_WIDTH + mapX];
}

int getDoorInFront(float playerX, float playerY, float playerRot) {
    float nx = playerX + cosf(playerRot) * 0.5f;
    float ny = playerY + sinf(playerRot) * 0.5f;

    for (int i = 0; i < doorCount; i++) {
        float dx = nx - (doors[i].x + 0.5f);
        float dy = ny - (doors[i].y + 0.5f);
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1.2f) {
            return i;
        }
    }
    return -1;
}



int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    state.window = SDL_CreateWindow("SDL2 Raycaster",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    state.renderer = SDL_CreateRenderer(state.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    state.texture = SDL_CreateTexture(state.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    // load all textures for world
    load_texture_to_slot("assets/bluestone.png", 1);
    load_texture_to_slot("assets/bluestone-grid.png", 2);
    load_texture_to_slot("assets/bluestone-grid-skull.png", 3);
    load_texture_to_slot("assets/bluestone-stand.png", 4);
    load_texture_to_slot("assets/bluewall-death.png", 5);
    load_texture_to_slot("assets/bluewall-sign.png", 6);
    load_texture_to_slot("assets/redbrick.png", 7);
    load_texture_to_slot("assets/wood.png", 8);
    load_texture_to_slot("assets/greystone.png", 9);
    load_texture_to_slot("assets/doorslot.png", 10);
    load_texture_to_slot("assets/door.png", 11);

    // load textu for gun
    if (!loadGunTextures(state.renderer)) {
        printf("Failed to load gun textures!\n");
        return 1;
    }

    // Player
    f32 playerX = 1.5f, playerY = 7.5f, playerRot = 0.0f;
    f32 playerFOV = 1.047f;
    u32 ciel = 0x383838;
    u32 sol  = 0x707070;

    bool isShooting = false;
    int gunFrame = 0;
    u32 lastGunTime = 0;
    u32 gunDelay = 30; // time in ms betw 2 frames

    bool running = true;
    SDL_Event event;
    u32 lastTime = SDL_GetTicks();

    doors[doorCount++] = (Door){5, 7, true};
    doors[doorCount++] = (Door){9, 7, false};


    for (int i = 0; i < MAP_HEIGHT * MAP_WIDTH; i++) {
        // default all wall are solid
        COLLISION_MAP[i] = MAPDATA[i] != 0;
    }

    for (int i = 0; i < doorCount; i++) {
        int mapIndex = (MAP_HEIGHT - 1 - doors[i].y) * MAP_WIDTH + doors[i].x;
        COLLISION_MAP[mapIndex] = doors[i].isClosed;
    }



    while (running) {
        u32 currentTime = SDL_GetTicks();
        f32 deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        const u8 *keystate = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Player mouvements
        f32 moveSpeed = 6.5f * deltaTime;
        f32 rotSpeed = 3.5f * deltaTime;
        f32 newX = playerX, newY = playerY;

        if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) {
            newX += cosf(playerRot) * moveSpeed;
            newY += sinf(playerRot) * moveSpeed;
        }
        if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) {
            newX -= cosf(playerRot) * moveSpeed;
            newY -= sinf(playerRot) * moveSpeed;
        }
        if (!check_collision(newX, newY)) {
            playerX = newX;
            playerY = newY;
        }

        if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) playerRot += rotSpeed;
        if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) playerRot -= rotSpeed;

        // background sky/ground
        for (int y = 0; y < SCREEN_HEIGHT / 2; y++)
            for (int x = 0; x < SCREEN_WIDTH; x++)
                state.pixels[y * SCREEN_WIDTH + x] = ciel;
        for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++)
            for (int x = 0; x < SCREEN_WIDTH; x++)
                state.pixels[y * SCREEN_WIDTH + x] = sol;

        // Raycasting
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            f32 cameraX = 2.0f * col / (f32)SCREEN_WIDTH - 1.0f;
            f32 rayDirX = cosf(playerRot) + sinf(playerRot) * cameraX * tanf(playerFOV / 2);
            f32 rayDirY = sinf(playerRot) - cosf(playerRot) * cameraX * tanf(playerFOV / 2);

            int mapX = (int)playerX, mapY = (int)playerY;
            f32 deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
            f32 deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);

            int stepX = (rayDirX < 0) ? -1 : 1;
            int stepY = (rayDirY < 0) ? -1 : 1;

            f32 sideDistX = (rayDirX < 0) ? (playerX - mapX) * deltaDistX : (mapX + 1.0f - playerX) * deltaDistX;
            f32 sideDistY = (rayDirY < 0) ? (playerY - mapY) * deltaDistY : (mapY + 1.0f - playerY) * deltaDistY;

            u8 val = 0; int side = 0; f32 perpDist = 0;
            while (val == 0) {
                if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
                else { sideDistY += deltaDistY; mapY += stepY; side = 1; }
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) break;
                val = MAPDATA[(MAP_HEIGHT - 1 - mapY) * MAP_WIDTH + mapX];
            }
            if (val == 0) continue;

            if (side == 0) perpDist = (mapX - playerX + (1 - stepX) / 2) / rayDirX;
            else perpDist = (mapY - playerY + (1 - stepY) / 2) / rayDirY;

            f32 hitWallX = (side == 0) ? playerY + perpDist * rayDirY : playerX + perpDist * rayDirX;
            hitWallX -= floorf(hitWallX);

            Texture* tex = NULL;

            // search for special blocks
            for (int i = 0; i < numSpecialBlocks; i++) {
                if (mapX == specialBlocks[i].x && mapY == specialBlocks[i].y) {
                    if (side == 0) tex = (rayDirX > 0) ? &state.textures[specialBlocks[i].texEast]
                                                      : &state.textures[specialBlocks[i].texWest];
                    else           tex = (rayDirY > 0) ? &state.textures[specialBlocks[i].texSouth]
                                                      : &state.textures[specialBlocks[i].texNorth];
                    break;
                }
            }

            // if no special blocks
            if (!tex && val > 0 && val < MAX_TEXTURES) {
                tex = &state.textures[val];
            }



            int texX = 0;
            if (tex) {
                texX = (int)(hitWallX * (float)tex->width);
                if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
                    texX = tex->width - texX - 1;
            }

            f32 wallHeight = SCREEN_HEIGHT / perpDist;
            int y0 = SCREEN_HEIGHT / 2 - (int)(wallHeight / 2);
            int y1 = SCREEN_HEIGHT / 2 + (int)(wallHeight / 2);

            f32 lightFactor = 1.0f / (1.0f + perpDist * 0.15f);
            if (lightFactor < 0.4f) lightFactor = 0.4f;

            verline_tex(col, y0, y1, tex, texX, wallHeight, 0xFFFFFFFF, lightFactor);
        }

        
        // detect if lctrl is pressed for the animation
        static bool prevCtrl = false;
        bool currCtrl = keystate[SDL_SCANCODE_LCTRL];

        if (currCtrl && !prevCtrl) {
            isShooting = true;
            gunFrame = 0;
            lastGunTime = SDL_GetTicks();
        }
        prevCtrl = currCtrl;

        // do the animation
        if (isShooting) {
            u32 now = SDL_GetTicks();
            if (now - lastGunTime > gunDelay) {
                gunFrame++;
                lastGunTime = now;
                if (gunFrame >= GUN_FRAMES) {
                    gunFrame = 0;
                    isShooting = false; // end of the animations
                }
            }
        } else {
            gunFrame = 0; // Initial frame if no shoot
        }

        static bool prevSpace = false;
        bool currSpace = keystate[SDL_SCANCODE_SPACE];

        // when we press
        if (currSpace && !prevSpace) {
            int i = getDoorInFront(playerX, playerY, playerRot);
            if (i != -1) {
                doors[i].isClosed = !doors[i].isClosed;

                int mapIndex = (MAP_HEIGHT - 1 - doors[i].y) * MAP_WIDTH + doors[i].x;

                // collision = if doors state
                COLLISION_MAP[mapIndex] = doors[i].isClosed;

                // change texture on the map
                MAPDATA[mapIndex] = doors[i].isClosed ? 11 : 0; // 11 = closed doors
            }
        }
        prevSpace = currSpace;

        printf("X: %f\n", playerX);
        printf("Y: %f\n", playerY);
        SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH * sizeof(u32));

        int winW, winH;
        SDL_GetWindowSize(state.window, &winW, &winH);
        f32 targetRatio = (f32)SCREEN_WIDTH / SCREEN_HEIGHT;
        int drawW = winW, drawH = winH;
        if ((f32)winW / winH > targetRatio) drawW = (int)(winH * targetRatio);
        else drawH = (int)(winW / targetRatio);
        int offsetX = (winW - drawW) / 2, offsetY = (winH - drawH) / 2;
        SDL_Rect dstRect = {offsetX, offsetY, drawW, drawH};

        SDL_RenderClear(state.renderer);
        SDL_RenderCopy(state.renderer, state.texture, NULL, &dstRect);

        SDL_Rect gunRect = { SCREEN_WIDTH/2 - 256, SCREEN_HEIGHT - 512, 512, 512 };
        SDL_RenderCopy(state.renderer, gunFrames[gunFrame], NULL, &gunRect);

        SDL_RenderPresent(state.renderer);
    }

    for (int i = 0; i < MAX_TEXTURES; i++) if (state.textures[i].pixels) free(state.textures[i].pixels);
    for (int i = 0; i < GUN_FRAMES; i++) if (gunFrames[i]) SDL_DestroyTexture(gunFrames[i]);

    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    IMG_Quit();
    SDL_Quit();
    return 0;
}

