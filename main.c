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
#define SCROLL_STEP_X 5
#define SCROLL_STEP_Y 2
#define SCROLL_DIRECTION -1
#define MAX_HORIZONTAL_SCROLL 200
#define X_OFFSET 40
#define Y_OFFSET 20
#define SCROLLBAR_WIDTH 15

typedef struct {
    size_t line;
    size_t index;
} Cursor;

typedef struct {
    Vec2 windowSize;
    size_t scrollCountX;
    size_t scrollCountY;
} Viewport;

typedef struct {
    Text* text;
    Cursor cursor;
    Viewport view;
    Glyph_Map* glyphMap;
} Editor;

// List of clickable items
typedef struct {
    SDL_Rect* clickableRects;
    int count;
} ClickableItems;

void sdl_cc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
}

void* sdl_cp(void* ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
    return ptr;
}

// Load selected font
void loadFont(const char* fontFile, int fontSize, TTF_Font** font_ptr)
{
    if (font_ptr != NULL) {
        TTF_CloseFont(*font_ptr);
        *font_ptr = NULL;
    }
    *font_ptr = (TTF_OpenFont(fontFile, fontSize));
    return;
}

void copyRect_GS(Glyph_Rect* srcRect, SDL_Rect* dstRect)
{
    dstRect->x = srcRect->x;
    dstRect->y = srcRect->y;
    dstRect->w = srcRect->w;
    dstRect->h = srcRect->h;
    return;
}

bool isInViewBox(Vec2 coord, Vec2 windowSize)
{
    return coord.x >= 0.0f &&
        coord.y >= 0.0f &&
        coord.x <= windowSize.x &&
        coord.y <= windowSize.y;
}

SDL_Texture* cacheTexture(SDL_Renderer* renderer, TTF_Font* font, Glyph_Map* glyphMap)
{
    if (!TTF_FontFaceIsFixedWidth(font)) {
        printf("Error: Text Editor is only compatable with monospace fonts!\n");
        exit(1);
    }

    SDL_Color color = { 255, 255, 255, 255 };
    // TTF_SetFontKerning(font, false);
    // int testH = TTF_FontHeight(font);
    // int ascent = TTF_FontAscent(font);
    // int descent = TTF_FontDescent(font);

    int width = 0;
    int height = 0;
    TTF_SizeUTF8(font, "W", &width, &height);

    // if (ascent - descent != testH) {
    //     testH = ascent - descent;
    // }

    int maxWidth = height * 12;
    int maxHeight = height * 12;

    glyphMap->glyphWidth = width;
    glyphMap->glyphHeight = height;

    printf("%d, %d\n", width, height);

    // This only works on little endian machine
    SDL_Surface* cacheSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, maxWidth, maxHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));

    // Create ASCII string to generate glyphs
    char asciiString[96] = "";
    char c = 32;
    for (int i = 0; i < 95; i++) {
        asciiString[i] = c++;
    }

    SDL_Rect cacheCursor = { .x = 0, .y = 0, .w = 0, .h = height };
    // Generate Glyphs to cache to surface
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
        SDL_FreeSurface(glyphSurface);
    }
    SDL_Texture* cacheTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cacheSurface));
    SDL_FreeSurface(cacheSurface);
    return cacheTexture;
}

