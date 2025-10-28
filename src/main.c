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

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define MAP_WIDTH 15
#define MAP_HEIGHT 15
#define MAX_TEXTURES 5

// Map : 0 = vide, 1-4 = types de murs
static u8 MAPDATA[MAP_HEIGHT * MAP_WIDTH] = {
    1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,3,2,2,2,4,0,0,0,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
    1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

// Structure pour textures
typedef struct {
    u32* pixels;
    int width;
    int height;
} Texture;

// État du moteur
struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    Texture textures[MAX_TEXTURES];
} state;

// Charger une texture PNG
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

// Dessiner une colonne texturée avec mapping correct
void verline_tex(int x, int y0, int y1, Texture* tex, int texX, f32 wallHeight, u32 fallbackColor) {
    if (!tex || !tex->pixels) {
        // Fallback sans texture
        if (y0 < 0) y0 = 0;
        if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;
        for (int y = y0; y <= y1; y++) {
            state.pixels[y * SCREEN_WIDTH + x] = fallbackColor;
        }
        return;
    }

    // Calcul du mapping de texture correct
    f32 step = (f32)tex->height / wallHeight;
    f32 texPos = 0;
    
    // Si le mur commence au-dessus de l'écran, ajuster texPos
    if (y0 < 0) {
        texPos = -y0 * step;
        y0 = 0;
    }
    if (y1 >= SCREEN_HEIGHT) {
        y1 = SCREEN_HEIGHT - 1;
    }

    // Dessiner chaque pixel avec le bon échantillonnage de texture
    for (int y = y0; y <= y1; y++) {
        int texY = (int)texPos;
        
        // Répéter la texture verticalement si nécessaire
        texY = texY % tex->height;
        if (texY < 0) texY += tex->height;
        
        state.pixels[y * SCREEN_WIDTH + x] = tex->pixels[texY * tex->width + texX];
        texPos += step;
    }
}

// Vérification collision
bool check_collision(f32 x, f32 y) {
    int mapX = (int)x;
    int mapY = (int)y;
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) return true;
    // Inversion de mapY
    return MAPDATA[(MAP_HEIGHT - 1 - mapY) * MAP_WIDTH + mapX] != 0;
}


