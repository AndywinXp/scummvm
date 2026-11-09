/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/array.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

void RenderDroid::Model3Module::clearFrameCounters() {
	_numDrawnModels = 0;
}

void RenderDroid::Model3Module::activateClipPlane(int onOff) {
	_clipPlaneActive = onOff;
}

void RenderDroid::Model3Module::setClipPlane(RdVector3 norm, RdVector3 pt) {
	_clipNorm[0] = norm[0];
	_clipNorm[1] = norm[1];
	_clipNorm[2] = norm[2];
	_clipPt[0] = pt[0];
	_clipPt[1] = pt[1];
	_clipPt[2] = pt[2];
}

int RenderDroid::Model3Module::newEntry(RdModel3 *pModel3) {
	memset(pModel3, 0, sizeof(RdModel3));
	Common::strlcpy(pModel3->aName, "UNKNOWN", 32);
	pModel3->curGeoSet = 0;

	return 0;
}

void RenderDroid::Model3Module::calcBoundingBoxes(RdModel3 *pModel3) {
	pModel3->bound.min[0] = 3.4e38f;
	pModel3->bound.min[1] = 3.4e38f;
	pModel3->bound.min[2] = 3.4e38f;
	pModel3->bound.max[0] = -3.4e38f;
	pModel3->bound.max[1] = -3.4e38f;
	pModel3->bound.max[2] = -3.4e38f;

	for (uint i = 0; i < pModel3->numGeos; i++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[i];

		for (uint j = 0; j < pGeo->numMeshes; j++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[j];

			pMesh->bound.min[0] = 3.4e38f;
			pMesh->bound.min[1] = 3.4e38f;
			pMesh->bound.min[2] = 3.4e38f;
			pMesh->bound.max[0] = -3.4e38f;
			pMesh->bound.max[1] = -3.4e38f;
			pMesh->bound.max[2] = -3.4e38f;

			for (uint k = 0; k < pMesh->numVertices; k++) {
				float *pVertex = pMesh->aVertices[k];

				if (pVertex[0] < (double)pMesh->bound.min[0])
					pMesh->bound.min[0] = pVertex[0];
				if (pVertex[1] < (double)pMesh->bound.min[1])
					pMesh->bound.min[1] = pVertex[1];
				if (pVertex[2] < (double)pMesh->bound.min[2])
					pMesh->bound.min[2] = pVertex[2];
				if (pVertex[0] > (double)pMesh->bound.max[0])
					pMesh->bound.max[0] = pVertex[0];
				if (pVertex[1] > (double)pMesh->bound.max[1])
					pMesh->bound.max[1] = pVertex[1];
				if (pVertex[2] > (double)pMesh->bound.max[2])
					pMesh->bound.max[2] = pVertex[2];
			}

			// The demo takes the box of the model from the boxes of the meshes
			if (_rd->isDemo()) {
				if (pMesh->bound.min[0] < (double)pModel3->bound.min[0])
					pModel3->bound.min[0] = pMesh->bound.min[0];
				if (pMesh->bound.min[1] < (double)pModel3->bound.min[1])
					pModel3->bound.min[1] = pMesh->bound.min[1];
				if (pMesh->bound.min[2] < (double)pModel3->bound.min[2])
					pModel3->bound.min[2] = pMesh->bound.min[2];
				if (pMesh->bound.max[0] > (double)pModel3->bound.max[0])
					pModel3->bound.max[0] = pMesh->bound.max[0];
				if (pMesh->bound.max[1] > (double)pModel3->bound.max[1])
					pModel3->bound.max[1] = pMesh->bound.max[1];
				if (pMesh->bound.max[2] > (double)pModel3->bound.max[2])
					pModel3->bound.max[2] = pMesh->bound.max[2];
			}
		}
	}

	// The demo has no center of the geometry and no radius from the box
	if (_rd->isDemo())
		return;

	pModel3->bound.min[0] = 3.4e38f;
	pModel3->bound.min[1] = 3.4e38f;
	pModel3->bound.min[2] = 3.4e38f;
	pModel3->bound.max[0] = -3.4e38f;
	pModel3->bound.max[1] = -3.4e38f;
	pModel3->bound.max[2] = -3.4e38f;
	calcModelBoundingBox(pModel3, pModel3->aHNodes, _rd->_identMatrix34);

	double dy = (double)pModel3->bound.min[1] - pModel3->bound.max[1];
	double dx = (double)pModel3->bound.min[0] - pModel3->bound.max[0];
	double dz = (double)pModel3->bound.min[2] - pModel3->bound.max[2];
	float dxStored = (float)dx;
	double dxSquared = dx * dxStored;
	float dzStored = (float)dz;
	double centerY = ((double)pModel3->bound.max[1] - pModel3->bound.min[1]) * 0.5 + pModel3->bound.min[1];
	double centerZ = ((double)pModel3->bound.max[2] - pModel3->bound.min[2]) * 0.5 + pModel3->bound.min[2];

	pModel3->geomCenter[0] = (float)(((double)pModel3->bound.max[0] - pModel3->bound.min[0]) * 0.5 + pModel3->bound.min[0]);
	pModel3->geomCenter[1] = (float)centerY;
	pModel3->geomCenter[2] = (float)centerZ;
	pModel3->radius = (float)(sqrt(dxSquared + dy * dy + dz * dzStored) * 0.5);
}

void RenderDroid::Model3Module::calcRadii(RdModel3 *pModel3) {
	for (uint i = 0; i < pModel3->aGeo[0].numMeshes; i++) {
		RdModel3Mesh *pMesh = &pModel3->aGeo[0].aMeshes[i];
		double maxLength = 0.0;

		for (uint j = 0; j < pMesh->numVertices; j++) {
			float *pVertex = pMesh->aVertices[j];
			double length = sqrt((double)pVertex[1] * pVertex[1] + (double)pVertex[2] * pVertex[2] + (double)pVertex[0] * pVertex[0]);

			if (length > maxLength) {
				float stored = (float)length;
				maxLength = stored;
			}
		}

		pMesh->radius = (float)(maxLength * 0.1 + maxLength);
	}

	_maxGeoRadius = 0.0f;
	buildExpandedRadius(pModel3, pModel3->aHNodes, _rd->_identMatrix34);

	// The demo replaces the radius of the model with the expanded one
	if (_rd->isDemo())
		pModel3->radius = (float)((double)_maxGeoRadius * 0.1 + _maxGeoRadius);
}

void RenderDroid::Model3Module::calcNodeDepths(RdModel3 *pModel3) {
	for (uint i = 0; i < pModel3->numHNodes; i++) {
		RdModel3HNode *pHNode = &pModel3->aHNodes[i];

		pHNode->depth = 0;
		for (RdModel3HNode *pParent = pHNode->pParentNode; pParent; pParent = pParent->pParentNode)
			pHNode->depth++;
	}
}

void RenderDroid::Model3Module::calcFaceNormals(RdModel3 *pModel3) {
	for (uint i = 0; i < pModel3->numGeos; i++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[i];

		for (uint j = 0; j < pGeo->numMeshes; j++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[j];

			for (uint k = 0; k < pMesh->numFaces; k++) {
				RdFace3 *pFace = &pMesh->aFaces[k];
				int numVertices = pFace->numVertices;
				int vertex = 0;
				uint previous = 0;
				uint next = 0;
				int following = 0;

				if (numVertices > 0) {
					do {
						previous = vertex - 1;
						if (vertex - 1 < 0)
							previous = numVertices - 1;

						following = vertex + 1;
						next = following % numVertices;

						if (!_rd->pointsCollinear(pMesh->aVertices[pFace->aVertices[vertex]], pMesh->aVertices[pFace->aVertices[next]], pMesh->aVertices[pFace->aVertices[previous]]))
							break;

						numVertices = pFace->numVertices;
						vertex++;
					} while (following < (int)pFace->numVertices);
				}

				if (vertex < (int)pFace->numVertices)
					_rd->calcSurfaceNormal(pFace->normal, pMesh->aVertices[pFace->aVertices[vertex]], pMesh->aVertices[pFace->aVertices[next]], pMesh->aVertices[pFace->aVertices[previous]]);
			}
		}
	}
}

