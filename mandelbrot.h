#ifndef _MANDELBROT_H_
#define _MANDELBROT_H_

#include "glad/glad.h"

#include "input.h"
#include "shaders.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>

int          windowWidth;
int          windowHeight;
unsigned int scaleLoc;
unsigned int offsetLoc;
double       _scale;
double       _graph_x;
double       _graph_y;

#endif // _MANDELBROT_H_
