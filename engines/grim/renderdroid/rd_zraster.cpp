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

RenderDroid::ZRasterModule::ZRasterModule(RenderDroid *rd) : Module(rd), _numVertices(0), _maxMipMap(0), _yMinVert(0), _yMaxVert(0),
		_leftVertexStep(0), _rightVertexStep(0), _aIntensities(nullptr), _aVertices(nullptr), _aTexVertices(nullptr),
		_aRGBToPalette(nullptr), _aRGBMap(nullptr), _aMonoMap(nullptr), _aAlphaMap(nullptr), _pPixels(nullptr),
		_uWrap(0), _vWrap(0), _mipMapNum(0), _texPitch(0), _texRowShift(0), _zScaleShift(0), _dUdXModifier(0),
		_dVdXModifier(0), _dOneOverZdXFixed(0), _oneOverZStep(0.0f), _uOverZStep(0.0f), _vOverZStep(0.0f),
		_dOneOverZdX(0.0f), _dOneOverZdY(0.0f), _dUOverZdX(0.0f), _dUOverZdY(0.0f), _dVOverZdX(0.0f),
		_dVOverZdY(0.0f), _RGBcolor(0), _color(0), _caseU(0), _caseV(0), _caseUStep(0), _caseVStep(0) {
	memset(_aOneOverZ, 0, sizeof(_aOneOverZ));
	memset(_aUOverZ, 0, sizeof(_aUOverZ));
	memset(_aVOverZ, 0, sizeof(_aVOverZ));
}

bool RenderDroid::ZRasterModule::setZScaleShift() {
	_zScaleShift = _rd->_camera.getCurrent()->pCanvas->zScaleShift;

	if (_zScaleShift < 0 || _zScaleShift > 31) {
		warning("RenderDroid::ZRasterModule::setZScaleShift(): invalid z scale shift %d", _zScaleShift);
		return false;
	}

	return true;
}

float RenderDroid::ZRasterModule::truncToFloat(double value) {
	float result = (float)value;

	if (fabs((double)result) > fabs(value))
		result = nextafterf(result, 0.0f);

	return result;
}

int RenderDroid::ZRasterModule::toFixed(double value) {
	return (int)floor((float)(value * 65536.0));
}

int RenderDroid::ZRasterModule::wrapVertex(int index, int last) {
	if (index < 0)
		return last;

	if (index > last)
		return 0;

	return index;
}

void RenderDroid::ZRasterModule::selectGradientVertices(int *pVertA, int *pVertB, int *pRefVert) {
	*pVertA = 0;
	*pVertB = 1;
	*pRefVert = 2;

	if (_numVertices == 3)
		return;

	int last = (int)_numVertices - 1;
	int direction;
	int refVert = -1;
	int nextVert = wrapVertex(_yMinVert + 1, last);

	if (nextVert == _yMaxVert) {
		direction = -1;
	} else if (wrapVertex(_yMinVert - 1, last) == _yMaxVert) {
		direction = 1;
	} else {
		refVert = nextVert;
		double slope = ((double)_aVertices[_yMaxVert][1] - _aVertices[_yMinVert][1]) / ((double)_aVertices[_yMaxVert][0] - _aVertices[_yMinVert][0]);
		double deviation = _aVertices[nextVert][1] - (slope * ((double)_aVertices[nextVert][0] - _aVertices[_yMinVert][0]) + _aVertices[_yMinVert][1]);
		if (deviation < 0.0)
			deviation = -deviation;
		direction = (deviation >= 1.0) ? 1 : -1;
	}

	float maxDeviation = 0.0f;
	int firstVert = wrapVertex(_yMinVert + direction, last);
	for (int vert = firstVert; vert != _yMaxVert; vert = wrapVertex(vert + direction, last)) {
		float midX = (float)(((double)_aVertices[_yMinVert][0] + _aVertices[_yMaxVert][0]) * 0.5);
		double deviation = (double)_aVertices[vert][0] - midX;
		if (deviation < 0.0)
			deviation = -deviation;
		if (deviation > maxDeviation) {
			maxDeviation = (float)deviation;
			refVert = vert;
		}
	}

	if (refVert < 0) {
		warning("RenderDroid::ZRasterModule::selectGradientVertices(): no reference vertex found");
		refVert = firstVert;
	}

	*pVertA = _yMinVert;
	*pVertB = _yMaxVert;
	*pRefVert = refVert;
}

void RenderDroid::ZRasterModule::selectMipLevel(float z) {
	switch (_maxMipMap) {
	case 0:
		_mipMapNum = 0;
		break;
	case 1:
		_mipMapNum = (z > (double)_rd->_aMipDistances[1]) ? 1 : 0;
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
		warning("RenderDroid::ZRasterModule::selectMipLevel(): invalid mip level count");
		break;
	}
}

int RenderDroid::ZRasterModule::gradientModifier(double dX, double dY) {
	if (dX > 0.0)
		return 0x8000;

	if (dX < 0.0)
		return 0x7FFF;

	return (dY < 0.0) ? 0x7FFF : 0x8000;
}

void RenderDroid::ZRasterModule::calcAffineGradients(float z) {
	int vertA;
	int vertB;
	int refVert;

	selectGradientVertices(&vertA, &vertB, &refVert);

	int numVertices = (int)_numVertices;
	if (numVertices > 32) {
		warning("RenderDroid::ZRasterModule::calcAffineGradients(): too many vertices");
		numVertices = 32;
	}

	for (int i = 0; i < numVertices; i++)
		_aOneOverZ[i] = (float)(1.0 / ((double)_aVertices[i][2] + 1.0));

	double dxA = (double)_aVertices[vertA][0] - _aVertices[refVert][0];
	double dxB = (double)_aVertices[vertB][0] - _aVertices[refVert][0];
	double dyA = (double)_aVertices[vertA][1] - _aVertices[refVert][1];
	double dyB = (double)_aVertices[vertB][1] - _aVertices[refVert][1];
	float xA = (float)dxA;
	float xB = (float)dxB;
	double duA = (double)_aTexVertices[vertA][0] - _aTexVertices[refVert][0];
	double duB = (double)_aTexVertices[vertB][0] - _aTexVertices[refVert][0];
	double dvB = (double)_aTexVertices[vertB][1] - _aTexVertices[refVert][1];
	float uA = (float)duA;
	float uB = (float)duB;
	float vB = (float)dvB;
	float vA = (float)((double)_aTexVertices[vertA][1] - _aTexVertices[refVert][1]);
	double det = 1.0 / (dyA * xB - dyB * xA);
	float invDet = (float)det;
	float negInvDet = (float)-det;
	float dUdX = (float)((duB * dyA - duA * dyB) * invDet);
	float dUdY = (float)(((double)uB * xA - (double)uA * xB) * negInvDet);
	float dVdX = (float)((vB * dyA - vA * dyB) * invDet);
	float dVdY = (float)(((double)vB * xA - (double)vA * xB) * negInvDet);

	selectMipLevel(z);
	_dUdXModifier = gradientModifier(dUdX, dUdY);
	_dVdXModifier = gradientModifier(dVdX, dVdY);
}

