#include "Model.hpp"

Model::Model() {}

Model::~Model() {
	glDeleteBuffers(1, &vertexbuffer_triangles);
	glDeleteBuffers(1, &uvbuffer_triangles);
	glDeleteBuffers(1, &normalbuffer_triangles);
	glDeleteProgram(program_handle);
	glDeleteTextures(1, &textureId);

	// Destroy all objects created by the FBX SDK.
	DestroySdkObjects(lSdkManager, true);
}

bool Model::init() {
	//if (!loadModel(MODEL_SRC))  exit(EXIT_FAILURE);
	if (!loadFBX(MODEL_SRC)) exit(EXIT_FAILURE);
	if (!initVBO()) exit(EXIT_FAILURE);
	textureId = loadTex(TEXTURE_SRC);
	return true;
}

bool Model::initVBO() {
	glGenBuffers(1, &vertexbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &materialbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, materialbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, materials.size() * sizeof(glm::vec3), &materials[0], GL_STATIC_DRAW);

	program_handle = loadShaders("Dependencies/Shaders/Model.vert", "Dependencies/Shaders/Model.frag");
	texture_handle = glGetUniformLocation(program_handle, "myTextureSampler");
	mvp_handle = glGetUniformLocation(program_handle, "MVP");
	ViewMatrix_handle = glGetUniformLocation(program_handle, "V");
	ModelMatrix_handle = glGetUniformLocation(program_handle, "M");
	Light_handle = glGetUniformLocation(program_handle, "LightPosition_worldspace");

	return true;
}

bool Model::loadFBX(std::string filename) {
	bool lResult = true;

	// Prepare the FBX SDK.
	InitializeSdkObjects(lSdkManager, lScene);

	// Load the scene
	FbxString lFilePath(filename.c_str());
	lResult = LoadScene(lSdkManager, lScene, lFilePath.Buffer());

	// Load the model
	FbxNode* lNode = lScene->GetRootNode();
	this->getFBXData(lNode);

	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(lSdkManager);
	lGeomConverter.Triangulate(lScene, /*replace*/true);

	FbxArray<FbxString*> mAnimStackNameArray;
	lScene->FillAnimStackNameArray(mAnimStackNameArray);

	// Prepare the Point Cache data
	mCache_Start = FBXSDK_TIME_INFINITE;
	mCache_Stop = FBXSDK_TIME_MINUS_INFINITE;
	PreparePointCacheData(lScene, mCache_Start, mCache_Stop);

	// Initialize the frame period.
	mFrameTime.SetTime(0, 0, 0, 1, 0, lScene->GetGlobalSettings().GetTimeMode());

	int pIndex = 0;

	FbxAnimStack * lCurrentAnimationStack = lScene->FindMember<FbxAnimStack>(mAnimStackNameArray[pIndex]->Buffer());
	mCurrentAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();
	lScene->SetCurrentAnimationStack(lCurrentAnimationStack);

	FbxTakeInfo* lCurrentTakeInfo = lScene->GetTakeInfo(*(mAnimStackNameArray[pIndex]));
	if (lCurrentTakeInfo)
	{
		mStart = lCurrentTakeInfo->mLocalTimeSpan.GetStart();
		mStop = lCurrentTakeInfo->mLocalTimeSpan.GetStop();
	}
	else
	{
		// Take the time line value
		FbxTimeSpan lTimeLineTimeSpan;
		lScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(lTimeLineTimeSpan);

		mStart = lTimeLineTimeSpan.GetStart();
		mStop = lTimeLineTimeSpan.GetStop();
	}

	// check for smallest start with cache start
	if (mCache_Start < mStart)
		mStart = mCache_Start;

	// check for biggest stop with cache stop
	if (mCache_Stop  > mStop)
		mStop = mCache_Stop;

	mCurrentTime = mStart;

	shocked = happy = jawLow = upperLip = lipStr = outBrow = 0;
	newResult = false;

	// Get the pose to work with
	lPose = lScene->GetPose(0);

	setDefaultPose();

	// Display hierarchy
	if (DEBUG_MODE) {
		FBXSDK_printf("\n\n---------\nHierarchy\n---------\n\n");
		DisplayHierarchy(lNode, 0);
	}

	return lResult;
}

