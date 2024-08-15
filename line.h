#ifndef LINE_H_
#define LINE_H_

#include "gap.h"

typedef struct {
    size_t lineCount;
    GapBuffer** lines;
    size_t maxSize;
} Text;

Text* createText();
void freeText(Text* lines);
void createNewLine(Text* text, size_t index, size_t linePos);
void insertOnLine(Text* text, int line, char* string, size_t stringLength);
void deleteFromLine(Text* text, int line);

#endif // LINE_H_