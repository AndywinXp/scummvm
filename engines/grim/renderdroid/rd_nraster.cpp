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

RenderDroid::NRasterModule::NRasterModule(RenderDroid *rd) : Module(rd),
	_numVertices(0), _yMinVert(0), _yMaxVert(0), _leftVertexStep(0), _rightVertexStep(0),
	_aVertices(nullptr), _aTexVertices(nullptr), _aIntensities(nullptr),
	_pDest(nullptr), _pWDest(nullptr), _pPixels(nullptr),
	_mipMapNum(0), _maxMipMap(0), _texPitch(0), _texRowShift(0), _uWrap(0), _vWrap(0),
	_uStep(0), _vStep(0), _fixU(0), _fixV(0),
	_dOneOverZdX(0.0f), _dOneOverZdY(0.0f), _dUOverZdX(0.0f), _dUOverZdY(0.0f), _dVOverZdX(0.0f), _dVOverZdY(0.0f),
	_dUdXModifier(0), _dVdXModifier(0),
	_oneOverZStep(0.0f), _uOverZStep(0.0f), _vOverZStep(0.0f),
	_aAlphaMap(nullptr), _aMonoMap(nullptr), _aRGBMap(nullptr), _aRGBToPalette(nullptr),
	_RGBcolor(0), _color(0) {
	for (int i = 0; i < 32; i++) {
		_aOneOverZ[i] = 0.0f;
		_aUOverZ[i] = 0.0f;
		_aVOverZ[i] = 0.0f;
	}
}

int RenderDroid::NRasterModule::ftol(double value) {
	if (!(value > -2147483649.0 && value < 2147483648.0)) {
		warning("RenderDroid::NRasterModule::ftol(): value out of range");
		return -2147483647 - 1;
	}

	return (int)value;
}

int RenderDroid::NRasterModule::floorToInt(double value) {
	if (!(value >= -2147483648.0 && value < 2147483648.0)) {
		warning("RenderDroid::NRasterModule::floorToInt(): value out of range");
		return -2147483647 - 1;
	}

	return (int)floor(value);
}

int RenderDroid::NRasterModule::fixedMultiply(int a, int b) {
	return (int)((uint64)((int64)a * b) >> 16);
}

int RenderDroid::NRasterModule::ceilToPixel(int x) {
	if ((x & 0xFFFF) != 0)
		return (int)(((uint)x + 0x10000) & 0xFFFF0000);

	return x;
}

int RenderDroid::NRasterModule::wrapVertex(int index) {
	int last = (int)_numVertices - 1;

	if (index < 0)
		return last;

	if (index > last)
		return 0;

	return index;
}

float RenderDroid::NRasterModule::lowRound(double value) {
	float result = (float)value;
	if (result != 0.0f && fabs((double)result) > fabs(value)) {
		uint32 bits;
		memcpy(&bits, &result, sizeof(bits));
		bits--;
		memcpy(&result, &bits, sizeof(bits));
	}

	return result;
}

float RenderDroid::NRasterModule::lowAdd(float a, float b) {
	return lowRound((double)a + b);
}

float RenderDroid::NRasterModule::lowSub(float a, float b) {
	return lowRound((double)a - b);
}

float RenderDroid::NRasterModule::lowMul(float a, float b) {
	return lowRound((double)a * b);
}

float RenderDroid::NRasterModule::lowDiv(float a, float b) {
	return lowRound((double)a / b);
}

int RenderDroid::NRasterModule::selectGradientVertex() {
	int direction = -1;
	int vertC = -1;

	if (wrapVertex(_yMinVert + 1) == _yMaxVert) {
		direction = -1;
	} else if (wrapVertex(_yMinVert - 1) == _yMaxVert) {
		direction = 1;
	} else {
		vertC = wrapVertex(_yMinVert + 1);
		float *pNext = _aVertices[vertC];
		float *pMin = _aVertices[_yMinVert];
		float *pMax = _aVertices[_yMaxVert];

		int64 dxMax = ftol((double)pNext[0] - pMax[0]);
		if (dxMax < 0)
			dxMax = -dxMax;

		bool offLine = (double)dxMax > 1.0;
		if (!offLine) {
			int64 dxMin = ftol((double)pNext[0] - pMin[0]);
			if (dxMin < 0)
				dxMin = -dxMin;

			offLine = (double)dxMin > 1.0;
		}

		if (offLine) {
			double expectedY = ((double)pMax[1] - pMin[1]) / ((double)pMax[0] - pMin[0]) * ((double)pNext[0] - pMin[0]) + pMin[1];
			double dist = pNext[1] - expectedY;
			if (dist < 0.0)
				dist = -dist;

			direction = (dist >= 1.0) ? 1 : -1;
		}
	}

	float bestDist = 0.0f;
	int cur = wrapVertex(direction + _yMinVert);
	while (cur != _yMaxVert) {
		float midX = ((double)_aVertices[_yMinVert][0] + _aVertices[_yMaxVert][0]) * 0.5f;
		double dist = _aVertices[cur][0] - midX;
		if (dist < 0.0)
			dist = -dist;

		if (dist > bestDist) {
			bestDist = dist;
			vertC = cur;
		}

		cur = wrapVertex(cur + direction);
	}

	if (vertC < 0) {
		warning("RenderDroid::NRasterModule::selectGradientVertex(): no reference vertex found");
		vertC = wrapVertex(direction + _yMinVert);
	}

	return vertC;
}

