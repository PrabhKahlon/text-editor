#include "line.h"
#include <string.h>
#include <stdio.h>

#define MIN_LINES 100

// Creates the Text object that holds line data. Initially only the first line is set to a buffer.
// The rest is set to NULL.
Text* createText()
{
    Text* text = (Text*)malloc(sizeof(Text));
    text->lines = (GapBuffer**)calloc(MIN_LINES, sizeof(GapBuffer*));
    text->maxSize = MIN_LINES;
    text->lineCount = 1;
    text->lines[0] = createBuffer();
    return text;
}

void freeText(Text* text)
{
    for (size_t i = 0; i < text->lineCount; i++) {
        freeBuffer(text->lines[i]);
    }
    free(text);
}

// Creates a new line. Checks to see if there is enough space in the array.
// If there is it adds a line otherwise it expands the array and then adds the line.
void createNewLine(Text* text, size_t index, size_t linePos)
{
    if (index > text->lineCount) {
        return;
    }
    text->lineCount++;
    if (text->lineCount >= text->maxSize) {
        text->maxSize = text->maxSize * 2;
        text->lines = (GapBuffer**)realloc(text->lines, sizeof(GapBuffer*) * text->maxSize);
    }
    if (index == text->lineCount - 1) {
        text->lines[index] = createBuffer();
    }
    else {
        // Inserting in the middle so shift by one
        memmove(text->lines + index + 1, text->lines + index, sizeof(GapBuffer*) * (text->lineCount - 1) - index);
        text->lines[index] = createBuffer();
    }

    // Move text from previous line buffer dependant on the cursor's index.
    size_t oldLine = index - 1;
    size_t endLine = text->lines[oldLine]->cursor + text->lines[oldLine]->length - text->lines[oldLine]->gapEnd;
    if (linePos < endLine) {
        copyBuffer(text->lines[index], text->lines[oldLine]);
        // Delete what we just copied from the buffer
        size_t deleteSize = text->lines[oldLine]->length - text->lines[oldLine]->gapEnd;
        moveCursor(text->lines[oldLine], endLine);
        for (size_t i = 0; i < deleteSize; i++) {
            deleteFromBuffer(text->lines[oldLine]);
        }
    }
    return;
}

//Deletes line at position lineNum and free's the associated buffer.
void deleteLine(Text* text, size_t lineNum, size_t linePos)
{
    text->lineCount--;
    GapBuffer* oldBuffer = text->lines[lineNum];

    //Copy contents after cursor to end of line
    size_t endLine = oldBuffer->cursor + oldBuffer->length - oldBuffer->gapEnd;
    if (linePos < endLine) {
        moveCursorToEnd(text->lines[lineNum - 1]);
        copyBuffer(text->lines[lineNum - 1], oldBuffer);
    }

    if (lineNum < text->lineCount) {
        memmove(text->lines + lineNum, text->lines + lineNum + 1, sizeof(GapBuffer*) * (text->lineCount - 1) - lineNum);
    }

    free(oldBuffer);
}

void insertOnLine(Text* text, int line, char* string, size_t stringLength)
{
    insertBuffer(text->lines[line], string, stringLength);
}

void deleteFromLine(Text* text, int line)
{
    deleteFromBuffer(text->lines[line]);
}