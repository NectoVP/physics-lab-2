#include <iostream>
#include <cmath>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "implot/implot.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "InitStuff.h"
#include "Shader.h"

#define FIELD_SIZE 8*8*8*3
#define CIRCLE_SIZE 360 * 3
#define DOTS_AMOUNT 8*8*8
#define pi 3.1415926535
#define PLOT_SIZE 100 

struct PlaneCoords {
	PlaneCoords(float x, float y, float z) : x1(-x), x2(x), y1(-y), y2(y), z1(-z), z2(z) {}
	PlaneCoords() : PlaneCoords(0.f, 0.f, 0.f) {}

	float x1 = -0.4f;
	float x2 = 0.4f;

	float y1 = -0.3f;
	float y2 = 0.3f;

	float z1 = -0.3f;
	float z2 = 0.3f;
};

bool operator==(const PlaneCoords& a, const PlaneCoords& b) {
	if (a.x1 == b.x1 && a.x2 == b.x2 && a.y1 == b.y1 && a.y2 == b.y2 && a.z1 == b.z1 && a.z2 == b.z2)
		return true;
	return false;
}

bool operator!=(const PlaneCoords& a, const PlaneCoords& b) {
	return !(a == b);
}

GLfloat len(GLfloat* vec) {
	return sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

struct vec3 {
	vec3(GLfloat x, GLfloat y, GLfloat z) : x(x), y(y), z(z) { }

	GLfloat x;
	GLfloat y;
	GLfloat z;
};

GLfloat len(vec3 vec) {
	return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

void print(GLfloat* vec, int i) {
	std::cout << vec[i] << " " << vec[i + 1] << std::endl;
	std::cout << vec[i + 3] << " " << vec[i + 4] << std::endl;
}

bool checkIfInsideCapacitor(GLfloat* vec, int i) {
	//if ((vec[i] <= planeCoords.x1 && vec[i + 1] <= planeCoords.y2 && vec[i + 1] >= planeCoords.y1 && vec[i] >= planeCoords.x2) || (vec[i + 3] >= planeCoords.x2 && vec[i + 4] <= planeCoords.y2 && vec[i + 4] >= planeCoords.y1 && vec[i + 3] <= planeCoords.x1))
	//	return true;
	return false;
}

vec3 calculateCorners(GLfloat lx, GLfloat ly, GLfloat lz, GLfloat rx, GLfloat ry, GLfloat rz, GLfloat Eleft, GLfloat Eright, GLfloat* vec, int i) {
	GLfloat templeft[3] = { 0.0f, 0.0f, 0.0f };
	GLfloat tempright[3] = { 0.0f, 0.0f, 0.0f };

	templeft[0] = (lx - vec[i]) * Eleft;
	templeft[1] = (ly - vec[i + 1]) * Eleft;
	templeft[2] = (lz - vec[i + 2]) * Eleft;
	tempright[0] = -(rx - vec[i]) * Eright;
	tempright[1] = -(ry - vec[i + 1]) * Eright;
	tempright[2] = -(rz - vec[i + 2]) * Eright;

	return vec3(templeft[0] + tempright[0], templeft[1] + tempright[1], templeft[2] + tempright[2]);
}

vec3 calculateHorizontalEdges(GLfloat lx, GLfloat ly, GLfloat rx, GLfloat ry, GLfloat Eleft, GLfloat Eright, GLfloat* vec, int i) {
	GLfloat templeft[3] = { 0.0f, 0.0f, 0.0f };
	GLfloat tempright[3] = { 0.0f, 0.0f, 0.0f };

	templeft[0] = (lx - vec[i]) * Eleft;
	templeft[1] = (ly - vec[i + 1]) * Eleft;
	tempright[0] = -(rx - vec[i]) * Eright;
	tempright[1] = -(ry - vec[i + 1]) * Eright;;

	return vec3(templeft[0] + tempright[0], templeft[1] + tempright[1], vec[i + 2]);
}

vec3 calculateVerticalEdges(GLfloat lx, GLfloat lz, GLfloat rx, GLfloat rz, GLfloat Eleft, GLfloat Eright, GLfloat* vec, int i) {
	GLfloat templeft[3] = { 0.0f, 0.0f, 0.0f };
	GLfloat tempright[3] = { 0.0f, 0.0f, 0.0f };

	templeft[0] = (lx - vec[i]) * Eleft;
	templeft[2] = (lz - vec[i + 2]) * Eleft;
	tempright[0] = -(rx - vec[i]) * Eright;
	tempright[2] = -(rz - vec[i + 2]) * Eright;;

	return vec3(templeft[0] + tempright[0], vec[i + 1], templeft[2] + tempright[2]);
}

void updateMainCalculation(GLfloat Eleft, GLfloat Eright, float LineSize, GLfloat* vertices, GLfloat* AllVectorPoints, GLuint VAO, GLuint VBO, GLuint EBO, GLuint* indices, int PointsSize, int IndicesSize, PlaneCoords& planeCoords, bool whatToDisplay, float radius, bool applyrot, bool dielmod, PlaneCoords& dielCords, float koef) {
	int k = 0;
	for (int i = 0; i < FIELD_SIZE * 2 - 5; i += 6, k += 3) {
		AllVectorPoints[i] = vertices[k];
		AllVectorPoints[i + 1] = vertices[k + 1];
		AllVectorPoints[i + 2] = vertices[k + 2];

		GLfloat templeft1[3] = { 0.0f, 0.0f, 0.0f };
		GLfloat tempright1[3] = { 0.0f, 0.0f, 0.0f };
		GLfloat tempvec2[3] = { 0.0f, 0.0f, 0.0f };

		if (whatToDisplay) {
			if (AllVectorPoints[i + 1] * AllVectorPoints[i + 1] + AllVectorPoints[i + 2] * AllVectorPoints[i + 2] <= radius * radius) {
				templeft1[0] = (planeCoords.x1 - AllVectorPoints[i]) * Eleft;
				tempright1[0] = -(planeCoords.x2 - AllVectorPoints[i]) * Eright;
				tempvec2[0] = templeft1[0] + tempright1[0];
				tempvec2[1] = AllVectorPoints[i + 1];
				tempvec2[2] = AllVectorPoints[i + 2];
			}
			else {
				templeft1[0] = (planeCoords.x1 - AllVectorPoints[i]) * Eleft;
				tempright1[0] = -(planeCoords.x2 - AllVectorPoints[i]) * Eright;
				templeft1[1] = (0 - radius - AllVectorPoints[i+1]) * Eleft;
				tempright1[1] = -(0 - radius - AllVectorPoints[i+1]) * Eright;
				templeft1[2] = (0 - radius - AllVectorPoints[i+2]) * Eleft;
				tempright1[2] = -(0 - radius - AllVectorPoints[i+2]) * Eright;
				tempvec2[0] = templeft1[0] + tempright1[0];
				tempvec2[1] = templeft1[1] + tempright1[1];
				tempvec2[2] = templeft1[2] + tempright1[2];
			}
		}
		else {
			if (AllVectorPoints[i + 1] > planeCoords.y1 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 2] > planeCoords.z1 && AllVectorPoints[i + 2] < planeCoords.z2) {
				templeft1[0] = (planeCoords.x1 - AllVectorPoints[i]) * Eleft;
				tempright1[0] = -(planeCoords.x2 - AllVectorPoints[i]) * Eright;
				tempvec2[0] = templeft1[0] + tempright1[0];
				tempvec2[1] = AllVectorPoints[i + 1];
				tempvec2[2] = AllVectorPoints[i + 2];
			}

			if (AllVectorPoints[i + 1] <= planeCoords.y1 && AllVectorPoints[i + 2] <= planeCoords.z1) {
				vec3 result = calculateCorners(planeCoords.x1, planeCoords.y1, planeCoords.z1, planeCoords.x2, planeCoords.y1, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] <= planeCoords.y1 && AllVectorPoints[i + 2] >= planeCoords.z2) {
				vec3 result = calculateCorners(planeCoords.x1, planeCoords.y1, planeCoords.z2, planeCoords.x2, planeCoords.y1, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] >= planeCoords.y2 && AllVectorPoints[i + 2] <= planeCoords.z1) {
				vec3 result = calculateCorners(planeCoords.x1, planeCoords.y2, planeCoords.z1, planeCoords.x2, planeCoords.y2, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] >= planeCoords.y2 && AllVectorPoints[i + 2] >= planeCoords.z2) {
				vec3 result = calculateCorners(planeCoords.x1, planeCoords.y2, planeCoords.z2, planeCoords.x2, planeCoords.y2, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}

			if (AllVectorPoints[i + 1] < planeCoords.y1 && AllVectorPoints[i + 2] < planeCoords.z2 && AllVectorPoints[i + 2] > planeCoords.z1) {
				vec3 result = calculateHorizontalEdges(planeCoords.x1, planeCoords.y1, planeCoords.x2, planeCoords.y1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] > planeCoords.y2 && AllVectorPoints[i + 2] < planeCoords.z2 && AllVectorPoints[i + 2] > planeCoords.z1) {
				vec3 result = calculateHorizontalEdges(planeCoords.x1, planeCoords.y2, planeCoords.x2, planeCoords.y2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 2] < planeCoords.z1 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 1] > planeCoords.y1) {
				vec3 result = calculateVerticalEdges(planeCoords.x1, planeCoords.z1, planeCoords.x2, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 2] > planeCoords.z2 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 1] > planeCoords.y1) {
				vec3 result = calculateVerticalEdges(planeCoords.x1, planeCoords.z2, planeCoords.x2, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
		}

		if (dielmod) {
			if (Eleft > 0) {
				Eleft = Eleft - Eleft / koef;
				Eright = Eright + Eright / koef;
			}
			else {
				Eleft = Eleft + Eleft / koef;
				Eright = Eright - Eright / koef;
			}
			if (AllVectorPoints[i + 1] > dielCords.y1 && AllVectorPoints[i + 1] < dielCords.y2 && AllVectorPoints[i + 2] > dielCords.z1 && AllVectorPoints[i + 2] < dielCords.z2) {
				templeft1[0] = (dielCords.x1 - AllVectorPoints[i]) * Eleft;
				tempright1[0] = -(dielCords.x2 - AllVectorPoints[i]) * Eright;
				tempvec2[0] = templeft1[0] + tempright1[0];
				tempvec2[1] = AllVectorPoints[i + 1];
				tempvec2[2] = AllVectorPoints[i + 2];
			}

			if (AllVectorPoints[i + 1] <= dielCords.y1 && AllVectorPoints[i + 2] <= dielCords.z1) {
				vec3 result = calculateCorners(dielCords.x1, dielCords.y1, dielCords.z1, dielCords.x2, dielCords.y1, dielCords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] <= dielCords.y1 && AllVectorPoints[i + 2] >= dielCords.z2) {
				vec3 result = calculateCorners(dielCords.x1, dielCords.y1, dielCords.z2, dielCords.x2, dielCords.y1, dielCords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] >= dielCords.y2 && AllVectorPoints[i + 2] <= dielCords.z1) {
				vec3 result = calculateCorners(dielCords.x1, dielCords.y2, dielCords.z1, dielCords.x2, dielCords.y2, dielCords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] >= dielCords.y2 && AllVectorPoints[i + 2] >= dielCords.z2) {
				vec3 result = calculateCorners(dielCords.x1, dielCords.y2, dielCords.z2, dielCords.x2, dielCords.y2, dielCords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}

			if (AllVectorPoints[i + 1] < dielCords.y1 && AllVectorPoints[i + 2] < dielCords.z2 && AllVectorPoints[i + 2] > dielCords.z1) {
				vec3 result = calculateHorizontalEdges(dielCords.x1, dielCords.y1, dielCords.x2, dielCords.y1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 1] > dielCords.y2 && AllVectorPoints[i + 2] < dielCords.z2 && AllVectorPoints[i + 2] > dielCords.z1) {
				vec3 result = calculateHorizontalEdges(dielCords.x1, dielCords.y2, dielCords.x2, dielCords.y2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 2] < dielCords.z1 && AllVectorPoints[i + 1] < dielCords.y2 && AllVectorPoints[i + 1] > dielCords.y1) {
				vec3 result = calculateVerticalEdges(dielCords.x1, dielCords.z1, dielCords.x2, dielCords.z1, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}
			if (AllVectorPoints[i + 2] > dielCords.z2 && AllVectorPoints[i + 1] < dielCords.y2 && AllVectorPoints[i + 1] > dielCords.y1) {
				vec3 result = calculateVerticalEdges(dielCords.x1, dielCords.z2, dielCords.x2, dielCords.z2, Eleft, Eright, AllVectorPoints, i);
				tempvec2[0] = result.x;
				tempvec2[1] = result.y;
				tempvec2[2] = result.z;
			}

		}

		AllVectorPoints[i + 3] = vertices[k] + tempvec2[0] / len(tempvec2) / LineSize;
		AllVectorPoints[i + 4] = vertices[k + 1] + tempvec2[1] / len(tempvec2) / LineSize;
		AllVectorPoints[i + 5] = vertices[k + 2] + tempvec2[2] / len(tempvec2) / LineSize;

		if (applyrot) {
			glm::vec3 first(AllVectorPoints[i], AllVectorPoints[i + 1], AllVectorPoints[i + 2]);
			glm::vec3 second(AllVectorPoints[i + 3], AllVectorPoints[i + 4], AllVectorPoints[i + 5]);
			first = glm::rotateX(first, glm::radians(90.f));
			first = glm::rotateY(first, glm::radians(90.f));
			first = glm::rotateZ(first, glm::radians(90.f));
			second = glm::rotateX(second, glm::radians(90.f));
			second = glm::rotateY(second, glm::radians(90.f));
			second = glm::rotateZ(second, glm::radians(90.f));
			AllVectorPoints[i] = first[0];
			AllVectorPoints[i + 1] = first[1];
			AllVectorPoints[i + 2] = first[2];
			AllVectorPoints[i + 3] = second[0];
			AllVectorPoints[i + 4] = second[1];
			AllVectorPoints[i + 5] = second[2];
		}
	}

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, PointsSize, AllVectorPoints, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndicesSize, indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void doMathStuff(float& rot, double& prevTime, float tempCameraDistance, float* tempColor, Shader& shader, bool equiviewmode) {
	glm::vec4 color = glm::vec4(tempColor[0], tempColor[1], tempColor[2], tempColor[3]);
	double crntTime = glfwGetTime();
	if (crntTime - prevTime >= 1 / 2)
	{
		rot += 0.01f;
		prevTime = crntTime;
	}

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);

	if (equiviewmode) {
		model = glm::rotate(model, glm::radians(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::translate(view, glm::vec3(0.f, 0.0f, tempCameraDistance));
	}
	else {
		model = glm::rotate(model, glm::radians(45.f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::translate(view, glm::vec3(0.f, 0.1f, tempCameraDistance));
	}
	
	proj = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.0f);

	int modelLoc = glGetUniformLocation(shader.ID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	int viewLoc = glGetUniformLocation(shader.ID, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	int projLoc = glGetUniformLocation(shader.ID, "proj");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
	int colorLoc = glGetUniformLocation(shader.ID, "color");
	glUniform4fv(colorLoc, 1, glm::value_ptr(color));
}

void drawOpenGl(GLuint VAO, GLuint VAO2, GLuint VAO3, GLuint VAO4, GLuint VAO5, int HowManyTriangles, int HowManyTriangles2, bool whattodisplay, bool dielMod, int lastSize) {
	glBindVertexArray(VAO);
	glDrawElements(GL_LINES, DOTS_AMOUNT * 2, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	if (whattodisplay) {
		glBindVertexArray(VAO3);
		glDrawElements(GL_TRIANGLES, HowManyTriangles2, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glBindVertexArray(VAO4);
		glDrawElements(GL_TRIANGLES, HowManyTriangles2, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	else {
		glBindVertexArray(VAO2);
		glDrawElements(GL_TRIANGLES, HowManyTriangles, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	if (dielMod) {
		glBindVertexArray(VAO5);
		glDrawElements(GL_TRIANGLES, lastSize, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

void updateFrame(Shader& shader) {
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	shader.Activate();
}

void updatePlaneCondensator(GLuint VAO2, GLuint VBO2, GLuint EBO2, int PlaneVertSize, GLfloat* capacitorVertices, int PlaneIndSize, GLuint* capacitorIndices) {
	glBindVertexArray(VAO2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, PlaneVertSize, capacitorVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void updatePlaneCoords(GLfloat* vertices, const PlaneCoords& cords) {
	vertices[0] = vertices[3] = vertices[6] = vertices[9] = cords.x1;
	vertices[12] = vertices[15] = vertices[18] = vertices[21] = cords.x2;
	vertices[1] = vertices[7] = vertices[13] = vertices[19] = cords.y1;
	vertices[4] = vertices[10] = vertices[16] = vertices[22] = cords.y2;
	vertices[2] = vertices[5] = vertices[14] = vertices[17] = cords.z1;
	vertices[8] = vertices[11] = vertices[20] = vertices[23] = cords.z2;
}

void updateCircleCondensator(GLfloat x, GLuint VAO, GLuint VBO, GLfloat radius, GLfloat* vertices) {
	GLfloat angle = 0.f;

	for (int i = 0; i < CIRCLE_SIZE - 2; i += 3) {
		vertices[i] = x;
		vertices[i + 1] = radius * std::sinf(angle * pi / 180);
		vertices[i + 2] = -radius * std::cosf(angle * pi / 180);
		if (vertices[i + 1] < 0.01f && vertices[i + 1] > 0.0f)
			vertices[i + 1] = 0.0f;
		if (vertices[i + 1] > -0.01f && vertices[i + 1] <= 0.0f)
			vertices[i + 1] = 0.0f;
		if (vertices[i + 2] < 0.01f && vertices[i + 2] > 0.0f)
			vertices[i + 2] = 0.0f;
		if (vertices[i + 2] > -0.01f && vertices[i + 2] <= 0.0f)
			vertices[i + 2] = 0.0f;
		angle += 1.f;
	}
	vertices[CIRCLE_SIZE] = x;
	vertices[CIRCLE_SIZE + 1] = 0.0f;
	vertices[CIRCLE_SIZE + 2] = 0.0f;

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (CIRCLE_SIZE + 3) * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void updateDielCoords(GLfloat* vertices, const PlaneCoords& cords, GLuint VAO5, GLuint VBO5) {
	vertices[1] = cords.y2;
	vertices[2] = cords.z2;
	vertices[4] = cords.y1;
	vertices[5] = cords.z2;
	vertices[7] = cords.y2;
	vertices[8] = cords.z1;
	vertices[10] = cords.y1;
	vertices[11] = cords.z1;

	glBindVertexArray(VAO5);

	glBindBuffer(GL_ARRAY_BUFFER, VBO5);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void rotateDiel(GLfloat* vertices, GLuint VAO5, GLuint VBO5, float rot, double prevTime, PlaneCoords& dielcords, float& allrot) {
	double crntTime = glfwGetTime();
	if (crntTime - prevTime >= 1 / 2)
	{
		rot += 1.f;
		allrot = rot;
		prevTime = crntTime;
	}

	glm::vec3 vec1(vertices[0], vertices[1], vertices[2]);
	glm::vec3 vec2(vertices[3], vertices[4], vertices[5]);
	glm::vec3 vec3(vertices[6], vertices[7], vertices[8]);
	glm::vec3 vec4(vertices[9], vertices[10], vertices[11]);
	vec1 = glm::rotateX(vec1, glm::radians(rot));
	vec2 = glm::rotateX(vec2, glm::radians(rot));
	vec3 = glm::rotateX(vec3, glm::radians(rot));
	vec4 = glm::rotateX(vec4, glm::radians(rot));
	vertices[0] = vec1[0];
	vertices[1] = vec1[1];
	vertices[2] = vec1[2];
	vertices[3] = vec2[0];
	vertices[4] = vec2[1];
	vertices[5] = vec2[2];
	vertices[6] = vec3[0];
	vertices[7] = vec3[1];
	vertices[8] = vec3[2];
	vertices[9] = vec4[0];
	vertices[10] = vec4[1];
	vertices[11] = vec4[2];

	dielcords = PlaneCoords(std::abs(vertices[0]), std::abs(vertices[1]), std::abs(vertices[2]));
	
	glBindVertexArray(VAO5);

	glBindBuffer(GL_ARRAY_BUFFER, VBO5);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void plot(float* plot1, float* plot2, int& idx, std::vector<float>& surf, float koef) {
	if (ImPlot::BeginPlot("C(t)")) {
		ImPlot::PlotLine("C(t)", plot1, plot2, PLOT_SIZE);
		ImPlot::EndPlot();
	}
}

float dist(float a1, float b1, float a2, float b2) {
	return sqrtf((a2 - a1) * (a2 - a1) + (b2 - b1) * (b2 - b1));
}

float calcSurf(PlaneCoords& plane, PlaneCoords& diel, float allrot) {
	//if (diel.y2 < plane.y2 && diel.z2 < plane.z2)
		//return diel.y2 * diel.z2;
//	if (diel.y2 > plane.y2 && diel.z2 > plane.z2 && diel.y1 < plane.y1 && diel.z1 < plane.z1)
	//	return plane.y1 * plane.z1;
	float h = dist(diel.y1, diel.z1, diel.y2, diel.z1);
	allrot /= 360.f;
	allrot /= 90.f;
	float a = dist(diel.y1 / cos(allrot), plane.z1, diel.y1, diel.z1);
	float b = dist(diel.y1 / cos(allrot) + 2 * diel.y1, plane.z1, diel.y2, diel.z1);
	return diel.y1 * diel.z1;
	return abs(diel.y1 * diel.z1) - abs((a + b) * h / 2);
}

int main() {

	std::vector<float> surf(PLOT_SIZE);
	GLFWwindow* window = SetupWindow();

	GLfloat* vertices = new GLfloat[FIELD_SIZE];
	GLfloat AllVectorPoints[FIELD_SIZE * 2] = {};

	GLfloat dielectricVert[] = {
		0.0f, 0.4f, 0.4f,
		0.0f, -0.4f, 0.4f,
		0.0f, 0.4f, -0.4f,
		0.0f, -0.4f, -0.4f,
	};
	GLuint dielectricInd[] = {
		0,1,2,
		2,3,1
	};

	GLfloat tempX = -0.96f;
	GLfloat tempY = -0.96f;
	GLfloat tempZ = -0.96f;

	PlaneCoords planeCoords(0.4f, 0.3f, 0.3f);
	GLfloat capacitorVertices[] = {
		planeCoords.x1, planeCoords.y1, planeCoords.z1,
		planeCoords.x1, planeCoords.y2, planeCoords.z1,

		planeCoords.x1, planeCoords.y1, planeCoords.z2,
		planeCoords.x1, planeCoords.y2, planeCoords.z2,

		planeCoords.x2, planeCoords.y1, planeCoords.z1,
		planeCoords.x2, planeCoords.y2, planeCoords.z1,

		planeCoords.x2, planeCoords.y1, planeCoords.z2,
		planeCoords.x2, planeCoords.y2, planeCoords.z2,
	};
	GLuint capacitorIndices[] = {
		0,1,2,
		2,3,1,
		4,5,6,
		6,7,5
	};

	GLfloat* LeftSircleCapacitorVertices = new GLfloat[CIRCLE_SIZE + 3];
	GLfloat* RightSircleCapacitorVertices = new GLfloat[CIRCLE_SIZE + 3];
	GLuint* SircleCapacitorIndices = new GLuint[CIRCLE_SIZE];

	int tempk = 0;
	for (int i = 0; i < CIRCLE_SIZE - 2; i += 3, ++tempk) {
		SircleCapacitorIndices[i] = tempk;
		SircleCapacitorIndices[i + 1] = tempk +1;
		SircleCapacitorIndices[i + 2] = 360 / 1;
	}
	SircleCapacitorIndices[CIRCLE_SIZE - 2] = 0;

	GLfloat Eleft = -1 / (8.85f * 0.48);
	GLfloat Eright = 1 / (8.85f * 0.48);
	
	for (int i = 0; i < FIELD_SIZE - 2; i += 3) {
		vertices[i] = tempX;
		vertices[i + 1] = tempY;
		vertices[i + 2] = tempZ;
		tempX += 0.25f;
		if (tempX > 1.f) {
			tempY += 0.25f;
			tempX = -0.95f;
		}
		if (tempY > 1.f) {
			tempZ += 0.25f;
			tempY = -0.95f;
		}
	}

	int k = 0;
	for (int i = 0; i < FIELD_SIZE * 2 - 5; i += 6, k += 3) {
		AllVectorPoints[i] = vertices[k];
		AllVectorPoints[i + 1] = vertices[k + 1];
		AllVectorPoints[i + 2] = vertices[k + 2];

		GLfloat templeft1[3] = { 0.0f, 0.0f, 0.0f };
		GLfloat tempright1[3] = { 0.0f, 0.0f, 0.0f };
		GLfloat tempvec2[3] = { 0.0f, 0.0f, 0.0f };

		if (AllVectorPoints[i + 1] > planeCoords.y1 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 2] > planeCoords.z1 && AllVectorPoints[i + 2] < planeCoords.z2) {
			templeft1[0] = (planeCoords.x1 - AllVectorPoints[i]) * Eleft;
			tempright1[0] = -(planeCoords.x2 - AllVectorPoints[i]) * Eright;
			tempvec2[0] = templeft1[0] + tempright1[0];
			tempvec2[1] = AllVectorPoints[i + 1];
			tempvec2[2] = AllVectorPoints[i + 2];
		}

		if (AllVectorPoints[i + 1] <= planeCoords.y1 && AllVectorPoints[i + 2] <= planeCoords.z1) {
			vec3 result = calculateCorners(planeCoords.x1, planeCoords.y1, planeCoords.z1, planeCoords.x2, planeCoords.y1, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 1] <= planeCoords.y1 && AllVectorPoints[i + 2] >= planeCoords.z2) {
			vec3 result = calculateCorners(planeCoords.x1, planeCoords.y1, planeCoords.z2, planeCoords.x2, planeCoords.y1, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 1] >= planeCoords.y2 && AllVectorPoints[i + 2] <= planeCoords.z1) {
			vec3 result = calculateCorners(planeCoords.x1, planeCoords.y2, planeCoords.z1, planeCoords.x2, planeCoords.y2, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 1] >= planeCoords.y2 && AllVectorPoints[i + 2] >= planeCoords.z2) {
			vec3 result = calculateCorners(planeCoords.x1, planeCoords.y2, planeCoords.z2, planeCoords.x2, planeCoords.y2, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}

		if (AllVectorPoints[i + 1] < planeCoords.y1 && AllVectorPoints[i + 2] < planeCoords.z2 && AllVectorPoints[i + 2] > planeCoords.z1) {
			vec3 result = calculateHorizontalEdges(planeCoords.x1, planeCoords.y1, planeCoords.x2, planeCoords.y1, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 1] > planeCoords.y2 && AllVectorPoints[i + 2] < planeCoords.z2 && AllVectorPoints[i + 2] > planeCoords.z1) {
			vec3 result = calculateHorizontalEdges(planeCoords.x1, planeCoords.y2, planeCoords.x2, planeCoords.y2, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 2] < planeCoords.z1 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 1] > planeCoords.y1) {
			vec3 result = calculateVerticalEdges(planeCoords.x1, planeCoords.z1, planeCoords.x2, planeCoords.z1, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}
		if (AllVectorPoints[i + 2] > planeCoords.z2 && AllVectorPoints[i + 1] < planeCoords.y2 && AllVectorPoints[i + 1] > planeCoords.y1) {
			vec3 result = calculateVerticalEdges(planeCoords.x1, planeCoords.z2, planeCoords.x2, planeCoords.z2, Eleft, Eright, AllVectorPoints, i);
			tempvec2[0] = result.x;
			tempvec2[1] = result.y;
			tempvec2[2] = result.z;
		}

		AllVectorPoints[i + 3] = vertices[k] + tempvec2[0] / len(tempvec2) / 1.f;
		AllVectorPoints[i + 4] = vertices[k + 1] + tempvec2[1] / len(tempvec2) / 1.f;
		AllVectorPoints[i + 5] = vertices[k + 2] + tempvec2[2] / len(tempvec2) / 1.f;
	}

	GLuint indices[DOTS_AMOUNT * 2] = {};
	for (int i = 0; i < DOTS_AMOUNT * 2; ++i) {
		indices[i] = i;
	}

	Shader shader("default.vert", "default.frag");
	shader.Activate();

	GLuint VAO, VBO, EBO;
	GLuint VAO2, VBO2, EBO2;
	GLuint VAO3, VBO3, EBO3;
	GLuint VAO4, VBO4, EBO4;
	GLuint VAO5, VBO5, EBO5;
	{
		{
			glGenVertexArrays(1, &VAO);
			glGenVertexArrays(1, &VAO2);
			glGenVertexArrays(1, &VAO3);
			glGenVertexArrays(1, &VAO4);
			glGenVertexArrays(1, &VAO5);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &VBO2);
			glGenBuffers(1, &VBO3);
			glGenBuffers(1, &VBO4);
			glGenBuffers(1, &VBO5);
			glGenBuffers(1, &EBO);
			glGenBuffers(1, &EBO2);
			glGenBuffers(1, &EBO3);
			glGenBuffers(1, &EBO4);
			glGenBuffers(1, &EBO5);
		}
	{
			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(AllVectorPoints), AllVectorPoints, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		 {
			glBindVertexArray(VAO2);

			glBindBuffer(GL_ARRAY_BUFFER, VBO2);
			glBufferData(GL_ARRAY_BUFFER, sizeof(capacitorVertices), capacitorVertices, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(capacitorIndices), capacitorIndices, GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		 {
			 glBindVertexArray(VAO3);

			 glBindBuffer(GL_ARRAY_BUFFER, VBO3);
			 glBufferData(GL_ARRAY_BUFFER, (CIRCLE_SIZE + 3) * sizeof(GLfloat), LeftSircleCapacitorVertices, GL_DYNAMIC_DRAW);

			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO3);
			 glBufferData(GL_ELEMENT_ARRAY_BUFFER, CIRCLE_SIZE * sizeof(GLuint), SircleCapacitorIndices, GL_DYNAMIC_DRAW);

			 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			 glEnableVertexAttribArray(0);
			 glBindBuffer(GL_ARRAY_BUFFER, 0);
			 glBindVertexArray(0);
			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		 }
		 {
			 glBindVertexArray(VAO4);

			 glBindBuffer(GL_ARRAY_BUFFER, VBO4);
			 glBufferData(GL_ARRAY_BUFFER, (CIRCLE_SIZE + 3) * sizeof(GLfloat), RightSircleCapacitorVertices, GL_DYNAMIC_DRAW);

			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO4);
			 glBufferData(GL_ELEMENT_ARRAY_BUFFER, CIRCLE_SIZE * sizeof(GLuint), SircleCapacitorIndices, GL_DYNAMIC_DRAW);

			 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			 glEnableVertexAttribArray(0);
			 glBindBuffer(GL_ARRAY_BUFFER, 0);
			 glBindVertexArray(0);
			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		 }
		 {
			 glBindVertexArray(VAO5);

			 glBindBuffer(GL_ARRAY_BUFFER, VBO5);
			 glBufferData(GL_ARRAY_BUFFER, sizeof(dielectricVert), dielectricVert, GL_DYNAMIC_DRAW);

			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO5);
			 glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(dielectricInd), dielectricInd, GL_DYNAMIC_DRAW);

			 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			 glEnableVertexAttribArray(0);
			 glBindBuffer(GL_ARRAY_BUFFER, 0);
			 glBindVertexArray(0);
			 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		 }
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
	}

	float rot = 0.0f;
	double prevTime = glfwGetTime();
	double prevTime2 = glfwGetTime();

	{
		glEnable(GL_DEPTH_TEST);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	float tempColor[4] = {255.f, 255.f, 255.f, 1.f}	;
	float LineSize = 10.f;
	float tempCameraDistance = -1.f;
	float tempx = 0.4f;
	float tempy = 0.3f;
	float tempz = 0.3f;
	float q1 = -1.f;
	float q2 = 1.f;
	bool resetFlag = false;
	float radius = 0.5f;
	bool applyrot = false;
	bool equiviewmode = false;
	float dielkoef = 20.f;

	bool dielmod = false;
	bool whatToDisplay = false;
	bool switchDisplay = false;

	float tempdiely = 0.2f;
	float tempdielz = 0.6f;

	PlaneCoords dielcords = PlaneCoords(0.0f, 0.4f, 0.4f);

	float* plot1 = new float[PLOT_SIZE];
	float* plot2 = new float[PLOT_SIZE];

	for (int i = 0; i < PLOT_SIZE; ++i) {
		plot1[i] = 0.f;
		plot2[i] = 0.f;
	}

	int idx = 0;
	float allrot = 0.0f;

	float someconstant = 0.5f;

	while (!glfwWindowShouldClose(window)) {	
		
		updateMainCalculation(Eleft, Eright, LineSize, vertices, AllVectorPoints, VAO, VBO, EBO, indices, sizeof(AllVectorPoints), sizeof(indices), planeCoords, whatToDisplay, radius, applyrot, dielmod, dielcords, dielkoef);		
		
		if (dielmod)
			rotateDiel(dielectricVert, VAO5, VBO5, rot, prevTime, dielcords, allrot);

		updatePlaneCondensator(VAO2, VBO2, EBO2, sizeof(capacitorVertices), capacitorVertices, sizeof(capacitorIndices), capacitorIndices);
		updateCircleCondensator(tempx, VAO3, VBO3, radius, LeftSircleCapacitorVertices);
		updateCircleCondensator(-tempx, VAO4, VBO4, radius, RightSircleCapacitorVertices);

		updateFrame(shader);
		
		doMathStuff(rot, prevTime, tempCameraDistance, tempColor, shader, equiviewmode);
		
		drawOpenGl(VAO, VAO2, VAO3, VAO4, VAO5, sizeof(indices) / sizeof(GLuint), 3 *360, whatToDisplay, dielmod, sizeof(dielectricInd) / sizeof(GLuint));

		ImGui::Begin("input");
		ImGui::SliderFloat("field line size", &LineSize, 0.5f, 30.f);
		ImGui::SliderFloat("camera distance", &tempCameraDistance, -5.f, -0.5f);
		ImGui::SliderFloat("change distance between two plates", &tempx, 0.f, 1.f);
		ImGui::SliderFloat("change height of two plates", &tempy, 0.f, 1.f);
		ImGui::SliderFloat("change width of two plates", &tempz, 0.f, 1.f);
		ImGui::SliderFloat("change radius of two plates", &radius, 0.f, 1.f);
		ImGui::SliderFloat("change height of dielectric", &tempdiely, 0.f, 1.f);
		ImGui::SliderFloat("change width of dielectric", &tempdielz, 0.f, 1.f);
		ImGui::SliderFloat("change dieletric constant", &dielkoef, 20.f, 1500.f);
		ImGui::SliderFloat("change q1", &q1, -5.f, 5.f);
		ImGui::SliderFloat("change q2", &q2, -5.f, 5.f);
		ImGui::Checkbox("reset", &resetFlag);
		ImGui::Checkbox("change from planes to circles", &switchDisplay);
		ImGui::Checkbox("show equipotential", &applyrot);
		ImGui::Checkbox("get top view", &equiviewmode);
		ImGui::Checkbox("dielectric mode", &dielmod);
		ImGui::ColorEdit4("Color", tempColor);

		double curtime = glfwGetTime();
		if (curtime - prevTime2 >= someconstant) {
			if (idx == PLOT_SIZE)
				idx = 0;
			surf[idx] = calcSurf(planeCoords, dielcords, allrot) / (planeCoords.y2 * planeCoords.z2 * planeCoords.x2 * 2) + 1;
			std::cout << calcSurf(planeCoords, dielcords, allrot)  << std::endl;
			++idx;
			for (int i = 0; i < PLOT_SIZE; ++i) {
				plot1[i] = i;
				plot2[i] = dielkoef * 8.85 * surf[i];
			}
			prevTime2 = curtime;
		}
		if (dielmod)
			plot(plot1, plot2, idx, surf, dielkoef);
		
		ImGui::End();

		updatePlaneCoords(capacitorVertices, PlaneCoords(tempx, tempy, tempz));
		planeCoords = PlaneCoords(tempx, tempy, tempz);
		if (dielcords != PlaneCoords(0.0f, tempdiely, tempdielz)) {
			dielcords = PlaneCoords(0.0f, tempdiely, tempdielz);
			updateDielCoords(dielectricVert, dielcords, VAO5, VBO5);
		}

		if (switchDisplay) {
			switchDisplay = false;
			whatToDisplay = !whatToDisplay;
		}

		if (resetFlag) {
			updatePlaneCoords(capacitorVertices, PlaneCoords(0.4f, 0.3f, 0.3f));
			LineSize = 10.f;
			tempx = 0.4f;
			tempy = 0.3f;
			tempz = 0.3f;
			tempCameraDistance = -1.f;
			tempColor[0] = tempColor[1] = tempColor[2] = 255.f;
			tempColor[3] = 1.f;
			q1 = -1.f;
			q2 = 1.f;
			resetFlag = false;
			radius = 0.5f;
			applyrot = false;
			equiviewmode = false;
			dielmod = false;
			dielkoef = 20.f;
		}

		Eleft = q1 / (8.85f * 4 * tempy * tempz * 1.7);
		Eright = q2 / (8.85f * 4 * tempy * tempz * 1.7);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete[] plot1;
	delete[] plot2;
	delete[] vertices;
	delete[] LeftSircleCapacitorVertices;
	delete[] RightSircleCapacitorVertices;
	delete[] SircleCapacitorIndices;
	FreeMemory(window, VAO, VBO, EBO, shader);
	return 0;
}