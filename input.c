#include "input.h"

int _cursor_position_x = -1;
int _cursor_position_y = -1;
float _scroll = 0;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	_cursor_position_x = xpos;
	_cursor_position_y = ypos;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		printf("LEFT CLICK @ (%i,%i)\n", _cursor_position_x, _cursor_position_y);
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		printf("RIGHT CLICK @ (%i,%i)\n", _cursor_position_x, _cursor_position_y);
	}
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
	_scroll += yoffset;
	printf("Scroll: %f\n", _scroll);
}
