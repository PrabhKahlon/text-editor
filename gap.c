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

void expandBuffer(GapBuffer* gapBuffer) 
{
    if(gapBuffer == NULL) {
        return;
    }
    //Get new length for buffer and allocate memory. For now we just double the length.
    size_t oldLength = gapBuffer->length;
    size_t newLength = oldLength*2; // Add size check so it does not overflow
    char* newString = (char*)realloc(gapBuffer->string, sizeof(char)*newLength);
    //Need to copy everything after the gap till the end.
    size_t afterGapText = gapBuffer->length - gapBuffer->gapEnd;
    memmove(newString + newLength - afterGapText, gapBuffer->string + gapBuffer->gapEnd, afterGapText);
    gapBuffer->string = newString;
    gapBuffer->gapEnd = newLength - afterGapText;
    gapBuffer->length = newLength;
    return;
}

void insertBuffer(GapBuffer* gapBuffer, char* text, size_t textSize)
{
    if (gapBuffer == NULL) {
        return;
    }
    //Check if gap is used and if there is space, insert into gap
    for (size_t i = 0; i < textSize; i++) {
        if (gapBuffer->cursor == gapBuffer->gapEnd) {
            expandBuffer(gapBuffer);
        }
        gapBuffer->string[gapBuffer->cursor++] = text[i];
    }
    return;
}

void deleteFromBuffer(GapBuffer* gapBuffer)
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

void moveCursor(GapBuffer* gapBuffer, size_t position)
{
    //Check that the position is not invalid
    if(position > (gapBuffer->cursor + gapBuffer->length - gapBuffer->gapEnd)) {
        return;
    }
    //Move cursor right or left to get to correct position
    if(position > gapBuffer->cursor) {
        size_t positionsToMove = (position - gapBuffer->cursor);
        for(size_t i = 0; i < positionsToMove; i++) {
            cursorRight(gapBuffer);
        }
    } else {
        size_t positionsToMove = (gapBuffer->cursor - position);
        for(size_t i = 0; i < positionsToMove; i++) {
            cursorLeft(gapBuffer);
        }
    }
    return;
}

// Copies everything from the src buffer after the cursor to the new buffer where the cursor is.
void copyBuffer(GapBuffer* dest, GapBuffer* src)
{
    char* copy = src->string + src->gapEnd;
    size_t copySize = src->length - src->gapEnd;
    insertBuffer(dest, copy, copySize);
    return;
}