void RenderDroid::ZRasterModule::calcPerspGradients(float z) {
	int vertA;
	int vertB;
	int refVert;

	selectGradientVertices(&vertA, &vertB, &refVert);

	int numVertices = (int)_numVertices;
	if (numVertices > 32) {
		warning("RenderDroid::ZRasterModule::calcPerspGradients(): too many vertices");
		numVertices = 32;
	}

	for (int i = 0; i < numVertices; i++) {
		double oneOverZ = 1.0 / ((double)_aVertices[i][2] + 1.0);
		_aOneOverZ[i] = (float)oneOverZ;
		_aUOverZ[i] = (float)(_aTexVertices[i][0] * oneOverZ);
		_aVOverZ[i] = (float)((double)_aTexVertices[i][1] * _aOneOverZ[i]);
	}

	double dyA = (double)_aVertices[vertA][1] - _aVertices[refVert][1];
	double dyB = (double)_aVertices[vertB][1] - _aVertices[refVert][1];
	float dxA = (float)((double)_aVertices[vertA][0] - _aVertices[refVert][0]);
	float dxB = (float)((double)_aVertices[vertB][0] - _aVertices[refVert][0]);
	float dOneA = (float)((double)_aOneOverZ[vertA] - _aOneOverZ[refVert]);
	float dOneB = (float)((double)_aOneOverZ[vertB] - _aOneOverZ[refVert]);
	float dUA = (float)((double)_aUOverZ[vertA] - _aUOverZ[refVert]);
	float dUB = (float)((double)_aUOverZ[vertB] - _aUOverZ[refVert]);
	float dVA = (float)((double)_aVOverZ[vertA] - _aVOverZ[refVert]);
	float dVB = (float)((double)_aVOverZ[vertB] - _aVOverZ[refVert]);
	float invDet = (float)(1.0 / (dyA * dxB - dyB * dxA));
	float negInvDet = -invDet;

	double dOneOverZdX = (dOneB * dyA - dOneA * dyB) * invDet;
	_dOneOverZdX = (float)dOneOverZdX;
	_dOneOverZdY = (float)(((double)dOneB * dxA - (double)dOneA * dxB) * negInvDet);
	_dOneOverZdXFixed = (int)(int64)(dOneOverZdX * 65536.0);
	_dUOverZdX = (float)((dUB * dyA - dUA * dyB) * invDet);
	_dUOverZdY = (float)(((double)dUB * dxA - (double)dUA * dxB) * negInvDet);
	_dVOverZdX = (float)((dVB * dyA - dVA * dyB) * invDet);
	_dVOverZdY = (float)(((double)dVB * dxA - (double)dVA * dxB) * negInvDet);

	selectMipLevel(z);

	double uOverZ = _aUOverZ[vertA];
	double vOverZ = _aVOverZ[vertA];
	double oneOverZ = _aOneOverZ[vertA];
	_dUdXModifier = gradientModifier(oneOverZ * _dUOverZdX - uOverZ * _dOneOverZdX, oneOverZ * _dUOverZdY - uOverZ * _dOneOverZdY);
	_dVdXModifier = gradientModifier(oneOverZ * _dVOverZdX - vOverZ * _dOneOverZdX, oneOverZ * _dVOverZdY - vOverZ * _dOneOverZdY);
}

bool RenderDroid::ZRasterModule::findEdgeSegment(RdZRasterEdge *pEdge, int vertexStep, uint *pVertIdx, uint *pNextIdx, float *pYStart, int *pNumLines) {
	uint vertIdx = pEdge->vertIdx;
	if (vertIdx == (uint)_yMaxVert)
		return false;

	while (true) {
		uint nextIdx = (uint)wrapVertex((int)vertIdx + vertexStep, (int)_numVertices - 1);
		float yStart = (float)ceil(_aVertices[vertIdx][1]);
		int numLines = (int)ceil(_aVertices[nextIdx][1]) - (int)yStart;

		pEdge->y = (int)yStart;
		pEdge->numLines = numLines;

		if (numLines > 0) {
			*pVertIdx = vertIdx;
			*pNextIdx = nextIdx;
			*pYStart = yStart;
			*pNumLines = numLines;
			return true;
		}

		vertIdx = nextIdx;
		if (nextIdx == (uint)_yMaxVert)
			return false;
	}
}

int RenderDroid::ZRasterModule::interpolateEdgeValue(float from, float to, float invDy, float yOffset, int *pStep) {
	double delta = ((double)to - from) * invDy;
	float deltaStored = (float)delta;

	*pStep = toFixed(delta);

	return toFixed((double)deltaStored * yOffset + from);
}

int RenderDroid::ZRasterModule::setupEdgeLinear(RdZRasterEdge *pEdge, int vertexStep, bool textured, bool gouraud) {
	uint vertIdx;
	uint nextIdx;
	float yStart;
	int numLines;

	if (!findEdgeSegment(pEdge, vertexStep, &vertIdx, &nextIdx, &yStart, &numLines))
		return 1;

	if (numLines >= 2) {
		float yOffset = (float)((double)yStart - _aVertices[vertIdx][1]);
		float invDy = (float)(1.0 / ((double)_aVertices[nextIdx][1] - _aVertices[vertIdx][1]));

		pEdge->x = interpolateEdgeValue(_aVertices[vertIdx][0], _aVertices[nextIdx][0], invDy, yOffset, &pEdge->dx);

		if (textured) {
			pEdge->u = interpolateEdgeValue(_aTexVertices[vertIdx][0], _aTexVertices[nextIdx][0], invDy, yOffset, &pEdge->du);
			pEdge->v = interpolateEdgeValue(_aTexVertices[vertIdx][1], _aTexVertices[nextIdx][1], invDy, yOffset, &pEdge->dv);
		}

		if (gouraud)
			pEdge->intensity = interpolateEdgeValue(_aIntensities[vertIdx], _aIntensities[nextIdx], invDy, yOffset, &pEdge->dIntensity);

		double dOneOverZ = ((double)_aOneOverZ[nextIdx] - _aOneOverZ[vertIdx]) * invDy;
		pEdge->dOneOverZ = (float)dOneOverZ;
		pEdge->oneOverZ = (float)(yOffset * dOneOverZ + _aOneOverZ[vertIdx]);
	} else {
		pEdge->x = toFixed(_aVertices[vertIdx][0]);

		if (textured) {
			pEdge->u = toFixed(_aTexVertices[vertIdx][0]);
			pEdge->v = toFixed(_aTexVertices[vertIdx][1]);
		}

		if (gouraud)
			pEdge->intensity = toFixed(_aIntensities[vertIdx]);

		pEdge->oneOverZ = _aOneOverZ[vertIdx];
	}

	pEdge->vertIdx = nextIdx;

	return 0;
}

