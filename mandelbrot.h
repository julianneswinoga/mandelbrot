#ifndef _MANDELBROT_H_
#define _MANDELBROT_H_

#include "glad/glad.h"

#include "float.h"
#include "input.h"
#include "shaders.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>

void doubleToVec2(double num, float (*vec4)[2]);

int          windowWidth;
int          windowHeight;
unsigned int scaleLoc;
unsigned int offsetxLoc;
unsigned int offsetyLoc;
double  _scale;
double  _graph_x;
double  _graph_y;
float        _scale_v4[2], _graph_x_v4[2], _graph_y_v4[2];

#endif // _MANDELBROT_H_
