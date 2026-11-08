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

#include "common/algorithm.h"
#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

RenderDroid::CacheModule::CacheModule(RenderDroid *rd) : Module(rd), _numProcFaces(0), _numUsedVertices(0), _numUsedTexVertices(0), _numUsedIntensities(0), _drawnFaces(0),
		_totalNormalTris(0), _totalSolidTris(0), _totalVerts(0) {
	memset(_viewportRect, 0, sizeof(_viewportRect));
	_aHWVertices = new Std3DVertex[32768]();
	_aHWNormalTris = new Std3DTri[1024]();
	_aHWSolidTris = new Std3DTri[1024]();
}

RenderDroid::CacheModule::~CacheModule() {
	delete[] _aHWVertices;
	delete[] _aHWNormalTris;
	delete[] _aHWSolidTris;
}

int RenderDroid::CacheModule::startup() {
	return 1;
}

int RenderDroid::CacheModule::finishFrame() {
	int result = _rd->_curAcceleration;

#if defined(USE_OPENGL_SHADERS)
	if (result > 0)
		return _rd->_pStd3D->endScene();
#endif

	return result;
}

int RenderDroid::CacheModule::reset() {
	_numProcFaces = 0;
	_numUsedVertices = 0;
	_numUsedTexVertices = 0;
	_numUsedIntensities = 0;

	// Only the demo resets the viewport rectangle with the cache
	if (_rd->isDemo()) {
		_viewportRect[0] = 0x7FFFFFFF;
		_viewportRect[1] = 0x7FFFFFFF;
		_viewportRect[2] = 0;
		_viewportRect[3] = 0;
	}

	return 0;
}

void RenderDroid::CacheModule::clearFrameCounters() {
	_drawnFaces = 0;
}

RdProcFace *RenderDroid::CacheModule::getProcEntry() {
	size_t numProcFaces = _numProcFaces;

	if (numProcFaces >= 0x400) {
		flush();
		numProcFaces = _numProcFaces;
	}

	if (0x8000 - _numUsedVertices < 0x20)
		return nullptr;

	if (0x8000 - _numUsedTexVertices < 0x20)
		return nullptr;

	if (0x8000 - _numUsedIntensities < 0x20)
		return nullptr;

	RdProcFace *pProcFace = &_aProcFaces[numProcFaces];

	pProcFace->aVertices = (RdVector3 *)&_aVertices[3 * _numUsedVertices];
	pProcFace->aTexVertices = (RdVector2 *)&_aTexVertices[2 * _numUsedTexVertices];
	pProcFace->aIntensities = &_aIntensities[_numUsedIntensities];
	pProcFace->aRedIntensities = &_aIntensities[_numUsedIntensities];
	pProcFace->aGreenIntensities = &_aGreenIntensities[_numUsedIntensities];
	pProcFace->aBlueIntensities = &_aBlueIntensities[_numUsedIntensities];

	return pProcFace;
}

int RenderDroid::CacheModule::flush() {
	int result = (int)_numProcFaces;

	if (!_numProcFaces)
		return result;

	if (_rd->_curSortingMethod == 2) {
		sort(_aProcFaces, _numProcFaces, sizeof(RdProcFace), procFaceCompare);
	}

	if (_rd->_curAcceleration <= 0) {
		StdVBuffer *pLockedBuffer = nullptr;
		RdCamera *pCamera = _rd->_camera.getCurrent();

		if (pCamera && pCamera->pCanvas && pCamera->pCanvas->pVBuffer && pCamera->pCanvas->pVBuffer->type == 1) {
			if (_rd->_stdDisplay->vBufferLock(pCamera->pCanvas->pVBuffer))
				pLockedBuffer = pCamera->pCanvas->pVBuffer;
		}

		if (_rd->_curOcclusionMethod) {
			if (_rd->_curOcclusionMethod == 1) {
				_rd->_active.advanceFrame();
				_rd->_active.drawScene();
			}
		} else if (_rd->_curZBufferMethod) {
			if (_rd->_curZBufferMethod == 2) {
				for (size_t i = 0; i < _numProcFaces; i++) {
					if (_aProcFaces[i].flags & 1)
						drawFaceUser(&_aProcFaces[i]);
					else
						drawFaceZ(&_aProcFaces[i]);
				}
			}
		} else {
			for (size_t i = 0; i < _numProcFaces; i++) {
				if (_aProcFaces[i].flags & 1)
					drawFaceUser(&_aProcFaces[i]);
				else
					drawFaceN(&_aProcFaces[i]);
			}
		}

		if (pLockedBuffer)
			_rd->_stdDisplay->vBufferUnlock(pLockedBuffer);
	} else {
#if defined(USE_OPENGL_SHADERS)
		sendFaceListToHardware();
#endif
	}

	_drawnFaces += (uint)_numProcFaces;

	return reset();
}

