# Textogl - A simple OpenGL text rendering library.

Textogl renders text in OpenGL from nearly any font (any format supported by
Freetype).

### [textogl on Github](https://github.com/mattvchandler/textogl)

### [Documentation](https://mattvchandler.github.io/textogl/index.html)

## Usage

1. Create a textogl::Font_sys object for the desired font.
2. Call textogl::Font_sys::render_text() with the desired text, position, and
   color.
3. If the text will not change each frame, consider using textogl::Static_text
   object. This will prevent needing to rebuild quads for each rendering call

## Building & Installation

### Dependencies

* [Freetype](https://www.freetype.org/)
* OpenGL 3.0 +
    * OpenGL ES Support coming soon™

Textogl uses CMake to build

    $ mkdir build && cd build
    $ cmake ..
    $ make

Library will be built at build/src/textogl.a

#### Documentation
If doxygen is installed, library documentation can be generated with: `$ make doc`
