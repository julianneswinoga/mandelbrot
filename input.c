#include "input.h"

int    _cursor_position_x = -1;
int    _cursor_position_y = -1;
bool   _mouse_l_button    = false;
double _scroll            = 0;

double click_x = 0.0f;
double click_y = 0.0f;
double last_x  = 0.0f;
double last_y  = 0.0f;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

double cursor_to_world_coords(int cursorpos, int windowsize) {
	return (((2.0f * cursorpos) / windowsize) - 1.0f) * _scale;
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	_cursor_position_x = xpos;
	_cursor_position_y = ypos;
	if (_mouse_l_button) {
		_graph_x = (click_x + last_x) - cursor_to_world_coords(_cursor_position_x, windowWidth);
		_graph_y = (-click_y + last_y) + cursor_to_world_coords(_cursor_position_y, windowHeight);
		glUniform2f(offsetLoc, _graph_x, _graph_y);
		printf("_mouse_x: %f\n", ((((2.0f * _cursor_position_x) / (float)windowWidth) - 1.0f) * _scale));
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		printf("LEFT CLICK @ (%i,%i)\n", _cursor_position_x, _cursor_position_y);

		if (action == GLFW_PRESS) {
			_mouse_l_button = true;
			click_x         = cursor_to_world_coords(_cursor_position_x, windowWidth);
			click_y         = cursor_to_world_coords(_cursor_position_y, windowHeight);
		} else {
			_mouse_l_button = false;
			last_x          = _graph_x;
			last_y          = _graph_y;
			printf("last_x: %f\n", last_x);
		}
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		printf("RIGHT CLICK @ (%i,%i)\n", _cursor_position_x, _cursor_position_y);
	}
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
	_scroll += yoffset;
	_scale = pow(2, -_scroll);
	printf("%f\n", _scale);
}
