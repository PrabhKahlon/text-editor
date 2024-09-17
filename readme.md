# Text Editor

This is a simple text editor built in C using the SDL library, leveraging OpenGL for rendering. The project serves as a personal exploration into low-level text rendering and window management, designed to challenge and expand my understanding of C programming.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Getting Started](#getting-started)
- [Development Notes](#development-notes)
- [Challenges](#challenges)
- [Acknowledgements](#acknowledgements)

## Overview

This project began as an experiment to create a basic text editor using C and SDL (Simple DirectMedia Layer). By building this editor, I aimed to sharpen my programming skills, particularly in system-level programming, graphics handling, and real-time text rendering. This project has taught me a lot about managing resources in C, performance optimization, and data structures.

The editor supports basic ASCII characters, with plans to extend to Unicode in the future. Text rendering is achieved by creating a sprite sheet from loaded fonts, allowing for faster rendering performance.

## Features

- **Basic Text Rendering**: Renders text to the screen using SDL_ttf and OpenGL.
- **Custom Font Loading**: Supports loading and displaying any TTF font.
- **ASCII Support**: Full support for ASCII characters.
- **Multi-line Support**: The editor handles multi-line text with correct positioning.
- **Cursor**: A visible, movable cursor that correctly inserts text at the cursor's position.
- **Efficient Font Rendering**: Pre-creates a font texture/sprite sheet for performance improvements.

### Planned Features

- **Unicode Support**: Full support for rendering Unicode characters.
- **Window Resizing**: Dynamically adjust the window size while preserving text layout.
- **Copy, Cut, and Paste**: Implement basic clipboard operations.
- **Mouse Support**: Enable cursor movement and text selection via mouse.
- **Windows Port**: Currently, the project is developed and tested on Linux, but there are plans to bring it to Windows.

## Getting Started

### Prerequisites

To build and run the project, you will need:

- A C compiler (GCC, Clang, etc.)
- SDL2 and SDL_ttf libraries installed
- OpenGL (required for rendering)

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/PrabhKahlon/text-editor.git
   ```

2. Install SDL and SDL_ttf libraries. If you're on Linux, you can use:
   ```bash
   sudo apt-get install libsdl2-dev libsdl2-ttf-dev
   ```

3. Build the project using the provided `Makefile`:
   ```bash
   make
   ```

4. Run the editor:
   ```bash
   ./text
   ```

### Makefile

Here is the makefile used for the project:

```makefile
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags sdl2 SDL2_ttf`
LIBS=`pkg-config --libs sdl2 SDL2_ttf` -lm

text: main.c
      $(CC) $(CFLAGS) -o text main.c vec.c glyph.c gap.c line.c file_linux.c $(LIBS)
```

Make sure you have the necessary SDL2 and SDL_ttf packages installed on your system before building.

## Development Notes

- The project is built using SDL to create a window and render text with the help of SDL_ttf. The initial implementation was slow, especially with SDL_ttf, but I optimized it by caching font glyphs as textures.
- A gap buffer was chosen as the underlying data structure for editing text. It works efficiently with small to medium-sized files (~128MB). Alternatives like ropes or piece tables were considered but deemed unnecessary for the current scope.

### Key Learnings

1. **C Syntax**: I delved deeper into C, learning advanced concepts such as compound literals and designated initializers.
2. **SDL Performance**: SDL_ttf can be slow when rendering text on the fly. I solved this by pre-creating a sprite sheet of font glyphs, which improved performance significantly.
3. **Text Data Structures**: A gap buffer was used due to its simplicity and efficiency with small files. Other options like ropes or piece tables were explored but not implemented.

## Challenges

- **Text Rendering Performance**: The default method of rendering text with SDL_ttf was slow for large texts. To overcome this, I created a cached texture of font glyphs that is rendered directly, greatly improving performance.
  
- **Varying Font Sizes**: Different font sizes and styles don't render uniformly. This causes issues with vertical cursor movement as it tends to jump around the screen due to the font spacing. A temporary fix involves using monospace fonts to ensure correct spacing.

## Acknowledgements

- **SDL**: For making cross-platform window and rendering so accessible.
- **SDL_ttf**: For handling TrueType fonts.
