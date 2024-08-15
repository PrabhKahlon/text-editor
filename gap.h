#ifndef GAP_H_
#define GAP_H_

#include <stdlib.h>

typedef struct {
    size_t cursor;
    size_t gapEnd;
    size_t length;
    char* string;
} GapBuffer;

GapBuffer* createBuffer();
void freeBuffer(GapBuffer* gapBuffer);
void expandBuffer(GapBuffer* gapBuffer);
void insertBuffer(GapBuffer* gapBuffer, char* text, size_t textSize);
void deleteFromBuffer(GapBuffer* gapBuffer);
void cursorLeft(GapBuffer* gapBuffer);
void cursorRight(GapBuffer* gapBuffer);
void moveCursor(GapBuffer* gapBuffer, size_t position);

#endif // GAP_H_