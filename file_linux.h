#ifndef FILE_LINUX_H_
#define FILE_LINUX_H_

#ifdef __linux__

#include "line.h"

void openFile(char const* fileName, Text* text);
void saveFile(char const* fileName, Text* text);

#endif

#endif