#pragma once

#include "Globals.hpp"
#include "Graphics.hpp"

class Kinect {

public:
	Kinect();
	~Kinect();

	bool init();
	bool initKinect();
	bool initVBO();
	void getKinectData(GLubyte* dest);
	void render();

private:

	// OpenGL Variables
	GLuint textureId;              // ID of the texture to contain Kinect RGB Data
	GLubyte data[WNDW_WIDTH*WNDW_HEIGHT * 4];  // BGRA array containing the texture data
	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint program_handle;
	GLuint texture_handle;

	// Kinect variables
	HANDLE rgbStream;              // The identifier of the Kinect's RGB Camera
	INuiSensor* sensor;            // The kinect sensor
};