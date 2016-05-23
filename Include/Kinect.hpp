#pragma once

#include "Globals.hpp"
#include "Graphics.hpp"
#include "Time.hpp"

class Kinect {

public:
	Kinect();
	~Kinect();

	bool init();
	bool initKinect();
	bool initFaceTrack();
	bool initVBO();
	void getKinectVideo();
	void getKinectDepth();
	void update();
	void render();

	void playRecord();
	void record();
	void stopRecord();

	IFTImage*   GetVideoBuffer() { return(m_VideoBuffer); };
	IFTImage*   GetDepthBuffer() { return(m_DepthBuffer); };
	HRESULT GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig);
	HRESULT GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig);
	void SetCenterOfImage(IFTResult* pResult);

	FLOAT* GetScale() { return scale; };
	FLOAT* GetRotation() { return rotation; };
	FLOAT* GetTranslation() { return translation; };

private:

	// OpenGL Variables
	GLuint textureId;              // ID of the texture to contain Kinect RGB Data
	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint program_handle;
	GLuint texture_handle;

	// Kinect variables
	HANDLE rgbStream;				// The handler of the Kinect's RGB Camera
	HANDLE depthStream;				// The handler of the Kinect's depth Camera
	INuiSensor* sensor;				// The Kinect sensor
	IFTFaceTracker* pFT;			// The Face Tracking COM interface
	IFTResult* pFTResult;			// The Face Tracking result object
	IFTImage* pColorFrame;			// Image interface that holds RGB data
	IFTImage* pDepthFrame;			// Image interface that holds depth data
	FT_SENSOR_DATA sensorData;		// Sensor data structure
	std::ofstream file;
	bool kinect;
	bool isTracked;
	bool isRecording;

	// Face Tracking data structures
	IFTImage*   m_VideoBuffer;
	IFTImage*	m_DepthBuffer;
	float       m_XCenterFace;
	float       m_YCenterFace;

	// Face Tracking results
	FLOAT* scale;
	FLOAT* rotation;
	FLOAT* translation;
};