int RenderDroid::CacheModule::addProcFace(uint flags, uint numVertices, char bufferFlags) {
	if (_numProcFaces >= 0x400)
		return 0;

	RdProcFace *pProcFace = &_aProcFaces[_numProcFaces];
	const float *aVerts = (const float *)pProcFace->aVertices;

	pProcFace->flags = flags;
	pProcFace->numVertices = numVertices;
	pProcFace->userData = _rd->_curProcFaceUserData;

	float xMax = aVerts[0];
	float xMin = xMax;
	double yMin = aVerts[1];
	float yMax = aVerts[1];

	pProcFace->yMaxIdx = 0;
	pProcFace->yMinIdx = 0;
	pProcFace->zMax = aVerts[2];
	pProcFace->zMin = aVerts[2];

	for (uint i = 1; i < numVertices; i++) {
		float x = aVerts[3 * i];
		float y = aVerts[3 * i + 1];
		float z = aVerts[3 * i + 2];

		if (x >= xMin) {
			if (x > xMax)
				xMax = x;
		} else {
			xMin = x;
		}

		if (y >= yMin) {
			if (y > (double)yMax) {
				pProcFace->yMaxIdx = i;
				yMax = y;
			}
		} else {
			yMin = y;
			pProcFace->yMinIdx = i;
		}

		if (z >= (double)pProcFace->zMin) {
			if (z > (double)pProcFace->zMax)
				pProcFace->zMax = z;
		} else {
			pProcFace->zMin = z;
		}
	}

	if (xMin >= (double)xMax)
		return 0;

	if (yMin >= yMax)
		return 0;

	pProcFace->xMin = (int)(float)ceil((double)xMin);
	pProcFace->xMax = (int)(float)ceil((double)xMax);
	pProcFace->yMin = (int)(float)ceil((double)(float)yMin);
	pProcFace->yMax = (int)(float)ceil((double)yMax);

	if (bufferFlags & 1)
		_numUsedVertices += numVertices;

	if (bufferFlags & 2)
		_numUsedTexVertices += numVertices;

	if (bufferFlags & 4)
		_numUsedIntensities += numVertices;

	pProcFace->pColormap = _rd->_colormap.getCurrent();
	_numProcFaces++;

	if ((uint)pProcFace->xMin < (uint)_viewportRect[0])
		_viewportRect[0] = pProcFace->xMin;

	if ((uint)pProcFace->xMax > (uint)_viewportRect[2])
		_viewportRect[2] = pProcFace->xMax;

	if ((uint)pProcFace->yMin < (uint)_viewportRect[1])
		_viewportRect[1] = pProcFace->yMin;

	if ((uint)pProcFace->yMax > (uint)_viewportRect[3])
		_viewportRect[3] = pProcFace->yMax;

	return 1;
}

int RenderDroid::CacheModule::procFaceCompare(const void *pA, const void *pB) {
	const RdProcFace *pFaceA = (const RdProcFace *)pA;
	const RdProcFace *pFaceB = (const RdProcFace *)pB;

	if (pFaceA->zMin == pFaceB->zMin)
		return 0;

	if (pFaceA->zMin >= (double)pFaceB->zMin)
		return -1;

	return 1;
}