void RenderDroid::Model3Module::calcVertexNormals(RdModel3 *pModel3) {
	for (uint i = 0; i < pModel3->numGeos; i++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[i];

		for (uint j = 0; j < pGeo->numMeshes; j++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[j];
			float *pNormal = pMesh->aVertexNormals[0];

			for (uint vertex = 0; vertex < pMesh->numVertices; vertex++) {
				double sumX = 0.0;
				double sumY = 0.0;
				double sumZ = 0.0;
				double count = 0.0;
				float countStored = 0.0f;
				float sumYStored = 0.0f;
				float sumZStored = 0.0f;

				if (pMesh->numFaces) {
					for (uint k = 0; k < pMesh->numFaces; k++) {
						RdFace3 *pFace = &pMesh->aFaces[k];

						for (uint m = 0; m < pFace->numVertices; m++) {
							if ((uint)pFace->aVertices[m] == vertex) {
								sumX = sumX + pFace->normal[0];
								sumY = sumY + pFace->normal[1];
								sumZ = sumZ + pFace->normal[2];
								count = count - -1.0;
								break;
							}
						}
					}

					countStored = (float)count;
					sumZStored = (float)sumZ;
					sumYStored = (float)sumY;
				}

				if (countStored == 0.0) {
					pNormal[0] = 1.0f;
					pNormal[1] = 0.0f;
					pNormal[2] = 0.0f;
				} else {
					if (countStored == 1.0) {
						RdFace3 *pFirstFace = &pMesh->aFaces[0];
						uint numFaceVertices = pFirstFace->numVertices;
						float *pPrevious = pMesh->aVertices[pFirstFace->aVertices[numFaceVertices - 1]];
						float *pNext = pMesh->aVertices[pFirstFace->aVertices[1 % numFaceVertices]];
						float *pFirst = pMesh->aVertices[0];
						float dyPrevious = (float)((double)pFirst[1] - pPrevious[1]);
						double dzNext = (double)pFirst[2] - pNext[2];
						double dxNext = (double)pFirst[0] - pNext[0];
						float dzPrevious = (float)((double)pFirst[2] - pPrevious[2]);
						double dxPrevious = (double)pFirst[0] - pPrevious[0];

						pNormal[1] = (float)(((double)pFirst[1] - pNext[1]) + dyPrevious);
						pNormal[2] = (float)(dzNext + dzPrevious);
						pNormal[0] = (float)(dxNext + dxPrevious);
					} else {
						pNormal[0] = (float)(sumX / countStored);
						pNormal[1] = (float)((double)sumYStored / countStored);
						pNormal[2] = (float)((double)sumZStored / countStored);
					}

					_rd->normalize3Acc(pNormal);
				}

				pNormal += 3;
			}
		}
	}
}

int RenderDroid::Model3Module::getMeshMatrix(RdThing *pThing, RdMatrix34 orient, uint nodeNum, RdMatrix34 meshOrient) {
	if (nodeNum >= pThing->data.pModel3->numHNodes)
		return 0;

	memcpy(meshOrient, pThing->aJointMatrices[nodeNum], sizeof(RdMatrix34));

	return 1;
}

void RenderDroid::Model3Module::buildExpandedRadius(RdModel3 *pModel3, RdModel3HNode *pHNode, RdMatrix34 parentMatrix) {
	RdMatrix34 nodeMatrix;
	RdMatrix34 translation;
	RdMatrix34 accumulated;

	_rd->build34(nodeMatrix, pHNode->pyr, pHNode->xyz);
	_rd->buildTranslate34(translation, pHNode->pivot);
	_rd->postMultiply34(translation, nodeMatrix);

	if (pHNode->pParentNode) {
		RdVector3 negatedPivot;
		negatedPivot[0] = -pHNode->pParentNode->pivot[0];
		negatedPivot[1] = -pHNode->pParentNode->pivot[1];
		negatedPivot[2] = -pHNode->pParentNode->pivot[2];
		_rd->postTranslate34(translation, negatedPivot);
	}

	_rd->multiply34(accumulated, parentMatrix, translation);

	if (pHNode->meshNodeIndex != -1) {
		RdModel3Mesh *pMesh = &pModel3->aGeo[0].aMeshes[pHNode->meshNodeIndex];

		for (uint i = 0; i < pMesh->numVertices; i++) {
			RdVector3 transformed;

			_rd->transformPoint34(transformed, pMesh->aVertices[i], accumulated);

			double length = sqrt((double)transformed[0] * transformed[0] + (double)transformed[1] * transformed[1] + (double)transformed[2] * transformed[2]);
			if (length > _maxGeoRadius)
				_maxGeoRadius = (float)length;
		}
	}

	if (pHNode->numChildren) {
		RdModel3HNode *pChild = pHNode->pFirstChildNode;

		for (uint i = 0; i < pHNode->numChildren; i++) {
			buildExpandedRadius(pModel3, pChild, accumulated);
			pChild = pChild->pSiblingNode;
		}
	}
}

void RenderDroid::Model3Module::calcModelBoundingBox(RdModel3 *pModel3, RdModel3HNode *pHNode, RdMatrix34 parentMatrix) {
	RdMatrix34 nodeMatrix;
	RdMatrix34 translation;
	RdMatrix34 accumulated;

	_rd->build34(nodeMatrix, pHNode->pyr, pHNode->xyz);
	_rd->buildTranslate34(translation, pHNode->pivot);
	_rd->postMultiply34(translation, nodeMatrix);

	if (pHNode->pParentNode) {
		RdVector3 negatedPivot;
		negatedPivot[0] = -pHNode->pParentNode->pivot[0];
		negatedPivot[1] = -pHNode->pParentNode->pivot[1];
		negatedPivot[2] = -pHNode->pParentNode->pivot[2];
		_rd->postTranslate34(translation, negatedPivot);
	}

	_rd->multiply34(accumulated, parentMatrix, translation);

	if (pHNode->meshNodeIndex != -1) {
		RdModel3Mesh *pMesh = &pModel3->aGeo[0].aMeshes[pHNode->meshNodeIndex];

		for (uint i = 0; i < pMesh->numVertices; i++) {
			RdVector3 transformed;

			_rd->transformPoint34(transformed, pMesh->aVertices[i], accumulated);

			if (transformed[0] > (double)pModel3->bound.max[0])
				pModel3->bound.max[0] = transformed[0];
			if (transformed[1] > (double)pModel3->bound.max[1])
				pModel3->bound.max[1] = transformed[1];
			if (transformed[2] > (double)pModel3->bound.max[2])
				pModel3->bound.max[2] = transformed[2];
			if (transformed[0] < (double)pModel3->bound.min[0])
				pModel3->bound.min[0] = transformed[0];
			if (transformed[1] < (double)pModel3->bound.min[1])
				pModel3->bound.min[1] = transformed[1];
			if (transformed[2] < (double)pModel3->bound.min[2])
				pModel3->bound.min[2] = transformed[2];
		}
	}

	if (pHNode->numChildren) {
		RdModel3HNode *pChild = pHNode->pFirstChildNode;

		for (uint i = 0; i < pHNode->numChildren; i++) {
			calcModelBoundingBox(pModel3, pChild, accumulated);
			pChild = pChild->pSiblingNode;
		}
	}
}

int RenderDroid::Model3Module::hasBytes(Common::SeekableReadStream *pFile, uint count, uint elementSize) {
	return (uint64)count * elementSize <= (uint64)(pFile->size() - pFile->pos());
}

RdModel3 *RenderDroid::Model3Module::load(char *pFilename) {
	RdModel3 *pModel3 = new RdModel3;
	memset(pModel3, 0, sizeof(RdModel3));

	if (loadEntry(pFilename, pModel3)) {
		loadPostProcess(pModel3);
		return pModel3;
	}

	destroy(pModel3);
	return nullptr;
}

int RenderDroid::Model3Module::loadEntry(char *pFilename, RdModel3 *pModel3) {
	Common::SeekableReadStream *pFile = _rd->_pHS->openFile(pFilename);

	if (!pFile)
		return 0;

	uint32 magic = pFile->readUint32LE();

	if (magic == 0x4D4F444C) {
		int result = loadEntryBinary(pFile, pModel3);
		delete pFile;
		return result;
	}

	delete pFile;
	return loadEntryText(pFilename, pModel3);
}

