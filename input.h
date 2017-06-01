#ifndef _INPUT_H_
#define _INPUT_H_

#include "glad/glad.h"

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

int _cursor_position_x;
int _cursor_position_y;
float _scroll;

#endif // _INPUT_H_
