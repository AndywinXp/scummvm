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

#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

int RenderDroid::Primit3Module::getScreenCoord(RdVector3 pt, int *pScreen) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 transformed;
	RdVector3 projected;

	_rd->transformPoint34(transformed, pt, pCamera->viewMatrix);
	if (!_rd->_clip.point3(pCamera->pClipFrustum, transformed))
		return 0;

	(_rd->_camera.*pCamera->pfProject)(projected, transformed);

	if (pScreen) {
		pScreen[0] = (int)projected[0];
		pScreen[1] = (int)projected[1];
		memcpy(&pScreen[2], &projected[2], sizeof(float));
	}

	return 1;
}

int RenderDroid::Primit3Module::drawLine(RdVector3 p0, RdVector3 p1, int *pScreen0, int *pScreen1, int *pClipped0, int *pClipped1, uint16 color, uint pattern) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 transformed0;
	RdVector3 transformed1;
	RdVector3 projected0;
	RdVector3 projected1;

	_rd->transformPoint34(transformed0, p0, pCamera->viewMatrix);
	_rd->transformPoint34(transformed1, p1, pCamera->viewMatrix);

	if (!_rd->_clip.line3(pCamera->pClipFrustum, transformed0, transformed1, pClipped0, pClipped1))
		return 0;

	(_rd->_camera.*pCamera->pfProject)(projected0, transformed0);
	(_rd->_camera.*pCamera->pfProject)(projected1, transformed1);

	int x0 = (int)projected0[0];
	int y0 = (int)projected0[1];

	if (pScreen0) {
		pScreen0[0] = x0;
		pScreen0[1] = y0;
		memcpy(&pScreen0[2], &projected0[2], sizeof(float));
	}

	int x1 = (int)projected1[0];
	int y1 = (int)projected1[1];

	if (pScreen1) {
		pScreen1[0] = x1;
		pScreen1[1] = y1;
		memcpy(&pScreen1[2], &projected1[2], sizeof(float));
	}

	_rd->_primit2.drawLine(pCamera->pCanvas, x0, y0, x1, y1, color, pattern);

	return 1;
}

void RenderDroid::Primit3Module::drawPoint(RdVector3 pt, float radius, float angleStep, uint16 color, uint pattern) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 transformed;
	RdVector3 shifted;
	RdVector3 projected;
	RdVector3 projectedShifted;

	_rd->transformPoint34(transformed, pt, pCamera->viewMatrix);
	shifted[1] = transformed[1];
	shifted[2] = transformed[2];
	shifted[0] = (float)((double)transformed[0] + radius);

	if (transformed[1] > 0.0) {
		(_rd->_camera.*pCamera->pfProject)(projected, transformed);
		(_rd->_camera.*pCamera->pfProject)(projectedShifted, shifted);

		float projectedRadius = (float)((double)projectedShifted[0] - projected[0]);

		_rd->_primit2.drawCircle(pCamera->pCanvas, (int)((double)projected[0] + 0.5), (int)((double)projected[1] + 0.5), projectedRadius, angleStep, color, pattern);
	}
}

int RenderDroid::Primit3Module::noClipFace(int geometryMode, int lightingMode, int textureMode, RdVertexLists *pSrc, RdVertexLists *pDst, RdVector2 uvDelta) {
	bool copyTexVertices = false;
	bool copyIntensities = false;

	switch (geometryMode) {
	case 0:
	case 1:
	case 2:
		break;
	case 3:
		if (lightingMode >= 0 && lightingMode <= 2)
			break;

		if (lightingMode != 3)
			return lightingMode;

		copyIntensities = true;
		break;
	case 4:
		if (lightingMode < 0 || lightingMode > 3)
			return lightingMode;

		copyTexVertices = true;
		if (lightingMode == 3)
			copyIntensities = true;
		break;
	default:
		return geometryMode;
	}

	for (uint i = 0; i < pSrc->numVertices; i++) {
		pDst->aVertices[i][0] = pSrc->aVertices[pSrc->aVertIndices[i]][0];
		pDst->aVertices[i][1] = pSrc->aVertices[pSrc->aVertIndices[i]][1];
		pDst->aVertices[i][2] = pSrc->aVertices[pSrc->aVertIndices[i]][2];

		if (copyTexVertices) {
			pDst->aTexVertices[i][0] = pSrc->aTexVertices[pSrc->aTexIndices[i]][0];
			pDst->aTexVertices[i][1] = pSrc->aTexVertices[pSrc->aTexIndices[i]][1];
			pDst->aTexVertices[i][0] = (float)((double)uvDelta[0] + pDst->aTexVertices[i][0]);
			pDst->aTexVertices[i][1] = (float)((double)pDst->aTexVertices[i][1] + uvDelta[1]);
		}

		if (copyIntensities) {
			double intensity = pSrc->aIntensities[pSrc->aVertIndices[i]];

			if (pSrc->aAddedIntensities)
				intensity = intensity + pSrc->aAddedIntensities[i];

			if (intensity < 0.0)
				intensity = 0.0;
			else if (intensity > 1.0)
				intensity = 1.0;

			pDst->aIntensities[i] = (float)intensity;
		}
	}

	pDst->numVertices = pSrc->numVertices;

	return pSrc->numVertices;
}

