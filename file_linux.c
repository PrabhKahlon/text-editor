#ifdef __linux__

#include "file_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gap.h"

#define MAX_BUFFER 256

void openFile(char const* fileName, Text* text)
{
    printf("%s\n", fileName);
    FILE* txtFile = fopen(fileName, "r");
    if(txtFile == NULL) {
        return;
    }
    printf("File loaded\n");
    size_t line = 0;
    char buffer[MAX_BUFFER] = "";
    while (fgets(buffer, MAX_BUFFER, txtFile)) {
        int newLine = strcspn(buffer, "\n");
        buffer[newLine] = 0;
        insertBuffer(text->lines[line], buffer, strlen(buffer));
        if(newLine < MAX_BUFFER-1) {
            line++;
            createNewLine(text, line, 0);
        }
    }
    fclose(txtFile);
    return;
}

void saveFile(char const* fileName, Text* text)
{
    FILE* txtFile = fopen(fileName, "w");
    if(txtFile == NULL) {
        return;
    }
    for(size_t i = 0; i < text->lineCount; i++) {
        moveCursorToEnd(text->lines[i]);
        fwrite(text->lines[i]->string, sizeof(char), text->lines[i]->cursor, txtFile);
        //fputs(text->lines[i]->string, txtFile);
        fputs("\n", txtFile);
    }
    fclose(txtFile);
    return;
}

#endif