void Model::getFBXData(FbxNode* node) {
	int numChilds = node->GetChildCount();

	for (int i = 0; i < numChilds; i++) {
		FbxNode* childNode = node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		if (mesh != NULL && childNode->GetName() == (std::string)"head") {
			// Get control points (=vertices for a mesh)
			FbxVector4* fbxControlPoints = mesh->GetControlPoints();

			// For each polygon in the input mesh
			for (int iPolygon = 0; iPolygon < mesh->GetPolygonCount(); iPolygon++) {
				// For each vertex in the polygon
				for (unsigned iPolygonVertex = 0; iPolygonVertex < 3; iPolygonVertex++) {
					int fbxCornerIndex = mesh->GetPolygonVertex(iPolygon, iPolygonVertex);

					// Get vertex position
					FbxVector4 fbxVertex = fbxControlPoints[fbxCornerIndex];
					vertices.push_back(glm::vec3((float)fbxVertex[0], (float)fbxVertex[1], (float)fbxVertex[2]));

					// Get normal
					FbxVector4 fbxNormal;
					mesh->GetPolygonVertexNormal(iPolygon, iPolygonVertex, fbxNormal);
					fbxNormal.Normalize();
					normals.push_back(glm::vec3((float)fbxNormal[0], (float)fbxNormal[1], (float)fbxNormal[2]));

					// Get texture coordinate
					FbxVector2 fbxUV = FbxVector2(0.0, 0.0);
					FbxLayerElementUV* fbxLayerUV = mesh->GetLayer(0)->GetUVs();
					if (fbxLayerUV) {
						int iUVIndex = 0;
						switch (fbxLayerUV->GetMappingMode()) {
						case FbxLayerElement::eByControlPoint:
							iUVIndex = fbxCornerIndex;
							break;

						case FbxLayerElement::eByPolygonVertex:
							iUVIndex = mesh->GetTextureUVIndex(iPolygon, iPolygonVertex, FbxLayerElement::eTextureDiffuse);
							break;
						}
						fbxUV = fbxLayerUV->GetDirectArray().GetAt(iUVIndex);
					}
					uvs.push_back(glm::vec2((float)fbxUV[0], (float)fbxUV[1]));
				}
			}
		}
		this->getFBXData(childNode);
	}
}

void Model::setDefaultPose() {
	// Index of the head matrices are 0 and 3, because of how the model is built
	FbxNode * node;
	FbxMatrix matrixJaw;
	lPose->SetIsBindPose(false);
	int index = 0;
	matrixHeadO = lPose->GetMatrix(index);
	index = 3;
	matrixHead = lPose->GetMatrix(index);
	
	// In the case of the other skeletons, we just look for their indices in the pose
	// and rebuild the model with local matrices
	matrixJaw = lPose->GetMatrix(lPose->Find("jaw"));
	matrixJawEnd = lPose->GetMatrix(lPose->Find("jawEnd"));
	
	matrixJawEnd = matrixJaw.Inverse() * matrixJawEnd;
	index = lPose->Find("jawEnd");
	node = lPose->GetNode(index);
	lPose->Remove(index);
	lPose->Add(node, matrixJawEnd, true);

	matrixJaw = matrixHead.Inverse() * matrixJaw;
	index = lPose->Find("jaw");
	node = lPose->GetNode(index);
	lPose->Remove(index);
	lPose->Add(node, matrixJaw, true);

	lRotation[0] = lRotation[1] = lRotation[2] = 0.0f;
}

void Model::modifyHead(FbxVector4 T, FbxVector4 R, FbxVector4 S) {
	FbxNode * node;
	FbxMatrix matrix;
	FbxVector4 pTranslation, pRotation, pShearing, pScaling;
	double pSign;
	int index;
	bool isLocalMatrix = false;

	// Translate the head and rotate the head
	index = lPose->Find("head");
	node = lPose->GetNode(index);
	lPose->Remove(index);
	lPose->Add(node, matrixHeadO, isLocalMatrix);

	index = lPose->Find("head");
	node = lPose->GetNode(index);
	matrix = lPose->GetMatrix(index);
	isLocalMatrix = lPose->IsLocalMatrix(index);

	matrixHead.GetElements(pTranslation, pRotation, pShearing, pScaling, pSign);
	matrix.SetTRS(pTranslation + T, pRotation + R, pScaling*S);
	lPose->Remove(index);
	lPose->Add(node, matrix, isLocalMatrix);
}