void RenderDroid::NRasterModule::selectMipLevel(float z) {
	switch (_maxMipMap) {
	case 0:
		_mipMapNum = 0;
		break;
	case 1:
		_mipMapNum = 0;
		if (z > (double)_rd->_aMipDistances[1])
			_mipMapNum = 1;
		break;
	case 2:
		if (z <= (double)_rd->_aMipDistances[0])
			_mipMapNum = 0;
		else if (z > (double)_rd->_aMipDistances[1])
			_mipMapNum = 2;
		else
			_mipMapNum = 1;
		break;
	case 3:
		if (z <= (double)_rd->_aMipDistances[0])
			_mipMapNum = 0;
		else if (z <= (double)_rd->_aMipDistances[1])
			_mipMapNum = 1;
		else if (z > (double)_rd->_aMipDistances[2])
			_mipMapNum = 3;
		else
			_mipMapNum = 2;
		break;
	default:
		break;
	}
}

void RenderDroid::NRasterModule::calcAffineGradients(float z) {
	int vertA = 0;
	int vertB = 1;
	int vertC = 2;

	if (_numVertices != 3) {
		vertA = _yMinVert;
		vertB = _yMaxVert;
		vertC = selectGradientVertex();
	}

	float *pRef = _aVertices[vertC];
	double dyA = _aVertices[vertA][1] - pRef[1];
	double dyB = _aVertices[vertB][1] - pRef[1];
	float dxA = (double)_aVertices[vertA][0] - pRef[0];
	float dxB = (double)_aVertices[vertB][0] - pRef[0];
	double duA = (double)_aTexVertices[vertA][0] - _aTexVertices[vertC][0];
	double duB = (double)_aTexVertices[vertB][0] - _aTexVertices[vertC][0];
	float duBStored = duB;
	float duAStored = duA;
	double dvB = (double)_aTexVertices[vertB][1] - _aTexVertices[vertC][1];
	double uCross = duB * dyA - duA * dyB;
	double invDet = 1.0 / (dyA * dxB - dyB * dxA);
	float invDetStored = invDet;
	float dvBStored = dvB;
	float dvA = (double)_aTexVertices[vertA][1] - _aTexVertices[vertC][1];
	float negInvDet = -invDet;
	float dUdX = uCross * invDetStored;
	float dUdY = ((double)duBStored * dxA - (double)duAStored * dxB) * negInvDet;
	float dVdX = (dvBStored * dyA - dvA * dyB) * invDetStored;
	float dVdY = ((double)dvBStored * dxA - (double)dvA * dxB) * negInvDet;

	selectMipLevel(z);

	if (dUdX > 0.0f)
		_dUdXModifier = 0x8000;
	else if (dUdX < 0.0f || dUdY < 0.0f)
		_dUdXModifier = 0x7FFF;
	else
		_dUdXModifier = 0x8000;

	if (dVdX > 0.0f)
		_dVdXModifier = 0x8000;
	else if (dVdX < 0.0f || dVdY < 0.0f)
		_dVdXModifier = 0x7FFF;
	else
		_dVdXModifier = 0x8000;
}

void RenderDroid::NRasterModule::calcPerspGradients(float z) {
	int vertA = 0;
	int vertB = 1;
	int vertC = 2;

	if (_numVertices != 3) {
		vertA = _yMinVert;
		vertB = _yMaxVert;
		vertC = selectGradientVertex();
	}

	int numVertices = _numVertices;
	if (numVertices > 32) {
		warning("RenderDroid::NRasterModule::calcPerspGradients(): more than 32 vertices");
		numVertices = 32;
	}

	for (int i = 0; i < numVertices; i++) {
		double oneOverZ = 1.0 / _aVertices[i][2];
		_aOneOverZ[i] = oneOverZ;
		_aUOverZ[i] = _aTexVertices[i][0] * oneOverZ;
		_aVOverZ[i] = (double)_aTexVertices[i][1] * _aOneOverZ[i];
	}

	float *pA = _aVertices[vertA];
	float *pB = _aVertices[vertB];
	float *pC = _aVertices[vertC];
	double dyA = (double)pA[1] - pC[1];
	double dyB = (double)pB[1] - pC[1];
	float dyAStored = dyA;
	float dyBStored = dyB;
	float dxA = (double)pA[0] - pC[0];
	float dxB = (double)pB[0] - pC[0];
	float dOneOverZA = (double)_aOneOverZ[vertA] - _aOneOverZ[vertC];
	float dUOverZB = (double)_aUOverZ[vertB] - _aUOverZ[vertC];
	float dUOverZA = (double)_aUOverZ[vertA] - _aUOverZ[vertC];
	float dVOverZB = (double)_aVOverZ[vertB] - _aVOverZ[vertC];
	double dOneOverZB = (double)_aOneOverZ[vertB] - _aOneOverZ[vertC];
	float dVOverZA = (double)_aVOverZ[vertA] - _aVOverZ[vertC];
	float invDet = 1.0 / (dyA * dxB - dyB * dxA);
	float negInvDet = -invDet;

	_dOneOverZdX = (dOneOverZB * dyAStored - (double)dOneOverZA * dyBStored) * invDet;
	_dOneOverZdY = (dOneOverZB * dxA - (double)dOneOverZA * dxB) * negInvDet;
	_dUOverZdX = ((double)dUOverZB * dyAStored - (double)dUOverZA * dyBStored) * invDet;
	_dUOverZdY = ((double)dUOverZB * dxA - (double)dUOverZA * dxB) * negInvDet;
	_dVOverZdX = ((double)dVOverZB * dyAStored - (double)dVOverZA * dyBStored) * invDet;
	_dVOverZdY = ((double)dVOverZB * dxA - (double)dVOverZA * dxB) * negInvDet;

	selectMipLevel(z);

	double uCrossX = (double)_aOneOverZ[vertA] * _dUOverZdX - (double)_aUOverZ[vertA] * _dOneOverZdX;
	if (uCrossX > 0.0)
		_dUdXModifier = 0x8000;
	else if (uCrossX < 0.0 || (double)_aOneOverZ[vertA] * _dUOverZdY - (double)_aUOverZ[vertA] * _dOneOverZdY < 0.0)
		_dUdXModifier = 0x7FFF;
	else
		_dUdXModifier = 0x8000;

	double vCrossX = (double)_aOneOverZ[vertA] * _dVOverZdX - (double)_aVOverZ[vertA] * _dOneOverZdX;
	if (vCrossX > 0.0)
		_dVdXModifier = 0x8000;
	else if (vCrossX < 0.0 || (double)_aOneOverZ[vertA] * _dVOverZdY - (double)_aVOverZ[vertA] * _dOneOverZdY < 0.0)
		_dVdXModifier = 0x7FFF;
	else
		_dVdXModifier = 0x8000;
}

