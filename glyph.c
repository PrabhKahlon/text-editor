#include "glyph.h"
#include <stdlib.h>


Glyph_Map* createGlyphMap()
{
    Glyph_Map* glyphMap = (Glyph_Map*)malloc(sizeof(Glyph_Map));

    glyphMap->maxGlyphs = MAX_GLYPHS;
    glyphMap->glyphs = (Glyph_Rect**)malloc(sizeof(Glyph_Rect*) * MAX_GLYPHS);

    for (int i = 0; i < glyphMap->maxGlyphs; i++) {
        glyphMap->glyphs[i] = NULL;
    }

    glyphMap->glyphHeight = 0;

    return glyphMap;
}

void freeGlyphMap(Glyph_Map* glyphMap)
{
    if (glyphMap == NULL)
        return;

    for (int i = 0; i < glyphMap->maxGlyphs; ++i)
    {
        Glyph_Rect* rect = glyphMap->glyphs[i];
        if (rect != NULL)
        {
            free(rect);
        }
    }

    free(glyphMap->glyphs);
    free(glyphMap);
}

void addGlyph(Glyph_Map* glyphMap, char c, SDL_Rect* glyph)
{
    int mapIndex = c - 32;

    if (glyphMap == NULL) {
        return;
    }

    //Create a new node and add to the map if no node exists
    if (glyphMap->glyphs[mapIndex] == NULL) {
        Glyph_Rect* newGlyph = (Glyph_Rect*)malloc(sizeof(Glyph_Rect));
        newGlyph->x = glyph->x;
        newGlyph->y = glyph->y;
        newGlyph->w = glyph->w;
        newGlyph->h = glyph->h;
        glyphMap->glyphs[mapIndex] = newGlyph;
    }
    else {
        //Rewrite the node if it exists.
        Glyph_Rect* newGlyph = glyphMap->glyphs[mapIndex];
        newGlyph->x = glyph->x;
        newGlyph->y = glyph->y;
        newGlyph->w = glyph->w;
        newGlyph->h = glyph->h;
        glyphMap->glyphs[mapIndex] = newGlyph;
    }
    return;
}

void setGlyphHeight(Glyph_Map* glyphMap, int height) 
{
    glyphMap->glyphHeight = height;
}