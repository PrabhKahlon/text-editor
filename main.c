#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>

void sdl_cc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
}

void* sdl_cp(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
    return ptr;
}

int main(void)
{
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    bool exit = false;

    while (!exit) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                exit = true;
                break;
            }
        }
    }

    SDL_Quit();
    return 0;
}