int RenderDroid::NRasterModule::drawNGonLV(RdProcFace *pProcFace, RdMatCel *pCel) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	int bpp = pVBuffer->tex.format.bpp;
	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;

	if (bpp == 8) {
		_color = pCel->colorIndex;
		for (int i = 0; i < (int)_numVertices; i++) {
			int y = floorToInt(_aVertices[i][1]);
			int x = floorToInt(_aVertices[i][0]);
			pVBuffer->pPixels[y * pVBuffer->tex.pitchInBytes + x] = _color;
		}
	} else if (bpp == 16) {
		_RGBcolor = *(uint16 *)&pProcFace->pColormap->pRGB16[2 * (pCel->colorIndex & 0xFF) + 32256];
		for (int i = 0; i < (int)_numVertices; i++) {
			int y = floorToInt(_aVertices[i][1]);
			int x = floorToInt(_aVertices[i][0]);
			*(uint16 *)&pVBuffer->pPixels[2 * x + 2 * y * (pVBuffer->tex.pitchInBytes >> 1)] = _RGBcolor;
		}
	}

	return 0;
}

int RenderDroid::NRasterModule::drawNGonLW(RdProcFace *pProcFace, RdMatCel *pCel) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	int bpp = pCanvas->pVBuffer->tex.format.bpp;
	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;

	if (_numVertices == 0) {
		warning("RenderDroid::NRasterModule::drawNGonLW(): face without vertices");
		return 0;
	}

	uint16 color;
	if (bpp == 8) {
		_color = pCel->colorIndex;
		color = _color;
	} else if (bpp == 16) {
		_RGBcolor = *(uint16 *)&pProcFace->pColormap->pRGB16[2 * (pCel->colorIndex & 0xFF) + 32256];
		color = _RGBcolor;
	} else {
		return 0;
	}

	int i = 0;
	for (; i < (int)_numVertices - 1; i++)
		_rd->_primit2.drawLine(pCanvas, floorToInt(_aVertices[i][0]), floorToInt(_aVertices[i][1]), floorToInt(_aVertices[i + 1][0]), floorToInt(_aVertices[i + 1][1]), color, 0xFFFFFFFF);

	_rd->_primit2.drawLine(pCanvas, floorToInt(_aVertices[i][0]), floorToInt(_aVertices[i][1]), floorToInt(_aVertices[0][0]), floorToInt(_aVertices[0][1]), color, 0xFFFFFFFF);
	return 0;
}

