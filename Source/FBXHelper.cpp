#include "FBXHelper.hpp"

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pManager->GetIOSettings()))
#endif

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
	if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int i, lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}

void DisplayHierarchy(FbxNode* pNode, int pDepth)
{
	FbxString lString;
	int i;

	for (i = 0; i < pDepth; i++)
	{
		lString += "     ";
	}

	lString += pNode->GetName();
	lString += "\n";

	FBXSDK_printf(lString.Buffer());

	for (i = 0; i < pNode->GetChildCount(); i++)
	{
		DisplayHierarchy(pNode->GetChild(i), pDepth + 1);
	}
}

FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition)
{
	FbxAMatrix lGlobalPosition;
	bool        lPositionFound = false;

	if (pPose)
	{
		int lNodeIndex = pPose->Find(pNode);

		if (lNodeIndex > -1)
		{
			// The bind pose is always a global matrix.
			// If we have a rest pose, we need to check if it is
			// stored in global or local space.
			if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
			{
				lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
			}
			else
			{
				// We have a local matrix, we need to convert it to
				// a global space matrix.
				FbxAMatrix lParentGlobalPosition;

				if (pParentGlobalPosition)
				{
					lParentGlobalPosition = *pParentGlobalPosition;
				}
				else
				{
					if (pNode->GetParent())
					{
						lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose);
					}
				}

				FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
				lGlobalPosition = lParentGlobalPosition * lLocalPosition;
			}

			lPositionFound = true;
		}
	}

	if (!lPositionFound)
	{
		// There is no pose entry for that node, get the current global position instead.

		// Ideally this would use parent global position and local position to compute the global position.
		// Unfortunately the equation 
		//    lGlobalPosition = pParentGlobalPosition * lLocalPosition
		// does not hold when inheritance type is other than "Parent" (RSrs).
		// To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
		lGlobalPosition = pNode->EvaluateGlobalTransform(pTime); 
	}

	return lGlobalPosition;
}

// Get the matrix of the given pose
FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex)
{
	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

	return lPoseMatrix;
}

// Get the geometry offset to a node. It is never inherited by the children.
FbxAMatrix GetGeometry(FbxNode* pNode)
{
	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void PreparePointCacheData(FbxScene* pScene, FbxTime &pCache_Start, FbxTime &pCache_Stop)
{
	// This function show how to cycle through scene elements in a linear way.
	const int lNodeCount = pScene->GetSrcObjectCount<FbxNode>();
	FbxStatus lStatus;

	for (int lIndex = 0; lIndex<lNodeCount; lIndex++)
	{
		FbxNode* lNode = pScene->GetSrcObject<FbxNode>(lIndex);

		if (lNode->GetGeometry())
		{
			int i, lVertexCacheDeformerCount = lNode->GetGeometry()->GetDeformerCount(FbxDeformer::eVertexCache);

			// There should be a maximum of 1 Vertex Cache Deformer for the moment
			lVertexCacheDeformerCount = lVertexCacheDeformerCount > 0 ? 1 : 0;

			for (i = 0; i<lVertexCacheDeformerCount; ++i)
			{
				// Get the Point Cache object
				FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(lNode->GetGeometry()->GetDeformer(i, FbxDeformer::eVertexCache));
				if (!lDeformer) continue;
				FbxCache* lCache = lDeformer->GetCache();
				if (!lCache) continue;

				// Process the point cache data only if the constraint is active
				if (lDeformer->Active.Get())
				{
					if (lCache->GetCacheFileFormat() == FbxCache::eMaxPointCacheV2)
					{
						// This code show how to convert from PC2 to MC point cache format
						// turn it on if you need it.
#if 0 
						if (!lCache->ConvertFromPC2ToMC(FbxCache::eMCOneFile,
							FbxTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode())))
						{
							// Conversion failed, retrieve the error here
							FbxString lTheErrorIs = lCache->GetStaus().GetErrorString();
						}
#endif
					}
					else if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
					{
						// This code show how to convert from MC to PC2 point cache format
						// turn it on if you need it.
						//#if 0 
						if (!lCache->ConvertFromMCToPC2(FbxTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode()), 0, &lStatus))
						{
							// Conversion failed, retrieve the error here
							FbxString lTheErrorIs = lStatus.GetErrorString();
						}
						//#endif
					}


					// Now open the cache file to read from it
					if (!lCache->OpenFileForRead(&lStatus))
					{
						// Cannot open file 
						FbxString lTheErrorIs = lStatus.GetErrorString();

						// Set the deformer inactive so we don't play it back
						lDeformer->Active = false;
					}
					else
					{
						// get the start and stop time of the cache
						FbxTime lChannel_Start;
						FbxTime lChannel_Stop;
						int lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
						if (lCache->GetAnimationRange(lChannelIndex, lChannel_Start, lChannel_Stop))
						{
							// get the smallest start time
							if (lChannel_Start < pCache_Start) pCache_Start = lChannel_Start;

							// get the biggest stop time
							if (lChannel_Stop  > pCache_Stop)  pCache_Stop = lChannel_Stop;
						}
					}
				}
			}
		}
	}
}

