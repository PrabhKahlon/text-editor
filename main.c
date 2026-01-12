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
    Vec2 windowSize;
    size_t scrollCountX;
    size_t scrollCountY;
} Viewport;

typedef struct {
    Text *text;
    Cursor cursor;
    Viewport view;
    Glyph_Map *glyphs;
} Editor;

typedef struct {
    size_t line;
    size_t index;
    // Vec2 pos;
} Cursor;

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
    if(!TTF_FontFaceIsFixedWidth(font)) {
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
void renderCursor(SDL_Renderer* renderer, float offsetX, float offsetY, Cursor* cursor, GapBuffer* text, SDL_Texture* cursorTexture, Glyph_Map* glyphMap, Viewport* viewport)
{
    SDL_Rect destRect = {
        .x = 0,
        .y = cursor->line * glyphMap->glyphHeight,
        .w = glyphMap->glyphWidth,
        .h = glyphMap->glyphHeight
    };

    destRect.x += offsetX + X_OFFSET;
    destRect.y += offsetY + Y_OFFSET;

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

    // Check if cursor should be rendered in view
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
void renderChar(SDL_Renderer* renderer, const char c, Vec2* pos, SDL_Texture* font, SDL_Color color, Glyph_Map* glyphMap)
{
    // temp index
    size_t index = (int)c - 32;
    if (c < 32) {
        if (c == 10) {
            // pos->y += glyphMap->glyphHeight;
            // pos->x = 20;
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
        .h = fontRect.h };
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

void renderText(SDL_Renderer* renderer, Text* text, Cursor* cursor, SDL_Texture* fontTexture, SDL_Texture* cursorTexture, SDL_Color color, Glyph_Map* glyphMap, Viewport* viewport)
{
    Vec2 pen = {
        .x = 0,
        .y = 0
    };

    // scroll count should never be negative and should be clamped to linecount
    // Maybe call clamp scroll here?
    float scrollOffsetX = -(float)viewport->scrollCountX * ((float)glyphMap->glyphWidth);
    float scrollOffsetY = -(float)viewport->scrollCountY * (float)glyphMap->glyphHeight;
    pen.x = scrollOffsetX + X_OFFSET;
    pen.y = scrollOffsetY + Y_OFFSET;

    for (size_t i = 0; i < text->lineCount; i++) {
        // Only render what is visible to the user
        if (pen.y > viewport->windowSize.y) {
            break;
        }
        renderLine(renderer, &pen, text->lines[i], fontTexture, cursorTexture, color, glyphMap);
        pen.y += glyphMap->glyphHeight;
        pen.x = scrollOffsetX + X_OFFSET;
    }
    renderCursor(renderer, scrollOffsetX, scrollOffsetY, cursor, text->lines[cursor->line], cursorTexture, glyphMap, viewport);
}

// Cursor Helper Functions

void scroll(int x, int y, Text* text, Viewport* viewport) {
    int scrollAmountX = x * SCROLL_STEP_X * SCROLL_DIRECTION;
    int scrollAmountY = y * SCROLL_STEP_Y * SCROLL_DIRECTION;
    long newIndexX = (long)viewport->scrollCountX + scrollAmountX;
    long newIndexY = (long)viewport->scrollCountY + scrollAmountY;
    if (newIndexX < 0) {
        viewport->scrollCountX = 0;
    }
    else if ((viewport->scrollCountX + scrollAmountX) > MAX_HORIZONTAL_SCROLL) {
        viewport->scrollCountX = MAX_HORIZONTAL_SCROLL;
    }
    else {
        viewport->scrollCountX += scrollAmountX;
    }
    if (newIndexY < 0) {
        viewport->scrollCountY = 0;
    }
    else if ((viewport->scrollCountY + scrollAmountY) > (text->lineCount - 1)) {
        viewport->scrollCountY = text->lineCount - 1;
    }
    else {
        viewport->scrollCountY += scrollAmountY;
    }
}

void scrollTo(int lineNum, Text* text, Viewport* viewport) {
    int scrollAmountX = 0 * SCROLL_STEP_X * SCROLL_DIRECTION;
    int newY = lineNum - viewport->scrollCountY;
    int scrollAmountY = newY;
    long newIndexX = (long)viewport->scrollCountX + scrollAmountX;
    long newIndexY = (long)viewport->scrollCountY + scrollAmountY;
    if (newIndexX < 0) {
        viewport->scrollCountX = 0;
    }
    else if ((viewport->scrollCountX + scrollAmountX) > MAX_HORIZONTAL_SCROLL) {
        viewport->scrollCountX = MAX_HORIZONTAL_SCROLL;
    }
    else {
        viewport->scrollCountX += scrollAmountX;
    }
    if (newIndexY < 0) {
        viewport->scrollCountY = 0;
    }
    else if ((viewport->scrollCountY + scrollAmountY) > (text->lineCount - 1)) {
        viewport->scrollCountY = text->lineCount - 1;
    }
    else {
        viewport->scrollCountY += scrollAmountY;
    }
}

bool mouseOnButton(int curMouseX, int curMouseY, ClickableItems* buttons) {
    for(int i = 0; i < buttons->count; i++) {
        SDL_Rect* buttonRect = &buttons->clickableRects[i];
        // printf("Button Detect: %d, %d, %d, %d\n", curMouseX, curMouseY, buttonRect->x, buttonRect->y);
        if(curMouseX >= buttonRect->x &&
        curMouseY >= buttonRect->y &&
        curMouseX <= (buttonRect->x + buttonRect->w) &&
        curMouseY <= (buttonRect->y + buttonRect->h)) {
            return true;
        }
    }
    return false;
}

void mouseToLinePos(size_t* newMouseX, size_t* newMouseY, int curMouseX, int curMouseY, int glyphWidth, int glyphHeight, Viewport* viewport)
{
    // Calculate scroll offset like usual and subtract from mouse pos to get line (double negative)
    float scrollOffsetX = -(float)viewport->scrollCountX * ((float)glyphHeight / 2.0f);
    float scrollOffsetY = -(float)viewport->scrollCountY * (float)glyphHeight;
    int newCurPosX = (curMouseX - scrollOffsetX - X_OFFSET) / (glyphWidth);
    int newCurPosY = (curMouseY - scrollOffsetY - Y_OFFSET) / glyphHeight;
    *newMouseX = newCurPosX >= 0 ? newCurPosX : 0;
    *newMouseY = newCurPosY;
}

void cursorToPos(Cursor* cursor, Vec2* cursorPos, int glyphWidth, int glyphHeight)
{
    float newCursorPosX = (float)cursor->index * (float)glyphWidth;
    float newCursorPosY = ((float)cursor->line * (float)glyphHeight);
    // printf("curx = %d, cury = %d\n", newCursorPosX, newCursorPosY);
    cursorPos->x = newCursorPosX;
    cursorPos->y = newCursorPosY;
}

void moveCursorDown(Cursor* cursor, Text* text)
{
    if (cursor->line < text->lineCount - 1) {
        cursor->line++;
        // No index memory. Doing it the notepad way for now.
        if (cursor->index > gapUsed(text->lines[cursor->line])) {
            cursor->index = moveCursorToEnd(text->lines[cursor->line]);
        }
        else {
            moveCursor(text->lines[cursor->line], cursor->index);
        }
    }
}

void moveCursorUp(Cursor* cursor, Text* text)
{
    if (cursor->line > 0) {
        cursor->line--;
        // No index memory. Doing it the notepad way for now.
        if (cursor->index > gapUsed(text->lines[cursor->line])) {
            cursor->index = moveCursorToEnd(text->lines[cursor->line]);
        }
        else {
            moveCursor(text->lines[cursor->line], cursor->index);
        }
    }
}

void renderScrollBar(SDL_Renderer* renderer, Text* text, int glyphHeight, ClickableItems* buttons, Viewport* viewport) {
    SDL_Surface* sqSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, 50, 50, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
    sdl_cc(SDL_FillRect(sqSurface, NULL, 0xAAFFFFFF));
    SDL_Texture* sqTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, sqSurface));
    // Calculate scroll bar size
    int scrollbarHeight = (viewport->windowSize.y) / (text->lineCount - 1) * glyphHeight;
    float scrollbarOffset = ((float)viewport->scrollCountY / (float)(text->lineCount - 1)) * ((float)viewport->windowSize.y - (float)scrollbarHeight);
    // printf("%f\n", scrollbarOffset);
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
    // Initialize SDL window, renderer and font
    sdl_cc(SDL_Init(SDL_INIT_VIDEO));
    sdl_cc(TTF_Init());
    TTF_Font* font = NULL;
    loadFont("DejaVuSansMono.ttf", 18, &font);
    SDL_Window* window = sdl_cp(SDL_CreateWindow("Text", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer = sdl_cp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    // Intialize global structs
    Editor editor;
    Viewport viewport;
    // White color in rgba
    SDL_Color color = { 255, 255, 255, 255 };
    Glyph_Map* glyphMap = createGlyphMap();
    SDL_Texture* fontTexture = cacheTexture(renderer, font, glyphMap);
    SDL_Surface* cursorSurface = sdl_cp(SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
    // Make the cursor transparent.
    sdl_cc(SDL_FillRect(cursorSurface, NULL, 0xAAFFFFFF));
    SDL_Texture* cursorTexture = sdl_cp(SDL_CreateTextureFromSurface(renderer, cursorSurface));
    Cursor cursor = { .line = 0, .index = 0 };
    int mouseX = 0;
    int mouseY = 0;
    int lshift = 0;
    int lctrl = 0;
    int rctrl = 0;
    int scrollWheelClicked = 0;

    // Init viewport
    viewport.scrollCountX = 0;
    viewport.scrollCountY = 0;

    Text* text = createText();
    bool exit = false;

    // Define Clickable Buttons (TODO: Make this a function to check for bounds)
    SDL_Rect buttonsLocations[10];
    ClickableItems buttons = {.clickableRects = buttonsLocations, .count = 0};
    buttons.clickableRects[0] = (SDL_Rect) {.h = 0, .w = 0, .x = 0, .y = 0};
    buttons.count += 1;

    if (argc >= 2) {
        char const* fileName = argv[1];
        openFile(fileName, text);
        moveCursor(text->lines[cursor.line], cursor.index);
    }

    while (!exit) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int winW, winH = 0;
                    SDL_GetWindowSize(window, &winW, &winH);
                    viewport.windowSize.x = winW;
                    viewport.windowSize.y = winH;
                    // printf("Window size w=%d, h=%d\n", windowW, windowH);
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_GetMouseState(&mouseX, &mouseY);
                // Check if a button has been clicked
                if(mouseOnButton(mouseX, mouseY, &buttons)) {
                    printf("Button Clicked!\n");
                    // Currently the only button is the scrollwheel so start scrolling
                    scrollWheelClicked = 1;
                    break;
                }
                size_t newMouseX = 0;
                size_t newMouseY = 0;
                mouseToLinePos(&newMouseX, &newMouseY, mouseX, mouseY, glyphMap->glyphWidth, glyphMap->glyphHeight, &viewport);
                // Check if line postion is valid
                if (newMouseY > text->lineCount - 1) {
                    newMouseY = text->lineCount - 1;
                }
                cursor.line = newMouseY;
                // Check if column position is valid
                size_t lineLength = (text->lines[cursor.line]->cursor + text->lines[cursor.line]->length) - text->lines[cursor.line]->gapEnd;
                if (newMouseX > lineLength) {
                    newMouseX = lineLength;
                }
                cursor.index = newMouseX;
                moveCursor(text->lines[cursor.line], cursor.index);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                scrollWheelClicked = 0;
                break;
            }
            case SDL_MOUSEMOTION:
            {
                if(scrollWheelClicked) {
                    SDL_GetMouseState(&mouseX, &mouseY);
                    Vec2 mousePos = {.x = mouseX, .y = mouseY};
                    int scrollbarHeight = buttons.clickableRects[0].h;
                    float lineNum = mouseY / ((float)viewport.windowSize.y - (float)scrollbarHeight) * (float)(text->lineCount - 1);
                    scrollTo((int)lineNum, text, &viewport);
                }
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                // printf("Mouse Wheel Event\n");
                // printf("Direction=%d\n", event.wheel.direction);
                // printf("Scroll Amount=%d\n", event.wheel.y);
                int shiftmod = lshift;
                if (shiftmod) {
                    if (event.wheel.direction == 0) {
                        scroll(event.wheel.y, event.wheel.x, text, &viewport);
                    }
                }
                else {
                    if (event.wheel.direction == 0) {
                        scroll(event.wheel.x, event.wheel.y, text, &viewport);
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
                if (ctrlMod == 0) {
                    size_t textSize = strlen(event.text.text);
                    insertOnLine(text, cursor.line, event.text.text, textSize);
                    cursor.index += textSize;
                }
                break;
            }
            case SDL_KEYUP:
            {
                switch (event.key.keysym.sym) {
                case SDLK_LCTRL:
                {
                    lctrl = 0;
                    break;
                }
                case SDLK_LSHIFT:
                {
                    lshift = 0;
                    break;
                }
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
                    if (ctrlMod) {
                        if (argc >= 2) {
                            size_t prevLine = cursor.line;
                            size_t prevIndex = cursor.index;
                            char const* fileName = argv[1];
                            saveFile(fileName, text);
                            moveCursor(text->lines[prevLine], prevIndex);
                            cursor.line = prevLine;
                            cursor.index = prevIndex;
                        }
                    }
                    break;
                }
                case SDLK_BACKSPACE:
                {
                    if (cursor.index > 0) {
                        cursor.index--;
                        deleteFromLine(text, cursor.line);
                    }
                    else {
                        if (cursor.line > 0) {
                            // Delete line
                            // printf("Current Line=%ld, Total Lines = %ld\n", cursor.line, text->lineCount);
                            size_t newIndex = deleteLine(text, cursor.line, cursor.index);
                            cursor.line--;
                            cursor.index = newIndex;
                            // printf("Current Line=%ld, Total Lines = %ld\n", cursor.line, text->lineCount);
                        }
                    }
                    break;
                }
                case SDLK_RETURN:
                {
                    cursor.line++;
                    createNewLine(text, cursor.line, cursor.index);
                    cursor.index = 0;
                    moveCursor(text->lines[cursor.line], cursor.index);
                    break;
                }
                case SDLK_LEFT:
                {
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
                case SDLK_RIGHT:
                {
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
                case SDLK_UP:
                {
                    // Move Cursor
                    moveCursorUp(&cursor, text);
                    Vec2 cursorPos = {
                        .x = 0,
                        .y = 0
                    };
                    cursorToPos(&cursor, &cursorPos, glyphMap->glyphWidth, glyphMap->glyphHeight);
                    float scrollOffsetY = -(float)viewport.scrollCountY * (float)glyphMap->glyphHeight;
                    cursorPos.y += scrollOffsetY;
                    if (!isInViewBox(cursorPos, viewport.windowSize)) {
                        scroll(0, 1, text, &viewport);
                        moveCursorUp(&cursor, text);
                    }
                    break;
                }
                case SDLK_DOWN:
                {
                    // Move cursor
                    moveCursorDown(&cursor, text);
                    Vec2 cursorPos = {
                        .x = 0,
                        .y = 0
                    };
                    cursorToPos(&cursor, &cursorPos, glyphMap->glyphWidth, glyphMap->glyphHeight);
                    float scrollOffsetY = -(float)viewport.scrollCountY * (float)glyphMap->glyphHeight;
                    cursorPos.y += scrollOffsetY;
                    cursorPos.y += glyphMap->glyphHeight;
                    if (!isInViewBox(cursorPos, viewport.windowSize)) {
                        scroll(0, -1, text, &viewport);
                        moveCursorDown(&cursor, text);
                    }
                    break;
                }
                case SDLK_PAGEUP:
                {
                    if (viewport.scrollCountX == 0) {
                        break;
                    }
                    viewport.scrollCountX--;
                    // moveCursorUp(&cursor, text);
                    break;
                }
                case SDLK_PAGEDOWN:
                {
                    if (viewport.scrollCountX == text->lineCount - 1) {
                        break;
                    }
                    viewport.scrollCountX++;
                    // moveCursorDown(&cursor, text);
                    break;
                }
                case SDLK_TAB:
                {
                    // Add 4 spaces for each tab key press
                    char* tabString = "    ";
                    insertOnLine(text, cursor.line, tabString, strlen(tabString));
                    cursor.index += strlen(tabString);
                    // printf("This is sizeof=%ld\n", sizeof(tabString));
                }
                }
                break;
            }
            }
        }

        sdl_cc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        sdl_cc(SDL_RenderClear(renderer));
        renderText(renderer, text, &cursor, fontTexture, cursorTexture, color, glyphMap, &viewport);
        renderScrollBar(renderer, text, glyphMap->glyphHeight, &buttons, &viewport);
        // SDL_RenderCopy(renderer, fontTexture, NULL, &tempRect);
        SDL_RenderPresent(renderer);
    }
    freeText(text);
    freeGlyphMap(glyphMap);
    TTF_CloseFont(font);
    SDL_DestroyTexture(cursorTexture);
    SDL_FreeSurface(cursorSurface);
    SDL_DestroyTexture(fontTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
