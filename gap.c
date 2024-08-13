#include "gap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_BUFFER 1024

GapBuffer* createBuffer()
{
    GapBuffer* newBuffer = (GapBuffer*)malloc(sizeof(GapBuffer));
    if (newBuffer == NULL) {
        return newBuffer;
    }
    newBuffer->cursor = 0;
    newBuffer->gapEnd = MIN_BUFFER;
    newBuffer->length = MIN_BUFFER;
    newBuffer->string = (char*)malloc(sizeof(char) * MIN_BUFFER);
    return newBuffer;
}

void freeBuffer(GapBuffer* gapBuffer)
{
    if (gapBuffer == NULL) {
        return;
    }
    free(gapBuffer->string);
    free(gapBuffer);
}

void insertBuffer(GapBuffer* gapBuffer, char* text, size_t textSize)
{
    if (gapBuffer == NULL) {
        return;
    }
    //Check if gap is used and if there is space, insert into gap
    for (size_t i = 0; i < textSize; i++) {
        if (gapBuffer->cursor == gapBuffer->gapEnd) {
            //GAP IS ALL USED UP
            //TODO: EXPAND BUFFER
        }
        gapBuffer->string[gapBuffer->cursor++] = text[i];
    }
    return;
}

void deleteBuffer(GapBuffer* gapBuffer)
{
    if (gapBuffer->cursor > 0) {
        gapBuffer->cursor--;
    }
    return;
}

void cursorLeft(GapBuffer* gapBuffer)
{
    if(gapBuffer->cursor > 0) {
        gapBuffer->string[--gapBuffer->gapEnd] = gapBuffer->string[--gapBuffer->cursor];
    }
    return;
}

void cursorRight(GapBuffer* gapBuffer)
{
    if(gapBuffer->gapEnd < gapBuffer->length) {
        gapBuffer->string[gapBuffer->cursor++] = gapBuffer->string[gapBuffer->gapEnd++];
    }
    return;
}