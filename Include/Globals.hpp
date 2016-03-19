// C++ libraries
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>

// openGL
#include <GL/glew.h>

// SFML library
#include <SFML/Graphics.hpp> 
#include <SFML/OpenGL.hpp>

// Math library
#define GLM_FORCE_RADIANS 1
#define PI				  3.141592653589793238462643383279502884197169399375105820974944592307816406286f
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

// Kinect lirary
#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>
#include <FaceTrackLib.h>

// Global variables
#define DEBUG_MODE		0	// 0 off | 1 on
#define APP_NAME		"Kinect driven face simulation"
#define WNDW_WIDTH		1280
#define WNDW_HEIGHT		480
#define KINECT_WIDTH	640
#define KINECT_HEIGHT	480
#define MODEL_SRC		"Assets/BorisT.obj"
#define TEXTURE_SRC		"Assets/model.jpg"