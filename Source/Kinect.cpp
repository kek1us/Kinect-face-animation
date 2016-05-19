#include "Kinect.hpp"

Kinect::Kinect() {}

Kinect::~Kinect() {
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(program_handle);
	glDeleteTextures(1, &textureId);
	if (kinect) {
		pFTResult->Release();
		pColorFrame->Release();
		pDepthFrame->Release();
		pFT->Release();
	}
}

bool Kinect::init() {
	//if (!initKinect()) exit(EXIT_FAILURE);
	kinect = true;
	isRecording = false;
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WNDW_WIDTH, WNDW_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)NULL);
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

	sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH,            // Depth camera or rgb camera?
		NUI_IMAGE_RESOLUTION_320x240,    // Image resolution
		0,      // Image stream flags, e.g. near mode
		2,      // Number of frames to buffer
		NULL,   // Event handle
		&depthStream);

	// KEK
	m_VideoBuffer = FTCreateImage();
	if (!m_VideoBuffer)
	{
		return false;
	}

	HRESULT hr = m_VideoBuffer->Allocate(640, 480, FTIMAGEFORMAT_UINT8_B8G8R8X8);
	if (FAILED(hr))
	{
		return false;
	}

	m_DepthBuffer = FTCreateImage();
	if (!m_DepthBuffer)
	{
		return false;
	}

	hr = m_DepthBuffer->Allocate(320, 240, FTIMAGEFORMAT_UINT16_D13P3);
	if (FAILED(hr))
	{
		return false;
	}

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
	pDepthFrame = FTCreateImage();
	if (!pColorFrame || FAILED(hr = pColorFrame->Allocate(640, 480, FTIMAGEFORMAT_UINT8_B8G8R8X8))) return false;
	if (!pDepthFrame || FAILED(hr = pDepthFrame->Allocate(320, 240, FTIMAGEFORMAT_UINT16_D13P3))) return false;


	sensorData.pVideoFrame = pColorFrame;
	sensorData.pDepthFrame = pDepthFrame;
	sensorData.ZoomFactor = 1.0f;       // Not used must be 1.0
	sensorData.ViewOffset = POINT{0, 0}; // Not used must be (0,0)

	isTracked = false;
	SetCenterOfImage(NULL);

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

void Kinect::getKinectVideo() {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	texture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{	// Copy image frame		
		memcpy(m_VideoBuffer->GetBuffer(), PBYTE(LockedRect.pBits), std::min(m_VideoBuffer->GetBufferSize(), UINT(texture->BufferLen())));
	}
	else
	{
		std::cout << "Buffer length of received image texture is bogus" << std::endl;
	}

	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);
}

void Kinect::getKinectDepth() {
	NUI_IMAGE_FRAME pImageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &pImageFrame) < 0) return;
	INuiFrameTexture* pTexture = pImageFrame.pFrameTexture;
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{   // Copy depth frame
		memcpy(m_DepthBuffer->GetBuffer(), PBYTE(LockedRect.pBits), std::min(m_DepthBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
	}
	else
	{
		std::cout << "Buffer length of received depth texture is bogus" << std::endl;
	}

	pTexture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(depthStream, &pImageFrame);
}

void Kinect::update() {
	getKinectVideo();
	getKinectDepth();

	HRESULT hrFT = E_FAIL;

	if (kinect && GetVideoBuffer()) {
		HRESULT hrCopy = m_VideoBuffer->CopyTo(pColorFrame, NULL, 0, 0);

		if (SUCCEEDED(hrCopy) && GetDepthBuffer()) {
			hrCopy = m_DepthBuffer->CopyTo(pDepthFrame, NULL, 0, 0);
		}
		
		if (SUCCEEDED(hrCopy)) {
			if (!isTracked) {
				hrFT = pFT->StartTracking(&sensorData, NULL, NULL, pFTResult);
			}
			else {
				hrFT = pFT->ContinueTracking(&sensorData, NULL, pFTResult);
			}
		}
	}

	isTracked = SUCCEEDED(hrFT) && SUCCEEDED(pFTResult->GetStatus());
	SetCenterOfImage(pFTResult);

	// Do something with pFTResult.
	if (isTracked) {
		std::cout << "DETECTS FACE" << std::endl;
		FLOAT* pSU = NULL;
		UINT numSU;
		BOOL suConverged;
		pFT->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
		POINT viewOffset = { 0, 0 };
		FT_CAMERA_CONFIG cameraConfig;
		if (kinect)
		{
			GetVideoConfiguration(&cameraConfig);
		}
		else
		{
			cameraConfig.Width = 640;
			cameraConfig.Height = 480;
			cameraConfig.FocalLength = 500.0f;
		}
		IFTModel* ftModel;
		HRESULT hr = pFT->GetFaceModel(&ftModel);
		if (SUCCEEDED(hr))
		{
			hr = VisualizeFaceModel(pColorFrame, ftModel, &cameraConfig, pSU, 1.0, viewOffset, pFTResult, 0x00FFFF00);
			
			if (isRecording) {
				FLOAT pHeadScale = 15;
				FLOAT** pSUCoefs = NULL;
				UINT pSUCount;
				BOOL pHaveConverged;
				HRESULT hr_su = pFT->GetShapeUnits(&pHeadScale, pSUCoefs, &pSUCount, &pHaveConverged);
				if (!SUCCEEDED(hr_su)) {
					std::cout << "Could not read the Shape Units of the Face Model" << std::endl;
				}
				else {
					// Print the data of the Face Model to the file

				}
			}

			ftModel->Release();
		}
		else {
			std::cout << "Could not get the Face Model" << std::endl;
		}

		if (!SUCCEEDED(hr)) {
			std::cout << "Could not draw the Face Model" << std::endl;
		}
	}

	// Terminate on some criteria.
}

void Kinect::render() {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);
	if (kinect) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, KINECT_WIDTH, KINECT_HEIGHT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)pColorFrame->GetBuffer());
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

