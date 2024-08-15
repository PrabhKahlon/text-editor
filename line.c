#include "line.h"

#define MIN_LINES 100

Text* createText()
{
    Text* text = (Text*)malloc(sizeof(Text));
    text->lines = (GapBuffer**)malloc(sizeof(GapBuffer*) * MIN_LINES);
    for(size_t i = 0; i < MIN_LINES; i++) {
        text->lines[i] = createBuffer();
    }
    text->lineCount = 1;
    return text;
}

void freeText(Text* text)
{
    for(size_t i = 0; i < text->lineCount; i++) {
        freeBuffer(text->lines[i]);
    }
    free(text);
}

void createNewLine(Text* text)
{
    text->lineCount++;
    //text->lines = (GapBuffer**)realloc(text, sizeof(GapBuffer*) * text->lineCount);
    //text->lines[text->lineCount-1] = createBuffer();
    return;
}

void insertOnLine(Text* text, int line, char* string, size_t stringLength)
{
    insertBuffer(text->lines[line], string, stringLength);
}

void deleteFromLine(Text* text, int line)
{
    deleteFromBuffer(text->lines[line]);
}