#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>


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
    sdl_cc(TTF_Init());
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    TTF_Font* font = sdl_cp(TTF_OpenFont("OpenSans-Regular.ttf", 12));
    //White color in rgba
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = sdl_cp(TTF_RenderText_Blended(font, "Hello, World", color));
    SDL_Texture* texture = sdl_cp(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);

    SDL_Rect fontRect = {
        .x = 0,
        .y = 0,
        .w = surface->w,
        .h = surface->h
    };

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

        sdl_cc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));

        sdl_cc(SDL_RenderClear(renderer));
        sdl_cc(SDL_RenderCopy(renderer, texture, &fontRect, &fontRect));
        
        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}