int RenderDroid::Primit3Module::clipFace(RdClipFrustum *pClip, int geometryMode, int lightingMode, int textureMode, RdVertexLists *pSrc, RdVertexLists *pDst, RdVector2 uvDelta) {
	bool bPerspective = _rd->_camera.getCurrent()->projectType == 1;
	bool bTextured;
	bool bIntensities;
	bool bWire = false;
	int result;

	switch (geometryMode) {
	case 0:
		bTextured = false;
		bIntensities = false;
		break;
	case 1:
	case 2:
		bTextured = false;
		bIntensities = false;
		bWire = true;
		break;
	case 3:
		if (lightingMode >= 0 && lightingMode <= 2) {
			bTextured = false;
			bIntensities = false;
		} else if (lightingMode == 3) {
			bTextured = false;
			bIntensities = true;
		} else {
			return lightingMode;
		}
		break;
	case 4:
		if (lightingMode >= 0 && lightingMode <= 2) {
			bTextured = true;
			bIntensities = false;
		} else if (lightingMode == 3) {
			bTextured = true;
			bIntensities = true;
		} else {
			return lightingMode;
		}
		break;
	default:
		return geometryMode;
	}

	for (uint i = 0; i < pSrc->numVertices; i++) {
		const float *srcVertex = pSrc->aVertices[pSrc->aVertIndices[i]];

		pDst->aVertices[i][0] = srcVertex[0];
		pDst->aVertices[i][1] = srcVertex[1];
		pDst->aVertices[i][2] = srcVertex[2];

		if (bTextured) {
			const float *srcTexVertex = pSrc->aTexVertices[pSrc->aTexIndices[i]];

			pDst->aTexVertices[i][0] = srcTexVertex[0];
			pDst->aTexVertices[i][1] = srcTexVertex[1];
			pDst->aTexVertices[i][0] = (float)((double)uvDelta[0] + pDst->aTexVertices[i][0]);
			pDst->aTexVertices[i][1] = (float)((double)pDst->aTexVertices[i][1] + uvDelta[1]);
		}

		if (bIntensities) {
			double intensity = pSrc->aIntensities[pSrc->aVertIndices[i]];

			if (pSrc->aAddedIntensities)
				intensity = intensity + pSrc->aAddedIntensities[i];

			if (intensity < 0.0)
				intensity = 0.0;
			else if (intensity > 1.0)
				intensity = 1.0;

			pDst->aIntensities[i] = (float)intensity;
		}
	}

	if (bWire) {
		if (bPerspective)
			result = _rd->_clip.face3W(pClip, pDst->aVertices, pSrc->numVertices);
		else
			result = _rd->_clip.face3WOrtho(pClip, pDst->aVertices, pSrc->numVertices);
	} else if (bTextured && bIntensities) {
		if (bPerspective)
			result = _rd->_clip.face3GT(pClip, pDst->aVertices, pDst->aTexVertices, pDst->aIntensities, pSrc->numVertices);
		else
			result = _rd->_clip.face3GTOrtho(pClip, pDst->aVertices, pDst->aTexVertices, pDst->aIntensities, pSrc->numVertices);
	} else if (bTextured) {
		if (bPerspective)
			result = _rd->_clip.face3T(pClip, pDst->aVertices, pDst->aTexVertices, pSrc->numVertices);
		else
			result = _rd->_clip.face3TOrtho(pClip, pDst->aVertices, pDst->aTexVertices, pSrc->numVertices);
	} else if (bIntensities) {
		if (bPerspective)
			result = _rd->_clip.face3GS(pClip, pDst->aVertices, pDst->aIntensities, pSrc->numVertices);
		else
			result = _rd->_clip.face3GSOrtho(pClip, pDst->aVertices, pDst->aIntensities, pSrc->numVertices);
	} else {
		if (bPerspective)
			result = _rd->_clip.face3S(pClip, pDst->aVertices, pSrc->numVertices);
		else
			result = _rd->_clip.face3SOrtho(pClip, pDst->aVertices, pSrc->numVertices);
	}

	pDst->numVertices = result;

	return result;
}