void ReadVertexCacheData(FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray)
{
	FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(pMesh->GetDeformer(0, FbxDeformer::eVertexCache));
	FbxCache*               lCache = lDeformer->GetCache();
	int                     lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
	unsigned int            lVertexCount = (unsigned int)pMesh->GetControlPointsCount();
	bool                    lReadSucceed = false;
	float*                  lReadBuf = NULL;
	unsigned int			BufferSize = 0;

	if (lDeformer->Type.Get() != FbxVertexCacheDeformer::ePositions)
		// only process positions
		return;

	unsigned int Length = 0;
	lCache->Read(NULL, Length, FBXSDK_TIME_ZERO, lChannelIndex);
	if (Length != lVertexCount * 3)
		// the content of the cache is by vertex not by control points (we don't support it here)
		return;

	lReadSucceed = lCache->Read(&lReadBuf, BufferSize, pTime, lChannelIndex);
	if (lReadSucceed)
	{
		unsigned int lReadBufIndex = 0;

		while (lReadBufIndex < 3 * lVertexCount)
		{
			// In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
			// on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
			// So separate them.
			pVertexArray[lReadBufIndex / 3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
		}
	}
}

void DrawNodeRecursive(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pParentGlobalPosition, FbxPose* pPose, std::vector<glm::vec3>* vertices, std::vector<double>* weights)
{
	FbxAMatrix lGlobalPosition = GetGlobalPosition(pNode, pTime, pPose, &pParentGlobalPosition);

	if (pNode->GetNodeAttribute())
	{
		// Geometry offset.
		// it is not inherited by the children.
		FbxAMatrix lGeometryOffset = GetGeometry(pNode);
		FbxAMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;

		DrawNode(pNode, pTime, pAnimLayer, pParentGlobalPosition, lGlobalOffPosition, pPose, vertices, weights);
	}

	const int lChildCount = pNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		DrawNodeRecursive(pNode->GetChild(lChildIndex), pTime, pAnimLayer, lGlobalPosition, pPose, vertices, weights);
	}
}

void DrawNode(FbxNode* pNode,
	FbxTime& pTime, FbxAnimLayer* pAnimLayer,
	FbxAMatrix& pParentGlobalPosition,
	FbxAMatrix& pGlobalPosition,
	FbxPose* pPose, 
	std::vector<glm::vec3>* vertices,
	std::vector<double>* weights)
{
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

	if (lNodeAttribute)
	{
		// All lights has been processed before the whole scene because they influence every geometry.
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMarker)
		{
			DrawMarker(pGlobalPosition);
		}
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			DrawSkeleton(pNode, pParentGlobalPosition, pGlobalPosition);
		}
		// NURBS and patch have been converted into triangluation meshes.
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			DrawMesh(pNode, pTime, pAnimLayer, pGlobalPosition, pPose, vertices, weights);
		}
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eCamera)
		{
			// We don't draw cameras.
		}
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull)
		{
			DrawNull(pGlobalPosition);
		}
	}
	else
	{
		// Draw a Null for nodes without attribute.
		DrawNull(pGlobalPosition);
	}
}