int RenderDroid::ZRasterModule::setupLeftEdgePerspective(RdZRasterEdge *pEdge, bool gouraud) {
	uint vertIdx;
	uint nextIdx;
	float yStart;
	int numLines;

	if (!findEdgeSegment(pEdge, _leftVertexStep, &vertIdx, &nextIdx, &yStart, &numLines))
		return 1;

	float yOffset = (float)((double)yStart - _aVertices[vertIdx][1]);
	float xAtStart;
	float intensityFixed = 0.0f;

	if (numLines >= 2) {
		double dx = (double)_aVertices[nextIdx][0] - _aVertices[vertIdx][0];
		double dy = (double)_aVertices[nextIdx][1] - _aVertices[vertIdx][1];
		float invDy = 0.0f;
		double slope;

		if (gouraud) {
			invDy = (float)(1.0 / dy);
			slope = dx * invDy;
		} else {
			slope = dx / dy;
		}

		float slopeStored = (float)slope;
		double xValue = (double)slopeStored * yOffset + _aVertices[vertIdx][0];

		xAtStart = (float)xValue;
		pEdge->dx = toFixed(slope);
		pEdge->x = toFixed(xValue);
		pEdge->dOneOverZ = (float)((double)slopeStored * _dOneOverZdX + _dOneOverZdY);
		pEdge->dUOverZ = (float)((double)slopeStored * _dUOverZdX + _dUOverZdY);
		pEdge->dVOverZ = (float)((double)slopeStored * _dVOverZdX + _dVOverZdY);

		if (gouraud) {
			double dIntensity = ((double)_aIntensities[nextIdx] - _aIntensities[vertIdx]) * invDy;
			float dIntensityStored = (float)dIntensity;
			pEdge->dIntensity = toFixed(dIntensity);
			intensityFixed = (float)(((double)dIntensityStored * yOffset + _aIntensities[vertIdx]) * 65536.0);
		}
	} else {
		xAtStart = _aVertices[vertIdx][0];
		pEdge->x = toFixed(xAtStart);
		if (gouraud)
			intensityFixed = (float)(_aIntensities[vertIdx] * 65536.0);
	}

	if (gouraud)
		pEdge->intensity = (int)floor(intensityFixed);

	double xDelta = (double)xAtStart - _aVertices[vertIdx][0];
	pEdge->oneOverZ = (float)((double)yOffset * _dOneOverZdY + _aOneOverZ[vertIdx] + xDelta * _dOneOverZdX);
	pEdge->uOverZ = (float)((double)yOffset * _dUOverZdY + _aUOverZ[vertIdx] + xDelta * _dUOverZdX);
	pEdge->vOverZ = (float)(xDelta * _dVOverZdX + _aVOverZ[vertIdx] + (double)yOffset * _dVOverZdY);
	pEdge->vertIdx = nextIdx;

	return 0;
}

int RenderDroid::ZRasterModule::setupRightEdgePerspective(RdZRasterEdge *pEdge, bool gouraud) {
	uint vertIdx;
	uint nextIdx;
	float yStart;
	int numLines;

	if (!findEdgeSegment(pEdge, _rightVertexStep, &vertIdx, &nextIdx, &yStart, &numLines))
		return 1;

	float xFixed;
	float intensityFixed = 0.0f;

	if (numLines >= 2) {
		float yOffset = (float)((double)yStart - _aVertices[vertIdx][1]);
		double dx = (double)_aVertices[nextIdx][0] - _aVertices[vertIdx][0];
		double dy = (double)_aVertices[nextIdx][1] - _aVertices[vertIdx][1];
		float invDy = 0.0f;
		double slope;

		if (gouraud) {
			invDy = (float)(1.0 / dy);
			slope = dx * invDy;
		} else {
			slope = dx / dy;
		}

		float slopeStored = (float)slope;
		pEdge->dx = toFixed(slope);
		xFixed = (float)(((double)slopeStored * yOffset + _aVertices[vertIdx][0]) * 65536.0);

		if (gouraud) {
			double dIntensity = ((double)_aIntensities[nextIdx] - _aIntensities[vertIdx]) * invDy;
			float dIntensityStored = (float)dIntensity;
			pEdge->dIntensity = toFixed(dIntensity);
			intensityFixed = (float)(((double)dIntensityStored * yOffset + _aIntensities[vertIdx]) * 65536.0);
		}
	} else {
		xFixed = (float)(_aVertices[vertIdx][0] * 65536.0);
		if (gouraud)
			intensityFixed = (float)(_aIntensities[vertIdx] * 65536.0);
	}

	pEdge->vertIdx = nextIdx;
	pEdge->x = (int)floor(xFixed);

	if (gouraud)
		pEdge->intensity = (int)floor(intensityFixed);

	return 0;
}

int RenderDroid::ZRasterModule::setupLeftEdge(RdZRasterEdge *pEdge, int mapping, int shading) {
	bool gouraud = (shading == kGouraud);

	if (mapping == kPerspective)
		return setupLeftEdgePerspective(pEdge, gouraud);

	return setupEdgeLinear(pEdge, _leftVertexStep, mapping == kAffine, gouraud);
}

int RenderDroid::ZRasterModule::setupRightEdge(RdZRasterEdge *pEdge, int mapping, int shading) {
	bool gouraud = (shading == kGouraud);

	if (mapping == kPerspective)
		return setupRightEdgePerspective(pEdge, gouraud);

	return setupEdgeLinear(pEdge, _rightVertexStep, mapping == kAffine, gouraud);
}

void RenderDroid::ZRasterModule::stepEdge(RdZRasterEdge *pEdge, int mapping, int shading, bool isLeft) {
	if (mapping == kPerspective) {
		if (isLeft) {
			pEdge->oneOverZ += pEdge->dOneOverZ;
			pEdge->uOverZ += pEdge->dUOverZ;
			pEdge->vOverZ += pEdge->dVOverZ;
		}
	} else {
		pEdge->oneOverZ += pEdge->dOneOverZ;
	}

	pEdge->x += pEdge->dx;

	if (mapping == kAffine) {
		pEdge->u += pEdge->du;
		pEdge->v += pEdge->dv;
	}

	if (shading == kGouraud)
		pEdge->intensity += pEdge->dIntensity;

	pEdge->y++;
}