int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    state.window = SDL_CreateWindow("SDL2 Raycaster - Texture Mapping Correct",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    state.renderer = SDL_CreateRenderer(state.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    state.texture = SDL_CreateTexture(state.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    // Charger textures
    load_texture_to_slot("assets/bluestone.png", 1);
    load_texture_to_slot("assets/redbrick.png", 2);
    load_texture_to_slot("assets/wood.png", 3);
    load_texture_to_slot("assets/greystone.png", 4);

    // Joueur
    f32 playerX = 1.5f, playerY = 7.5f, playerRot = 0.0f;
    f32 playerFOV = 1.047f;
    u32 ciel = 0xFF87CEEB;
    u32 sol  = 0xFF3A3A3A;

    bool running = true;
    SDL_Event event;
    u32 lastTime = SDL_GetTicks();

    while (running) {
        u32 currentTime = SDL_GetTicks();
        f32 deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        f32 moveSpeed = 3.0f * deltaTime;
        f32 rotSpeed = 2.0f * deltaTime;

        const u8 *keystate = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) 
                running = false;
        }

        // Déplacement
        f32 newX = playerX, newY = playerY;
        if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) { 
            newX += cosf(playerRot)*moveSpeed; 
            newY += sinf(playerRot)*moveSpeed; 
        }
        if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) { 
            newX -= cosf(playerRot)*moveSpeed; 
            newY -= sinf(playerRot)*moveSpeed; 
        }
        if (!check_collision(newX, newY)) { 
            playerX = newX; 
            playerY = newY; 
        }

        // Rotation
        if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) 
            playerRot -= rotSpeed;
        if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) 
            playerRot += rotSpeed;

        // Strafe
        if (keystate[SDL_SCANCODE_Q]) {
            newX = playerX + cosf(playerRot - M_PI/2) * moveSpeed;
            newY = playerY + sinf(playerRot - M_PI/2) * moveSpeed;
            if (!check_collision(newX, newY)) { playerX = newX; playerY = newY; }
        }
        if (keystate[SDL_SCANCODE_E]) {
            newX = playerX + cosf(playerRot + M_PI/2) * moveSpeed;
            newY = playerY + sinf(playerRot + M_PI/2) * moveSpeed;
            if (!check_collision(newX, newY)) { playerX = newX; playerY = newY; }
        }

        // Fond ciel/sol
        for (int y=0; y<SCREEN_HEIGHT/2; y++) 
            for (int x=0; x<SCREEN_WIDTH; x++) 
                state.pixels[y*SCREEN_WIDTH+x] = ciel;
        for (int y=SCREEN_HEIGHT/2; y<SCREEN_HEIGHT; y++) 
            for (int x=0; x<SCREEN_WIDTH; x++) 
                state.pixels[y*SCREEN_WIDTH+x] = sol;

        // Raycasting DDA
        for (int col=0; col<SCREEN_WIDTH; col++) {
            f32 cameraX = 2.0f*col/(f32)SCREEN_WIDTH - 1.0f;
            f32 rayDirX = cosf(playerRot) + sinf(playerRot)*cameraX*tanf(playerFOV/2);
            f32 rayDirY = sinf(playerRot) - cosf(playerRot)*cameraX*tanf(playerFOV/2);

            int mapX=(int)playerX, mapY=(int)playerY;
            f32 deltaDistX = (rayDirX==0) ? 1e30f : fabsf(1.0f/rayDirX);
            f32 deltaDistY = (rayDirY==0) ? 1e30f : fabsf(1.0f/rayDirY);

            int stepX = (rayDirX<0) ? -1 : 1;
            int stepY = (rayDirY<0) ? -1 : 1;

            f32 sideDistX = (rayDirX<0) ? (playerX-mapX)*deltaDistX : (mapX+1.0f-playerX)*deltaDistX;
            f32 sideDistY = (rayDirY<0) ? (playerY-mapY)*deltaDistY : (mapY+1.0f-playerY)*deltaDistY;

            u8 val=0; int side=0; f32 perpDist=0;
            while(val==0) {
                if(sideDistX < sideDistY) { 
                    sideDistX += deltaDistX; 
                    mapX += stepX; 
                    side = 0; 
                } else { 
                    sideDistY += deltaDistY; 
                    mapY += stepY; 
                    side = 1; 
                }
                if(mapX<0 || mapX>=MAP_WIDTH || mapY<0 || mapY>=MAP_HEIGHT) break;
                val = MAPDATA[(MAP_HEIGHT - 1 - mapY) * MAP_WIDTH + mapX];
            }
            if(val==0) continue;

            if(side==0) perpDist = (mapX - playerX + (1-stepX)/2) / rayDirX;
            else perpDist = (mapY - playerY + (1-stepY)/2) / rayDirY;

            // Coordonnée horizontale sur le mur (0..1)
            f32 hitWallX = (side==0) ? playerY + perpDist*rayDirY : playerX + perpDist*rayDirX;
            hitWallX -= floorf(hitWallX);

            Texture* tex = (val>0 && val<MAX_TEXTURES) ? &state.textures[val] : NULL;
            int texX = 0;
            if (tex && tex->pixels) {
                texX = (int)(hitWallX * tex->width);
                if(texX < 0) texX = 0; 
                if(texX >= tex->width) texX = tex->width-1;
            }

            // Hauteur du mur à l'écran
            f32 wallHeight = SCREEN_HEIGHT / perpDist;
            int y0 = SCREEN_HEIGHT/2 - (int)(wallHeight/2);
            int y1 = SCREEN_HEIGHT/2 + (int)(wallHeight/2);

            u32 fallbackColors[] = {0, 0xFF5555FF, 0xFF55FF55, 0xFFFF5555, 0xFFFFFF55};
            u32 fallbackColor = (val < 5) ? fallbackColors[val] : 0xFFFFFFFF;
            
            // Assombrir les côtés Y pour effet 3D
            if (side == 1 && !tex) {
                fallbackColor = (fallbackColor & 0xFF000000) | ((fallbackColor & 0xFEFEFE) >> 1);
            }

            verline_tex(col, y0, y1, tex, texX, wallHeight, fallbackColor);
        }

        SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH*sizeof(u32));
        
        int winW, winH; 
        SDL_GetWindowSize(state.window, &winW, &winH);
        f32 targetRatio = (f32)SCREEN_WIDTH / SCREEN_HEIGHT;
        int drawW = winW, drawH = winH;
        if((f32)winW/winH > targetRatio) drawW = (int)(winH * targetRatio);
        else drawH = (int)(winW / targetRatio);
        int offsetX = (winW - drawW)/2, offsetY = (winH - drawH)/2;
        SDL_Rect dstRect = {offsetX, offsetY, drawW, drawH};

        SDL_RenderClear(state.renderer);
        SDL_RenderCopy(state.renderer, state.texture, NULL, &dstRect);
        SDL_RenderPresent(state.renderer);
    }

    for(int i=0; i<MAX_TEXTURES; i++) 
        if(state.textures[i].pixels) free(state.textures[i].pixels);
    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