// Very basic cursor
void renderCursor(Editor* editor, SDL_Renderer* renderer, float offsetX, float offsetY, SDL_Texture* cursorTexture)
{
    Cursor* cursor = &editor->cursor;
    GapBuffer* text = editor->text->lines[cursor->line];
    Glyph_Map* glyphMap = editor->glyphMap;
    Viewport* viewport = &editor->view;

    size_t cursorLine = cursor->line;
    size_t textPos = text->position;
    size_t gapEnd = text->gapEnd;
    size_t textLen = text->length;
    char* string = text->string;

    int glyphW = glyphMap->glyphWidth;
    int glyphH = glyphMap->glyphHeight;

    SDL_Rect destRect = {
        .x = 0,
        .y = cursorLine * glyphH,
        .w = glyphW,
        .h = glyphH
    };

    destRect.x += offsetX + X_OFFSET;
    destRect.y += offsetY + Y_OFFSET;

    for (size_t i = 0; i < textPos; i++) {
        int glyph = string[i];

        if (glyph == 10) {
            return;
        }

        if (glyph >= 32 && glyph <= 126) {
            int glyphIndex = glyph - 32;
            destRect.x += glyphMap->glyphs[glyphIndex]->w;
        }
    }

    if (gapEnd != textLen) {
        int glyph = string[gapEnd];

        if (glyph >= 32 && glyph <= 126) {
            int index = glyph - 32;
            destRect.w = glyphMap->glyphs[index]->w;
            destRect.h = glyphMap->glyphs[index]->h;
        }
    }

    Vec2 cursorPos = {
        .x = destRect.x,
        .y = destRect.y
    };

    if (!isInViewBox(cursorPos, viewport->windowSize)) {
        return;
    }

    sdl_cc(SDL_RenderCopy(renderer, cursorTexture, NULL, &destRect));
}