bool RenderDroid::ZRasterModule::beginScanLine(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int bpp, ScanState *pState) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	StdVBuffer *pVBuffer = pCanvas->pVBuffer;
	StdVBuffer *pZBuffer = pCanvas->pZBuffer;

	pState->xLeft = pLeft->x;
	pState->xStart = pState->xLeft;
	if ((pState->xLeft & 0xFFFF) != 0)
		pState->xStart = (int)(((uint)pState->xLeft + 0x10000) & 0xFFFF0000);

	uint xEnd = pRight->x;
	if ((xEnd & 0xFFFF) != 0)
		xEnd = (xEnd + 0x10000) & 0xFFFF0000;

	int x = pState->xStart >> 16;
	pState->x = x;
	pState->numPixels = (int)(xEnd >> 16) - x;
	if (pState->numPixels <= 0)
		return false;

	if (bpp == 8)
		pState->pDest = &pVBuffer->pPixels[pLeft->y * pVBuffer->tex.pitchInBytes + x];
	else
		pState->pDest = &pVBuffer->pPixels[2 * x + 2 * pLeft->y * (pVBuffer->tex.pitchInBytes >> 1)];

	pState->pZ = (uint16 *)&pZBuffer->pPixels[2 * x + pLeft->y * pZBuffer->tex.pitchInBytes];
	pState->u = 0;
	pState->v = 0;
	pState->uStep = 0;
	pState->vStep = 0;
	pState->intensity = 0;
	pState->intensityStep = 0;
	pState->bDither = false;
	pState->ditherX = 0;
	pState->ditherY = pLeft->y;

	return true;
}

void RenderDroid::ZRasterModule::setupLinearZ(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, ScanState *pState) {
	double dOneOverZ = ((double)pRight->oneOverZ - pLeft->oneOverZ) * _rd->_raster.getOneOverNFlex(pState->numPixels);
	float dOneOverZStored = (float)dOneOverZ;
	double fraction = (double)(pState->xStart - pState->xLeft) * 0.0000152587890625;

	pState->z = (int)floor((float)(((double)dOneOverZStored * fraction + pLeft->oneOverZ) * 65536.0));
	pState->zStep = (int)floor((float)(dOneOverZ * 65536.0));
}

void RenderDroid::ZRasterModule::setupLinearIntensity(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, ScanState *pState) {
	int oneOverN = _rd->_raster.getOneOverNFixed(pState->numPixels);
	int xOffset = pState->xStart - pState->xLeft;

	pState->intensityStep = (int)((uint64)((int64)oneOverN * (pRight->intensity - pLeft->intensity)) >> 16);
	if ((uint)pState->intensityStep > 0xFFFFFAE2)
		pState->intensityStep = 0;

	pState->intensity = pLeft->intensity + (int)((uint64)((int64)pState->intensityStep * xOffset) >> 16);
}

int RenderDroid::ZRasterModule::lightIndex(int index, bool gouraud) {
	if (gouraud && (index < 0 || index > 0x3FFF)) {
		warning("RenderDroid::ZRasterModule::lightIndex(): light table index out of range");
		return (index < 0) ? 0 : 0x3FFF;
	}

	return index;
}

int RenderDroid::ZRasterModule::readRGB(int index) {
	return *(uint16 *)&_aRGBMap[2 * index];
}

bool RenderDroid::ZRasterModule::plotTexel(const PixelMode &mode, byte *pDest, byte texel, int lightBase) {
	bool transparent = (mode.flags & kTransparent) != 0;
	bool masked = (mode.flags & kMasked) != 0;
	bool gouraud = (mode.shading == kGouraud);

	if (masked && texel == 0)
		return false;

	if (mode.bpp == 8) {
		if (mode.shading == kUnlit) {
			*pDest = transparent ? _aAlphaMap[256 * texel + *pDest] : texel;
		} else if (transparent && masked) {
			*pDest = _aMonoMap[lightIndex(_aAlphaMap[256 * texel + *pDest] + lightBase, gouraud)];
		} else {
			byte lit = _aMonoMap[lightIndex(texel + lightBase, gouraud)];
			*pDest = transparent ? _aAlphaMap[256 * lit + *pDest] : lit;
		}

		return true;
	}

	uint16 *pDest16 = (uint16 *)pDest;

	if (!transparent) {
		*pDest16 = readRGB(lightIndex(lightBase + texel, gouraud));
	} else if (!masked && mode.shading == kFlat && !mode.perspective) {
		*pDest16 = readRGB(_aAlphaMap[256 * _aRGBToPalette[readRGB(texel)] + _aRGBToPalette[*pDest16]]);
	} else if (!masked && mode.shading != kUnlit && mode.perspective) {
		uint litIndex = (uint)readRGB(lightIndex(lightBase + texel, gouraud)) << 8;
		byte litPalette = 0;

		if (litIndex < 0x10000)
			litPalette = _aRGBToPalette[litIndex];
		else
			warning("RenderDroid::ZRasterModule::plotTexel(): palette index out of range");

		*pDest16 = readRGB(_aAlphaMap[litPalette + _aRGBToPalette[*pDest16]]);
	} else {
		*pDest16 = readRGB(lightIndex(lightBase + _aAlphaMap[256 * texel + _aRGBToPalette[*pDest16]], gouraud));
	}

	return true;
}

void RenderDroid::ZRasterModule::drawTextureRun(int count, ScanState *pState, const ScanParams &params) {
	const int aDither[4] = { 0, 49152, 0x8000, 0x4000 };
	int bytesPerPixel = params.mode.bpp / 8;
	bool gouraud = (params.mode.shading == kGouraud);

	for (int i = 0; i < count; i++) {
		int zShifted = pState->z >> _zScaleShift;

		if ((uint16)zShifted > *pState->pZ) {
			byte texel = _pPixels[((pState->u & params.uMask) >> 16) | (_vWrap & (pState->v >> params.vShift))];
			int lightBase = 0;

			if (gouraud) {
				if (pState->bDither)
					lightBase = ((pState->intensity + aDither[2 * (pState->ditherY % 2) + pState->ditherX % 2]) >> 8) & ~0xFF;
				else
					lightBase = (pState->intensity >> 8) & params.lightMask;
			}

			if (plotTexel(params.mode, pState->pDest, texel, lightBase))
				*pState->pZ = (uint16)zShifted;
		}

		pState->u += pState->uStep;
		pState->v += pState->vStep;
		pState->intensity += pState->intensityStep;
		pState->z += pState->zStep;
		pState->ditherX--;
		pState->pDest += bytesPerPixel;
		pState->pZ++;
	}
}

