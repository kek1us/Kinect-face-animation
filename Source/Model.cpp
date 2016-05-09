#include "Model.hpp"

Model::Model() {}

Model::~Model() {
	glDeleteBuffers(1, &vertexbuffer_quads);
	glDeleteBuffers(1, &uvbuffer_quads);
	glDeleteBuffers(1, &normalbuffer_quads);
	glDeleteBuffers(1, &vertexbuffer_triangles);
	glDeleteBuffers(1, &uvbuffer_triangles);
	glDeleteBuffers(1, &normalbuffer_triangles);
	glDeleteProgram(program_handle);
	glDeleteTextures(1, &textureId);
}

bool Model::init() {
	//if (!loadModel(MODEL_SRC))  exit(EXIT_FAILURE);
	if (!loadFBX(MODEL_SRC)) exit(EXIT_FAILURE);
	if (!initVBO()) exit(EXIT_FAILURE);
	textureId = loadTex(TEXTURE_SRC);
	return true;
}

bool Model::initVBO() {
	glGenBuffers(1, &vertexbuffer_quads);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_quads);
	glBufferData(GL_ARRAY_BUFFER, vertices_quads.size() * sizeof(glm::vec3), &vertices_quads[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vertexbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer_quads);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_quads);
	glBufferData(GL_ARRAY_BUFFER, uvs_quads.size() * sizeof(glm::vec2), &uvs_quads[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer_quads);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_quads);
	glBufferData(GL_ARRAY_BUFFER, normals_quads.size() * sizeof(glm::vec3), &normals_quads[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer_triangles);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_triangles);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	program_handle = loadShaders("Dependencies/Shaders/Model.vert", "Dependencies/Shaders/Model.frag");
	texture_handle = glGetUniformLocation(program_handle, "myTextureSampler");
	mvp_handle = glGetUniformLocation(program_handle, "MVP");
	ViewMatrix_handle = glGetUniformLocation(program_handle, "V");
	ModelMatrix_handle = glGetUniformLocation(program_handle, "M");
	Light_handle = glGetUniformLocation(program_handle, "LightPosition_worldspace");

	return true;
}

