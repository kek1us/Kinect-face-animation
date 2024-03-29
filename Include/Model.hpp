#pragma once

#include "Globals.hpp"
#include "Graphics.hpp"
#include "FBXHelper.hpp"

class Model {

public:
	Model();
	~Model();

	bool init();
	bool initVBO();
	bool loadFBX(std::string filename);
	void getFBXData(FbxNode* node);
	void update();
	void render();

	std::vector<glm::vec3>* getVerticesArray() { return &vertices; };
	std::vector<glm::vec3>* getNormalsArray() { return &normals; };
	std::vector<glm::vec2>* getUVsArray() { return &uvs; };

	// PLACEHOLDER
	void setShocked() { doShocked = true; };
	void undoShocked() { doShocked = false; };
	void setDefaultPose();
	void modifyHead(FbxVector4 T, FbxVector4 R, FbxVector4 S);
	void modifyJaw(double value);
	void registerResult(FLOAT* scale, FLOAT* rotation, FLOAT* translation, FLOAT* AU, UINT* numAU, FLOAT* SU, UINT* numSU);
	void stopAnimation() { stopAnim = true; };

	void increaseRotation(FLOAT xAxis, FLOAT yAxis, FLOAT zAxis);

private:

	// OpenGL Variables
	GLuint textureId;
	GLuint vertexbuffer_triangles;
	GLuint uvbuffer_triangles;
	GLuint normalbuffer_triangles;
	GLuint materialbuffer_triangles;
	GLuint program_handle;
	GLuint texture_handle;
	GLuint mvp_handle;
	GLuint ViewMatrix_handle;
	GLuint ModelMatrix_handle;
	GLuint Light_handle;

	// FBX data
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	FbxAnimLayer * mCurrentAnimLayer;
	FbxPose * lPose;
	mutable FbxTime mFrameTime, mStart, mStop, mCurrentTime;
	mutable FbxTime mCache_Start, mCache_Stop;

	// Model info
	std::vector < glm::vec3 > vertices_quads;
	std::vector < glm::vec2 > uvs_quads;
	std::vector < glm::vec3 > normals_quads;
	std::vector < glm::vec3 > vertices_triangles;
	std::vector < glm::vec2 > uvs_triangles;
	std::vector < glm::vec3 > normals_triangles;
	std::vector < glm::vec3 > material_triangles;

	std::vector < glm::vec3 > vertices;
	std::vector < glm::vec2 > uvs;
	std::vector < glm::vec3 > normals;
	std::vector < glm::vec3 > materials;

	// PLACEHOLDER
	double shocked;
	double happy;
	double jawLow;
	double upperLip;
	double lipStr;
	double outBrow;
	bool doShocked;
	bool newResult;
	bool stopAnim;
	FbxMatrix matrixHeadO;
	FbxMatrix matrixHead;
	FbxMatrix matrixJawEnd;

	// RESULTSHOLDER
	FLOAT lScale;
	FLOAT lRotation[3];
	FLOAT* lTranslation;
	FLOAT* lAU;
	FLOAT* lSU;
	UINT lNumAU;
	UINT lNumSU;
};