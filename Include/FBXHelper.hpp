#pragma once

#include "Globals.hpp"

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);