// Renders one specific character
void renderChar(Editor* editor, SDL_Renderer* renderer, char c, Vec2* pos, SDL_Texture* font)
{
    Glyph_Map* glyphMap = editor->glyphMap;

    size_t index = (size_t)c - 32;

    if ((unsigned char)c < 32) {
        return;
    }

    if (index >= 95) {
        index = 94;
    }

    SDL_Rect fontRect = { 0, 0, 0, 0 };
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

void renderLine(Editor* editor, SDL_Renderer* renderer, Vec2* linePos, GapBuffer* line, SDL_Texture* font)
{
    for (size_t i = 0; i < line->position; i++) {
        renderChar(editor, renderer, line->string[i], linePos, font);
    }
    for (size_t i = line->gapEnd; i < line->length; i++) {
        renderChar(editor, renderer, line->string[i], linePos, font);
    }
}

void renderText(Editor* editor, SDL_Renderer* renderer, SDL_Texture* fontTexture, SDL_Texture* cursorTexture)
{
    Text* text = editor->text;
    Viewport* viewport = &editor->view;
    Glyph_Map* glyphMap = editor->glyphMap;

    Vec2 pen = { 0, 0 };

    float scrollOffsetX = -(float)viewport->scrollCountX * (float)glyphMap->glyphWidth;
    float scrollOffsetY = -(float)viewport->scrollCountY * (float)glyphMap->glyphHeight;

    pen.x = scrollOffsetX + X_OFFSET;
    pen.y = scrollOffsetY + Y_OFFSET;

    for (size_t i = 0; i < text->lineCount; i++) {
        if (pen.y > viewport->windowSize.y) {
            break;
        }

        renderLine(editor, renderer, &pen, text->lines[i], fontTexture);

        pen.y += glyphMap->glyphHeight;
        pen.x = scrollOffsetX + X_OFFSET;
    }

    renderCursor(editor, renderer, scrollOffsetX, scrollOffsetY, cursorTexture);
}

// Cursor Helper Functions

void scroll(Editor* editor, int x, int y) {
    Text* text = editor->text;
    Viewport* viewport = &editor->view;

    int scrollAmountX = x * SCROLL_STEP_X * SCROLL_DIRECTION;
    int scrollAmountY = y * SCROLL_STEP_Y * SCROLL_DIRECTION;

    long newIndexX = (long)viewport->scrollCountX + scrollAmountX;
    long newIndexY = (long)viewport->scrollCountY + scrollAmountY;

    if (newIndexX < 0) {
        viewport->scrollCountX = 0;
    }
    else if (newIndexX > MAX_HORIZONTAL_SCROLL) {
        viewport->scrollCountX = MAX_HORIZONTAL_SCROLL;
    }
    else {
        viewport->scrollCountX += scrollAmountX;
    }

    if (newIndexY < 0) {
        viewport->scrollCountY = 0;
    }
    else if (newIndexY > (long)(text->lineCount - 1)) {
        viewport->scrollCountY = text->lineCount - 1;
    }
    else {
        viewport->scrollCountY += scrollAmountY;
    }
}

void scrollTo(Editor* editor, int lineNum) {
    Text* text = editor->text;
    Viewport* viewport = &editor->view;

    int scrollAmountX = 0 * SCROLL_STEP_X * SCROLL_DIRECTION;
    int newY = lineNum - (int)viewport->scrollCountY;
    int scrollAmountY = newY;

    long newIndexX = (long)viewport->scrollCountX + scrollAmountX;
    long newIndexY = (long)viewport->scrollCountY + scrollAmountY;

    if (newIndexX < 0) {
        viewport->scrollCountX = 0;
    }
    else if (newIndexX > MAX_HORIZONTAL_SCROLL) {
        viewport->scrollCountX = MAX_HORIZONTAL_SCROLL;
    }
    else {
        viewport->scrollCountX += scrollAmountX;
    }

    if (newIndexY < 0) {
        viewport->scrollCountY = 0;
    }
    else if (newIndexY > (long)(text->lineCount - 1)) {
        viewport->scrollCountY = text->lineCount - 1;
    }
    else {
        viewport->scrollCountY += scrollAmountY;
    }
}

bool mouseOnButton(int curMouseX, int curMouseY, ClickableItems* buttons) {
    for (int i = 0; i < buttons->count; i++) {
        SDL_Rect* buttonRect = &buttons->clickableRects[i];
        // printf("Button Detect: %d, %d, %d, %d\n", curMouseX, curMouseY, buttonRect->x, buttonRect->y);
        if (curMouseX >= buttonRect->x &&
            curMouseY >= buttonRect->y &&
            curMouseX <= (buttonRect->x + buttonRect->w) &&
            curMouseY <= (buttonRect->y + buttonRect->h)) {
            return true;
        }
    }
    return false;
}

void mouseToLinePos(Editor* editor, size_t* newMouseX, size_t* newMouseY, int curMouseX, int curMouseY)
{
    Viewport* viewport = &editor->view;
    Glyph_Map* glyphMap = editor->glyphMap;

    float scrollOffsetX = -(float)viewport->scrollCountX * ((float)glyphMap->glyphHeight / 2.0f);
    float scrollOffsetY = -(float)viewport->scrollCountY * (float)glyphMap->glyphHeight;

    int newCurPosX = (curMouseX - scrollOffsetX - X_OFFSET) / glyphMap->glyphWidth;
    int newCurPosY = (curMouseY - scrollOffsetY - Y_OFFSET) / glyphMap->glyphHeight;

    *newMouseX = newCurPosX >= 0 ? newCurPosX : 0;
    *newMouseY = newCurPosY;
}

void cursorToPos(Editor* editor, Vec2* cursorPos)
{
    Cursor* cursor = &editor->cursor;
    Glyph_Map* glyphMap = editor->glyphMap;

    float newCursorPosX = (float)cursor->index * (float)glyphMap->glyphWidth;
    float newCursorPosY = (float)cursor->line * (float)glyphMap->glyphHeight;

    cursorPos->x = newCursorPosX;
    cursorPos->y = newCursorPosY;
}

void moveCursorDown(Editor* editor)
{
    Cursor* cursor = &editor->cursor;
    Text* text = editor->text;

    if (cursor->line < text->lineCount - 1) {
        cursor->line++;

        size_t lineUsed = gapUsed(text->lines[cursor->line]);
        if (cursor->index > lineUsed) {
            cursor->index = moveCursorToEnd(text->lines[cursor->line]);
        }
        else {
            moveCursor(text->lines[cursor->line], cursor->index);
        }
    }
}

void moveCursorUp(Editor* editor)
{
    Cursor* cursor = &editor->cursor;
    Text* text = editor->text;

    if (cursor->line > 0) {
        cursor->line--;

        size_t lineUsed = gapUsed(text->lines[cursor->line]);
        if (cursor->index > lineUsed) {
            cursor->index = moveCursorToEnd(text->lines[cursor->line]);
        }
        else {
            moveCursor(text->lines[cursor->line], cursor->index);
        }
    }
}

// TODO: Move allocation and deallocation to 1 time call
void renderScrollBar(Editor* editor, SDL_Renderer* renderer, ClickableItems* buttons)
{
    Text* text = editor->text;
    Viewport* viewport = &editor->view;

    SDL_Surface* sqSurface = sdl_cp(SDL_CreateRGBSurface(
        SDL_SWSURFACE, 50, 50, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
    sdl_cc(SDL_FillRect(sqSurface, NULL, 0xAAFFFFFF));

    SDL_Texture* sqTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, sqSurface));

    int glyphHeight = editor->glyphMap->glyphHeight;
    int scrollbarHeight = (viewport->windowSize.y) / (text->lineCount - 1) * glyphHeight;
    float scrollbarOffset = ((float)viewport->scrollCountY / (float)(text->lineCount - 1)) *
        ((float)viewport->windowSize.y - (float)scrollbarHeight);

    SDL_Rect* sqRect = &buttons->clickableRects[0];
    sqRect->h = scrollbarHeight;
    sqRect->w = SCROLLBAR_WIDTH;
    sqRect->x = viewport->windowSize.x - SCROLLBAR_WIDTH;
    sqRect->y = (int)scrollbarOffset;

    SDL_RenderCopy(renderer, sqTexture, NULL, sqRect);

    SDL_FreeSurface(sqSurface);
    SDL_DestroyTexture(sqTexture);
}

int main(int argc, char const* argv[])
{
    // SDL and font initialization
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    sdl_cc(TTF_Init());

    TTF_Font* font = NULL;
    loadFont("DejaVuSansMono.ttf", 18, &font);

    SDL_Window* window = sdl_cp(SDL_CreateWindow(
        "Text", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    // Editor and viewport initialization
    Editor editor = { 0 };
    editor.glyphMap = createGlyphMap();
    editor.text = createText();
    editor.cursor = (Cursor){ 0, 0 };
    editor.view = (Viewport){ .windowSize = {0, 0}, .scrollCountX = 0, .scrollCountY = 0 };

    // Colors
    // SDL_Color white = { 255, 255, 255, 255 };

    // Font texture
    SDL_Texture* fontTexture = cacheTexture(renderer, font, editor.glyphMap);

    // Cursor texture
    SDL_Surface* cursorSurface = sdl_cp(SDL_CreateRGBSurface(
        SDL_SWSURFACE, 8, 8, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
    sdl_cc(SDL_FillRect(cursorSurface, NULL, 0xAAFFFFFF));
    SDL_Texture* cursorTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cursorSurface));

    // Input state
    int mouseX = 0, mouseY = 0;
    int lshift = 0, lctrl = 0, rctrl = 0;
    int scrollWheelClicked = 0;

    // Clickable buttons
    SDL_Rect buttonsLocations[10] = { 0 };
    ClickableItems buttons = { .clickableRects = buttonsLocations, .count = 1 };

    // File loading
    if (argc >= 2) {
        const char* fileName = argv[1];
        openFile(fileName, editor.text);
        moveCursor(editor.text->lines[editor.cursor.line], editor.cursor.index);
    }

    // Main loop
    bool exit = false;
    while (!exit) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int winW = 0, winH = 0;
                    SDL_GetWindowSize(window, &winW, &winH);

                    Viewport* viewport = &editor.view;
                    viewport->windowSize.x = winW;
                    viewport->windowSize.y = winH;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseOnButton(mouseX, mouseY, &buttons)) {
                    // printf("Button Clicked!\n");
                    scrollWheelClicked = 1;
                    break;
                }

                size_t newMouseX = 0, newMouseY = 0;
                mouseToLinePos(&editor, &newMouseX, &newMouseY, mouseX, mouseY);

                Text* text = editor.text;
                Cursor* cursor = &editor.cursor;

                if (newMouseY > text->lineCount - 1) {
                    newMouseY = text->lineCount - 1;
                }
                cursor->line = newMouseY;

                GapBuffer* lineBuffer = text->lines[cursor->line];
                size_t lineLength = (lineBuffer->position + lineBuffer->length) - lineBuffer->gapEnd;

                if (newMouseX > lineLength) {
                    newMouseX = lineLength;
                }
                cursor->index = newMouseX;

                moveCursor(lineBuffer, cursor->index);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                scrollWheelClicked = 0;
                break;
            }
            case SDL_MOUSEMOTION:
            {
                if (scrollWheelClicked) {
                    SDL_GetMouseState(&mouseX, &mouseY);

                    // Vec2 mousePos = { .x = mouseX, .y = mouseY };

                    Viewport* viewport = &editor.view;
                    Text* text = editor.text;
                    SDL_Rect* scrollbarRect = &buttons.clickableRects[0];

                    int scrollbarHeight = scrollbarRect->h;
                    float lineNum = mouseY / ((float)viewport->windowSize.y - (float)scrollbarHeight) * (float)(text->lineCount - 1);

                    scrollTo(&editor, (int)lineNum);
                }
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                int shiftMod = lshift;

                int wheelX = event.wheel.x;
                int wheelY = event.wheel.y;
                int direction = event.wheel.direction;

                if (direction == 0) {
                    if (shiftMod) {
                        scroll(&editor, wheelY, wheelX);
                    }
                    else {
                        scroll(&editor, wheelX, wheelY);
                    }
                }

                break;
            }
            case SDL_QUIT:
            {
                exit = true;
                break;
            }
            case SDL_TEXTINPUT:
            {
                int ctrlMod = lctrl || rctrl;

                if (!ctrlMod) {
                    Text* text = editor.text;
                    Cursor* cursor = &editor.cursor;

                    size_t textSize = strlen(event.text.text);
                    insertOnLine(text, cursor->line, event.text.text, textSize);
                    cursor->index += textSize;
                }

                break;
            }
            case SDL_KEYUP:
            {
                switch (event.key.keysym.sym) {
                case SDLK_LCTRL:
                    lctrl = 0;
                    break;
                case SDLK_LSHIFT:
                    lshift = 0;
                    break;
                }
                break;
            }
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym) {
                case SDLK_LCTRL:
                {
                    lctrl = 1;
                    break;
                }
                case SDLK_LSHIFT:
                {
                    lshift = 1;
                    break;
                }
                case SDLK_s:
                {
                    int ctrlMod = lctrl || rctrl;

                    if (ctrlMod && argc >= 2) {
                        Cursor* cursor = &editor.cursor;
                        Text* text = editor.text;

                        size_t prevLine = cursor->line;
                        size_t prevIndex = cursor->index;
                        const char* fileName = argv[1];

                        saveFile(fileName, text);
                        moveCursor(text->lines[prevLine], prevIndex);

                        cursor->line = prevLine;
                        cursor->index = prevIndex;
                    }

                    break;
                }
                case SDLK_BACKSPACE:
                {
                    Cursor* cursor = &editor.cursor;
                    Text* text = editor.text;

                    if (cursor->index > 0) {
                        cursor->index--;
                        deleteFromLine(text, cursor->line);
                    }
                    else if (cursor->line > 0) {
                        size_t newIndex = deleteLine(text, cursor->line, cursor->index);
                        cursor->line--;
                        cursor->index = newIndex;
                    }

                    break;
                }
                case SDLK_RETURN:
                {
                    Cursor* cursor = &editor.cursor;
                    Text* text = editor.text;

                    cursor->line++;
                    createNewLine(text, cursor->line, cursor->index);
                    cursor->index = 0;
                    moveCursor(text->lines[cursor->line], cursor->index);

                    break;
                }
                case SDLK_LEFT:
                {
                    Cursor* cursor = &editor.cursor;
                    Text* text = editor.text;
                    GapBuffer* lineBuffer = text->lines[cursor->line];

                    if (cursor->index > 0) {
                        cursorLeft(lineBuffer);
                        cursor->index--;
                    }
                    else if (cursor->line > 0) {
                        cursor->line--;
                        lineBuffer = text->lines[cursor->line];
                        cursor->index = moveCursorToEnd(lineBuffer);
                    }
                    break;
                }

                case SDLK_RIGHT:
                {
                    Cursor* cursor = &editor.cursor;
                    Text* text = editor.text;
                    GapBuffer* lineBuffer = text->lines[cursor->line];
                    size_t lineLength = (lineBuffer->position + lineBuffer->length) - lineBuffer->gapEnd;

                    if (cursor->index < lineLength) {
                        cursorRight(lineBuffer);
                        cursor->index++;
                    }
                    else if (cursor->line < text->lineCount - 1) {
                        cursor->line++;
                        cursor->index = 0;
                        lineBuffer = text->lines[cursor->line];
                        moveCursor(lineBuffer, cursor->index);
                    }
                    break;
                }

                case SDLK_UP:
                {
                    moveCursorUp(&editor);

                    Vec2 cursorPos = { 0, 0 };
                    cursorToPos(&editor, &cursorPos);

                    Viewport* viewport = &editor.view;
                    Glyph_Map* glyphMap = editor.glyphMap;
                    cursorPos.y += -(float)viewport->scrollCountY * (float)glyphMap->glyphHeight;

                    if (!isInViewBox(cursorPos, viewport->windowSize)) {
                        scroll(&editor, 0, 1);
                        moveCursorUp(&editor);
                    }
                    break;
                }

                case SDLK_DOWN:
                {
                    moveCursorDown(&editor);

                    Vec2 cursorPos = { 0, 0 };
                    cursorToPos(&editor, &cursorPos);

                    Viewport* viewport = &editor.view;
                    Glyph_Map* glyphMap = editor.glyphMap;
                    cursorPos.y += -(float)viewport->scrollCountY * (float)glyphMap->glyphHeight;
                    cursorPos.y += glyphMap->glyphHeight;

                    if (!isInViewBox(cursorPos, viewport->windowSize)) {
                        scroll(&editor, 0, -1);
                        moveCursorDown(&editor);
                    }
                    break;
                }
                case SDLK_PAGEUP:
                {
                    // TODO
                    break;
                }
                case SDLK_PAGEDOWN:
                {
                    // TODO
                    break;
                }
                case SDLK_TAB:
                {
                    // Add 4 spaces for each tab key press
                    char* tabString = "    ";
                    insertOnLine(editor.text, editor.cursor.line, tabString, strlen(tabString));
                    editor.cursor.index += strlen(tabString);
                }
                }
                break;
            }
            }
        }

        sdl_cc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        sdl_cc(SDL_RenderClear(renderer));
        renderText(&editor, renderer, fontTexture, cursorTexture);
        renderScrollBar(&editor, renderer, &buttons);
        SDL_RenderPresent(renderer);
    }
    // Free allocated Text and Glyphmap objects
    freeText(editor.text);
    freeGlyphMap(editor.glyphMap);

    // Free font
    TTF_CloseFont(font);

    // Free Surfaces and textures
    SDL_DestroyTexture(cursorTexture);
    SDL_FreeSurface(cursorSurface);
    SDL_DestroyTexture(fontTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