void Model::modifyJaw(double value) {
	if (value < 0) value = 0;
	else value = (value / 100) * 1.5;
	FbxNode * node;
	FbxMatrix matrix;
	FbxVector4 pTranslation, pRotation, pShearing, pScaling;
	double pSign;
	int index;
	bool isLocalMatrix = false;

	index = lPose->Find("jawEnd");
	node = lPose->GetNode(index);
	matrix = lPose->GetMatrix(index);
	isLocalMatrix = lPose->IsLocalMatrix(index);

	matrixJawEnd.GetElements(pTranslation, pRotation, pShearing, pScaling, pSign);
	matrix.SetTRS(pTranslation - FbxVector4(0, value, 0, 1), pRotation, pScaling);
	lPose->Remove(index);
	lPose->Add(node, matrix, isLocalMatrix);
}


void Model::registerResult(FLOAT* scale, FLOAT* rotation, FLOAT* translation, FLOAT* AU, UINT* numAU, FLOAT* SU, UINT* numSU) {
	lScale = *scale;
	lRotation[0] = rotation[0];
	lRotation[1] = rotation[1];
	lRotation[2] = rotation[2];
	lTranslation = translation;
	lAU = AU;
	lNumAU = *numAU;
	lSU = SU;
	lNumSU = *numSU;
	newResult = true;
}

void Model::increaseRotation(FLOAT xAxis, FLOAT yAxis, FLOAT zAxis) {
	if (lRotation[0] + xAxis > -20 && lRotation[0] + xAxis < 20) lRotation[0] += xAxis;
	if (lRotation[1] + yAxis > -20 && lRotation[1] + yAxis < 20) lRotation[1] += yAxis;
	if (lRotation[2] + zAxis > -20 && lRotation[2] + zAxis < 20) lRotation[2] += zAxis;
	newResult = true;
}

void Model::update() {
	FbxAMatrix lDummyGlobalPosition;

	if (newResult) {
		if (lAU != NULL) {
			shocked = -(lAU[3] * 100) * 4 + 100;
			happy = -(lAU[4] * 100) * 3 + 20;
			jawLow = (lAU[1] * 100) * 3 + 50;
			upperLip = (lAU[0] * 100) * 3 - 50;
			lipStr = -(lAU[0] * 100) * 3 + 20;
		}
		modifyJaw(jawLow);
		modifyHead(FbxVector4(0, 0, 0, 0), FbxVector4((double)-lRotation[1], (double)lRotation[0], (double)lRotation[2], 1), FbxVector4(1, 1, 1, 0));

		newResult = false;
	} else if (stopAnim) {
		modifyHead(FbxVector4(0, 0, 0, 0), FbxVector4(0, 0, 0, 1), FbxVector4(1, 1, 1, 0));
		shocked = 50;
		stopAnim = false;
	}

	// Reset the vertices array
	vertices.clear();

	// Set our own weights
	std::vector<double> weights;
	weights.push_back(shocked);
	weights.push_back(happy);
	weights.push_back(jawLow);
	weights.push_back(lipStr);
	weights.push_back(upperLip);
	weights.push_back(outBrow);

	// Obtain the new deformed model
	DrawNodeRecursive(lScene->GetRootNode(), mCurrentTime, mCurrentAnimLayer, lDummyGlobalPosition, lPose, getVerticesArray(), &weights);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
}

void Model::render() {
	glm::mat4 Projection = glm::perspective(glm::radians(20.0f), (float)WNDW_WIDTH / (float)WNDW_HEIGHT, 0.1f, 1.0f);

	// Camera matrix
	//glm::mat4 View = glm::lookAt(
	//	glm::vec3(-1, 0, 25), // Camera is at (4,3,3), in World Space
	//	glm::vec3(2, -1, 1), // and looks at the origin
	//	glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	//	);

	// Boris
	glm::mat4 View = glm::lookAt(
		glm::vec3(-3.5, 0, 15), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);

	glm::mat4 Model = glm::translate(glm::vec3(-3.5,-2.25,0)) * glm::scale(glm::vec3(0.25,0.25,0.25)) * glm::rotate(0.0f, glm::vec3(0,1,0));

	glm::mat4 mvp = Projection * View * Model;

	glUseProgram(program_handle);

	glUniformMatrix4fv(mvp_handle, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(ModelMatrix_handle, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(ViewMatrix_handle, 1, GL_FALSE, &View[0][0]);
	glUniform3f(Light_handle, -3.5, 0, 25);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	// Set our "myTextureSampler" sampler to user Texture Unit 0
	glUniform1i(texture_handle, 0);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_triangles);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_triangles);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,	                       // type
		GL_TRUE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
		);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_triangles);
	glVertexAttribPointer(
		2,                                // attribute
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
		);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, materialbuffer_triangles);
	glVertexAttribPointer(
		3,                                // attribute
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) vertices.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
}