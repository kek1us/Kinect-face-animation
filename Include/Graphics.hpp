#pragma once

#include "Globals.hpp"

GLuint loadShaders(const char * vertex_file_path,const char * fragment_file_path);

GLuint loadTex(std::string filename);

HRESULT VisualizeFaceModel(IFTImage* pColorImg, IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef,
	FLOAT zoomFactor, POINT viewOffset, IFTResult* pAAMRlt, UINT32 color);