void RenderDroid::ZRasterModule::scanLineSolid(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int bpp, int shading, int flags) {
	ScanState state;

	if (!beginScanLine(pLeft, pRight, bpp, &state))
		return;

	bool gouraud = (shading == kGouraud);
	bool transparent = (flags & kTransparent) != 0;

	setupLinearZ(pLeft, pRight, &state);
	if (gouraud)
		setupLinearIntensity(pLeft, pRight, &state);

	for (int i = 0; i < state.numPixels; i++) {
		int zShifted = state.z >> _zScaleShift;

		if ((uint16)zShifted > *state.pZ) {
			int lightBase = gouraud ? ((state.intensity >> 8) & 0x3F00) : 0;

			if (bpp == 8) {
				byte lit = gouraud ? _aMonoMap[lightBase + _color] : _color;
				*state.pDest = transparent ? _aAlphaMap[256 * lit + *state.pDest] : lit;
			} else {
				uint16 *pDest16 = (uint16 *)state.pDest;

				if (!transparent)
					*pDest16 = gouraud ? readRGB(lightBase + _color) : _RGBcolor;
				else
					*pDest16 = readRGB(lightBase + _aAlphaMap[256 * _color + _aRGBToPalette[*pDest16]]);
			}

			*state.pZ = (uint16)zShifted;
		}

		state.intensity += state.intensityStep;
		state.z += state.zStep;
		state.pDest += bpp / 8;
		state.pZ++;
	}
}

RenderDroid::ZRasterModule::ScanParams RenderDroid::ZRasterModule::makeScanParams(int bpp, int shading, int flags, bool perspective) {
	ScanParams params;

	params.mode.bpp = bpp;
	params.mode.shading = shading;
	params.mode.flags = flags;
	params.mode.perspective = perspective;
	params.uMask = _uWrap;
	params.vShift = 16 - _texRowShift;
	params.lightMask = 0x3F00;
	params.caseMode = false;

	return params;
}

void RenderDroid::ZRasterModule::scanLineAffine(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, const ScanParams &params) {
	ScanState state;

	if (!beginScanLine(pLeft, pRight, params.mode.bpp, &state))
		return;

	int oneOverN = _rd->_raster.getOneOverNFixed(state.numPixels);
	int xOffset = state.xStart - state.xLeft;
	int uStepFull = (int)((uint64)((int64)oneOverN * (pRight->u - pLeft->u)) >> 16);
	int vStepFull = (int)((uint64)((int64)oneOverN * (pRight->v - pLeft->v)) >> 16);

	state.uStep = uStepFull >> _mipMapNum;
	state.vStep = vStepFull >> _mipMapNum;
	state.u = _dUdXModifier + ((int)((uint)pLeft->u + (uint)((uint64)((int64)uStepFull * xOffset) >> 16)) >> _mipMapNum);
	state.v = _dVdXModifier + ((int)((uint)pLeft->v + (uint)((uint64)((int64)vStepFull * xOffset) >> 16)) >> _mipMapNum);

	setupLinearZ(pLeft, pRight, &state);
	if (params.mode.shading == kGouraud)
		setupLinearIntensity(pLeft, pRight, &state);

	if (params.mode.shading == kGouraud && params.mode.bpp == 16 && params.mode.flags == 0 && !params.caseMode) {
		state.bDither = true;
		state.ditherX = state.x + state.numPixels - 1;
	}

	drawTextureRun(state.numPixels, &state, params);
}

void RenderDroid::ZRasterModule::setRunSteps(ScanState *pState, float uEnd, float vEnd, float uStart, float vStart, double stepScale) {
	pState->uStep = (int)floor((float)(((double)uEnd - uStart) * stepScale * 65536.0)) >> _mipMapNum;
	pState->vStep = (int)floor((float)(((double)vEnd - vStart) * stepScale * 65536.0)) >> _mipMapNum;
	pState->u = _dUdXModifier + ((int)floor((float)(uStart * 65536.0)) >> _mipMapNum);
	pState->v = _dVdXModifier + ((int)floor((float)(vStart * 65536.0)) >> _mipMapNum);
}

void RenderDroid::ZRasterModule::scanLinePerspective(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, const ScanParams &params) {
	ScanState state;

	if (!beginScanLine(pLeft, pRight, params.mode.bpp, &state))
		return;

	int numPixels = state.numPixels;
	int headPixels = 0;

	if (state.x % 16 != 0) {
		headPixels = 16 - state.x % 16;
		if (numPixels < headPixels)
			headPixels = numPixels;
	}

	int restPixels = numPixels - headPixels;
	int numSpans = 0;
	int tailPixels = 0;

	if (restPixels != 0) {
		numSpans = restPixels / 16;
		tailPixels = restPixels % 16;
		if (tailPixels == 0) {
			tailPixels = 16;
			numSpans--;
		}
	}

	if (params.mode.shading == kGouraud)
		setupLinearIntensity(pLeft, pRight, &state);

	double fraction = (double)(state.xStart - state.xLeft) * 0.0000152587890625;
	float oneOverZ = truncToFloat(truncToFloat(fraction * _dOneOverZdX) + pLeft->oneOverZ);
	float uOverZ = truncToFloat(truncToFloat(fraction * _dUOverZdX) + pLeft->uOverZ);
	float vOverZ = truncToFloat(truncToFloat(fraction * _dVOverZdX) + pLeft->vOverZ);
	double invOneOverZ = 1.0 / oneOverZ;
	float uStart = (float)(invOneOverZ * uOverZ);
	float vStart = (float)(invOneOverZ * vOverZ);

	state.z = (int)floor((float)(oneOverZ * 65536.0));
	state.zStep = _dOneOverZdXFixed;

	if (params.caseMode) {
		state.u = _caseU;
		state.v = _caseV;
		state.uStep = _caseUStep;
		state.vStep = _caseVStep;
	}

	float oneOverZNext;
	float uOverZNext;
	float vOverZNext;

	if (headPixels != 0) {
		double head = (double)headPixels;
		float oneOverZEnd = (float)(head * _dOneOverZdX + oneOverZ);
		float uOverZEnd = (float)(head * _dUOverZdX + uOverZ);
		float vOverZEnd = (float)(head * _dVOverZdX + vOverZ);
		double invEnd = 1.0 / oneOverZEnd;
		float uEnd = (float)(invEnd * uOverZEnd);
		float vEnd = (float)(invEnd * vOverZEnd);

		setRunSteps(&state, uEnd, vEnd, uStart, vStart, _rd->_raster.getOneOverNFlex(headPixels));
		drawTextureRun(headPixels, &state, params);

		uStart = uEnd;
		vStart = vEnd;
		oneOverZNext = oneOverZEnd + _oneOverZStep;
		uOverZNext = uOverZEnd + _uOverZStep;
		vOverZNext = vOverZEnd + _vOverZStep;
	} else {
		if (!params.caseMode) {
			state.u = (params.mode.shading == kGouraud) ? state.intensity : state.z;
			state.v = state.u;
		}

		oneOverZNext = oneOverZ + _oneOverZStep;
		uOverZNext = uOverZ + _uOverZStep;
		vOverZNext = vOverZ + _vOverZStep;
	}

	for (int span = numSpans; span > 0; span--) {
		double invNext = 1.0 / oneOverZNext;
		float uEnd = (float)(invNext * uOverZNext);
		float vEnd = (float)(invNext * vOverZNext);

		setRunSteps(&state, uEnd, vEnd, uStart, vStart, 0.0625);
		drawTextureRun(16, &state, params);

		uStart = uEnd;
		vStart = vEnd;
		oneOverZNext += _oneOverZStep;
		uOverZNext += _uOverZStep;
		vOverZNext += _vOverZStep;
	}

	if (tailPixels != 0) {
		if (tailPixels != 1) {
			double tail = (double)(tailPixels - 1);
			double invTail = 1.0 / (tail * _dOneOverZdX - _oneOverZStep + oneOverZNext);
			float uEnd = (float)((tail * _dUOverZdX - _uOverZStep + uOverZNext) * invTail);
			float vEnd = (float)((tail * _dVOverZdX - _vOverZStep + vOverZNext) * invTail);

			setRunSteps(&state, uEnd, vEnd, uStart, vStart, _rd->_raster.getOneOverNFlex(tailPixels - 1));
		}

		drawTextureRun(tailPixels, &state, params);
	}

	if (params.caseMode) {
		_caseU = state.u;
		_caseV = state.v;
		_caseUStep = state.uStep;
		_caseVStep = state.vStep;
	}
}