void DrawMarker(FbxAMatrix& pGlobalPosition)
{
	// Markers
	//GlDrawMarker(pGlobalPosition);
}

void DrawSkeleton(FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition)
{
	// Skeleton
	//FbxSkeleton* lSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();

	//// Only draw the skeleton if it's a limb node and if 
	//// the parent also has an attribute of type skeleton.
	//if (lSkeleton->GetSkeletonType() == FbxSkeleton::eLimbNode &&
	//	pNode->GetParent() &&
	//	pNode->GetParent()->GetNodeAttribute() &&
	//	pNode->GetParent()->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	//{
	//	GlDrawLimbNode(pParentGlobalPosition, pGlobalPosition);
	//}
}

void DrawMesh(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pGlobalPosition, FbxPose* pPose, std::vector<glm::vec3>* vertices, std::vector<double>* weights)
{
	FbxMesh* lMesh = pNode->GetMesh();
	const int lVertexCount = lMesh->GetControlPointsCount();

	// No vertex to draw.
	if (lVertexCount == 0)
	{
		return;
	}

	// If it has some defomer connection, update the vertices position
	const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
		(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
	const bool lHasShape = lMesh->GetShapeCount() > 0;
	const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
	const bool lHasDeformation = lHasShape || lHasSkin;

	//std::cout << "DRAW MESH" << std::endl;

	FbxVector4* lVertexArray = NULL;
	lVertexArray = new FbxVector4[lVertexCount];
	memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));

	if (lHasDeformation)
	{
		// Active vertex cache deformer will overwrite any other deformer
		if (lHasVertexCache)
		{
			ReadVertexCacheData(lMesh, pTime, lVertexArray);
		}
		else
		{
			if (lHasShape)
			{
				// Deform the vertex array with the shapes.
				//std::cout << "  -> SHAPE" << std::endl;
				ComputeShapeDeformation(lMesh, pTime, pAnimLayer, lVertexArray, weights);
			}

			// We need to get the number of clusters
			const int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);
			int lClusterCount = 0;
			for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
			{
				lClusterCount += ((FbxSkin *)(lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
			}
			if (lClusterCount)
			{
				//std::cout << "  -> SKIN" << std::endl;
				// Deform the vertex array with the skin deformer.
				ComputeSkinDeformation(pGlobalPosition, lMesh, pTime, lVertexArray, pPose);
			}
		}
	}

	// Update the vertices array
	//FbxVector4* fbxControlPoints = lMesh->GetControlPoints();
	const int lPolygonCount = lMesh->GetPolygonCount();
	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++)
	{
		const int lVerticeCount = lMesh->GetPolygonSize(lPolygonIndex);
		for (int lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++)
		{
			// Get vertex position
			FbxVector4 fbxVertex = pGlobalPosition.MultT(lVertexArray[lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)]);
			vertices->push_back(glm::vec3((float)fbxVertex[0], (float)fbxVertex[1], (float)fbxVertex[2]));

			// Maybe do the same with the normals and UVs
		}
	}


	delete[] lVertexArray;
}


void DrawNull(FbxAMatrix& pGlobalPosition)
{
	// Crosshair
	//GlDrawCrossHair(pGlobalPosition);
}