RdMatCel *RenderDroid::CacheModule::selectCel(const RdProcFace *pProcFace) {
	RdMaterial *pMaterial = pProcFace->pMaterial;

	if (!pMaterial)
		return nullptr;

	if (!pMaterial->numCels) {
		warning("RenderDroid::CacheModule::selectCel(): material %s has no cels", pMaterial->aName);
		return nullptr;
	}

	int curCel = pProcFace->curCel;

	if (curCel == -1)
		curCel = pMaterial->curCel;

	if (curCel < 0)
		curCel = 0;
	else if (curCel > (int)(pMaterial->numCels - 1))
		curCel = pMaterial->numCels - 1;

	return pMaterial->apCels[curCel];
}

float RenderDroid::CacheModule::calcFaceLight(const RdProcFace *pProcFace, int lightingMode, float ambientLight) {
	float light = pProcFace->extraLight;

	if (lightingMode == 2)
		light = (float)((double)pProcFace->extraLight + pProcFace->diffuseLight);

	double clamped = light;

	if (light < 0.0)
		clamped = 0.0;
	else if (light > 1.0)
		clamped = 1.0;

	if (clamped <= ambientLight)
		clamped = ambientLight;

	return (float)(clamped * 63.0);
}

void RenderDroid::CacheModule::calcVertexLights(RdProcFace *pProcFace, float ambientLight) {
	for (uint i = 0; i < pProcFace->numVertices; i++) {
		float light = (float)((double)pProcFace->extraLight + pProcFace->aIntensities[i]);
		double clamped = light;

		if (light < 0.0)
			clamped = 0.0;
		else if (light > 1.0)
			clamped = 1.0;

		if (clamped <= ambientLight)
			clamped = ambientLight;

		pProcFace->aIntensities[i] = (float)(clamped * 63.0);
	}
}

