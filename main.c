#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "vec.h"

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

//Load selected font
void loadFont(const char* fontFile, int fontSize, TTF_Font** font_ptr)
{
    if (font_ptr != NULL)
    {
        TTF_CloseFont(*font_ptr);
        *font_ptr = NULL;
    }
    *font_ptr = (TTF_OpenFont(fontFile, fontSize));
    return;
}

//Renders one specific character
void renderChar(SDL_Renderer* renderer, const char c, Vec2* pos, TTF_Font* font, SDL_Color color)
{
    SDL_Surface* surface = sdl_cp(TTF_RenderGlyph_Blended(font, c, color));
    SDL_Texture* texture = sdl_cp(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_Rect fontRect = {
        .x = 0,
        .y = 0,
        .w = surface->w,
        .h = surface->h
    };
    SDL_Rect destRect = {
        .x = pos->x,
        .y = pos->y,
        .w = surface->w,
        .h = surface->h
    };
    sdl_cc(SDL_RenderCopy(renderer, texture, &fontRect, &destRect));
    pos->x += surface->w;
    SDL_FreeSurface(surface);
}

void renderText(SDL_Renderer* renderer, const char* text, TTF_Font* font, SDL_Color color)
{
    Vec2 pen = {
        .x = 0,
        .y = 0
    };

    for (size_t i = 0; i < strlen(text); i++) {
        renderChar(renderer, text[i], &pen, font, color);
    }
}

int main(void)
{
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    sdl_cc(TTF_Init());
    TTF_Font* font = NULL;
    loadFont("OpenSans-Regular.ttf", 12, &font);
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    //White color in rgba
    SDL_Color color = { 255, 255, 255, 255 };

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
        renderText(renderer, "Hello World", font, color);
        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}