int RenderDroid::Model3Module::loadEntryBinary(Common::SeekableReadStream *pFile, RdModel3 *pModel3) {
	Common::Array<int> faceMaterialIndices;
	char *aMaterialNames = nullptr;
	bool ok = true;

	newEntry(pModel3);

	uint numMaterials = pFile->readUint32LE();

	if (!hasBytes(pFile, numMaterials, 32)) {
		warning("RenderDroid::Model3Module::loadEntryBinary(): invalid material count");
		return 0;
	}

	pModel3->numMaterials = numMaterials;
	pModel3->apMaterials = nullptr;

	if (numMaterials) {
		aMaterialNames = new char[32 * numMaterials];
		pFile->read(aMaterialNames, 32 * numMaterials);
	}

	pFile->read(pModel3->aName, 32);
	pModel3->aName[31] = 0;
	pModel3->index = pFile->readSint32LE();
	uint numGeos = pFile->readUint32LE();

	if (numGeos > 4) {
		warning("RenderDroid::Model3Module::loadEntryBinary(): invalid geoset count");
		ok = false;
		numGeos = 0;
	}

	pModel3->numGeos = numGeos;

	for (uint g = 0; g < numGeos && ok; g++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[g];
		uint numMeshes = pFile->readUint32LE();

		if (!hasBytes(pFile, numMeshes, 60)) {
			warning("RenderDroid::Model3Module::loadEntryBinary(): invalid mesh count");
			ok = false;
			break;
		}

		pGeo->aMeshes = new RdModel3Mesh[numMeshes];
		memset(pGeo->aMeshes, 0, sizeof(RdModel3Mesh) * numMeshes);
		pGeo->numMeshes = numMeshes;

		for (uint m = 0; m < numMeshes && ok; m++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[m];

			pFile->read(pMesh->aName, 32);
			pMesh->aName[31] = 0;
			pMesh->num = pFile->readUint32LE();
			pMesh->geometryMode = pFile->readSint32LE();
			pMesh->lightingMode = pFile->readSint32LE();
			pMesh->textureMode = pFile->readSint32LE();
			uint numVertices = pFile->readUint32LE();
			uint numTexVertices = pFile->readUint32LE();
			uint numFaces = pFile->readUint32LE();

			if (!hasBytes(pFile, numVertices, 32) || !hasBytes(pFile, numTexVertices, 8) || !hasBytes(pFile, numFaces, 76)) {
				warning("RenderDroid::Model3Module::loadEntryBinary(): invalid mesh size");
				ok = false;
				break;
			}

			pMesh->numVertices = numVertices;
			pMesh->numTexVertices = numTexVertices;

			if (numVertices) {
				pMesh->aVertices = new RdVector3[numVertices];
				pMesh->aStaticVertexIntensities = new float[numVertices];
				pMesh->aDynamicVertexIntensities = new float[numVertices];
			}

			if (numTexVertices)
				pMesh->aTexVertices = new RdVector2[numTexVertices];

			for (uint i = 0; i < numVertices; i++) {
				pMesh->aVertices[i][0] = pFile->readFloatLE();
				pMesh->aVertices[i][1] = pFile->readFloatLE();
				pMesh->aVertices[i][2] = pFile->readFloatLE();
			}

			for (uint i = 0; i < numTexVertices; i++) {
				pMesh->aTexVertices[i][0] = pFile->readFloatLE();
				pMesh->aTexVertices[i][1] = pFile->readFloatLE();
			}

			for (uint i = 0; i < numVertices; i++)
				pMesh->aStaticVertexIntensities[i] = pFile->readFloatLE();

			for (uint i = 0; i < numVertices; i++)
				pMesh->aDynamicVertexIntensities[i] = pFile->readFloatLE();

			if (numFaces) {
				pMesh->aFaces = new RdFace3[numFaces];
				memset(pMesh->aFaces, 0, sizeof(RdFace3) * numFaces);
				pMesh->numFaces = numFaces;
			}

			for (uint f = 0; f < numFaces && ok; f++) {
				RdFace3 *pFace = &pMesh->aFaces[f];

				pFace->num = pFile->readUint32LE();
				pFace->type = pFile->readUint32LE();
				pFace->geometryMode = pFile->readSint32LE();
				pFace->lightingMode = pFile->readSint32LE();
				pFace->textureMode = pFile->readSint32LE();
				uint faceVertices = pFile->readUint32LE();
				pFile->skip(4);
				uint hasTexVertices = pFile->readUint32LE();
				uint hasMaterial = pFile->readUint32LE();
				pFace->curCel = pFile->readSint32LE();
				pFace->uvDelta[0] = pFile->readFloatLE();
				pFace->uvDelta[1] = pFile->readFloatLE();
				pFace->extraLight = pFile->readFloatLE();
				pFace->reserved52 = pFile->readSint32LE();
				pFace->reserved56 = pFile->readSint32LE();
				pFace->reserved60 = pFile->readSint32LE();
				pFace->normal[0] = pFile->readFloatLE();
				pFace->normal[1] = pFile->readFloatLE();
				pFace->normal[2] = pFile->readFloatLE();

				if (!hasBytes(pFile, faceVertices, hasTexVertices ? 8 : 4)) {
					warning("RenderDroid::Model3Module::loadEntryBinary(): invalid face size");
					ok = false;
					break;
				}

				pFace->numVertices = faceVertices;

				if (faceVertices) {
					pFace->aVertices = new int[faceVertices];

					for (uint i = 0; i < faceVertices; i++)
						pFace->aVertices[i] = pFile->readSint32LE();
				}

				if (hasTexVertices) {
					pFace->aTexVertices = new int[faceVertices];

					for (uint i = 0; i < faceVertices; i++)
						pFace->aTexVertices[i] = pFile->readSint32LE();
				}

				if (hasMaterial)
					faceMaterialIndices.push_back(pFile->readSint32LE());
				else
					faceMaterialIndices.push_back(-1);
			}

			if (!ok)
				break;

			if (numVertices) {
				pMesh->aVertexNormals = new RdVector3[numVertices];

				for (uint i = 0; i < numVertices; i++) {
					pMesh->aVertexNormals[i][0] = pFile->readFloatLE();
					pMesh->aVertexNormals[i][1] = pFile->readFloatLE();
					pMesh->aVertexNormals[i][2] = pFile->readFloatLE();
				}
			}

			pMesh->castShadow = pFile->readSint32LE();
			pMesh->userLightValue = pFile->readFloatLE();
			pMesh->radius = pFile->readFloatLE();

			for (uint i = 0; i < 3; i++)
				pMesh->bound.min[i] = pFile->readFloatLE();

			for (uint i = 0; i < 3; i++)
				pMesh->bound.max[i] = pFile->readFloatLE();
		}
	}

	if (ok) {
		pModel3->curGeoSet = pFile->readUint32LE();
		uint numHNodes = pFile->readUint32LE();

		if (!hasBytes(pFile, numHNodes, 184)) {
			warning("RenderDroid::Model3Module::loadEntryBinary(): invalid hierarchy node count");
			ok = false;
		} else {
			pModel3->aHNodes = new RdModel3HNode[numHNodes];
			memset(pModel3->aHNodes, 0, sizeof(RdModel3HNode) * numHNodes);
			pModel3->numHNodes = numHNodes;
		}

		for (uint i = 0; i < pModel3->numHNodes && ok; i++) {
			RdModel3HNode *pHNode = &pModel3->aHNodes[i];

			pFile->read(pHNode->aName, 64);
			pHNode->aName[63] = 0;
			pHNode->flags = pFile->readUint32LE();
			pHNode->num = pFile->readUint32LE();
			pHNode->type = pFile->readUint32LE();
			pHNode->meshNodeIndex = pFile->readSint32LE();
			pHNode->depth = pFile->readUint32LE();
			uint hasParent = pFile->readUint32LE();
			pHNode->numChildren = pFile->readUint32LE();
			uint hasFirstChild = pFile->readUint32LE();
			uint hasSibling = pFile->readUint32LE();

			for (uint k = 0; k < 3; k++)
				pHNode->pivot[k] = pFile->readFloatLE();

			for (uint k = 0; k < 3; k++)
				pHNode->xyz[k] = pFile->readFloatLE();

			for (uint k = 0; k < 3; k++)
				pHNode->pyr[k] = pFile->readFloatLE();

			for (uint row = 0; row < 4; row++) {
				for (uint col = 0; col < 3; col++)
					pHNode->mat[row][col] = pFile->readFloatLE();
			}

			if (hasParent) {
				uint index = pFile->readUint32LE();

				if (index >= pModel3->numHNodes) {
					warning("RenderDroid::Model3Module::loadEntryBinary(): invalid parent node index");
					ok = false;
					break;
				}

				pHNode->pParentNode = &pModel3->aHNodes[index];
			}

			if (hasFirstChild) {
				uint index = pFile->readUint32LE();

				if (index >= pModel3->numHNodes) {
					warning("RenderDroid::Model3Module::loadEntryBinary(): invalid child node index");
					ok = false;
					break;
				}

				pHNode->pFirstChildNode = &pModel3->aHNodes[index];
			}

			if (hasSibling) {
				uint index = pFile->readUint32LE();

				if (index >= pModel3->numHNodes) {
					warning("RenderDroid::Model3Module::loadEntryBinary(): invalid sibling node index");
					ok = false;
					break;
				}

				pHNode->pSiblingNode = &pModel3->aHNodes[index];
			}
		}
	}

	if (ok) {
		pModel3->radius = pFile->readFloatLE();

		for (uint i = 0; i < 3; i++)
			pModel3->bound.min[i] = pFile->readFloatLE();

		for (uint i = 0; i < 3; i++)
			pModel3->bound.max[i] = pFile->readFloatLE();

		for (uint i = 0; i < 3; i++)
			pModel3->geomCenter[i] = pFile->readFloatLE();

		for (uint i = 0; i < 3; i++)
			pModel3->insertOffset[i] = pFile->readFloatLE();

		if (numMaterials) {
			pModel3->apMaterials = new RdMaterial *[numMaterials];
			memset(pModel3->apMaterials, 0, sizeof(RdMaterial *) * numMaterials);
		}

		for (uint i = 0; i < numMaterials; i++) {
			char materialName[33];
			memcpy(materialName, aMaterialNames + 32 * i, 32);
			materialName[32] = 0;
			pModel3->apMaterials[i] = _rd->_material.load(materialName, 0, 0);
		}

		uint faceIndex = 0;

		for (uint g = 0; g < pModel3->numGeos; g++) {
			RdModel3Geo *pGeo = &pModel3->aGeo[g];

			for (uint m = 0; m < pGeo->numMeshes; m++) {
				RdModel3Mesh *pMesh = &pGeo->aMeshes[m];

				for (uint f = 0; f < pMesh->numFaces; f++) {
					int materialIndex = faceMaterialIndices[faceIndex++];
					RdFace3 *pFace = &pMesh->aFaces[f];

					if (materialIndex >= 0 && (uint)materialIndex < numMaterials) {
						pFace->pMaterial = pModel3->apMaterials[materialIndex];
					} else {
						if (materialIndex >= 0)
							warning("RenderDroid::Model3Module::loadEntryBinary(): invalid face material index");

						pFace->pMaterial = nullptr;
					}
				}
			}
		}
	}

	delete[] aMaterialNames;

	return ok ? 1 : 0;
}

