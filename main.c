#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "vec.h"

#define MAX_BUFFER_SIZE 1024

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

void copyRect(SDL_Rect* srcRect, SDL_Rect* dstRect) {
    dstRect->x = srcRect->x;
    dstRect->y = srcRect->y;
    dstRect->w = srcRect->w;
    dstRect->h = srcRect->h;
    return;
}

void cacheTexture(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_Color color = { 255, 255, 255, 255 };
    int height = TTF_FontHeight(font);
    int ascent = TTF_FontAscent(font);
    int descent = TTF_FontDescent(font);

    if(ascent-descent != height) {
        height = ascent-descent;
    }
    int maxWidth = height*12;
    int maxHeight = height*12;

    SDL_Surface* cacheSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, maxWidth, maxHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));

    //Create ASCII string to generate glyphs
    char asciiString[95] = "";
    char c = 32;
    for(int i = 0; i < 95; i++) {
        asciiString[i] = c++;
    }

    SDL_Rect cacheCursor = {.x = 0, .y = 0, .w = 0, .h = height};
    SDL_Rect lastCache = {.x = 0, .y = 0, .w = 0, .h = height};
    //Generate Glyphs to cache to surface
    for (int i = 0; i < strlen(asciiString); i++) {
        SDL_Surface* glyphSurface = sdl_cp(TTF_RenderGlyph_Blended(font, asciiString[i], color));
        sdl_cc(SDL_SetSurfaceBlendMode(glyphSurface, SDL_BLENDMODE_NONE));
        if(cacheCursor.x + glyphSurface->w >= maxWidth) {
            cacheCursor.x = 0;
            cacheCursor.y += height;
        }
        cacheCursor.w = glyphSurface->w;
        cacheCursor.h = glyphSurface->h;
        SDL_Rect srcRect = {0, 0, glyphSurface->w, glyphSurface->h};
        SDL_Rect dstRect = {cacheCursor.x, cacheCursor.y, cacheCursor.w, cacheCursor.h};
        sdl_cc(SDL_BlitSurface(glyphSurface, &srcRect, cacheSurface, &dstRect));
        cacheCursor.x += glyphSurface->w;
        copyRect(&cacheCursor, &lastCache);
    }
    SDL_Rect tempRect = {.x = 0, .y = 0, .w = cacheSurface->w, .h = cacheSurface->h};
    SDL_Texture* cacheTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cacheSurface));
    SDL_RenderCopy(renderer, cacheTexture, &tempRect, &tempRect);

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

void renderText(SDL_Renderer* renderer, const char* text, size_t textSize, TTF_Font* font, SDL_Color color)
{
    Vec2 pen = {
        .x = 0,
        .y = 0
    };

    for (size_t i = 0; i < textSize; i++) {
        renderChar(renderer, text[i], &pen, font, color);
    }
}

int main(void)
{
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    sdl_cc(TTF_Init());
    TTF_Font* font = NULL;
    loadFont("OpenSans-Regular.ttf", 32, &font);
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    //White color in rgba
    SDL_Color color = { 255, 255, 255, 255 };

    bool exit = false;
    char buffer[MAX_BUFFER_SIZE] = "";
    size_t buffer_size = 0;

    while (!exit) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                exit = true;
                break;
            }
            case SDL_TEXTINPUT: {
                size_t textSize = strlen(event.text.text);
                const size_t freeSpace = MAX_BUFFER_SIZE - buffer_size;
                if (textSize > freeSpace) {
                    textSize = freeSpace;
                }
                memcpy(buffer + buffer_size, event.text.text, textSize);
                buffer_size += textSize;
                break;
            }
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE:
                    if (buffer_size > 0) {
                        buffer_size -= 1;
                    }
                    break;
                }
                break;
            }
            }
        }

        sdl_cc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        sdl_cc(SDL_RenderClear(renderer));
        //renderText(renderer, buffer, buffer_size, font, color);
        cacheTexture(renderer, font);
        SDL_RenderPresent(renderer);
    }
    TTF_CloseFont(font);
    SDL_Quit();
    return 0;
}