void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxVector4* pVertexArray, std::vector<double>* weights)
{
	int lVertexCount = pMesh->GetControlPointsCount();

	FbxVector4* lSrcVertexArray = pVertexArray;
	FbxVector4* lDstVertexArray = new FbxVector4[lVertexCount];
	memcpy(lDstVertexArray, pVertexArray, lVertexCount * sizeof(FbxVector4));

	int lBlendShapeDeformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
	for (int lBlendShapeIndex = 0; lBlendShapeIndex<lBlendShapeDeformerCount; ++lBlendShapeIndex)
	{
		FbxBlendShape* lBlendShape = (FbxBlendShape*)pMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

		int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
		for (int lChannelIndex = 0; lChannelIndex<lBlendShapeChannelCount; ++lChannelIndex)
		{
			FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
			if (lChannel)
			{
				// Get the percentage of influence on this channel.
				FbxString name = lChannel->GetNameOnly();

				double lWeight = evaluateChannel(weights, name);

				/*FbxAnimCurve* lFCurve = pMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer);
				if (!lFCurve) continue;
				double lWeight = lFCurve->Evaluate(pTime);*/

				/*
				If there is only one targetShape on this channel, the influence is easy to calculate:
				influence = (targetShape - baseGeometry) * weight * 0.01
				dstGeometry = baseGeometry + influence

				But if there are more than one targetShapes on this channel, this is an in-between
				blendshape, also called progressive morph. The calculation of influence is different.

				For example, given two in-between targets, the full weight percentage of first target
				is 50, and the full weight percentage of the second target is 100.
				When the weight percentage reach 50, the base geometry is already be fully morphed
				to the first target shape. When the weight go over 50, it begin to morph from the
				first target shape to the second target shape.

				To calculate influence when the weight percentage is 25:
				1. 25 falls in the scope of 0 and 50, the morphing is from base geometry to the first target.
				2. And since 25 is already half way between 0 and 50, so the real weight percentage change to
				the first target is 50.
				influence = (firstTargetShape - baseGeometry) * (25-0)/(50-0) * 100
				dstGeometry = baseGeometry + influence

				To calculate influence when the weight percentage is 75:
				1. 75 falls in the scope of 50 and 100, the morphing is from the first target to the second.
				2. And since 75 is already half way between 50 and 100, so the real weight percentage change
				to the second target is 50.
				influence = (secondTargetShape - firstTargetShape) * (75-50)/(100-50) * 100
				dstGeometry = firstTargetShape + influence
				*/

				// Find the two shape indices for influence calculation according to the weight.
				// Consider index of base geometry as -1.

				int lShapeCount = lChannel->GetTargetShapeCount();
				double* lFullWeights = lChannel->GetTargetShapeFullWeights();

				// Find out which scope the lWeight falls in.
				int lStartIndex = -1;
				int lEndIndex = -1;
				for (int lShapeIndex = 0; lShapeIndex<lShapeCount; ++lShapeIndex)
				{
					if (lWeight > 0 && lWeight <= lFullWeights[0])
					{
						lEndIndex = 0;
						break;
					}
					if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex + 1])
					{
						lStartIndex = lShapeIndex;
						lEndIndex = lShapeIndex + 1;
						break;
					}
				}

				FbxShape* lStartShape = NULL;
				FbxShape* lEndShape = NULL;
				if (lStartIndex > -1)
				{
					lStartShape = lChannel->GetTargetShape(lStartIndex);
				}
				if (lEndIndex > -1)
				{
					lEndShape = lChannel->GetTargetShape(lEndIndex);
				}

				//The weight percentage falls between base geometry and the first target shape.
				if (lStartIndex == -1 && lEndShape)
				{
					double lEndWeight = lFullWeights[0];
					// Calculate the real weight.
					lWeight = (lWeight / lEndWeight) * 100;
					// Initialize the lDstVertexArray with vertex of base geometry.
					memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));
					for (int j = 0; j < lVertexCount; j++)
					{
						// Add the influence of the shape vertex to the mesh vertex.
						FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lSrcVertexArray[j]) * lWeight * 0.01;
						lDstVertexArray[j] += lInfluence;
					}
				}
				//The weight percentage falls between two target shapes.
				else if (lStartShape && lEndShape)
				{
					double lStartWeight = lFullWeights[lStartIndex];
					double lEndWeight = lFullWeights[lEndIndex];
					// Calculate the real weight.
					lWeight = ((lWeight - lStartWeight) / (lEndWeight - lStartWeight)) * 100;
					// Initialize the lDstVertexArray with vertex of the previous target shape geometry.
					memcpy(lDstVertexArray, lStartShape->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
					for (int j = 0; j < lVertexCount; j++)
					{
						// Add the influence of the shape vertex to the previous shape vertex.
						FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lStartShape->GetControlPoints()[j]) * lWeight * 0.01;
						lDstVertexArray[j] += lInfluence;
					}
				}
			}//If lChannel is valid
		}//For each blend shape channel
	}//For each blend shape deformer

	memcpy(pVertexArray, lDstVertexArray, lVertexCount * sizeof(FbxVector4));

	delete[] lDstVertexArray;
}

void ComputeSkinDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose)
{
	FbxSkin * lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(0, FbxDeformer::eSkin);
	FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();

	if (lSkinningType == FbxSkin::eLinear || lSkinningType == FbxSkin::eRigid)
	{
		ComputeLinearDeformation(pGlobalPosition, pMesh, pTime, pVertexArray, pPose);
	}
	else if (lSkinningType == FbxSkin::eDualQuaternion)
	{
		ComputeDualQuaternionDeformation(pGlobalPosition, pMesh, pTime, pVertexArray, pPose);
	}
	else if (lSkinningType == FbxSkin::eBlend)
	{
		int lVertexCount = pMesh->GetControlPointsCount();

		FbxVector4* lVertexArrayLinear = new FbxVector4[lVertexCount];
		memcpy(lVertexArrayLinear, pMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));

		FbxVector4* lVertexArrayDQ = new FbxVector4[lVertexCount];
		memcpy(lVertexArrayDQ, pMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));

		ComputeLinearDeformation(pGlobalPosition, pMesh, pTime, lVertexArrayLinear, pPose);
		ComputeDualQuaternionDeformation(pGlobalPosition, pMesh, pTime, lVertexArrayDQ, pPose);

		// To blend the skinning according to the blend weights
		// Final vertex = DQSVertex * blend weight + LinearVertex * (1- blend weight)
		// DQSVertex: vertex that is deformed by dual quaternion skinning method;
		// LinearVertex: vertex that is deformed by classic linear skinning method;
		int lBlendWeightsCount = lSkinDeformer->GetControlPointIndicesCount();
		for (int lBWIndex = 0; lBWIndex<lBlendWeightsCount; ++lBWIndex)
		{
			double lBlendWeight = lSkinDeformer->GetControlPointBlendWeights()[lBWIndex];
			pVertexArray[lBWIndex] = lVertexArrayDQ[lBWIndex] * lBlendWeight + lVertexArrayLinear[lBWIndex] * (1 - lBlendWeight);
		}
	}
}

void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

	int lVertexCount = pMesh->GetControlPointsCount();
	FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount * sizeof(FbxAMatrix));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));

	if (lClusterMode == FbxCluster::eAdditive)
	{
		for (int i = 0; i < lVertexCount; ++i)
		{
			lClusterDeformation[i].SetIdentity();
		}
	}

	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int lSkinIndex = 0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);

		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];

				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;

				double lWeight = lCluster->GetControlPointWeights()[k];

				if (lWeight == 0.0)
				{
					continue;
				}

				// Compute the influence of the link on the vertex.
				FbxAMatrix lInfluence = lVertexTransformMatrix;
				MatrixScale(lInfluence, lWeight);

				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Multiply with the product of the deformations on the vertex.
					MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
					lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];

					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					// Add to the sum of the deformations on the vertex.
					MatrixAdd(lClusterDeformation[lIndex], lInfluence);

					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex			
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeight = lClusterWeight[i];

		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeight != 0.0)
		{
			lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links. 
				lDstVertex /= lWeight;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links. 
				lSrcVertex *= (1.0 - lWeight);
				lDstVertex += lSrcVertex;
			}
		}
	}

	delete[] lClusterDeformation;
	delete[] lClusterWeight;
}

