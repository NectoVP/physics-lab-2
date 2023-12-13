#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
GLFWwindow* SetupWindow();
void FreeMemory(GLFWwindow* window, GLuint VAO, GLuint VBO, GLuint EBO, Shader& shader);