void Kinect::playRecord() {
	// Play the facial animation's data recorded in the file
}

void Kinect::record() {
	if (kinect) {
		// Record the facial animation's data to a file
		isRecording = true;
		file.open(FILE_NAME, std::ofstream::out | std::ofstream::trunc);
		file << "// Starting recording at " << timefmt("%c");
	}
}

void Kinect::stopRecord() {
	if (kinect) {
		// Stop recording the facial animation's data
		isRecording = false;
		file.close();
	}
}

HRESULT Kinect::GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig)
{
	if (!videoConfig)
	{
		return E_POINTER;
	}

	UINT width = m_VideoBuffer ? m_VideoBuffer->GetWidth() : 0;
	UINT height = m_VideoBuffer ? m_VideoBuffer->GetHeight() : 0;
	FLOAT focalLength = 0.f;

	if (width == 640 && height == 480)
	{
		focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
	}
	else if (width == 1280 && height == 960)
	{
		focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
	}

	if (focalLength == 0.f)
	{
		return E_UNEXPECTED;
	}


	videoConfig->FocalLength = focalLength;
	videoConfig->Width = width;
	videoConfig->Height = height;
	return(S_OK);
}

HRESULT Kinect::GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig)
{
	if (!depthConfig)
	{
		return E_POINTER;
	}

	UINT width = m_DepthBuffer ? m_DepthBuffer->GetWidth() : 0;
	UINT height = m_DepthBuffer ? m_DepthBuffer->GetHeight() : 0;
	FLOAT focalLength = 0.f;

	if (width == 80 && height == 60)
	{
		focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS / 4.f;
	}
	else if (width == 320 && height == 240)
	{
		focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
	}
	else if (width == 640 && height == 480)
	{
		focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
	}

	if (focalLength == 0.f)
	{
		return E_UNEXPECTED;
	}

	depthConfig->FocalLength = focalLength;
	depthConfig->Width = width;
	depthConfig->Height = height;

	return S_OK;
}

void Kinect::SetCenterOfImage(IFTResult* pResult)
{
	float centerX = ((float)pColorFrame->GetWidth()) / 2.0f;
	float centerY = ((float)pColorFrame->GetHeight()) / 2.0f;
	if (pResult)
	{
		if (SUCCEEDED(pResult->GetStatus()))
		{
			RECT faceRect;
			pResult->GetFaceRect(&faceRect);
			centerX = (faceRect.left + faceRect.right) / 2.0f;
			centerY = (faceRect.top + faceRect.bottom) / 2.0f;
		}
		m_XCenterFace += 0.02f*(centerX - m_XCenterFace);
		m_YCenterFace += 0.02f*(centerY - m_YCenterFace);
	}
	else
	{
		m_XCenterFace = centerX;
		m_YCenterFace = centerY;
	}
}

//HRESULT Kinect::GetClosestHint(FT_VECTOR3D* pHint3D)
//{
//	int selectedSkeleton = -1;
//	float smallestDistance = 0;
//
//	if (!pHint3D)
//	{
//		return(E_POINTER);
//	}
//
//	if (pHint3D[1].x == 0 && pHint3D[1].y == 0 && pHint3D[1].z == 0)
//	{
//		// Get the skeleton closest to the camera
//		for (int i = 0; i < NUI_SKELETON_COUNT; i++)
//		{
//			if (m_SkeletonTracked[i] && (smallestDistance == 0 || m_HeadPoint[i].z < smallestDistance))
//			{
//				smallestDistance = m_HeadPoint[i].z;
//				selectedSkeleton = i;
//			}
//		}
//	}
//	else
//	{   // Get the skeleton closest to the previous position
//		for (int i = 0; i < NUI_SKELETON_COUNT; i++)
//		{
//			if (m_SkeletonTracked[i])
//			{
//				float d = abs(m_HeadPoint[i].x - pHint3D[1].x) +
//					abs(m_HeadPoint[i].y - pHint3D[1].y) +
//					abs(m_HeadPoint[i].z - pHint3D[1].z);
//				if (smallestDistance == 0 || d < smallestDistance)
//				{
//					smallestDistance = d;
//					selectedSkeleton = i;
//				}
//			}
//		}
//	}
//	if (selectedSkeleton == -1)
//	{
//		return E_FAIL;
//	}
//
//	pHint3D[0] = m_NeckPoint[selectedSkeleton];
//	pHint3D[1] = m_HeadPoint[selectedSkeleton];
//
//	return S_OK;
//}
