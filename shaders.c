#include "shaders.h"

void constructShaders(char **vertexShaderSource, char **fragmentShaderSource, unsigned int *shaderProgram) {
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar *const *)vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	int  success;
	char infoLog[2048];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
		fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
		exit(1);
	}

	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar *const *)fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
		fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
		exit(1);
	}

	*shaderProgram = glCreateProgram();
	glAttachShader(*shaderProgram, vertexShader);
	glAttachShader(*shaderProgram, fragmentShader);
	glLinkProgram(*shaderProgram);
	glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(*shaderProgram, sizeof(infoLog), NULL, infoLog);
		fprintf(stderr, "Shader linker error\n%s\n", infoLog);
		exit(1);
	}

	glUseProgram(*shaderProgram);
	glDeleteShader(vertexShader); // Once linked to the program, we can delete the shaders
	glDeleteShader(fragmentShader);
}