void RenderDroid::ZRasterModule::scanLineCase(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int mapping, int shading, int pitch) {
	ScanParams params = makeScanParams(16, shading, 0, mapping == kPerspective);

	params.caseMode = true;

	if (pitch != 0) {
		int shift = 16;
		for (int size = pitch; size > 1; size >>= 1)
			shift--;

		params.uMask = (pitch - 1) << 16;
		params.vShift = shift;
	} else if (shading == kGouraud) {
		params.lightMask = -1;
	}

	if (mapping == kPerspective)
		scanLinePerspective(pLeft, pRight, params);
	else
		scanLineAffine(pLeft, pRight, params);
}

bool RenderDroid::ZRasterModule::setupGeometry(RdProcFace *pProcFace) {
	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;
	_yMinVert = pProcFace->yMinIdx;
	_yMaxVert = pProcFace->yMaxIdx;

	if (!setZScaleShift())
		return false;

	if (pProcFace->bBackface) {
		_leftVertexStep = -1;
		_rightVertexStep = 1;
	} else {
		_leftVertexStep = 1;
		_rightVertexStep = -1;
	}

	return true;
}

bool RenderDroid::ZRasterModule::setupSolidFace(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, int shading, int flags) {
	RdColormap *pColormap = pProcFace->pColormap;
	bool transparent = (flags & kTransparent) != 0;
	int colorIndex = pCel->colorIndex;

	if (shading == kUnlit && !transparent) {
		if (bpp == 8)
			_color = colorIndex;
		else if (bpp == 16)
			_RGBcolor = *(uint16 *)&pColormap->pRGB16[2 * (byte)colorIndex + 32256];
	} else if (shading == kUnlit) {
		if (bpp == 8) {
			_color = (byte)colorIndex;
		} else if (bpp == 16) {
			_aRGBMap = pColormap->pRGB16 + 32256;
			_RGBcolor = *(uint16 *)&pColormap->pRGB16[2 * colorIndex + 32256];
			_aRGBToPalette = pColormap->pRGB16Alpha;
			_color = _aRGBToPalette[_RGBcolor];
		}
	} else if (shading == kFlat) {
		int lightRow = (int)floor(light);

		if (bpp == 8) {
			_color = pColormap->pLightLevel[256 * lightRow + colorIndex];
		} else if (bpp == 16) {
			_RGBcolor = *(uint16 *)&pColormap->pRGB16[512 * lightRow + 2 * colorIndex];
			if (transparent) {
				_aRGBMap = pColormap->pRGB16 + 32256;
				_aRGBToPalette = pColormap->pRGB16Alpha;
				_color = _aRGBToPalette[_RGBcolor];
			}
		}
	} else {
		_color = colorIndex;
		if (bpp == 8) {
			_aMonoMap = pColormap->pLightLevel;
		} else if (bpp == 16) {
			_aRGBMap = pColormap->pRGB16;
			if (transparent) {
				_RGBcolor = *(uint16 *)&pColormap->pRGB16[2 * colorIndex + 32256];
				_aRGBToPalette = pColormap->pRGB16Alpha;
				_color = _aRGBToPalette[_RGBcolor];
			}
		}
		_aIntensities = pProcFace->aIntensities;
	}

	if (transparent)
		_aAlphaMap = _rd->_colormap.getIdentity()->pTransparency;

	if (!setupGeometry(pProcFace))
		return false;

	if (_numVertices > 32) {
		warning("RenderDroid::ZRasterModule::setupSolidFace(): too many vertices");
		return false;
	}

	for (uint i = 0; i < _numVertices; i++)
		_aOneOverZ[i] = (float)(1.0 / ((double)_aVertices[i][2] + 1.0));

	return true;
}

