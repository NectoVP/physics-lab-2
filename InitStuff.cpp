#include "InitStuff.h"
#include "imgui.h"
#include "implot/implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Shader.h"
#define WINDOWSIZE 1000

GLFWwindow* SetupWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "lab", NULL, NULL);
	glfwMakeContextCurrent(window);

	gladLoadGL();

	glViewport(0, 0, WINDOWSIZE, WINDOWSIZE);

	return window;
}

void FreeMemory(GLFWwindow* window, GLuint VAO, GLuint VBO, GLuint EBO, Shader& shader)
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	shader.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
}