int RenderDroid::NRasterModule::setupEdge(RdNRasterEdge *pEdge, int step, EdgeKind kind) {
	bool gouraud = kind == kEdgeGouraud || kind == kEdgeAffineGouraud || kind == kEdgePerspectiveGouraud;
	bool affine = kind == kEdgeAffine || kind == kEdgeAffineFlat || kind == kEdgeAffineGouraud;
	bool perspective = kind == kEdgePerspective || kind == kEdgePerspectiveFlat || kind == kEdgePerspectiveGouraud;
	bool directSlope = kind == kEdgeFlat || kind == kEdgePerspective || kind == kEdgePerspectiveFlat;
	bool exactSlopeX = kind == kEdgeAffineFlat || kind == kEdgeAffineGouraud;

	int vertIdx = pEdge->vertIdx;
	if (vertIdx == _yMaxVert)
		return 1;

	int nextIdx;
	int numLines;
	float yStart;
	while (true) {
		nextIdx = wrapVertex(vertIdx + step);
		yStart = ceil(_aVertices[vertIdx][1]);
		pEdge->y = floorToInt(yStart);
		numLines = floorToInt(ceil(_aVertices[nextIdx][1])) - pEdge->y;
		pEdge->numLines = numLines;
		if (numLines > 0)
			break;

		vertIdx = nextIdx;
		if (vertIdx == _yMaxVert)
			return 1;
	}

	float *pVert = _aVertices[vertIdx];
	float *pNext = _aVertices[nextIdx];
	float yDelta = (double)yStart - pVert[1];
	float xPos = pVert[0];
	float xFixed;
	float uFixed = 0.0f;
	float vFixed = 0.0f;
	float intensityFixed = 0.0f;

	if (numLines >= 2) {
		double deltaY = (double)pNext[1] - pVert[1];
		double invDyExact = 1.0 / deltaY;
		float invDy = invDyExact;
		double slopeX;
		if (directSlope)
			slopeX = ((double)pNext[0] - pVert[0]) / deltaY;
		else
			slopeX = ((double)pNext[0] - pVert[0]) * (exactSlopeX ? invDyExact : (double)invDy);

		float slopeXStored = slopeX;
		float dxFixed = slopeX * 65536.0;
		pEdge->dx = floorToInt(dxFixed);
		double xExact = (double)slopeXStored * yDelta + pVert[0];
		xPos = xExact;
		xFixed = xExact * 65536.0;

		if (affine) {
			double slopeU = ((double)_aTexVertices[nextIdx][0] - _aTexVertices[vertIdx][0]) * invDy;
			float slopeUStored = slopeU;
			float duFixed = slopeU * 65536.0;
			pEdge->du = floorToInt(duFixed);
			uFixed = ((double)slopeUStored * yDelta + _aTexVertices[vertIdx][0]) * 65536.0;

			double slopeV = ((double)_aTexVertices[nextIdx][1] - _aTexVertices[vertIdx][1]) * invDy;
			float slopeVStored = slopeV;
			float dvFixed = slopeV * 65536.0;
			pEdge->dv = floorToInt(dvFixed);
			vFixed = ((double)slopeVStored * yDelta + _aTexVertices[vertIdx][1]) * 65536.0;
		}

		if (perspective) {
			pEdge->dOneOverZ = (double)slopeXStored * _dOneOverZdX + _dOneOverZdY;
			pEdge->dUOverZ = (double)slopeXStored * _dUOverZdX + _dUOverZdY;
			pEdge->dVOverZ = (double)slopeXStored * _dVOverZdX + _dVOverZdY;
		}

		if (gouraud) {
			double slopeI = ((double)_aIntensities[nextIdx] - _aIntensities[vertIdx]) * invDy;
			float slopeIStored = slopeI;
			float dIntensityFixed = slopeI * 65536.0;
			pEdge->dIntensity = floorToInt(dIntensityFixed);
			intensityFixed = ((double)slopeIStored * yDelta + _aIntensities[vertIdx]) * 65536.0;
		}
	} else {
		xFixed = pVert[0] * 65536.0;
		if (affine) {
			uFixed = _aTexVertices[vertIdx][0] * 65536.0;
			vFixed = _aTexVertices[vertIdx][1] * 65536.0;
		}

		if (gouraud)
			intensityFixed = _aIntensities[vertIdx] * 65536.0;
	}

	pEdge->x = floorToInt(xFixed);
	if (affine) {
		pEdge->u = floorToInt(uFixed);
		pEdge->v = floorToInt(vFixed);
	}

	if (gouraud)
		pEdge->intensity = floorToInt(intensityFixed);

	if (perspective) {
		double xOffset = (double)xPos - pVert[0];
		double yOneOverZ = (double)yDelta * _dOneOverZdY;
		double yUOverZ = (double)yDelta * _dUOverZdY;
		double yVOverZ = (double)yDelta * _dVOverZdY;
		double xOneOverZ = xOffset * _dOneOverZdX;
		double xUOverZ = xOffset * _dUOverZdX;
		double xVOverZ = xOffset * _dVOverZdX;

		if (!_rd->isDemo()) {
			pEdge->oneOverZ = yOneOverZ + xOneOverZ + _aOneOverZ[vertIdx];
			pEdge->uOverZ = yUOverZ + xUOverZ + _aUOverZ[vertIdx];
			pEdge->vOverZ = yVOverZ + xVOverZ + _aVOverZ[vertIdx];
		} else {
			// The demo adds the vertex value to one of the two terms first, in a different order for each variant
			bool oneYFirst = false;
			bool uYFirst = false;
			bool vYFirst = false;

			if (kind == kEdgePerspective) {
				oneYFirst = true;
				vYFirst = true;
			}

			if (oneYFirst)
				pEdge->oneOverZ = yOneOverZ + _aOneOverZ[vertIdx] + xOneOverZ;
			else
				pEdge->oneOverZ = xOneOverZ + _aOneOverZ[vertIdx] + yOneOverZ;

			if (uYFirst)
				pEdge->uOverZ = yUOverZ + _aUOverZ[vertIdx] + xUOverZ;
			else
				pEdge->uOverZ = xUOverZ + _aUOverZ[vertIdx] + yUOverZ;

			if (vYFirst)
				pEdge->vOverZ = yVOverZ + _aVOverZ[vertIdx] + xVOverZ;
			else
				pEdge->vOverZ = xVOverZ + _aVOverZ[vertIdx] + yVOverZ;
		}
	}

	pEdge->vertIdx = nextIdx;
	return 0;
}

RenderDroid::NRasterModule::EdgeKind RenderDroid::NRasterModule::getEdgeKind(Geometry geometry, Lighting lighting, bool isLeft) {
	if (geometry == kGeometryAffine) {
		if (lighting == kLightingGouraud)
			return kEdgeAffineGouraud;

		return (lighting == kLightingFlat) ? kEdgeAffineFlat : kEdgeAffine;
	}

	if (lighting == kLightingGouraud) {
		if (geometry == kGeometryPerspective && isLeft)
			return kEdgePerspectiveGouraud;

		return kEdgeGouraud;
	}

	if (geometry == kGeometryPerspective && isLeft)
		return (lighting == kLightingFlat) ? kEdgePerspectiveFlat : kEdgePerspective;

	return kEdgeFlat;
}

void RenderDroid::NRasterModule::advanceEdge(RdNRasterEdge *pEdge, Geometry geometry, Lighting lighting, bool isLeft) {
	pEdge->x += pEdge->dx;

	if (geometry == kGeometryAffine) {
		pEdge->u += pEdge->du;
		pEdge->v += pEdge->dv;
	}

	if (geometry == kGeometryPerspective && isLeft) {
		pEdge->oneOverZ = pEdge->oneOverZ + pEdge->dOneOverZ;
		pEdge->uOverZ = pEdge->uOverZ + pEdge->dUOverZ;
		pEdge->vOverZ = pEdge->vOverZ + pEdge->dVOverZ;
	}

	if (lighting == kLightingGouraud)
		pEdge->intensity += pEdge->dIntensity;

	pEdge->y++;
}