int RenderDroid::Model3Module::loadEntryText(char *pFilename, RdModel3 *pModel3) {
	StdConffile *conffile = _rd->_stdConffile;
	char section[1024];
	char name[1024];
	char faceLine[1024];
	bool success = false;

	newEntry(pModel3);
	Common::strlcpy(pModel3->aName, _rd->_stdFile->fromPath(pFilename), 32);

	if (!conffile->open(pFilename))
		return 0;

	do {
		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
			break;

		int versionMajor;
		int versionMinor;

		if (!conffile->readLine())
			break;

		sscanf(conffile->getLine(), " 3do %d.%d", &versionMajor, &versionMinor);

		uint numMaterials;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1 ||
				!conffile->readLine() || sscanf(conffile->getLine(), " materials %d", &numMaterials) != 1)
			break;

		if (numMaterials) {
			if (numMaterials >= 0x80) {
				warning("RenderDroid::Model3Module::loadEntryText(): too many materials in %s", pFilename);
				break;
			}

			pModel3->apMaterials = new RdMaterial *[numMaterials];
			memset(pModel3->apMaterials, 0, sizeof(RdMaterial *) * numMaterials);
			pModel3->numMaterials = numMaterials;

			char *aPaths = new char[1024 * numMaterials];
			memset(aPaths, 0, 1024 * numMaterials);
			bool pathsOk = true;

			for (uint i = 0; i < numMaterials; i++) {
				int index;

				if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %s", &index, name) != 2) {
					pathsOk = false;
					break;
				}

				if (index < 0 || (uint)index >= numMaterials) {
					warning("RenderDroid::Model3Module::loadEntryText(): invalid material index in %s", pFilename);
					pathsOk = false;
					break;
				}

				Common::strlcpy(&aPaths[1024 * index], name, 1024);
			}

			for (uint i = 0; i < numMaterials && pathsOk; i++) {
				pModel3->apMaterials[i] = _rd->_material.load(&aPaths[1024 * i], 0, 0);

				if (!pModel3->apMaterials[i])
					pathsOk = false;
			}

			delete[] aPaths;

			if (!pathsOk)
				break;
		}

		pModel3->numMaterials = numMaterials;

		float radius;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1 ||
				!conffile->readLine() || sscanf(conffile->getLine(), " radius %f", &radius) != 1)
			break;

		pModel3->radius = radius;

		float offsetX;
		float offsetY;
		float offsetZ;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " insert offset %f %f %f", &offsetX, &offsetY, &offsetZ) != 3)
			break;

		pModel3->insertOffset[0] = offsetX;
		pModel3->insertOffset[1] = offsetY;
		pModel3->insertOffset[2] = offsetZ;

		uint numGeos;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " geosets %d", &numGeos) != 1)
			break;

		if (numGeos > 4) {
			warning("RenderDroid::Model3Module::loadEntryText(): too many geosets in %s", pFilename);
			break;
		}

		pModel3->numGeos = numGeos;
		bool ok = true;

		for (uint g = 0; g < numGeos && ok; g++) {
			RdModel3Geo *pGeo = &pModel3->aGeo[g];
			int geosetIndex;
			uint numMeshes;

			if (!conffile->readLine() || sscanf(conffile->getLine(), " geoset %d", &geosetIndex) != 1 ||
					!conffile->readLine() || sscanf(conffile->getLine(), " meshes %d", &numMeshes) != 1) {
				ok = false;
				break;
			}

			pGeo->aMeshes = new RdModel3Mesh[numMeshes];
			memset(pGeo->aMeshes, 0, sizeof(RdModel3Mesh) * numMeshes);
			pGeo->numMeshes = numMeshes;

			for (uint m = 0; m < numMeshes && ok; m++) {
				RdModel3Mesh *pMesh = &pGeo->aMeshes[m];
				int meshIndex;
				int castShadow;
				uint numVertices;
				uint numTexVertices;
				uint numFaces;

				pMesh->num = m;

				if (!conffile->readLine() || sscanf(conffile->getLine(), " mesh %d", &meshIndex) != 1 ||
						!conffile->readLine() || sscanf(conffile->getLine(), " name %s", name) != 1) {
					ok = false;
					break;
				}

				Common::strlcpy(pMesh->aName, name, 32);

				if (!conffile->readLine() || sscanf(conffile->getLine(), " radius %f", &pMesh->radius) != 1 ||
						!conffile->readLine()) {
					ok = false;
					break;
				}

				if (sscanf(conffile->getLine(), " shadow %d", &castShadow) == 1) {
					pMesh->castShadow = castShadow;

					if (!conffile->readLine()) {
						ok = false;
						break;
					}
				}

				if (sscanf(conffile->getLine(), " geometrymode %d", &pMesh->geometryMode) != 1 ||
						!conffile->readLine() || sscanf(conffile->getLine(), " lightingmode %d", &pMesh->lightingMode) != 1 ||
						!conffile->readLine() || sscanf(conffile->getLine(), " texturemode %d", &pMesh->textureMode) != 1 ||
						!conffile->readLine() || sscanf(conffile->getLine(), " vertices %d", &numVertices) != 1 ||
						numVertices > 0x200) {
					ok = false;
					break;
				}

				if (numVertices) {
					pMesh->aVertices = new RdVector3[numVertices];
					pMesh->aStaticVertexIntensities = new float[numVertices];
					pMesh->aDynamicVertexIntensities = new float[numVertices];
					memset(pMesh->aDynamicVertexIntensities, 0, numVertices);
				}

				for (uint i = 0; i < numVertices; i++) {
					int index;
					float x;
					float y;
					float z;
					float intensity;

					if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %f %f %f", &index, &x, &y, &z, &intensity) != 5) {
						ok = false;
						break;
					}

					pMesh->aVertices[i][0] = x;
					pMesh->aVertices[i][1] = y;
					pMesh->aVertices[i][2] = z;
					pMesh->aStaticVertexIntensities[i] = intensity;
				}

				if (!ok)
					break;

				pMesh->numVertices = numVertices;

				if (!conffile->readLine() || sscanf(conffile->getLine(), " texture vertices %d", &numTexVertices) != 1 ||
						numTexVertices > 0x300) {
					ok = false;
					break;
				}

				if (numTexVertices)
					pMesh->aTexVertices = new RdVector2[numTexVertices];

				for (uint i = 0; i < numTexVertices; i++) {
					int index;
					float u;
					float v;

					if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %f", &index, &u, &v) != 3) {
						ok = false;
						break;
					}

					pMesh->aTexVertices[i][0] = u;
					pMesh->aTexVertices[i][1] = v;
				}

				if (!ok)
					break;

				pMesh->numTexVertices = numTexVertices;

				if (!conffile->readLine()) {
					ok = false;
					break;
				}

				if (numVertices)
					pMesh->aVertexNormals = new RdVector3[numVertices];

				for (uint i = 0; i < numVertices; i++) {
					int index;
					float x;
					float y;
					float z;

					if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %f %f", &index, &x, &y, &z) != 4) {
						ok = false;
						break;
					}

					pMesh->aVertexNormals[i][0] = x;
					pMesh->aVertexNormals[i][1] = y;
					pMesh->aVertexNormals[i][2] = z;
				}

				if (!ok)
					break;

				if (!conffile->readLine() || sscanf(conffile->getLine(), " faces %d", &numFaces) != 1 || numFaces > 0x200) {
					ok = false;
					break;
				}

				if (numFaces) {
					pMesh->aFaces = new RdFace3[numFaces];
					memset(pMesh->aFaces, 0, sizeof(RdFace3) * numFaces);
				}

				pMesh->numFaces = numFaces;

				for (uint f = 0; f < numFaces && ok; f++) {
					RdFace3 *pFace = &pMesh->aFaces[f];
					int materialIndex;
					int extraLightValid;
					float extraLight = 0.0f;
					char *token;

					if (!conffile->readLine()) {
						ok = false;
						break;
					}

					_rd->_face.newEntry(pFace);
					Common::strlcpy(faceLine, conffile->getLine(), 1024);

					strtok(faceLine, " \t");
					token = strtok(nullptr, " \t");

					if (!token) {
						ok = false;
						break;
					}

					materialIndex = atoi(token);
					pFace->num = f;

					if (materialIndex == -1) {
						pFace->pMaterial = nullptr;
					} else if (materialIndex < 0 || (uint)materialIndex >= pModel3->numMaterials) {
						warning("RenderDroid::Model3Module::loadEntryText(): invalid face material index in %s", pFilename);
						ok = false;
						break;
					} else {
						pFace->pMaterial = pModel3->apMaterials[materialIndex];
					}

					token = strtok(nullptr, " \t");
					if (!token || sscanf(token, "%x", &pFace->type) != 1) {
						ok = false;
						break;
					}

					token = strtok(nullptr, " \t");
					if (!token || sscanf(token, "%d", &pFace->geometryMode) != 1) {
						ok = false;
						break;
					}

					token = strtok(nullptr, " \t");
					if (!token || sscanf(token, "%d", &pFace->lightingMode) != 1) {
						ok = false;
						break;
					}

					token = strtok(nullptr, " \t");
					if (!token || sscanf(token, "%d", &pFace->textureMode) != 1) {
						ok = false;
						break;
					}

					token = strtok(nullptr, " \t");
					extraLightValid = token ? sscanf(token, "%f", &extraLight) : 0;
					if (extraLightValid != 1) {
						ok = false;
						break;
					}

					pFace->extraLight = extraLight;

					token = strtok(nullptr, " \t");
					if (!token) {
						ok = false;
						break;
					}

					uint faceVertices = atoi(token);
					if (faceVertices == 0 || faceVertices > 0x18) {
						ok = false;
						break;
					}

					pFace->numVertices = faceVertices;
					pFace->aVertices = new int[faceVertices];

					if (pFace->pMaterial && (pFace->pMaterial->flags & 2)) {
						pFace->aTexVertices = new int[faceVertices];

						for (uint i = 0; i < faceVertices; i++) {
							token = strtok(nullptr, " \t,");
							if (!token) {
								ok = false;
								break;
							}
							pFace->aVertices[i] = atoi(token);

							token = strtok(nullptr, " \t,");
							if (!token) {
								ok = false;
								break;
							}
							pFace->aTexVertices[i] = atoi(token);
						}
					} else {
						for (uint i = 0; i < faceVertices; i++) {
							token = strtok(nullptr, " \t,");
							if (!token) {
								ok = false;
								break;
							}
							pFace->aVertices[i] = atoi(token);

							if (!strtok(nullptr, " \t,")) {
								ok = false;
								break;
							}
						}
					}
				}

				if (!ok)
					break;

				if (!conffile->readLine()) {
					ok = false;
					break;
				}

				for (uint f = 0; f < pMesh->numFaces; f++) {
					int index;
					float x;
					float y;
					float z;

					if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %f %f", &index, &x, &y, &z) != 4) {
						ok = false;
						break;
					}

					pMesh->aFaces[f].normal[0] = x;
					pMesh->aFaces[f].normal[1] = y;
					pMesh->aFaces[f].normal[2] = z;
				}
			}
		}

		if (!ok)
			break;

		uint numHNodes;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1 ||
				!conffile->readLine() || sscanf(conffile->getLine(), " hierarchy nodes %d", &numHNodes) != 1)
			break;

		pModel3->aHNodes = new RdModel3HNode[numHNodes];
		memset(pModel3->aHNodes, 0, sizeof(RdModel3HNode) * numHNodes);
		pModel3->numHNodes = numHNodes;

		for (uint i = 0; i < numHNodes && ok; i++) {
			RdModel3HNode *pHNode = &pModel3->aHNodes[i];
			int index;
			int parentIndex;
			int firstChildIndex;
			int siblingIndex;
			float x;
			float y;
			float z;
			float pitch;
			float yaw;
			float roll;
			float pivotX;
			float pivotY;
			float pivotZ;

			pHNode->num = i;

			if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %x %x %d %d %d %d %d %f %f %f %f %f %f %f %f %f %63s",
					&index, &pHNode->flags, &pHNode->type, &pHNode->meshNodeIndex, &parentIndex, &firstChildIndex,
					&siblingIndex, &pHNode->numChildren, &x, &y, &z, &pitch, &yaw, &roll, &pivotX, &pivotY, &pivotZ,
					pHNode->aName) != 18) {
				ok = false;
				break;
			}

			if (parentIndex < -1 || parentIndex >= (int)numHNodes || firstChildIndex < -1 || firstChildIndex >= (int)numHNodes ||
					siblingIndex < -1 || siblingIndex >= (int)numHNodes) {
				warning("RenderDroid::Model3Module::loadEntryText(): invalid node index in %s", pFilename);
				ok = false;
				break;
			}

			pHNode->pParentNode = parentIndex == -1 ? nullptr : &pModel3->aHNodes[parentIndex];
			pHNode->pFirstChildNode = firstChildIndex == -1 ? nullptr : &pModel3->aHNodes[firstChildIndex];
			pHNode->pSiblingNode = siblingIndex == -1 ? nullptr : &pModel3->aHNodes[siblingIndex];

			pHNode->xyz[0] = x;
			pHNode->xyz[1] = y;
			pHNode->xyz[2] = z;
			pHNode->pyr[0] = pitch;
			pHNode->pyr[1] = yaw;
			pHNode->pyr[2] = roll;
			pHNode->pivot[0] = pivotX;
			pHNode->pivot[1] = pivotY;
			pHNode->pivot[2] = pivotZ;
		}

		if (!ok)
			break;

		calcNodeDepths(pModel3);
		success = true;
	} while (false);

	conffile->close();

	return success ? 1 : 0;
}