RdProcFace *RenderDroid::Primit3Module::drawPolygon(RdVector3 *aVerts, int numVerts, RdMaterial *pMaterial) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 aTransformed[32];
	RdVector3 aScreen[32];
	float sumX = 0.0f;
	float sumY = 0.0f;
	float sumZ = 0.0f;

	if (numVerts > 32) {
		warning("RenderDroid::Primit3Module::drawPolygon(): too many vertices (%d)", numVerts);
		return nullptr;
	}

	for (int i = 0; i < numVerts; i++) {
		_rd->transformPoint34(aTransformed[i], aVerts[i], pCamera->viewMatrix);
		sumX = (float)((double)sumX + aTransformed[i][0]);
		sumY = (float)((double)aTransformed[i][1] + sumY);
		sumZ = (float)((double)aTransformed[i][2] + sumZ);
	}

	double count = (double)numVerts;
	float centerX = (float)(sumX / count);
	float centerY = (float)(sumY / count);
	float centerZ = (float)(sumZ / count);

	int numClipped = _rd->_clip.face3S(pCamera->pClipFrustum, aTransformed, numVerts);

	if (numClipped < 3)
		return nullptr;

	(_rd->_camera.*pCamera->pfProjectLst)(aScreen, aTransformed, numClipped);

	// The demo draws the polygon twice, as back face and as front face
	if (_bDemo) {
		RdProcFace *pLast = nullptr;

		for (int side = 1; side >= 0; side--) {
			RdProcFace *pDemoFace = _rd->_cache.getProcEntry();

			if (!pDemoFace) {
				warning("RenderDroid::Primit3Module::drawPolygon(): no free cache entry");
				return pLast;
			}

			pDemoFace->flags = 0;
			pDemoFace->type = 0;
			pDemoFace->diffuseLight = 0.0f;
			pDemoFace->extraLight = 0.0f;
			pDemoFace->geometryMode = 3;
			pDemoFace->lightingMode = 1;
			pDemoFace->numVertices = numClipped;
			memcpy(pDemoFace->aVertices, aScreen, sizeof(RdVector3) * numClipped);
			pDemoFace->curCel = -1;
			pDemoFace->pMaterial = pMaterial;
			pDemoFace->bBackface = side;
			_rd->_cache.addProcFace(0, numClipped, 1);
			pLast = pDemoFace;
		}

		return pLast;
	}

	RdProcFace *pProcFace = _rd->_cache.getProcEntry();

	if (!pProcFace) {
		warning("RenderDroid::Primit3Module::drawPolygon(): no free cache entry");
		return nullptr;
	}

	pProcFace->flags = 0;
	pProcFace->diffuseLight = 0.0f;
	pProcFace->extraLight = 0.0f;
	pProcFace->curCel = 0;
	pProcFace->type = 1;
	pProcFace->lightingMode = 1;
	pProcFace->geometryMode = 3;
	pProcFace->numVertices = numClipped;
	memcpy(pProcFace->aVertices, aScreen, sizeof(RdVector3) * numClipped);
	pProcFace->curCel = -1;
	pProcFace->pMaterial = pMaterial;

	RdVector3 edge0;
	RdVector3 edge1;
	RdVector3 normal;

	edge0[0] = aTransformed[1][0] - aTransformed[0][0];
	edge0[1] = aTransformed[1][1] - aTransformed[0][1];
	edge0[2] = aTransformed[1][2] - aTransformed[0][2];
	edge1[0] = aTransformed[2][0] - aTransformed[1][0];
	edge1[1] = aTransformed[2][1] - aTransformed[1][1];
	edge1[2] = aTransformed[2][2] - aTransformed[1][2];
	_rd->cross3(normal, edge0, edge1);

	pProcFace->bBackface = (double)normal[0] * centerX + (double)normal[1] * centerY + (double)normal[2] * centerZ > 0.0;
	_rd->_cache.addProcFace(0, numClipped, 1);

	return pProcFace;
}