void RenderDroid::NRasterModule::setupFace(RdProcFace *pProcFace) {
	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;
	_yMinVert = pProcFace->yMinIdx;
	_yMaxVert = pProcFace->yMaxIdx;

	if (pProcFace->bBackface) {
		_leftVertexStep = -1;
		_rightVertexStep = 1;
	} else {
		_leftVertexStep = 1;
		_rightVertexStep = -1;
	}
}

void RenderDroid::NRasterModule::setupSolidColors(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, Lighting lighting, Blend blend) {
	RdColormap *pColormap = pProcFace->pColormap;
	uint16 *pRGB16 = (uint16 *)pColormap->pRGB16;
	bool flat = lighting == kLightingFlat;
	bool gouraud = lighting == kLightingGouraud;
	bool transparent = blend == kBlendAlpha;
	int colorIndex = pCel->colorIndex;
	int lightLevel = flat ? floorToInt(light) : 0;

	if (bpp == 8) {
		_color = flat ? pColormap->pLightLevel[256 * lightLevel + colorIndex] : colorIndex;
		if (gouraud)
			_aMonoMap = pColormap->pLightLevel;
	} else if (bpp == 16) {
		if (gouraud) {
			_aRGBMap = pRGB16;
			_color = colorIndex;
			if (transparent)
				_RGBcolor = pRGB16[63 * 256 + colorIndex];
		} else if (transparent) {
			_aRGBMap = pRGB16 + 63 * 256;
			_RGBcolor = pRGB16[256 * (flat ? lightLevel : 63) + colorIndex];
		} else if (flat) {
			_RGBcolor = pRGB16[256 * lightLevel + colorIndex];
		} else {
			_RGBcolor = pRGB16[63 * 256 + (colorIndex & 0xFF)];
		}

		if (transparent) {
			_aRGBToPalette = pColormap->pRGB16Alpha;
			_color = _aRGBToPalette[_RGBcolor];
		}
	}

	if (transparent)
		_aAlphaMap = _rd->_colormap.getIdentity()->pTransparency;

	setupFace(pProcFace);
	if (gouraud)
		_aIntensities = pProcFace->aIntensities;
}

bool RenderDroid::NRasterModule::setupTexture(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, Geometry geometry, Lighting lighting, Blend blend) {
	RdColormap *pColormap = pProcFace->pColormap;
	RdTexture *pTexture = pCel->pTexture;
	uint16 *pRGB16 = (uint16 *)pColormap->pRGB16;
	bool transparent = blend == kBlendAlpha || blend == kBlendMaskedAlpha;
	bool perspective = geometry == kGeometryPerspective;
	int lightLevel = (lighting == kLightingFlat) ? floorToInt(light) : 0;

	_maxMipMap = pTexture->numMips - 1;
	_aTexVertices = pProcFace->aTexVertices;

	if (bpp == 8 && lighting == kLightingFlat)
		_aMonoMap = &pColormap->pLightLevel[256 * lightLevel];
	else if (bpp == 8 && lighting == kLightingGouraud)
		_aMonoMap = pColormap->pLightLevel;

	if (bpp == 16) {
		if (lighting == kLightingNone)
			_aRGBMap = pRGB16 + 63 * 256;
		else if (lighting == kLightingFlat)
			_aRGBMap = pRGB16 + 256 * lightLevel;
		else
			_aRGBMap = pRGB16;

		if (transparent)
			_aRGBToPalette = pColormap->pRGB16Alpha;
	}

	if (transparent)
		_aAlphaMap = _rd->_colormap.getIdentity()->pTransparency;

	setupFace(pProcFace);
	if (lighting == kLightingGouraud)
		_aIntensities = pProcFace->aIntensities;

	if (perspective)
		calcPerspGradients(pProcFace->zMin);
	else
		calcAffineGradients(pProcFace->zMin);

	if (_mipMapNum < 0 || _mipMapNum >= (int)pTexture->numMips) {
		warning("RenderDroid::NRasterModule::setupTexture(): mip level %d is not available", _mipMapNum);
		return false;
	}

	_pPixels = pTexture->apMips[_mipMapNum]->pPixels;
	_texRowShift = pTexture->widthLog2 - _mipMapNum;
	if (blend == kBlendOpaque)
		_texPitch = pTexture->apMips[_mipMapNum]->tex.pitchInBytes;

	_uWrap = (uint)pTexture->widthMask >> _mipMapNum << 16;

	if (perspective) {
		_oneOverZStep = (double)_dOneOverZdX * 16.0;
		_uOverZStep = (double)_dUOverZdX * 16.0;
		_vOverZStep = (double)_dVOverZdX * 16.0;
	}

	_vWrap = (uint)pTexture->heightMask >> _mipMapNum << _texRowShift;
	return true;
}

int RenderDroid::NRasterModule::drawNGon(RdProcFace *pProcFace, RdMatCel *pCel, float light, Geometry geometry, Lighting lighting, Blend blend) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (geometry == kGeometrySolid) {
		setupSolidColors(pProcFace, pCel, light, bpp, lighting, blend);
	} else if (!setupTexture(pProcFace, pCel, light, bpp, geometry, lighting, blend)) {
		return 0;
	}

	EdgeKind leftKind = getEdgeKind(geometry, lighting, true);
	EdgeKind rightKind = getEdgeKind(geometry, lighting, false);

	RdNRasterEdge left;
	RdNRasterEdge right;
	left.vertIdx = _yMinVert;
	int result = setupEdge(&left, _leftVertexStep, leftKind);
	if (result)
		return result;

	right.vertIdx = _yMinVert;
	result = setupEdge(&right, _rightVertexStep, rightKind);
	while (!result) {
		while (true) {
			drawScanLine(&left, &right, geometry, lighting, blend, bpp);

			if (--left.numLines) {
				advanceEdge(&left, geometry, lighting, true);
			} else {
				result = setupEdge(&left, _leftVertexStep, leftKind);
				if (result)
					return result;
			}

			if (!--right.numLines)
				break;

			advanceEdge(&right, geometry, lighting, false);
		}

		result = setupEdge(&right, _rightVertexStep, rightKind);
	}

	return result;
}