void RenderDroid::Model3Module::loadPostProcess(RdModel3 *pModel3) {
	calcBoundingBoxes(pModel3);
	calcRadii(pModel3);
	calcFaceNormals(pModel3);
	calcVertexNormals(pModel3);
	calcNodeDepths(pModel3);
	buildShadowModel(pModel3);

	if (_rd->isDemo() && !validate(pModel3))
		error("RenderDroid::Model3Module::loadPostProcess(): rdModel3_Validate(pModel3)");
}

int RenderDroid::Model3Module::validate(RdModel3 *pModel3) {
	for (uint g = 0; g < pModel3->numGeos; g++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[g];

		for (uint m = 0; m < pGeo->numMeshes; m++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[m];

			for (uint f = 0; f < pMesh->numFaces; f++) {
				RdFace3 *pFace = &pMesh->aFaces[f];

				if (pFace->pMaterial) {
					if (!(pFace->pMaterial->flags & 2) && pFace->geometryMode > 3)
						return 0;
				} else if (pFace->geometryMode > 0) {
					return 0;
				}
			}
		}
	}

	return 1;
}

void RenderDroid::Model3Module::destroy(RdModel3 *&pModel3) {
	if (pModel3) {
		freeEntry(pModel3);
		delete pModel3;
		pModel3 = nullptr;
	} else {
		warning("RenderDroid::Model3Module::destroy(): attempt to free NULL model ptr");
	}
}

void RenderDroid::Model3Module::freeEntry(RdModel3 *pModel3) {
	for (uint g = 0; g < pModel3->numGeos; g++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[g];

		for (uint m = 0; m < pGeo->numMeshes; m++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[m];

			delete[] pMesh->aVertices;
			pMesh->aVertices = nullptr;

			delete[] pMesh->aTexVertices;
			pMesh->aTexVertices = nullptr;

			if (pMesh->aFaces) {
				for (uint f = 0; f < pMesh->numFaces; f++)
					_rd->_face.freeEntry(&pMesh->aFaces[f]);

				delete[] pMesh->aFaces;
				pMesh->aFaces = nullptr;
			}

			delete[] pMesh->aStaticVertexIntensities;
			pMesh->aStaticVertexIntensities = nullptr;

			delete[] pMesh->aDynamicVertexIntensities;
			pMesh->aDynamicVertexIntensities = nullptr;

			delete[] pMesh->aVertexNormals;
			pMesh->aVertexNormals = nullptr;

			delete[] pMesh->aShadowVerts;
			pMesh->aShadowVerts = nullptr;
		}

		delete[] pGeo->aMeshes;
		pGeo->aMeshes = nullptr;
	}

	delete[] pModel3->aHNodes;
	pModel3->aHNodes = nullptr;

	if (pModel3->apMaterials) {
		for (uint i = 0; i < pModel3->numMaterials; i++) {
			if (pModel3->apMaterials[i])
				_rd->_material.destroy(pModel3->apMaterials[i]);
		}

		delete[] pModel3->apMaterials;
		pModel3->apMaterials = nullptr;
	}
}