bool Model::loadModel(std::string filename) {
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices, vertexIndices_triangles, uvIndices_triangles, normalIndices_triangles;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	FILE * file = fopen(filename.c_str(), "r");
	std::cout << filename.c_str() << std::endl;
	if (file == NULL) {
		std::cout << "Impossible to open the file!" << std::endl;
		return false;
	}

	char lineHeader[128];
	fpos_t position;
	// read the first word of the line
	int res = fscanf(file, "%s", lineHeader);
	while (1) {
		int matches;
		if (res == EOF) {
			break; // EOF = End Of File. Quit the loop.
		}
		else if (lineHeader == std::string("v")) {
			glm::vec3 vertex;
			matches = fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (lineHeader == std::string("vt")) {
			glm::vec2 uv;
			matches = fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (lineHeader == std::string("vn")) {
			glm::vec3 normal;
			matches = fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (lineHeader == std::string("f")) {
			fgetpos(file, &position);

			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2], &vertexIndex[3], &uvIndex[3], &normalIndex[3]);

			if (matches == 1) {
				fsetpos(file, &position);
				matches = fscanf(file, "%d//%d %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2], &vertexIndex[3], &normalIndex[3]);
				uvIndex[0] = 1;
				uvIndex[1] = 1;
				uvIndex[2] = 1;
				uvIndex[3] = 1;
			}

			if (matches != 12 && matches != 9 && matches != 8 && matches != 6) {
				printf("File can't be read by our simple parser : ( Try exporting with other options, %d\n", matches);
				return false;
			}
			if (matches == 12 || matches == 8) {
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				vertexIndices.push_back(vertexIndex[3]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				uvIndices.push_back(uvIndex[3]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
				normalIndices.push_back(normalIndex[3]);
			}
			else {
				vertexIndices_triangles.push_back(vertexIndex[0]);
				vertexIndices_triangles.push_back(vertexIndex[1]);
				vertexIndices_triangles.push_back(vertexIndex[2]);
				uvIndices_triangles.push_back(uvIndex[0]);
				uvIndices_triangles.push_back(uvIndex[1]);
				uvIndices_triangles.push_back(uvIndex[2]);
				normalIndices_triangles.push_back(normalIndex[0]);
				normalIndices_triangles.push_back(normalIndex[1]);
				normalIndices_triangles.push_back(normalIndex[2]);
			}

		}
		res = fscanf(file, "%s", lineHeader);
	}

	for (unsigned int i = 0; i<vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		vertices_quads.push_back(vertex);
	}

	for (unsigned int i = 0; i<uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		uvs_quads.push_back(uv);
	}

	for (unsigned int i = 0; i<normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		normals_quads.push_back(normal);
	}

	for (unsigned int i = 0; i<vertexIndices_triangles.size(); i++) {
		unsigned int vertexIndex = vertexIndices_triangles[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		vertices_triangles.push_back(vertex);
	}

	for (unsigned int i = 0; i<uvIndices_triangles.size(); i++) {
		unsigned int uvIndex = uvIndices_triangles[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		uvs_triangles.push_back(uv);
	}

	for (unsigned int i = 0; i<normalIndices_triangles.size(); i++) {
		unsigned int normalIndex = normalIndices_triangles[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		normals_triangles.push_back(normal);
	}
	return true;
}

bool Model::loadFBX(std::string filename) {
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	bool lResult = true;

	// Prepare the FBX SDK.
	InitializeSdkObjects(lSdkManager, lScene);

	// Load the scene
	FbxString lFilePath(filename.c_str());
	lResult = LoadScene(lSdkManager, lScene, lFilePath.Buffer());

	// Load the model
	FbxNode* lNode = lScene->GetRootNode();
	this->getFBXData(lNode);

	// Destroy all objects created by the FBX SDK.
	DestroySdkObjects(lSdkManager, lResult);

	if (lResult) FBXSDK_printf("THERE WE GO, BOIS!\n");

	return lResult;
}

void Model::getFBXData(FbxNode* node) {
	int numChilds = node->GetChildCount();

	for (int i = 0; i < numChilds; i++) {
		FbxNode* childNode = node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		if (mesh != NULL) {
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

void Model::update() {

}

void Model::render() {
	glm::mat4 Projection = glm::perspective(glm::radians(20.0f), (float)WNDW_WIDTH / (float)WNDW_HEIGHT, 0.1f, 1.0f);

	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(-1, 0, 25), // Camera is at (4,3,3), in World Space
		glm::vec3(2, -1, 1), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);

	// Boris
	//glm::mat4 View = glm::lookAt(
	//	glm::vec3(-3.5, 0, 15), // Camera is at (4,3,3), in World Space
	//	glm::vec3(0, 0, 0), // and looks at the origin
	//	glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	//	);

	glm::mat4 Model = glm::translate(glm::vec3(-3.5,-2.25,0)) * glm::scale(glm::vec3(0.25,0.25,0.25)) * glm::rotate(0.0f, glm::vec3(0,1,0));

	glm::mat4 mvp = Projection * View * Model;

	glUseProgram(program_handle);

	glUniformMatrix4fv(mvp_handle, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(ModelMatrix_handle, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(ViewMatrix_handle, 1, GL_FALSE, &View[0][0]);
	glUniform3f(Light_handle, 4, 4 + 20, 4);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	// Set our "myTextureSampler" sampler to user Texture Unit 0
	glUniform1i(texture_handle, 0);

	//// 1rst attribute buffer : vertices
	//glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_quads);
	//glVertexAttribPointer(
	//	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	//	3,                  // size
	//	GL_FLOAT,           // type
	//	GL_FALSE,           // normalized?
	//	0,                  // stride
	//	(void*)0            // array buffer offset
	//	);
	//// 2nd attribute buffer : colors
	//glEnableVertexAttribArray(1);
	//glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_quads);
	//glVertexAttribPointer(
	//	1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
	//	2,                                // size
	//	GL_FLOAT,	                       // type
	//	GL_TRUE,                         // normalized?
	//	0,                                // stride
	//	(void*)0                          // array buffer offset
	//	);

	//// 3rd attribute buffer : normals
	//glEnableVertexAttribArray(2);
	//glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_quads);
	//glVertexAttribPointer(
	//	2,                                // attribute
	//	3,                                // size
	//	GL_FLOAT,                         // type
	//	GL_FALSE,                         // normalized?
	//	0,                                // stride
	//	(void*)0                          // array buffer offset
	//	);

	//glDrawArrays(GL_QUADS, 0, vertices_quads.size());

	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	//glDisableVertexAttribArray(2);

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

	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}