bool RenderDroid::ZRasterModule::setupTexturedFace(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, int mapping, int shading, int flags) {
	RdColormap *pColormap = pProcFace->pColormap;
	bool transparent = (flags & kTransparent) != 0;

	if (shading == kUnlit) {
		if (bpp == 16) {
			_aRGBMap = pColormap->pRGB16 + 32256;
			if (transparent)
				_aRGBToPalette = pColormap->pRGB16Alpha;
		}
	} else if (shading == kFlat) {
		int lightRow = (int)floor(light);

		if (bpp == 8) {
			_aMonoMap = &pColormap->pLightLevel[256 * lightRow];
		} else if (bpp == 16) {
			_aRGBMap = &pColormap->pRGB16[512 * lightRow];
			if (transparent)
				_aRGBToPalette = pColormap->pRGB16Alpha;
		}
	} else {
		if (bpp == 8) {
			_aMonoMap = pColormap->pLightLevel;
		} else if (bpp == 16) {
			_aRGBMap = pColormap->pRGB16;
			if (transparent)
				_aRGBToPalette = pColormap->pRGB16Alpha;
		}
		_aIntensities = pProcFace->aIntensities;
	}

	if (transparent)
		_aAlphaMap = _rd->_colormap.getIdentity()->pTransparency;

	RdTexture *pTexture = pCel->pTexture;

	_maxMipMap = pTexture->numMips - 1;
	_aTexVertices = pProcFace->aTexVertices;

	if (!setupGeometry(pProcFace))
		return false;

	if (mapping == kAffine) {
		calcAffineGradients(pProcFace->zMin);
	} else {
		calcPerspGradients(pProcFace->zMin);
	}

	_pPixels = pTexture->apMips[_mipMapNum]->pPixels;
	_texRowShift = pTexture->widthLog2 - _mipMapNum;
	_texPitch = pTexture->apMips[_mipMapNum]->tex.pitchInBytes;
	_uWrap = (int)((uint)pTexture->widthMask >> _mipMapNum << 16);
	_vWrap = (int)((uint)pTexture->heightMask >> _mipMapNum << _texRowShift);

	if (mapping == kPerspective) {
		_oneOverZStep = (float)(_dOneOverZdX * 16.0);
		_uOverZStep = (float)(_dUOverZdX * 16.0);
		_vOverZStep = (float)(_dVOverZdX * 16.0);
	}

	return true;
}

int RenderDroid::ZRasterModule::selectCasePitch(int mapping, int shading) {
	int firstPitch = (mapping == kAffine && shading != kUnlit) ? 4 : 8;

	for (int pitch = firstPitch; pitch <= 256; pitch *= 2) {
		if (_texPitch == pitch)
			return pitch;
	}

	return 0;
}

int RenderDroid::ZRasterModule::drawNGon(RdProcFace *pProcFace, RdMatCel *pCel, float light, int mapping, int shading, int flags) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (mapping == kSolid) {
		if (!setupSolidFace(pProcFace, pCel, light, bpp, shading, flags))
			return 0;
	} else if (!setupTexturedFace(pProcFace, pCel, light, bpp, mapping, shading, flags)) {
		return 0;
	}

	RdZRasterEdge left = RdZRasterEdge();
	RdZRasterEdge right = RdZRasterEdge();

	left.vertIdx = _yMinVert;
	int result = setupLeftEdge(&left, mapping, shading);
	if (result)
		return result;

	right.vertIdx = _yMinVert;
	result = setupRightEdge(&right, mapping, shading);
	if (result)
		return result;

	bool hasCases = (mapping != kSolid && flags == 0);
	bool useCase = (hasCases && !_zScaleShift && bpp == 16);
	bool drawLines = (bpp == 8 || bpp == 16) && !(hasCases && !_zScaleShift && bpp == 8);
	int casePitch = useCase ? selectCasePitch(mapping, shading) : 0;
	ScanParams params = makeScanParams(bpp, shading, flags, mapping == kPerspective);

	if (hasCases && bpp != 8 && bpp != 16)
		warning("RenderDroid::ZRasterModule::drawNGon(): unsupported colour depth");

	while (true) {
		if (useCase)
			scanLineCase(&left, &right, mapping, shading, casePitch);
		else if (drawLines && mapping == kSolid)
			scanLineSolid(&left, &right, bpp, shading, flags);
		else if (drawLines && mapping == kAffine)
			scanLineAffine(&left, &right, params);
		else if (drawLines)
			scanLinePerspective(&left, &right, params);

		if (--left.numLines) {
			stepEdge(&left, mapping, shading, true);
		} else {
			result = setupLeftEdge(&left, mapping, shading);
			if (result)
				return result;
		}

		if (--right.numLines) {
			stepEdge(&right, mapping, shading, false);
		} else {
			result = setupRightEdge(&right, mapping, shading);
			if (result)
				return result;
		}
	}
}

int RenderDroid::ZRasterModule::drawNGonLV(RdProcFace *pProcFace, RdMatCel *pCel) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	int bpp = pVBuffer->tex.format.bpp;

	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;

	if (bpp == 8)
		_color = (byte)pCel->colorIndex;
	else if (bpp == 16)
		_RGBcolor = *(uint16 *)&pProcFace->pColormap->pRGB16[2 * (byte)pCel->colorIndex + 32256];
	else
		return 0;

	for (uint i = 0; i < _numVertices; i++) {
		int x = (int)floor(_aVertices[i][0]);
		int y = (int)floor(_aVertices[i][1]);

		if (x < 0 || y < 0 || x >= pVBuffer->tex.width || y >= pVBuffer->tex.height) {
			warning("RenderDroid::ZRasterModule::drawNGonLV(): pixel outside the buffer");
			continue;
		}

		if (bpp == 8)
			pVBuffer->pPixels[y * pVBuffer->tex.pitchInBytes + x] = _color;
		else
			*(uint16 *)&pVBuffer->pPixels[2 * x + 2 * y * (pVBuffer->tex.pitchInBytes >> 1)] = _RGBcolor;
	}

	return 0;
}

int RenderDroid::ZRasterModule::drawNGonLW(RdProcFace *pProcFace, RdMatCel *pCel) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	int bpp = pCanvas->pVBuffer->tex.format.bpp;

	_aVertices = pProcFace->aVertices;
	_numVertices = pProcFace->numVertices;

	if (_numVertices == 0)
		return 0;

	uint16 color;
	if (bpp == 8)
		color = _color = (byte)pCel->colorIndex;
	else if (bpp == 16)
		color = _RGBcolor = *(uint16 *)&pProcFace->pColormap->pRGB16[2 * (byte)pCel->colorIndex + 32256];
	else
		return 0;

	for (uint i = 0; i < _numVertices; i++) {
		uint next = (i + 1 < _numVertices) ? i + 1 : 0;

		_rd->_primit2.drawLine(pCanvas, (int)floor(_aVertices[i][0]), (int)floor(_aVertices[i][1]), (int)floor(_aVertices[next][0]), (int)floor(_aVertices[next][1]), color, 0xFFFFFFFF);
	}

	return 0;
}

int RenderDroid::ZRasterModule::drawNGonLS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kSolid, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonLS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonLS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kUnlit, 0);
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonLS(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kSolid, kUnlit);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonLS(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kSolid, kUnlit);
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonFS(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kSolid, kFlat);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonFS(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kSolid, kFlat);
}

int RenderDroid::ZRasterModule::drawNGonTLS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kSolid, kUnlit, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kUnlit, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kUnlit, kTransparent);
}