int RenderDroid::Model3Module::draw(RdThing *pThing, RdMatrix34 mat) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 center;

	_pCurThing = pThing;
	_pCurModel3 = pThing->data.pModel3;
	if (_rd->isDemo()) {
		// The demo tests the origin of the model, not its geometric center
		_rd->transformPoint34(center, mat[3], pCamera->viewMatrix);
	} else {
		_rd->transformPoint34(center, _pCurModel3->geomCenter, mat);
		_rd->transformPoint34Acc(center, pCamera->viewMatrix);
	}

	// The demo uses the culling flags of the renderer, not the field of the thing
	bool bCull = _rd->isDemo() ? (_rd->_curCullFlags & 2) != 0 : pThing->frustrumCull != 0;

	if (bCull)
		_thingFrustrumCull = _rd->_clip.sphereInFrustrum(pCamera->pClipFrustum, center, _pCurModel3->radius);
	else
		_thingFrustrumCull = 1;

	if (_thingFrustrumCull == 2)
		return 0;

	_thingFrustrumCull = 1;

	uint curGeoSet;

	if (pThing->curGeoSet == -1)
		curGeoSet = _pCurModel3->curGeoSet;
	else
		curGeoSet = _pCurThing->curGeoSet;

	if (curGeoSet >= 4) {
		warning("RenderDroid::Model3Module::draw(): invalid geoset %u", curGeoSet);
		return 0;
	}

	_pCurGeo = &_pCurModel3->aGeo[curGeoSet];

	// The demo builds the joint matrices once per frame
	if (!_rd->isDemo() || _pCurThing->jointsFrame != _rd->getFrameTrue())
		_rd->_puppet.buildJointMatrices(_pCurThing, mat);

	_curGeometryMode = _pCurThing->curGeometryMode;

	if (_curGeometryMode >= _rd->_curGeometryMode)
		_curGeometryMode = _rd->_curGeometryMode;

	if ((_rd->_curRenderOptions & 2) && pCamera->ambientLight >= 1.0f) {
		_curLightingMode = 0;
	} else {
		_curLightingMode = _pCurThing->curLightingMode;

		if (_curLightingMode >= _rd->_curLightingMode)
			_curLightingMode = _rd->_curLightingMode;
	}

	_curTextureMode = _pCurThing->curTextureMode;

	if (_curTextureMode >= _rd->_curTextureMode)
		_curTextureMode = _rd->_curTextureMode;

	if (_curLightingMode > 1) {
		_numGeoLights = 0;

		for (uint i = 0; i < pCamera->numLights; i++) {
			RdLight *pLight = pCamera->apLights[i];

			if (!pLight->bActive)
				continue;

			double distSquared;

			if (pLight->type >= 2) {
				double dy = (double)mat[3][1] - pCamera->aLightPos[i][1];
				double dz = (double)mat[3][2] - pCamera->aLightPos[i][2];
				double dx = (double)mat[3][0] - pCamera->aLightPos[i][0];
				distSquared = dx * dx + dy * dy + dz * dz;
			} else {
				distSquared = 0.0;
			}

			double range = (double)pLight->falloffMin + _pCurModel3->radius;

			if (distSquared < range * range) {
				_apGeoLights[_numGeoLights] = pLight;
				_numGeoLights++;
			}
		}
	}

	drawHNode(_pCurModel3->aHNodes);
	_numDrawnModels++;

	return 1;
}

void RenderDroid::Model3Module::drawHNode(RdModel3HNode *pHNode) {
	int meshNodeIndex = pHNode->meshNodeIndex;

	if (meshNodeIndex != -1 && _pCurThing->abMeshVisible[pHNode->num]) {
		if (pHNode->flags & 2) {
			RdModel3HNode *pAncestor = pHNode->pParentNode;

			while (pAncestor->flags & 2)
				pAncestor = pAncestor->pParentNode;

			_pCurGeo->aMeshes[meshNodeIndex].lightingMode = 6;
			_pCurGeo->aMeshes[pHNode->meshNodeIndex].userLightValue = _pCurGeo->aMeshes[pAncestor->meshNodeIndex].userLightValue;
		}

		drawMesh(&_pCurGeo->aMeshes[pHNode->meshNodeIndex], _pCurThing->aJointMatrices[pHNode->num]);
	}

	if (pHNode->numChildren) {
		RdModel3HNode *pChild = pHNode->pFirstChildNode;

		for (uint i = 0; i < pHNode->numChildren; i++) {
			if (!_pCurThing->abJointAmputate[pChild->num])
				drawHNode(pChild);

			pChild = pChild->pSiblingNode;
		}
	}
}

void RenderDroid::Model3Module::drawMesh(RdModel3Mesh *pMesh, RdMatrix34 mat) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 center;
	RdMatrix34 modelView;
	RdMatrix34 inverse;

	_pCurMesh = pMesh;

	if (!pMesh->geometryMode)
		return;

	if (pMesh->numVertices > 512) {
		warning("RenderDroid::Model3Module::drawMesh(): too many vertices in mesh %s", pMesh->aName);
		return;
	}

	_rd->transformPoint34(center, mat[3], pCamera->viewMatrix);

	if (_thingFrustrumCull)
		_meshFrustrumCull = (_rd->_curCullFlags & 1) ? _rd->_clip.sphereInFrustrum(pCamera->pClipFrustum, center, _pCurMesh->radius) : 1;
	else
		_meshFrustrumCull = 0;

	if (_meshFrustrumCull == 2)
		return;

	if (_clipPlaneActive) {
		double side = ((double)mat[3][1] - _clipPt[1]) * _clipNorm[1] + ((double)mat[3][2] - _clipPt[2]) * _clipNorm[2] + ((double)mat[3][0] - _clipPt[0]) * _clipNorm[0];

		if (!(-_pCurMesh->radius <= side))
			return;
	}

	_rd->multiply34(modelView, pCamera->viewMatrix, mat);
	_rd->transformPointLst34(modelView, _pCurMesh->aVertices, _aView, _pCurMesh->numVertices);
	_rd->invertOrtho34(inverse, mat);

	_geometryMode = _pCurMesh->geometryMode;

	if (_geometryMode >= _curGeometryMode)
		_geometryMode = _curGeometryMode;

	_textureMode = _pCurMesh->textureMode;

	if (_textureMode >= _curTextureMode)
		_textureMode = _curTextureMode;

	int lightingMode = _pCurMesh->lightingMode;

	if (lightingMode != 6 && lightingMode >= _curLightingMode)
		lightingMode = _curLightingMode;

	_lightingMode = lightingMode;

	_vertexSrc.aVertices = _aView;
	_vertexSrc.aTexVertices = _pCurMesh->aTexVertices;
	_vertexSrc.aIntensities = _pCurMesh->aDynamicVertexIntensities;
	_vertexSrc.aAddedIntensities = nullptr;
	_vertexDst.aVertices = _aFaceVerts;

	if (lightingMode == 2 || lightingMode == 3) {
		_numMeshLights = 0;

		for (uint i = 0; i < _numGeoLights; i++) {
			RdLight *pLight = _apGeoLights[i];

			if (!pLight->bActive)
				continue;

			const float *lightPos = pCamera->aLightPos[pLight->id];
			bool bInRange;

			if (lightingMode == 2) {
				double dx = (double)lightPos[0] - mat[3][0];
				double dy = (double)lightPos[1] - mat[3][1];
				float dyStored = (float)dy;
				float dzStored = (float)((double)lightPos[2] - mat[3][2]);
				double distance = sqrt(dx * dx + dy * dyStored + (double)dzStored * dzStored);
				double range = (double)pLight->falloffMin + _pCurMesh->radius;

				bInRange = range > distance;
			} else {
				double distSquared;

				if (pLight->type >= 2) {
					double dx = (double)lightPos[0] - mat[3][0];
					double dy = (double)lightPos[1] - mat[3][1];
					double dz = (double)lightPos[2] - mat[3][2];
					distSquared = dx * dx + dy * dy + dz * dz;
				} else {
					distSquared = 0.0;
				}

				double range = (double)pLight->falloffMin + _pCurMesh->radius;

				bInRange = distSquared < range * range;
			}

			if (!bInRange)
				continue;

			_apMeshLights[_numMeshLights] = pLight;
			_rd->transformPoint34(_aLocalLightPos[_numMeshLights], pCamera->aLightPos[pLight->id], inverse);

			if (pLight->type != 2) {
				float translation[3];
				RdVector3 direction;

				translation[0] = inverse[3][0];
				translation[1] = inverse[3][1];
				translation[2] = inverse[3][2];
				inverse[3][0] = 0.0f;
				inverse[3][1] = 0.0f;
				inverse[3][2] = 0.0f;
				direction[0] = -pLight->direction[0];
				direction[1] = -pLight->direction[1];
				direction[2] = -pLight->direction[2];
				_rd->transformPoint34(_aLocalLightDir[_numMeshLights], direction, inverse);
				inverse[3][0] = translation[0];
				inverse[3][1] = translation[1];
				inverse[3][2] = translation[2];
			}

			_numMeshLights++;
		}

		if (lightingMode == 3) {
			pMesh->userLightValue = _rd->_light.calcVertexIntensities(_apMeshLights, _aLocalLightPos, _aLocalLightDir, _numMeshLights, _pCurMesh->aVertexNormals, _pCurMesh->aVertices, _pCurMesh->aStaticVertexIntensities, _pCurMesh->aDynamicVertexIntensities, _pCurMesh->numVertices, pCamera->attenuationMin);
		}
	} else if (lightingMode == 6) {
		for (uint i = 0; i < pMesh->numFaces; i++)
			pMesh->aFaces[i].extraLight = pMesh->userLightValue;
	}

	RdVector3 cameraPosition;

	_rd->_camera.getPosition(cameraPosition);
	_rd->transformPoint34(_localCamera, cameraPosition, inverse);

	for (uint i = 0; i < pMesh->numFaces; i++) {
		RdFace3 *pFace = &pMesh->aFaces[i];
		const float *vertex = _pCurMesh->aVertices[pFace->aVertices[0]];
		double facing;

		// The demo adds the terms in z, y, x order
		if (_rd->isDemo())
			facing = ((double)_localCamera[2] - vertex[2]) * pFace->normal[2] + ((double)_localCamera[1] - vertex[1]) * pFace->normal[1] + ((double)_localCamera[0] - vertex[0]) * pFace->normal[0];
		else
			facing = ((double)_localCamera[2] - vertex[2]) * pFace->normal[2] + ((double)_localCamera[0] - vertex[0]) * pFace->normal[0] + ((double)_localCamera[1] - vertex[1]) * pFace->normal[1];

		int bBackface = 0;

		if (facing <= 0.0) {
			bBackface = 1;

			if (!(pFace->type & 1) && (_rd->_curRenderOptions & 1))
				continue;
		}

		drawFace(pFace, bBackface);
	}
}