int RenderDroid::Primit3Module::drawPolylineFace(RdThing *pThing, RdFace3 *pFace, RdVector3 *aVerts, RdVertexLists *pSrc) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdProcFace *pProcFace = _rd->_cache.getProcEntry();

	if (!pProcFace)
		return 0;

	RdVertexLists dst;

	memset(&dst, 0, sizeof(RdVertexLists));
	dst.aVertices = _work;
	dst.aScreen = pProcFace->aVertices;
	dst.aTexVertices = pProcFace->aTexVertices;
	dst.aIntensities = pProcFace->aIntensities;

	pSrc->numVertices = pFace->numVertices;
	pSrc->aVertIndices = pFace->aVertices;
	pSrc->aTexIndices = pFace->aTexVertices;

	int geometryMode = _rd->_curGeometryMode;

	if (geometryMode >= pFace->geometryMode)
		geometryMode = pFace->geometryMode;

	if (geometryMode >= pThing->curGeometryMode)
		geometryMode = pThing->curGeometryMode;

	pProcFace->geometryMode = geometryMode;

	if ((_rd->_curRenderOptions & 2) && pCamera->ambientLight >= 1.0f) {
		pProcFace->lightingMode = 0;
	} else {
		int lightingMode = _rd->_curLightingMode;

		if (lightingMode >= pFace->lightingMode)
			lightingMode = pFace->lightingMode;

		if (lightingMode >= pThing->curLightingMode)
			lightingMode = pThing->curLightingMode;

		pProcFace->lightingMode = lightingMode;
	}

	int textureMode = _rd->_curTextureMode;

	if (textureMode >= pFace->textureMode)
		textureMode = pFace->textureMode;

	if (textureMode >= pThing->curTextureMode)
		textureMode = pThing->curTextureMode;

	pProcFace->textureMode = textureMode;

	clipFace(pCamera->pClipFrustum, geometryMode, pProcFace->lightingMode, textureMode, pSrc, &dst, pFace->uvDelta);

	if (dst.numVertices < 3)
		return dst.numVertices;

	(_rd->_camera.*pCamera->pfProjectLst)(dst.aScreen, dst.aVertices, dst.numVertices);

	if (_rd->_curRenderOptions & 2)
		pProcFace->ambientLight = pCamera->ambientLight;
	else
		pProcFace->ambientLight = 0.0f;

	RdColormap *pCurMap = _rd->_colormap.getCurrent();
	RdColormap *pIdentityMap = _rd->_colormap.getIdentity();
	int lightingMode = pProcFace->lightingMode;

	if (lightingMode) {
		if (pProcFace->ambientLight < 1.0f) {
			if (lightingMode == 2) {
				if (pProcFace->diffuseLight >= 1.0f && pCurMap == pIdentityMap)
					pProcFace->lightingMode = 0;
				else if (pProcFace->diffuseLight <= 0.0f)
					pProcFace->lightingMode = 1;
			} else if (lightingMode == 3) {
				float first = pProcFace->aIntensities[0];
				uint same = 1;

				while (same < dst.numVertices && pProcFace->aIntensities[same] == first)
					same++;

				if (same == dst.numVertices) {
					if (first == 1.0f) {
						if (pCurMap == pIdentityMap) {
							pProcFace->lightingMode = 0;
						} else {
							pProcFace->lightingMode = 2;
							pProcFace->diffuseLight = 1.0f;
						}
					} else if (first == 0.0f) {
						pProcFace->lightingMode = 1;
						pProcFace->diffuseLight = 0.0f;
					} else {
						pProcFace->lightingMode = 2;
						pProcFace->diffuseLight = first;
					}
				}
			}
		} else if (pCurMap == pIdentityMap) {
			pProcFace->lightingMode = 0;
		} else {
			pProcFace->lightingMode = 2;
			pProcFace->diffuseLight = 1.0f;
		}
	}

	char bufferFlags = 1;

	if (pProcFace->geometryMode >= 4)
		bufferFlags = 3;

	if (pProcFace->lightingMode >= 3)
		bufferFlags |= 4;

	pProcFace->curCel = pThing->curCel;
	pProcFace->bBackface = 0;
	pProcFace->type = pFace->type;
	pProcFace->extraLight = pFace->extraLight;
	pProcFace->pMaterial = pFace->pMaterial;

	return _rd->_cache.addProcFace(0, dst.numVertices, bufferFlags);
}

void RenderDroid::Primit3Module::clearFrameCounters() {
}

} // End of namespace Grim