void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxCluster* pCluster,
	FbxAMatrix& pVertexTransformMatrix,
	FbxTime pTime,
	FbxPose* pPose)
{
	FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();

	FbxAMatrix lReferenceGlobalInitPosition;
	FbxAMatrix lReferenceGlobalCurrentPosition;
	FbxAMatrix lAssociateGlobalInitPosition;
	FbxAMatrix lAssociateGlobalCurrentPosition;
	FbxAMatrix lClusterGlobalInitPosition;
	FbxAMatrix lClusterGlobalCurrentPosition;

	FbxAMatrix lReferenceGeometry;
	FbxAMatrix lAssociateGeometry;
	FbxAMatrix lClusterGeometry;

	FbxAMatrix lClusterRelativeInitPosition;
	FbxAMatrix lClusterRelativeCurrentPositionInverse;

	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		pCluster->GetTransformAssociateModelMatrix(lAssociateGlobalInitPosition);
		// Geometric transform of the model
		lAssociateGeometry = GetGeometry(pCluster->GetAssociateModel());
		lAssociateGlobalInitPosition *= lAssociateGeometry;
		lAssociateGlobalCurrentPosition = GetGlobalPosition(pCluster->GetAssociateModel(), pTime, pPose);

		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;
		lReferenceGlobalCurrentPosition = pGlobalPosition;

		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		// Multiply lClusterGlobalInitPosition by Geometric Transformation
		lClusterGeometry = GetGeometry(pCluster->GetLink());
		lClusterGlobalInitPosition *= lClusterGeometry;
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);

		// Compute the shift of the link relative to the reference.
		//ModelM-1 * AssoM * AssoGX-1 * LinkGX * LinkM-1*ModelM
		pVertexTransformMatrix = lReferenceGlobalInitPosition.Inverse() * lAssociateGlobalInitPosition * lAssociateGlobalCurrentPosition.Inverse() *
			lClusterGlobalCurrentPosition * lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
	}
	else
	{
		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		lReferenceGlobalCurrentPosition = pGlobalPosition;
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;

		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);

		// Compute the initial position of the link relative to the reference.
		lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

		// Compute the current position of the link relative to the reference.
		lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

		// Compute the shift of the link relative to the reference.
		pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
	}
}

void ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

	int lVertexCount = pMesh->GetControlPointsCount();
	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

	FbxDualQuaternion* lDQClusterDeformation = new FbxDualQuaternion[lVertexCount];
	memset(lDQClusterDeformation, 0, lVertexCount * sizeof(FbxDualQuaternion));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));

	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	for (int lSkinIndex = 0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			FbxQuaternion lQ = lVertexTransformMatrix.GetQ();
			FbxVector4 lT = lVertexTransformMatrix.GetT();
			FbxDualQuaternion lDualQuaternion(lQ, lT);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];

				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;

				double lWeight = lCluster->GetControlPointWeights()[k];

				if (lWeight == 0.0)
					continue;

				// Compute the influence of the link on the vertex.
				FbxDualQuaternion lInfluence = lDualQuaternion * lWeight;
				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Simply influenced by the dual quaternion.
					lDQClusterDeformation[lIndex] = lInfluence;

					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					if (lClusterIndex == 0)
					{
						lDQClusterDeformation[lIndex] = lInfluence;
					}
					else
					{
						// Add to the sum of the deformations on the vertex.
						// Make sure the deformation is accumulated in the same rotation direction. 
						// Use dot product to judge the sign.
						double lSign = lDQClusterDeformation[lIndex].GetFirstQuaternion().DotProduct(lDualQuaternion.GetFirstQuaternion());
						if (lSign >= 0.0)
						{
							lDQClusterDeformation[lIndex] += lInfluence;
						}
						else
						{
							lDQClusterDeformation[lIndex] -= lInfluence;
						}
					}
					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeightSum = lClusterWeight[i];

		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeightSum != 0.0)
		{
			lDQClusterDeformation[i].Normalize();
			lDstVertex = lDQClusterDeformation[i].Deform(lDstVertex);

			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links. 
				lDstVertex /= lWeightSum;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links. 
				lSrcVertex *= (1.0 - lWeightSum);
				lDstVertex += lSrcVertex;
			}
		}
	}

	delete[] lDQClusterDeformation;
	delete[] lClusterWeight;
}

// Scale all the elements of a matrix.
void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}


// Add a value to all the elements in the diagonal of the matrix.
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
	pMatrix[0][0] += pValue;
	pMatrix[1][1] += pValue;
	pMatrix[2][2] += pValue;
	pMatrix[3][3] += pValue;
}


// Sum two matrices element by element.
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pDstMatrix[i][j] += pSrcMatrix[i][j];
		}
	}
}

double evaluateChannel(std::vector<double>* weights, FbxString name) {
	double weight = 0;

	if (name == "mouthEyes_Blend.smile") {
		weight = weights->at(0);
		//std::cout << name << ": " << weight << std::endl;
	}

	return weight;
	//return weights->at(0);
}