int RenderDroid::Model3Module::drawFace(RdFace3 *pFace, int bBackface) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdProcFace *pProcFace = _rd->_cache.getProcEntry();

	if (!pProcFace)
		return 0;

	int geometryMode = pFace->geometryMode;

	if (_geometryMode < geometryMode)
		geometryMode = _geometryMode;

	pProcFace->geometryMode = geometryMode;

	int lightingMode = pFace->lightingMode;

	if (_lightingMode < lightingMode)
		lightingMode = _lightingMode;

	pProcFace->lightingMode = lightingMode;

	int textureMode = pFace->textureMode;

	if (_textureMode < textureMode)
		textureMode = _textureMode;

	pProcFace->textureMode = textureMode;

	if (pFace->type & 0x10)
		pProcFace->lightingMode = 1;

	_vertexDst.aScreen = pProcFace->aVertices;
	_vertexDst.aTexVertices = pProcFace->aTexVertices;
	_vertexDst.aIntensities = pProcFace->aIntensities;
	_vertexSrc.numVertices = pFace->numVertices;
	_vertexSrc.aVertIndices = pFace->aVertices;
	_vertexSrc.aTexIndices = pFace->aTexVertices;

	if (_meshFrustrumCull)
		_rd->_primit3.clipFace(pCamera->pClipFrustum, pProcFace->geometryMode, pProcFace->lightingMode, pProcFace->textureMode, &_vertexSrc, &_vertexDst, pFace->uvDelta);
	else
		_rd->_primit3.noClipFace(pProcFace->geometryMode, pProcFace->lightingMode, pProcFace->textureMode, &_vertexSrc, &_vertexDst, pFace->uvDelta);

	if (_vertexDst.numVertices < 3)
		return 0;

	if (_clipPlaneActive) {
		RdVector3 planeNormal;
		RdVector3 planePoint;

		_rd->transformVector34(planeNormal, _clipNorm, pCamera->viewMatrix);
		_rd->transformPoint34(planePoint, _clipPt, pCamera->viewMatrix);

		int numClipped = _rd->_clip.faceToPlane(planeNormal, planePoint, _vertexDst.aVertices, _vertexDst.aTexVertices, _vertexDst.aIntensities, _vertexDst.numVertices);

		if (numClipped < 2)
			return 0;

		_vertexDst.numVertices = numClipped;
		pProcFace->numVertices = numClipped;
	}

	if (pProcFace->lightingMode == 2) {
		RdVector3 normal;

		if (bBackface) {
			normal[0] = -pFace->normal[0];
			normal[1] = -pFace->normal[1];
			normal[2] = -pFace->normal[2];
		} else {
			normal[0] = pFace->normal[0];
			normal[1] = pFace->normal[1];
			normal[2] = pFace->normal[2];
		}

		pProcFace->diffuseLight = _rd->_light.calcFaceIntensity(_apMeshLights, _aLocalLightPos, _aLocalLightDir, _numMeshLights, pFace, normal, _pCurMesh->aVertices, pCamera->attenuationMin);
	}

	(_rd->_camera.*pCamera->pfProjectLst)(_vertexDst.aScreen, _vertexDst.aVertices, _vertexDst.numVertices);

	if (_rd->_curRenderOptions & 2)
		pProcFace->ambientLight = pCamera->ambientLight;
	else
		pProcFace->ambientLight = 0.0f;

	int curCel = pFace->curCel;

	if (curCel == -1 && pFace->pMaterial)
		curCel = pFace->pMaterial->curCel;

	pProcFace->curCel = curCel;

	RdColormap *pCurMap = _rd->_colormap.getCurrent();
	RdColormap *pIdentityMap = _rd->_colormap.getIdentity();

	if (pProcFace->ambientLight < 1.0f) {
		int procLightingMode = pProcFace->lightingMode;

		if (procLightingMode == 2) {
			if (pProcFace->diffuseLight >= 1.0f && pCurMap == pIdentityMap)
				pProcFace->lightingMode = 0;
			else if (pProcFace->diffuseLight <= 0.0f)
				pProcFace->lightingMode = 1;
		} else if (procLightingMode == 3) {
			float first = pProcFace->aIntensities[0];
			uint same = 1;

			while (same < _vertexDst.numVertices) {
				double difference = (double)pProcFace->aIntensities[same] - first;

				if (difference < 0.0)
					difference = -difference;

				if (difference > 0.015625)
					break;

				same++;
			}

			if (same == _vertexDst.numVertices) {
				if (first != 1.0f) {
					if (first == 0.0f) {
						pProcFace->lightingMode = 1;
						pProcFace->diffuseLight = 0.0f;
					} else {
						pProcFace->lightingMode = 2;
						pProcFace->diffuseLight = first;
					}
				} else if (pCurMap == pIdentityMap) {
					pProcFace->lightingMode = 0;
				} else {
					pProcFace->lightingMode = 2;
					pProcFace->diffuseLight = 1.0f;
				}
			}
		}
	} else if (pCurMap != pIdentityMap) {
		pProcFace->lightingMode = 2;
		pProcFace->diffuseLight = 1.0f;
	} else {
		pProcFace->lightingMode = 0;
	}

	char bufferFlags = 1;

	if (pProcFace->geometryMode >= 4)
		bufferFlags = 3;

	if (pProcFace->lightingMode >= 3)
		bufferFlags |= 4;

	pProcFace->bBackface = bBackface;
	pProcFace->type = pFace->type;
	pProcFace->extraLight = pFace->extraLight;
	pProcFace->pMaterial = pFace->pMaterial;
	_rd->_cache.addProcFace(0, _vertexDst.numVertices, bufferFlags);

	return 1;
}