int RenderDroid::CacheModule::drawFaceUser(RdProcFace *pProcFace) {
	int geometryMode = pProcFace->geometryMode;

	if (geometryMode >= _rd->_curGeometryMode)
		geometryMode = _rd->_curGeometryMode;

	int lightingMode = pProcFace->lightingMode;

	if (lightingMode >= _rd->_curLightingMode)
		lightingMode = _rd->_curLightingMode;

	int textureMode = pProcFace->textureMode;

	if (textureMode >= _rd->_curTextureMode)
		textureMode = _rd->_curTextureMode;

	if (!geometryMode)
		return 0;

	RdMatCel *pCel = selectCel(pProcFace);

	if (pCel && geometryMode == 4 && !(pCel->flags & 8))
		geometryMode = 3;

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = pProcFace->ambientLight;

	float light;

	switch (geometryMode) {
	case 1:
	case 2:
		if ((uint)lightingMode <= 4)
			pProcFace->pfUserDraw(pProcFace, pCel, 0.0f);
		break;
	case 3:
	case 4:
		// The demo draws textured faces only with the perspective mode
		if (geometryMode == 4 && ((_rd->isDemo() && textureMode != 1) || (textureMode != 0 && textureMode != 1)))
			break;

		switch (lightingMode) {
		case 0:
			pProcFace->pfUserDraw(pProcFace, pCel, 0.0f);
			break;
		case 1:
		case 2:
			light = calcFaceLight(pProcFace, lightingMode, ambientLight);
			pProcFace->diffuseLight = light;
			pProcFace->pfUserDraw(pProcFace, pCel, light);
			break;
		case 3:
			calcVertexLights(pProcFace, ambientLight);
			pProcFace->pfUserDraw(pProcFace, pCel, 0.0f);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	// The demo draws the wireframe over the face when the option is set
	if (_rd->isDemo() && (_rd->_curRenderOptions & 0x40))
		_rd->_nRaster.drawNGonLW(pProcFace, pCel);

	return 1;
}

int RenderDroid::CacheModule::drawFaceN(RdProcFace *pProcFace) {
	int geometryMode = pProcFace->geometryMode;

	if (geometryMode >= _rd->_curGeometryMode)
		geometryMode = _rd->_curGeometryMode;

	int lightingMode = pProcFace->lightingMode;

	if (lightingMode >= _rd->_curLightingMode)
		lightingMode = _rd->_curLightingMode;

	int textureMode = pProcFace->textureMode;

	if (textureMode >= _rd->_curTextureMode)
		textureMode = _rd->_curTextureMode;

	if (!geometryMode)
		return 0;

	RdMatCel *pCel = selectCel(pProcFace);

	if (pCel && geometryMode == 4 && !(pCel->flags & 8))
		geometryMode = 3;

	if (!pCel) {
		warning("RenderDroid::CacheModule::drawFaceN(): face without a material cel");
		return 1;
	}

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = pProcFace->ambientLight;

	float light;

	switch (geometryMode) {
	case 1:
		if ((uint)lightingMode <= 4)
			_rd->_nRaster.drawNGonLV(pProcFace, pCel);
		break;
	case 2:
		if ((uint)lightingMode <= 4)
			_rd->_nRaster.drawNGonLW(pProcFace, pCel);
		break;
	case 3:
		switch (lightingMode) {
		case 0:
			if (pProcFace->type & 2)
				_rd->_nRaster.drawNGonTLS(pProcFace, pCel);
			else
				_rd->_nRaster.drawNGonLS(pProcFace, pCel);
			break;
		case 1:
		case 2:
			light = calcFaceLight(pProcFace, lightingMode, ambientLight);
			pProcFace->diffuseLight = light;

			if (pProcFace->type & 2)
				_rd->_nRaster.drawNGonTFS(pProcFace, pCel, light);
			else
				_rd->_nRaster.drawNGonFS(pProcFace, pCel, light);
			break;
		case 3:
			calcVertexLights(pProcFace, ambientLight);

			if (pProcFace->type & 2)
				_rd->_nRaster.drawNGonTGS(pProcFace, pCel);
			else
				_rd->_nRaster.drawNGonGS(pProcFace, pCel);
			break;
		default:
			break;
		}
		break;
	case 4:
		if (textureMode == 0) {
			switch (lightingMode) {
			case 0:
				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTLAT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonMLAT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTLAT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonLAT(pProcFace, pCel);
				}
				break;
			case 1:
			case 2:
				light = calcFaceLight(pProcFace, lightingMode, ambientLight);
				pProcFace->diffuseLight = light;

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTFAT(pProcFace, pCel, light);
					else
						_rd->_nRaster.drawNGonMFAT(pProcFace, pCel, light);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTFAT(pProcFace, pCel, light);
					else
						_rd->_nRaster.drawNGonFAT(pProcFace, pCel, light);
				}
				break;
			case 3:
				calcVertexLights(pProcFace, ambientLight);

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTGAT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonMGAT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTGAT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonGAT(pProcFace, pCel);
				}
				break;
			default:
				break;
			}
		} else if (textureMode == 1) {
			switch (lightingMode) {
			case 0:
				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTLIT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonMLIT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTLIT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonLIT(pProcFace, pCel);
				}
				break;
			case 1:
			case 2:
				light = calcFaceLight(pProcFace, lightingMode, ambientLight);
				pProcFace->diffuseLight = light;

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTFIT(pProcFace, pCel, light);
					else
						_rd->_nRaster.drawNGonMFIT(pProcFace, pCel, light);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTFIT(pProcFace, pCel, light);
					else
						_rd->_nRaster.drawNGonFIT(pProcFace, pCel, light);
				}
				break;
			case 3:
				calcVertexLights(pProcFace, ambientLight);

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonMTGIT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonMGIT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_nRaster.drawNGonTGIT(pProcFace, pCel);
					else
						_rd->_nRaster.drawNGonGIT(pProcFace, pCel);
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	// The demo draws the wireframe over the face when the option is set
	if (_rd->isDemo() && (_rd->_curRenderOptions & 0x40))
		_rd->_nRaster.drawNGonLW(pProcFace, pCel);

	return 1;
}

