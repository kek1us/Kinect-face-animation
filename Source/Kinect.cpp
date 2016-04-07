#include "Kinect.hpp"

Kinect::Kinect() {}

Kinect::~Kinect() {
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(program_handle);
	glDeleteTextures(1, &textureId);
	pFTResult->Release();
	pColorFrame->Release();
	pFT->Release();
}

bool Kinect::init() {
	//if (!initKinect()) exit(EXIT_FAILURE);
	kinect = true;
	if (!initKinect()) kinect = false;
	if (!initFaceTrack()) kinect = false;
	if (!initVBO()) exit(EXIT_FAILURE);

	// Initialize textures
	if (!kinect) textureId = loadTex(KINECT_FAIL_SRC);
	else {
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WNDW_WIDTH, WNDW_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)data);
	}

	return kinect;
}

bool Kinect::initKinect() {
	// Get a working kinect sensor
	int numSensors;
	if (NuiGetSensorCount(&numSensors) < 0 || numSensors < 1) return false;
	if (NuiCreateSensorByIndex(0, &sensor) < 0) return false;

	// Initialize sensor
	sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
	sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,            // Depth camera or rgb camera?
		NUI_IMAGE_RESOLUTION_640x480,    // Image resolution
		0,      // Image stream flags, e.g. near mode
		2,      // Number of frames to buffer
		NULL,   // Event handle
		&rgbStream);
	return sensor;
}

bool Kinect::initFaceTrack() {
	// Create an instance of a face tracker
	pFT = FTCreateFaceTracker();
	if (!pFT) return false;
	// Video camera config with width, height, focal length in pixels
	// NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS focal length is computed for 640x480 resolution
	// If you use different resolutions, multiply this focal length by the scaling factor
	FT_CAMERA_CONFIG videoCameraConfig = { 640, 480, NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS };

	// Depth camera config with width, height, focal length in pixels
	// NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS focal length is computed for 320x240 resolution
	// If you use different resolutions, multiply this focal length by the scaling factor
	FT_CAMERA_CONFIG depthCameraConfig = { 320, 240, NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS };

	// Initialize the face tracker
	HRESULT hr = pFT->Initialize(&videoCameraConfig, &depthCameraConfig, NULL, NULL);
	if (FAILED(hr)) return false;

	// Create a face tracking result interface
	pFTResult = NULL;
	hr = pFT->CreateFTResult(&pFTResult);
	if (FAILED(hr)) return false;

	// Prepare image interfaces that hold RGB and depth data
	pColorFrame = FTCreateImage();
	//pDepthFrame = FTCreateImage();
	//if (!pColorFrame || !pDepthFrame) return false;
	if (!pColorFrame) return false;

	// Attach created interfaces to the RGB and depth buffers that are filled with
	// corresponding RGB and depth frame data from Kinect cameras
	pColorFrame->Attach(640, 480, data, FTIMAGEFORMAT_UINT8_B8G8R8A8, 640 * 3);
	//pDepthFrame->Attach(320, 240, depthCameraFrameBuffer, FTIMAGEFORMAT_UINT16_D13P3, 320 * 2);
	// You can also use Allocate() method in which case IFTImage interfaces own their memory.
	// In this case use CopyTo() method to copy buffers

	//sensorData.pVideoFrame = pColorFrame;
	sensorData.ZoomFactor = 1.0f;       // Not used must be 1.0
	sensorData.ViewOffset = POINT{0, 0}; // Not used must be (0,0)

	isTracked = false;

	return true;
}

bool Kinect::initVBO() {
	GLfloat vertices[] = {
		0.0,1.0,0,
		0.0,-1.0,0,
		2.0,-1.0,0,
		2.0,1.0,0
	};

	if (!kinect) {
		vertices[6] = 1.0;
		vertices[9] = 1.0;
	}

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLfloat uv[] = {
		0,0,
		0,1,
		1,1,
		1,0
	};

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

	program_handle = loadShaders("Dependencies/Shaders/Kinect.vert", "Dependencies/Shaders/Kinect.frag");
	texture_handle = glGetUniformLocation(program_handle, "myTextureSampler");

	return true;
}

void Kinect::getKinectData(GLubyte* dest) {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	texture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{
		const BYTE* curr = (const BYTE*)LockedRect.pBits;
		const BYTE* dataEnd = curr + (KINECT_WIDTH*KINECT_HEIGHT) * 4;

		while (curr < dataEnd) {
			*dest++ = *curr++;
		}
	}
	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);
}

void Kinect::update() {
	getKinectData(data);

	// Check if we are already tracking a face
	if (!isTracked)
	{
		// Initiate face tracking. This call is more expensive and
		// searches the input image for a face.
		HRESULT hr = pFT->StartTracking(&sensorData, NULL, NULL, pFTResult);
		if (SUCCEEDED(hr))
		{
			std::cout << "DETECTS FACE" << std::endl;
			isTracked = true;
		}

		else
		{
			// Handle errors
			isTracked = false;
		}
	}
	else
	{
		// Continue tracking. It uses a previously known face position,
		// so it is an inexpensive call.
		HRESULT hr = pFT->ContinueTracking(&sensorData, NULL, pFTResult);
		if (FAILED(hr))
		{
			// Handle errors
			isTracked = false;
		}
	}

	// Do something with pFTResult.

	// Terminate on some criteria.
}

void Kinect::render() {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);
	if (kinect) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, KINECT_WIDTH, KINECT_HEIGHT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)data);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1.0f, 1.0f, 1.0f);

	glUseProgram(program_handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glUniform1i(texture_handle, 0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	glVertexAttribPointer(
		0,           // attribute. No particular reason for 0, but must match the layout in the shader.
		3,           // size
		GL_FLOAT,    // type
		GL_FALSE,    // normalized?
		0,           // stride
		(void*)0     // array buffer offset
		);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(
		1,           // attribute. No particular reason for 1, but must match the layout in the shader.
		2,           // size
		GL_FLOAT,    // type
		GL_FALSE,    // normalized?
		0,           // stride
		(void*)0     // array buffer offset
		);

	glDrawArrays(GL_QUADS, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}