int RenderDroid::Model3Module::drawPolyline(RdThing *pThing, RdMatrix34 mat) {
	RdPolyline *pPolyline = pThing->data.pPolyline;
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdMatrix34 modelView;
	RdVector3 localEnd;
	RdVector3 end;
	RdVertexLists src;

	memset(&src, 0, sizeof(RdVertexLists));
	src.aVertices = (RdVector3 *)_polylineVerts;

	_rd->multiply34(modelView, pCamera->viewMatrix, mat);

	localEnd[0] = 0.0f;
	localEnd[1] = pPolyline->length;
	localEnd[2] = 0.0f;
	_rd->transformPoint34(end, localEnd, modelView);

	float endY = (float)((double)end[1] - -0.001f);

	_polylineVerts[0] = (float)((double)end[0] - pPolyline->endWidth);
	_polylineVerts[1] = endY;
	_polylineVerts[2] = (float)((double)end[2] - pPolyline->endWidth);
	_polylineVerts[3] = (float)((double)end[0] + pPolyline->endWidth);
	_polylineVerts[4] = endY;
	_polylineVerts[5] = (float)((double)end[2] - pPolyline->endWidth);
	_polylineVerts[6] = (float)((double)end[0] + pPolyline->endWidth);
	_polylineVerts[7] = endY;
	_polylineVerts[8] = (float)((double)end[2] + pPolyline->endWidth);
	_polylineVerts[9] = (float)((double)end[0] - pPolyline->endWidth);
	_polylineVerts[10] = endY;
	_polylineVerts[11] = (float)((double)end[2] + pPolyline->endWidth);
	src.aTexVertices = pPolyline->aTexVertsEnd;
	_rd->_primit3.drawPolylineFace(pThing, &pPolyline->faceEnd, (RdVector3 *)_polylineVerts, &src);

	float startY = (float)((double)modelView[3][1] - -0.001f);

	_polylineVerts[0] = (float)((double)modelView[3][0] - pPolyline->startWidth);
	_polylineVerts[1] = startY;
	_polylineVerts[2] = (float)((double)modelView[3][2] - pPolyline->startWidth);
	_polylineVerts[3] = (float)((double)modelView[3][0] + pPolyline->startWidth);
	_polylineVerts[4] = startY;
	_polylineVerts[5] = (float)((double)modelView[3][2] - pPolyline->startWidth);
	_polylineVerts[6] = (float)((double)modelView[3][0] + pPolyline->startWidth);
	_polylineVerts[7] = startY;
	_polylineVerts[8] = (float)((double)modelView[3][2] + pPolyline->startWidth);
	_polylineVerts[9] = (float)((double)modelView[3][0] - pPolyline->startWidth);
	_polylineVerts[10] = startY;
	_polylineVerts[11] = (float)((double)modelView[3][2] + pPolyline->startWidth);
	src.aTexVertices = pPolyline->aTexVertsEnd;
	_rd->_primit3.drawPolylineFace(pThing, &pPolyline->faceEnd, (RdVector3 *)_polylineVerts, &src);

	float dx = (float)((double)end[0] - modelView[3][0]);
	double dz = (double)end[2] - modelView[3][2];
	float length = (float)sqrt(dz * dz + (double)dx * dx);
	float sine = (float)(-(double)dx / length);
	float angle = _rd->_stdMath->arcSin3(sine);

	if ((float)dz < 0.0f) {
		if (dx <= 0.0f)
			angle = (float)-((double)angle - -180.0);
		else
			angle = (float)(180.0 - angle);
	}

	float sinAngle;
	float cosAngle;

	_rd->_stdMath->sinCos(angle, &sinAngle, &cosAngle);

	float sinLength = (float)((double)sinAngle * length);
	float cosLength = (float)((double)cosAngle * length);
	double endWidth = pPolyline->endWidth;
	double startWidth = pPolyline->startWidth;

	_polylineVerts[1] = end[1];
	_polylineVerts[0] = (float)(endWidth * cosAngle - sinLength + modelView[3][0]);
	_polylineVerts[2] = (float)(endWidth * sinAngle + cosLength + modelView[3][2]);
	_polylineVerts[3] = (float)(-endWidth * cosAngle - sinLength + modelView[3][0]);
	_polylineVerts[4] = end[1];
	_polylineVerts[5] = (float)(-endWidth * sinAngle + cosLength + modelView[3][2]);
	_polylineVerts[6] = (float)(-startWidth * cosAngle - 0.0 + modelView[3][0]);
	_polylineVerts[7] = modelView[3][1];
	_polylineVerts[8] = (float)(-startWidth * sinAngle + 0.0 + modelView[3][2]);
	_polylineVerts[9] = (float)(startWidth * cosAngle - 0.0 + modelView[3][0]);
	_polylineVerts[10] = modelView[3][1];
	_polylineVerts[11] = (float)(startWidth * sinAngle + 0.0 + modelView[3][2]);
	src.aTexVertices = pPolyline->aTexVertsBody;
	_rd->_primit3.drawPolylineFace(pThing, &pPolyline->faceBody, (RdVector3 *)_polylineVerts, &src);

	return 1;
}

int RenderDroid::Model3Module::drawParticle(RdThing *pThing, RdMatrix34 mat) {
	RdParticleCloud *pParticle = pThing->data.pParticle;
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 center;
	RdMatrix34 modelView;
	int frustrumCull;

	if (pParticle->numParticles > 256) {
		warning("RenderDroid::Model3Module::drawParticle(): too many particles (%u)", pParticle->numParticles);
		return 0;
	}

	_rd->transformPoint34(center, mat[3], pCamera->viewMatrix);

	if (_rd->_curCullFlags & 2)
		frustrumCull = _rd->_clip.sphereInFrustrum(pCamera->pClipFrustum, center, pParticle->radius);
	else
		frustrumCull = pThing->frustrumCull;

	if (frustrumCull == 2)
		return 0;

	_rd->multiply34(modelView, pCamera->viewMatrix, mat);

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = pCamera->ambientLight;

	int lightingMode;

	if (ambientLight < 1.0f) {
		if (ambientLight > 0.0f)
			lightingMode = pParticle->lightingMode;
		else
			lightingMode = 1;
	} else {
		lightingMode = 0;
	}

	if (lightingMode >= pParticle->lightingMode)
		lightingMode = pParticle->lightingMode;

	_rd->transformPointLst34(modelView, pParticle->aPositions, _particleTransformed, pParticle->numParticles);

	for (uint i = 0; i < pParticle->numParticles; i++) {
		RdProcFace *pProcFace = _rd->_cache.getProcEntry();

		if (!pProcFace)
			return 0;

		const float *particle = _particleTransformed[i];
		double size = pParticle->size;

		_particleVerts[0] = (float)((double)particle[0] - size);
		_particleVerts[1] = particle[1];
		_particleVerts[2] = (float)((double)particle[2] - size);
		_particleVerts[3] = (float)((double)particle[0] + size);
		_particleVerts[4] = particle[1];
		_particleVerts[5] = (float)((double)particle[2] - size);
		_particleVerts[6] = (float)((double)particle[0] + size);
		_particleVerts[7] = particle[1];
		_particleVerts[8] = (float)((double)particle[2] + size);
		_particleVerts[9] = (float)((double)particle[0] - size);
		_particleVerts[10] = particle[1];
		_particleVerts[11] = (float)((double)particle[2] + size);

		int numClipped = _rd->_clip.face3S(pCamera->pClipFrustum, (RdVector3 *)_particleVerts, 4);

		if (numClipped >= 3) {
			(_rd->_camera.*pCamera->pfProjectLst)(pProcFace->aVertices, (RdVector3 *)_particleVerts, numClipped);
			pProcFace->lightingMode = lightingMode;
			pProcFace->ambientLight = ambientLight;
			pProcFace->geometryMode = 3;
			pProcFace->type = 0;
			pProcFace->pMaterial = pParticle->pMaterial;
			pProcFace->curCel = pParticle->aCels[i];
			pProcFace->bBackface = 0;
			_rd->_cache.addProcFace(0, numClipped, 1);
		}
	}

	return 1;
}

void RenderDroid::Model3Module::buildShadowModel(RdModel3 *pModel3) {
	static const byte aCorners[16][3] = {
		{1, 1, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1},
		{1, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0},
		{0, 0, 0}, {1, 1, 0}, {1, 1, 1}, {0, 0, 1},
		{0, 1, 0}, {1, 0, 0}, {1, 0, 1}, {0, 1, 1}
	};

	for (uint g = 0; g < pModel3->numGeos; g++) {
		RdModel3Geo *pGeo = &pModel3->aGeo[g];

		for (uint m = 0; m < pGeo->numMeshes; m++) {
			RdModel3Mesh *pMesh = &pGeo->aMeshes[m];

			if (!pMesh->castShadow)
				continue;

			delete[] pMesh->aShadowVerts;
			pMesh->aShadowVerts = new RdVector3[16];

			for (uint v = 0; v < 16; v++) {
				for (uint k = 0; k < 3; k++)
					pMesh->aShadowVerts[v][k] = aCorners[v][k] ? pMesh->bound.max[k] : pMesh->bound.min[k];
			}
		}
	}
}

} // End of namespace Grim
