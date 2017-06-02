#include "mandelbrot.h"

char *vertexShaderSource =
#include "vertexShader.glsl"
    ;

char *fragmentShaderSource =
#include "fragmentShader.glsl"
    ;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, width, height);
}

void doubleToVec2(double num, float (*vec4)[2]) {
	(*vec4)[0] = (float)num;
	(*vec4)[1] = (float)((double)(num) - (double)((*vec4)[0]));
	//(*vec4)[2] = (float)(num - (double)(num));
	//(*vec4)[3] = (float)((num - (double)(num)) - (float)(num - (double)(num)));
}

int main() {
	windowWidth  = 200;
	windowHeight = 200;
	_scale       = 1.0f;
	_graph_x     = 0.0f;
	_graph_y     = 0.0f;

	if (!glfwInit()) {
		return 1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Mandelbrot", NULL, NULL);
	if (window == NULL) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		return 1;
	}

	glViewport(0, 0, windowWidth, windowHeight);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // Set the callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	float vertices[] = {
	    -1.0f, -1.0f, 0.0f,
	    1.0f, -1.0f, 0.0f,
	    1.0f, 1.0f, 0.0f,
	    -1.0f, 1.0f, 0.0f};
	unsigned int indices[] = {
	    0, 1, 3,
	    1, 2, 3};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);             // bind Vertex Array Object
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // copy our vertices array in a buffer for OpenGL to use
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0); // then set our vertex attributes pointers
	glEnableVertexAttribArray(0);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	unsigned int shaderProgram;
	constructShaders(&vertexShaderSource, &fragmentShaderSource, &shaderProgram);

	scaleLoc   = glGetUniformLocation(shaderProgram, "scale");
	offsetxLoc = glGetUniformLocation(shaderProgram, "offset_x");
	offsetyLoc = glGetUniformLocation(shaderProgram, "offset_y");

	doubleToVec2(_scale, &_scale_v4);
	glUniform2fv(scaleLoc, 1, _scale_v4);

	doubleToVec2(_graph_x, &_graph_x_v4);
	glUniform2fv(offsetxLoc, 1, _graph_x_v4);
	doubleToVec2(_graph_y, &_graph_y_v4);
	glUniform2fv(offsetyLoc, 1, _graph_y_v4);

	printf("Initialized\n");

	while (!glfwWindowShouldClose(window)) {
		glfwGetWindowSize(window, &windowWidth, &windowHeight);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(shaderProgram);

		doubleToVec2(_scale, &_scale_v4);
		glUniform2fv(scaleLoc, 1, _scale_v4);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
