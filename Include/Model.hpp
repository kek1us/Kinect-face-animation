#pragma once

#include "Globals.hpp"
#include "Graphics.hpp"

class Model {

public:
	Model();
	~Model();

	bool init();
	bool initVBO();
	bool loadModel(std::string filename);
	void update();
	void render();

private:

	// OpenGL Variables
	GLuint textureId;
	GLuint vertexbuffer_quads;
	GLuint uvbuffer_quads;
	GLuint normalbuffer_quads;
	GLuint vertexbuffer_triangles;
	GLuint uvbuffer_triangles;
	GLuint normalbuffer_triangles;
	GLuint program_handle;
	GLuint texture_handle;
	GLuint mvp_handle;
	GLuint ViewMatrix_handle;
	GLuint ModelMatrix_handle;
	GLuint Light_handle;

	// Model info
	std::vector < glm::vec3 > vertices_quads;
	std::vector < glm::vec2 > uvs_quads;
	std::vector < glm::vec3 > normals_quads;
	std::vector < glm::vec3 > vertices_triangles;
	std::vector < glm::vec2 > uvs_triangles;
	std::vector < glm::vec3 > normals_triangles;
};