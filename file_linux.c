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
    return;
}

#endif