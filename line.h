#ifndef LINE_H_
#define LINE_H_

#include "gap.h"

typedef struct {
    int lineCount;
    GapBuffer** lines;
} Text;

Text* createText();
void freeText(Text* lines);
void createNewLine(Text* text);
void insertOnLine(Text* text, int line, char* string, size_t stringLength);
void deleteFromLine(Text* text, int line);

#endif // LINE_H_