void RenderDroid::NRasterModule::drawScanLine(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Geometry geometry, Lighting lighting, Blend blend, int bpp) {
	if (bpp != 8 && bpp != 16)
		return;

	if (bpp == 8 && blend == kBlendOpaque && (geometry == kGeometryPerspective || (geometry == kGeometryAffine && lighting != kLightingNone)))
		return;

	if (geometry == kGeometrySolid)
		drawScanLineSolid(pLeft, pRight, (lighting == kLightingGouraud) ? kLightingGouraud : kLightingNone, blend, bpp);
	else if (geometry == kGeometryAffine)
		drawScanLineAffine(pLeft, pRight, lighting, blend, bpp);
	else
		drawScanLinePerspective(pLeft, pRight, lighting, blend, bpp);
}

int RenderDroid::NRasterModule::calcSpan(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, int *pXStart) {
	*pXStart = ceilToPixel(pLeft->x);
	int xEnd = ceilToPixel(pRight->x);
	return (xEnd >> 16) - (*pXStart >> 16);
}

void RenderDroid::NRasterModule::calcSpanIntensity(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, int width, int subPixel, int *pIntensity, int *pDIntensity) {
	int dIntensity = fixedMultiply(_rd->_raster.getOneOverNFixed(width), pRight->intensity - pLeft->intensity);
	if ((uint)dIntensity > 0xFFFFFAE2)
		dIntensity = 0;

	*pDIntensity = dIntensity;
	*pIntensity = pLeft->intensity + fixedMultiply(dIntensity, subPixel);
}

bool RenderDroid::NRasterModule::beginSpanPlot(SpanPlot *pPlot, int y, int xStart, Lighting lighting, Blend blend, int bpp, bool perspective, bool bufferedDestination) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	bool transparent = blend == kBlendAlpha || blend == kBlendMaskedAlpha;

	pPlot->lighting = lighting;
	pPlot->blend = blend;
	pPlot->bpp = bpp;
	pPlot->perspective = perspective;
	pPlot->pBackdrop = _pDest;
	pPlot->pDest8 = pVBuffer->pPixels + pVBuffer->tex.pitchInBytes * y + (xStart >> 16);
	pPlot->pDest16 = (uint16 *)(pVBuffer->pPixels + 2 * (xStart >> 16) + 2 * (pVBuffer->tex.pitchInBytes >> 1) * y);

	if (bufferedDestination) {
		_pDest = pPlot->pDest8;
		pPlot->pBackdrop = pPlot->pDest8;
		pPlot->pDest16 = _pWDest;
	}

	if (bpp == 16 && transparent && (bufferedDestination ? _pWDest == nullptr : _pDest == nullptr)) {
		warning("RenderDroid::NRasterModule::beginSpanPlot(): destination pointer was never set");
		return false;
	}

	return true;
}

byte RenderDroid::NRasterModule::lightTexel(Lighting lighting, byte texel, int intensity) {
	if (lighting == kLightingFlat)
		return _aMonoMap[texel];

	if (lighting == kLightingGouraud)
		return _aMonoMap[texel + ((intensity >> 8) & 0x3F00)];

	return texel;
}

void RenderDroid::NRasterModule::plotPixel(SpanPlot *pPlot, byte texel, int intensity) {
	Blend blend = pPlot->blend;
	bool masked = blend == kBlendMasked || blend == kBlendMaskedAlpha;

	if (pPlot->bpp == 8) {
		byte *pDest = pPlot->pDest8++;

		if (masked && texel == 0)
			return;

		if (blend == kBlendAlpha)
			*pDest = _aAlphaMap[256 * lightTexel(pPlot->lighting, texel, intensity) + *pDest];
		else if (blend == kBlendMaskedAlpha)
			*pDest = lightTexel(pPlot->lighting, _aAlphaMap[256 * texel + *pDest], intensity);
		else
			*pDest = lightTexel(pPlot->lighting, texel, intensity);

		return;
	}

	uint16 *pDest = pPlot->pDest16++;

	if (masked && texel == 0)
		return;

	int shade = (pPlot->lighting == kLightingGouraud) ? 256 * ((intensity >> 16) & 0x3F) : 0;

	if (blend == kBlendOpaque || blend == kBlendMasked) {
		*pDest = _aRGBMap[shade + texel];
		return;
	}

	byte backdrop = _aRGBToPalette[*pPlot->pBackdrop];

	if (blend == kBlendAlpha && pPlot->lighting == kLightingFlat && !pPlot->perspective) {
		*pDest = _aRGBMap[_aAlphaMap[256 * _aRGBToPalette[_aRGBMap[texel]] + backdrop]];
	} else if (blend == kBlendAlpha && pPlot->lighting != kLightingNone && pPlot->perspective) {
		uint16 color = _aRGBMap[shade + texel];
		byte colorIndex = 0;
		if (color < 256)
			colorIndex = _aRGBToPalette[color << 8];
		else
			warning("RenderDroid::NRasterModule::plotPixel(): palette index out of range");

		*pDest = _aRGBMap[_aAlphaMap[colorIndex + backdrop]];
	} else {
		*pDest = _aRGBMap[shade + _aAlphaMap[256 * texel + backdrop]];
	}
}

byte RenderDroid::NRasterModule::fetchTexel(int u, int v) {
	return _pPixels[((u >> 16) & (_uWrap >> 16)) | (_vWrap & (v >> (16 - _texRowShift)))];
}

