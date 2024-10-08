#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "vec.h"
#include "glyph.h"
#include "gap.h"
#include "line.h"
#include "file_linux.h"

#define MAX_BUFFER_SIZE 1024

typedef struct {
    size_t line;
    size_t index;
    //Vec2 pos;
} Cursor;

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

void copyRect_GS(Glyph_Rect* srcRect, SDL_Rect* dstRect) {
    dstRect->x = srcRect->x;
    dstRect->y = srcRect->y;
    dstRect->w = srcRect->w;
    dstRect->h = srcRect->h;
    return;
}

SDL_Texture* cacheTexture(SDL_Renderer* renderer, TTF_Font* font, Glyph_Map* glyphMap)
{
    SDL_Color color = { 255, 255, 255, 255 };
    int height = TTF_FontHeight(font);
    int ascent = TTF_FontAscent(font);
    int descent = TTF_FontDescent(font);

    if (ascent - descent != height) {
        height = ascent - descent;
    }
    int maxWidth = height * 12;
    int maxHeight = height * 12;
    glyphMap->glyphHeight = height;

    //This only works on little endian machine
    SDL_Surface* cacheSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, maxWidth, maxHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));

    //Create ASCII string to generate glyphs
    char asciiString[95] = "";
    char c = 32;
    for (int i = 0; i < 95; i++) {
        asciiString[i] = c++;
    }

    SDL_Rect cacheCursor = { .x = 0, .y = 0, .w = 0, .h = height };
    //Generate Glyphs to cache to surface
    for (size_t i = 0; i < strlen(asciiString); i++) {
        SDL_Surface* glyphSurface = sdl_cp(TTF_RenderGlyph_Blended(font, asciiString[i], color));
        sdl_cc(SDL_SetSurfaceBlendMode(glyphSurface, SDL_BLENDMODE_NONE));
        if (cacheCursor.x + glyphSurface->w >= maxWidth) {
            cacheCursor.x = 0;
            cacheCursor.y += height;
        }
        cacheCursor.w = glyphSurface->w;
        cacheCursor.h = glyphSurface->h;
        SDL_Rect srcRect = { 0, 0, glyphSurface->w, glyphSurface->h };
        SDL_Rect dstRect = { cacheCursor.x, cacheCursor.y, cacheCursor.w, cacheCursor.h };
        sdl_cc(SDL_BlitSurface(glyphSurface, &srcRect, cacheSurface, &dstRect));
        addGlyph(glyphMap, asciiString[i], &cacheCursor);
        cacheCursor.x += glyphSurface->w;
    }
    SDL_Texture* cacheTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cacheSurface));
    return cacheTexture;

}

//Very basic cursor
void renderCursor(SDL_Renderer* renderer, Cursor* cursor, GapBuffer* text, SDL_Texture* cursorTexture, Glyph_Map* glyphMap)
{
    SDL_Rect destRect = {
        .x = 0,
        .y = cursor->line * glyphMap->glyphHeight,
        .w = glyphMap->glyphHeight / 2,
        .h = glyphMap->glyphHeight
    };

    for (size_t i = 0; i < text->cursor; i++) {
        int glyph = text->string[i];
        if (glyph == 10) {
            // destRect.y += glyphMap->glyphHeight;
            // destRect.x = 0;
            return;
        }
        if (glyph >= 32 && glyph <= 126) {
            int glyphIndex = glyph - 32;
            destRect.x += glyphMap->glyphs[glyphIndex]->w;
        }
    }
    int glyph = 0;
    if (text->gapEnd != text->length) {
        glyph = text->string[text->gapEnd];
        int index = glyph - 32;
        if (glyph >= 32 && glyph <= 126) {
            destRect.w = glyphMap->glyphs[index]->w;
            destRect.h = glyphMap->glyphs[index]->h;
        }
    }

    sdl_cc(SDL_RenderCopy(renderer, cursorTexture, NULL, &destRect));
}

//Renders one specific character
void renderChar(SDL_Renderer* renderer, const char c, Vec2* pos, SDL_Texture* font, SDL_Color color, Glyph_Map* glyphMap)
{
    //temp index
    size_t index = (int)c - 32;
    if (c < 32) {
        if (c == 10) {
            //pos->y += glyphMap->glyphHeight;
            //pos->x = 0;
            return;
        }
        return;
    }
    SDL_Rect fontRect = { .x = 0, .y = 0, .w = 0, .h = 0 };
    if (index >= 95) {
        index = 94;
    }
    copyRect_GS(glyphMap->glyphs[index], &fontRect);
    SDL_Rect destRect = {
        .x = pos->x,
        .y = pos->y,
        .w = fontRect.w,
        .h = fontRect.h
    };
    sdl_cc(SDL_RenderCopy(renderer, font, &fontRect, &destRect));
    pos->x += fontRect.w;
}

void renderLine(SDL_Renderer* renderer, Vec2* linePos, GapBuffer* line, SDL_Texture* font, SDL_Texture* cursor, SDL_Color color, Glyph_Map* glyphMap)
{
    for (size_t i = 0; i < line->cursor; i++) {
        renderChar(renderer, line->string[i], linePos, font, color, glyphMap);
    }
    for (size_t i = line->gapEnd; i < line->length; i++) {
        renderChar(renderer, line->string[i], linePos, font, color, glyphMap);
    }
}