int RenderDroid::ZRasterModule::drawNGonFS(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kSolid, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonFS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonFS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kFlat, 0);
}

int RenderDroid::ZRasterModule::drawNGonTFS(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kSolid, kFlat, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kFlat, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kFlat, kTransparent);
}

int RenderDroid::ZRasterModule::drawNGonGS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kSolid, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonGS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonGS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kGouraud, 0);
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonGS(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kSolid, kGouraud);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonGS(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kSolid, kGouraud);
}

int RenderDroid::ZRasterModule::drawNGonTGS(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kSolid, kGouraud, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 8, kGouraud, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineSolid(pLeft, pRight, 16, kGouraud, kTransparent);
}

int RenderDroid::ZRasterModule::drawNGonLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kUnlit, 0, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kUnlit, 0, false));
}

int RenderDroid::ZRasterModule::drawNGonTLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kUnlit, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kUnlit, kTransparent, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kUnlit, kTransparent, false));
}

int RenderDroid::ZRasterModule::drawNGonMLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kUnlit, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kUnlit, kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kUnlit, kMasked, false));
}

int RenderDroid::ZRasterModule::drawNGonMTLAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kUnlit, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kUnlit, kTransparent | kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kUnlit, kTransparent | kMasked, false));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonLAT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kAffine, kUnlit);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonLAT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kAffine, kUnlit);
}

int RenderDroid::ZRasterModule::drawNGonFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kAffine, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kFlat, 0, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kFlat, 0, false));
}

int RenderDroid::ZRasterModule::drawNGonTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kAffine, kFlat, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kFlat, kTransparent, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kFlat, kTransparent, false));
}

int RenderDroid::ZRasterModule::drawNGonMFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kAffine, kFlat, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kFlat, kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kFlat, kMasked, false));
}

int RenderDroid::ZRasterModule::drawNGonMTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kAffine, kFlat, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kFlat, kTransparent | kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kFlat, kTransparent | kMasked, false));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonFAT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kAffine, kFlat);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonFAT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kAffine, kFlat);
}

int RenderDroid::ZRasterModule::drawNGonGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kGouraud, 0, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kGouraud, 0, false));
}

int RenderDroid::ZRasterModule::drawNGonTGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kGouraud, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kGouraud, kTransparent, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kGouraud, kTransparent, false));
}

int RenderDroid::ZRasterModule::drawNGonMGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kGouraud, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kGouraud, kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kGouraud, kMasked, false));
}

int RenderDroid::ZRasterModule::drawNGonMTGAT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kAffine, kGouraud, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(8, kGouraud, kTransparent | kMasked, false));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineAffine(pLeft, pRight, makeScanParams(16, kGouraud, kTransparent | kMasked, false));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonGAT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kAffine, kGouraud);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonGAT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kAffine, kGouraud);
}

int RenderDroid::ZRasterModule::drawNGonLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kUnlit, 0, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kUnlit, 0, true));
}

int RenderDroid::ZRasterModule::drawNGonTLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kUnlit, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kUnlit, kTransparent, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kUnlit, kTransparent, true));
}

int RenderDroid::ZRasterModule::drawNGonMLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kUnlit, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kUnlit, kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kUnlit, kMasked, true));
}

int RenderDroid::ZRasterModule::drawNGonMTLIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kUnlit, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kUnlit, kTransparent | kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kUnlit, kTransparent | kMasked, true));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonLIT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kPerspective, kUnlit);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonLIT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kPerspective, kUnlit);
}

int RenderDroid::ZRasterModule::drawNGonFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kPerspective, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kFlat, 0, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kFlat, 0, true));
}

int RenderDroid::ZRasterModule::drawNGonTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kPerspective, kFlat, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kFlat, kTransparent, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kFlat, kTransparent, true));
}

int RenderDroid::ZRasterModule::drawNGonMFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kPerspective, kFlat, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kFlat, kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kFlat, kMasked, true));
}

int RenderDroid::ZRasterModule::drawNGonMTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light) {
	return drawNGon(pProcFace, pCel, light, kPerspective, kFlat, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kFlat, kTransparent | kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kFlat, kTransparent | kMasked, true));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonFIT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kPerspective, kFlat);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonFIT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kPerspective, kFlat);
}

int RenderDroid::ZRasterModule::drawNGonGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawScanLineNGonGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kGouraud, 0, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kGouraud, 0, true));
}

int RenderDroid::ZRasterModule::drawNGonTGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kGouraud, kTransparent);
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kGouraud, kTransparent, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonTGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kGouraud, kTransparent, true));
}

int RenderDroid::ZRasterModule::drawNGonMGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kGouraud, kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kGouraud, kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kGouraud, kMasked, true));
}

int RenderDroid::ZRasterModule::drawNGonMTGIT(RdProcFace *pProcFace, RdMatCel *pCel) {
	return drawNGon(pProcFace, pCel, 0.0f, kPerspective, kGouraud, kTransparent | kMasked);
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(8, kGouraud, kTransparent | kMasked, true));
}

void RenderDroid::ZRasterModule::drawScanLineNGonMTGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLinePerspective(pLeft, pRight, makeScanParams(16, kGouraud, kTransparent | kMasked, true));
}

int RenderDroid::ZRasterModule::setupLeftEdgeNGonGIT(RdZRasterEdge *pEdge) {
	return setupLeftEdge(pEdge, kPerspective, kGouraud);
}

int RenderDroid::ZRasterModule::setupRightEdgeNGonGIT(RdZRasterEdge *pEdge) {
	return setupRightEdge(pEdge, kPerspective, kGouraud);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 8);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 16);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 32);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 64);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 128);
}

void RenderDroid::ZRasterModule::drawNGonLAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kUnlit, 256);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 4);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 8);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 16);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 32);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 64);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 128);
}

void RenderDroid::ZRasterModule::drawNGonFAT_Case7(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kFlat, 256);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 4);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 8);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 16);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 32);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 64);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 128);
}

void RenderDroid::ZRasterModule::drawNGonGAT_Case7(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kAffine, kGouraud, 256);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 0);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 8);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 16);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 32);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 64);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 128);
}

void RenderDroid::ZRasterModule::drawNGonLIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kUnlit, 256);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 0);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 8);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 16);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 32);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 64);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 128);
}

void RenderDroid::ZRasterModule::drawNGonFIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kFlat, 256);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 0);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 8);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 16);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 32);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 64);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 128);
}

void RenderDroid::ZRasterModule::drawNGonGIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight) {
	scanLineCase(pLeft, pRight, kPerspective, kGouraud, 256);
}

} // End of namespace Grim