byte RenderDroid::NRasterModule::fetchTexelByRows(int u, int v) {
	int index = ((u >> 16) & (_uWrap >> 16)) + (((v >> 16) & _vWrap) << _texRowShift);

	if (index > _vWrap + (1 << _texRowShift) - 1) {
		warning("RenderDroid::NRasterModule::fetchTexelByRows(): texture index out of range");
		return 0;
	}

	return _pPixels[index];
}

void RenderDroid::NRasterModule::drawScanLineSolid(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(pLeft, pRight, &xStart);
	if (width <= 0)
		return;

	int intensity = 0;
	int dIntensity = 0;
	if (lighting == kLightingGouraud)
		calcSpanIntensity(pLeft, pRight, width, xStart - pLeft->x, &intensity, &dIntensity);

	SpanPlot plot;
	if (!beginSpanPlot(&plot, pLeft->y, xStart, lighting, blend, bpp, false, false))
		return;

	if (lighting != kLightingGouraud && blend == kBlendOpaque) {
		if (bpp == 8) {
			memset(plot.pDest8, _color, width);
		} else {
			for (int i = 0; i < width; i++)
				*plot.pDest16++ = _RGBcolor;
		}
	} else {
		for (int i = 0; i < width; i++) {
			plotPixel(&plot, _color, intensity);
			intensity += dIntensity;
		}
	}

	if (bpp == 16)
		_pWDest = plot.pDest16;
}

void RenderDroid::NRasterModule::drawScanLineAffine(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(pLeft, pRight, &xStart);
	if (width <= 0)
		return;

	int subPixel = xStart - pLeft->x;
	int oneOverN = _rd->_raster.getOneOverNFixed(width);
	int uDelta = fixedMultiply(oneOverN, pRight->u - pLeft->u);
	_uStep = uDelta >> _mipMapNum;
	_fixU = (pLeft->u + fixedMultiply(uDelta, subPixel)) >> _mipMapNum;
	int vDelta = fixedMultiply(oneOverN, pRight->v - pLeft->v);
	_vStep = vDelta >> _mipMapNum;
	int vStart = (pLeft->v + fixedMultiply(vDelta, subPixel)) >> _mipMapNum;

	int intensity = 0;
	int dIntensity = 0;
	if (lighting == kLightingGouraud)
		calcSpanIntensity(pLeft, pRight, width, subPixel, &intensity, &dIntensity);

	_fixU += _dUdXModifier;
	_fixV = _dVdXModifier + vStart;

	bool rowsByShift = lighting == kLightingNone && blend == kBlendMaskedAlpha;
	bool bufferedDestination = lighting == kLightingFlat && blend == kBlendMaskedAlpha && bpp == 16;

	SpanPlot plot;
	if (!beginSpanPlot(&plot, pLeft->y, xStart, lighting, blend, bpp, false, bufferedDestination))
		return;

	for (int i = 0; i < width; i++) {
		byte texel = rowsByShift ? fetchTexelByRows(_fixU, _fixV) : fetchTexel(_fixU, _fixV);
		plotPixel(&plot, texel, intensity);
		_fixU += _uStep;
		_fixV += _vStep;
		intensity += dIntensity;
	}

	if (bpp == 8)
		_pDest = plot.pDest8;
	else
		_pWDest = plot.pDest16;
}

void RenderDroid::NRasterModule::startRun(SpanRun *pRun, float u, float v, float uStepFixed, float vStepFixed) {
	pRun->uStep = floorToInt(uStepFixed) >> _mipMapNum;
	pRun->vStep = floorToInt(vStepFixed) >> _mipMapNum;
	pRun->u = _dUdXModifier + (floorToInt(u * 65536.0f) >> _mipMapNum);
	pRun->v = _dVdXModifier + (floorToInt(v * 65536.0f) >> _mipMapNum);
}

void RenderDroid::NRasterModule::plotRun(SpanPlot *pPlot, SpanRun *pRun, int count) {
	for (int i = 0; i < count; i++) {
		plotPixel(pPlot, fetchTexel(pRun->u, pRun->v), pRun->intensity);
		pRun->u += pRun->uStep;
		pRun->v += pRun->vStep;
		pRun->intensity += pRun->dIntensity;
	}
}