int RenderDroid::CacheModule::drawFaceZ(RdProcFace *pProcFace) {
	int geometryMode = pProcFace->geometryMode;

	if (geometryMode >= _rd->_curGeometryMode)
		geometryMode = _rd->_curGeometryMode;

	int lightingMode = pProcFace->lightingMode;

	if (lightingMode >= _rd->_curLightingMode)
		lightingMode = _rd->_curLightingMode;

	int textureMode = pProcFace->textureMode;

	if (textureMode >= _rd->_curTextureMode)
		textureMode = _rd->_curTextureMode;

	if (!geometryMode)
		return 0;

	RdMatCel *pCel = selectCel(pProcFace);

	if (pCel && geometryMode == 4 && !(pCel->flags & 8))
		geometryMode = 3;

	if (!pCel) {
		warning("RenderDroid::CacheModule::drawFaceZ(): face without a material cel");
		return 1;
	}

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = pProcFace->ambientLight;

	float light;

	switch (geometryMode) {
	case 1:
		if ((uint)lightingMode <= 4)
			_rd->_zRaster.drawNGonLV(pProcFace, pCel);
		break;
	case 2:
		if ((uint)lightingMode <= 4)
			_rd->_zRaster.drawNGonLW(pProcFace, pCel);
		break;
	case 3:
		switch (lightingMode) {
		case 0:
			if (pProcFace->type & 2)
				_rd->_zRaster.drawNGonTLS(pProcFace, pCel);
			else
				_rd->_zRaster.drawNGonLS(pProcFace, pCel);
			break;
		case 1:
			pProcFace->diffuseLight = 0.0f;

			if (pProcFace->type & 2)
				_rd->_zRaster.drawNGonTFS(pProcFace, pCel, 0.0f);
			else
				_rd->_zRaster.drawNGonFS(pProcFace, pCel, 0.0f);
			break;
		case 2:
			light = calcFaceLight(pProcFace, lightingMode, ambientLight);
			pProcFace->diffuseLight = light;

			if (pProcFace->type & 2)
				_rd->_zRaster.drawNGonTFS(pProcFace, pCel, light);
			else
				_rd->_zRaster.drawNGonFS(pProcFace, pCel, light);
			break;
		case 3:
			calcVertexLights(pProcFace, ambientLight);

			if (pProcFace->type & 2)
				_rd->_zRaster.drawNGonTGS(pProcFace, pCel);
			else
				_rd->_zRaster.drawNGonGS(pProcFace, pCel);
			break;
		default:
			break;
		}
		break;
	case 4:
		if (textureMode == 0) {
			switch (lightingMode) {
			case 0:
				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTLAT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonMLAT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTLAT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonLAT(pProcFace, pCel);
				}
				break;
			case 1:
			case 2:
				light = calcFaceLight(pProcFace, lightingMode, ambientLight);
				pProcFace->diffuseLight = light;

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTFAT(pProcFace, pCel, light);
					else
						_rd->_zRaster.drawNGonMFAT(pProcFace, pCel, light);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTFAT(pProcFace, pCel, light);
					else
						_rd->_zRaster.drawNGonFAT(pProcFace, pCel, light);
				}
				break;
			case 3:
				calcVertexLights(pProcFace, ambientLight);

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTGAT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonMGAT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTGAT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonGAT(pProcFace, pCel);
				}
				break;
			default:
				break;
			}
		} else if (textureMode == 1) {
			switch (lightingMode) {
			case 0:
				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTLIT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonMLIT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTLIT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonLIT(pProcFace, pCel);
				}
				break;
			case 1:
			case 2:
				light = calcFaceLight(pProcFace, lightingMode, ambientLight);
				pProcFace->diffuseLight = light;

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTFIT(pProcFace, pCel, light);
					else
						_rd->_zRaster.drawNGonMFIT(pProcFace, pCel, light);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTFIT(pProcFace, pCel, light);
					else
						_rd->_zRaster.drawNGonFIT(pProcFace, pCel, light);
				}
				break;
			case 3:
				calcVertexLights(pProcFace, ambientLight);

				if (pCel->pTexture->flags & 1) {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonMTGIT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonMGIT(pProcFace, pCel);
				} else {
					if (pProcFace->type & 2)
						_rd->_zRaster.drawNGonTGIT(pProcFace, pCel);
					else
						_rd->_zRaster.drawNGonGIT(pProcFace, pCel);
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	// The demo draws the wireframe over the face when the option is set
	if (_rd->isDemo() && (_rd->_curRenderOptions & 0x40))
		_rd->_zRaster.drawNGonLW(pProcFace, pCel);

	return 1;
}

} // End of namespace Grim