void renderText(SDL_Renderer* renderer, Text* text, Cursor* cursor, SDL_Texture* fontTexture, SDL_Texture* cursorTexture, SDL_Color color, Glyph_Map* glyphMap)
{
    Vec2 pen = {
        .x = 0,
        .y = 0
    };

    for (size_t i = 0; i < text->lineCount; i++) {
        renderLine(renderer, &pen, text->lines[i], fontTexture, cursorTexture, color, glyphMap);
        pen.y += glyphMap->glyphHeight;
        pen.x = 0;
    }
    renderCursor(renderer, cursor, text->lines[cursor->line], cursorTexture, glyphMap);
}

int main(int argc, char const* argv[])
{
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    sdl_cc(TTF_Init());
    TTF_Font* font = NULL;
    loadFont("DejaVuSansMono.ttf", 24, &font);
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    //White color in rgba
    SDL_Color color = { 255, 255, 255, 255 };
    Glyph_Map* glyphMap = createGlyphMap();
    SDL_Texture* fontTexture = cacheTexture(renderer, font, glyphMap);
    SDL_Surface* cursorSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
    //Make the cursor transparent.
    sdl_cc(SDL_FillRect(cursorSurface, NULL, 0xAAFFFFFF));
    SDL_Texture* cursorTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cursorSurface));
    Cursor cursor = { .line = 0, .index = 0 };


    Text* text = createText();
    bool exit = false;

    if (argc >= 2) {
        char const* fileName = argv[1];
        openFile(fileName, text);
        moveCursor(text->lines[cursor.line], cursor.index);
    }

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
                insertOnLine(text, cursor.line, event.text.text, textSize);
                cursor.index += textSize;
                break;
            }
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_LCTRL: {
                    if (argc >= 2) {
                        size_t prevLine = cursor.line;
                        size_t prevIndex = cursor.index;
                        char const* fileName = argv[1];
                        saveFile(fileName, text);
                        moveCursor(text->lines[prevLine], prevIndex);
                        cursor.line = prevLine;
                        cursor.index = prevIndex;
                    }
                    break;
                }
                case SDLK_BACKSPACE: {
                    if (cursor.index > 0) {
                        cursor.index--;
                        deleteFromLine(text, cursor.line);
                    }
                    else {
                        if (cursor.line > 0) {
                            //Delete line
                            size_t newIndex = deleteLine(text, cursor.line, cursor.index);
                            cursor.line--;
                            cursor.index = newIndex;
                        }
                    }
                    break;
                }
                case SDLK_RETURN: {
                    cursor.line++;
                    createNewLine(text, cursor.line, cursor.index);
                    cursor.index = 0;
                    moveCursor(text->lines[cursor.line], cursor.index);
                    break;
                }
                case SDLK_LEFT: {
                    if (cursor.index > 0) {
                        cursorLeft(text->lines[cursor.line]);
                        cursor.index--;
                    }
                    else {
                        if (cursor.line > 0) {
                            cursor.line--;
                            cursor.index = moveCursorToEnd(text->lines[cursor.line]);
                        }
                    }
                    break;
                }
                case SDLK_RIGHT: {
                    if (cursor.index < (text->lines[cursor.line]->cursor + text->lines[cursor.line]->length) - text->lines[cursor.line]->gapEnd) {
                        cursorRight(text->lines[cursor.line]);
                        cursor.index++;
                    }
                    else {
                        if (cursor.line < text->lineCount - 1) {
                            cursor.line++;
                            cursor.index = 0;
                            moveCursor(text->lines[cursor.line], cursor.index);
                        }
                    }
                    break;
                }
                case SDLK_UP: {
                    if (cursor.line > 0) {
                        cursor.line--;
                        //No index memory. Doing it the notepad way for now.
                        if (cursor.index > gapUsed(text->lines[cursor.line])) {
                            cursor.index = moveCursorToEnd(text->lines[cursor.line]);
                        }
                        else {
                            moveCursor(text->lines[cursor.line], cursor.index);
                        }
                    }
                    break;
                }
                case SDLK_DOWN: {
                    if (cursor.line < text->lineCount - 1) {
                        cursor.line++;
                        //No index memory. Doing it the notepad way for now.
                        if (cursor.index > gapUsed(text->lines[cursor.line])) {
                            cursor.index = moveCursorToEnd(text->lines[cursor.line]);
                        }
                        else {
                            moveCursor(text->lines[cursor.line], cursor.index);
                        }
                    }
                    break;
                }
                }
                break;
            }
            }
        }

        sdl_cc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        sdl_cc(SDL_RenderClear(renderer));
        renderText(renderer, text, &cursor, fontTexture, cursorTexture, color, glyphMap);
        //SDL_RenderCopy(renderer, fontTexture, NULL, &tempRect);
        SDL_RenderPresent(renderer);
    }
    freeText(text);
    freeGlyphMap(glyphMap);
    TTF_CloseFont(font);
    SDL_Quit();
    return 0;
}