void RenderDroid::NRasterModule::drawScanLinePerspective(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(pLeft, pRight, &xStart);
	if (width <= 0)
		return;

	int firstRun = 0;
	int misalign = (xStart >> 16) % 16;
	if (misalign != 0) {
		firstRun = 16 - misalign;
		if (width < firstRun)
			firstRun = width;
	}

	int numSpans = 0;
	int lastRun = 0;
	int remaining = width - firstRun;
	if (remaining != 0) {
		numSpans = remaining / 16;
		lastRun = remaining % 16;
		if (lastRun == 0) {
			numSpans--;
			lastRun = 16;
		}
	}

	SpanRun run;
	run.intensity = 0;
	run.dIntensity = 0;
	if (lighting == kLightingGouraud)
		calcSpanIntensity(pLeft, pRight, width, xStart - pLeft->x, &run.intensity, &run.dIntensity);

	bool keepsState = lighting == kLightingGouraud && blend == kBlendOpaque && bpp == 16;
	bool divideLastRun = lighting == kLightingNone;

	float fraction = (float)(xStart - pLeft->x) * 1.52587890625e-05f;
	float oneOverZ = lowAdd(lowMul(fraction, _dOneOverZdX), pLeft->oneOverZ);
	float uOverZ = lowAdd(lowMul(fraction, _dUOverZdX), pLeft->uOverZ);
	float vOverZ = lowAdd(lowMul(fraction, _dVOverZdX), pLeft->vOverZ);
	float invZ = lowDiv(1.0f, oneOverZ);
	float u = lowMul(invZ, uOverZ);
	float v = lowMul(invZ, vOverZ);

	SpanPlot plot;
	if (!beginSpanPlot(&plot, pLeft->y, xStart, lighting, blend, bpp, true, false))
		return;

	float oneOverZEnd;
	float uOverZEnd;
	float vOverZEnd;

	if (firstRun != 0) {
		float runLength = firstRun;
		float invRun = _rd->_raster.getOneOverNFlex(firstRun);
		float oneOverZRun = lowAdd(lowMul(runLength, _dOneOverZdX), oneOverZ);
		float uOverZRun = lowAdd(lowMul(runLength, _dUOverZdX), uOverZ);
		float vOverZRun = lowAdd(lowMul(runLength, _dVOverZdX), vOverZ);
		float invZRun = lowDiv(1.0f, oneOverZRun);
		float uRun = lowMul(invZRun, uOverZRun);
		float vRun = lowMul(invZRun, vOverZRun);

		startRun(&run, u, v, lowMul(lowSub(uRun, u), invRun) * 65536.0f, lowMul(lowSub(vRun, v), invRun) * 65536.0f);
		plotRun(&plot, &run, firstRun);

		u = uRun;
		v = vRun;
		oneOverZEnd = lowAdd(oneOverZRun, _oneOverZStep);
		uOverZEnd = lowAdd(uOverZRun, _uOverZStep);
		vOverZEnd = lowAdd(vOverZRun, _vOverZStep);
	} else {
		oneOverZEnd = lowAdd(oneOverZ, _oneOverZStep);
		uOverZEnd = lowAdd(uOverZ, _uOverZStep);
		vOverZEnd = lowAdd(vOverZ, _vOverZStep);

		if (keepsState) {
			run.u = _fixU;
			run.v = _fixV;
			run.uStep = _uStep;
			run.vStep = _vStep;
		} else {
			memcpy(&run.u, &oneOverZEnd, sizeof(run.u));
			run.v = run.u;
			run.uStep = run.u;
			run.vStep = run.u;
		}
	}

	for (int span = 0; span < numSpans; span++) {
		float invZSpan = lowDiv(1.0f, oneOverZEnd);
		float uEnd = lowMul(invZSpan, uOverZEnd);
		float vEnd = lowMul(invZSpan, vOverZEnd);

		startRun(&run, u, v, lowSub(uEnd, u) * 0.0625f * 65536.0f, lowSub(vEnd, v) * 0.0625f * 65536.0f);
		plotRun(&plot, &run, 16);

		u = uEnd;
		v = vEnd;
		oneOverZEnd = lowAdd(oneOverZEnd, _oneOverZStep);
		uOverZEnd = lowAdd(uOverZEnd, _uOverZStep);
		vOverZEnd = lowAdd(vOverZEnd, _vOverZStep);
	}

	if (lastRun != 0) {
		int lastOffset = lastRun - 1;
		if (lastOffset != 0) {
			float runLength = lastOffset;
			float invZLast = lowDiv(1.0f, lowAdd(lowSub(lowMul(runLength, _dOneOverZdX), _oneOverZStep), oneOverZEnd));
			float uEnd = lowMul(lowAdd(lowSub(lowMul(runLength, _dUOverZdX), _uOverZStep), uOverZEnd), invZLast);
			float vEnd = lowMul(lowAdd(lowSub(lowMul(runLength, _dVOverZdX), _vOverZStep), vOverZEnd), invZLast);
			float uStepFixed;
			float vStepFixed;

			if (divideLastRun) {
				uStepFixed = lowDiv(lowSub(uEnd, u), runLength) * 65536.0f;
				vStepFixed = lowDiv(lowSub(vEnd, v), runLength) * 65536.0f;
			} else {
				float invRun = _rd->_raster.getOneOverNFlex(lastOffset);
				uStepFixed = lowMul(lowSub(uEnd, u), invRun) * 65536.0f;
				vStepFixed = lowMul(lowSub(vEnd, v), invRun) * 65536.0f;
			}

			startRun(&run, u, v, uStepFixed, vStepFixed);
		}

		plotRun(&plot, &run, lastRun);
	}

	if (bpp == 16)
		_pWDest = plot.pDest16;

	if (keepsState) {
		_fixU = run.u;
		_fixV = run.v;
		_uStep = run.uStep;
		_vStep = run.vStep;
	}
}

int RenderDroid::NRasterModule::drawNGonLS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometrySolid, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometrySolid, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonFS(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometrySolid, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFS(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometrySolid, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonGS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometrySolid, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometrySolid, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingNone, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingNone, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryAffine, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryAffine, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryAffine, kLightingFlat, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryAffine, kLightingFlat, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingNone, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingNone, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryPerspective, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryPerspective, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryPerspective, kLightingFlat, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kGeometryPerspective, kLightingFlat, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendMaskedAlpha);
}

void RenderDroid::NRasterModule::drawScanLineNGonLS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingGouraud, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonGS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineSolid(pLeft, pRight, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingNone, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingFlat, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLineAffine(pLeft, pRight, kLightingGouraud, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingNone, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingFlat, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight) {
	drawScanLinePerspective(pLeft, pRight, kLightingGouraud, kBlendMaskedAlpha, 16);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGS(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeGouraud);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeAffine);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeAffine);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeAffineFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeAffineFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgeAffineGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGAT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeAffineGouraud);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgePerspective);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgePerspectiveFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _leftVertexStep, kEdgePerspectiveGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGIT(RdNRasterEdge *pEdge) {
	return setupEdge(pEdge, _rightVertexStep, kEdgeGouraud);
}

} // End of namespace Grim
