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
	_vertices(nullptr), _texVertices(nullptr), _intensities(nullptr),
	_dest(nullptr), _wDest(nullptr), _pixels(nullptr),
	_mipMapNum(0), _maxMipMap(0), _texPitch(0), _texRowShift(0), _uWrap(0), _vWrap(0),
	_uStep(0), _vStep(0), _fixU(0), _fixV(0),
	_dOneOverZdX(0.0f), _dOneOverZdY(0.0f), _dUOverZdX(0.0f), _dUOverZdY(0.0f), _dVOverZdX(0.0f), _dVOverZdY(0.0f),
	_dUdXModifier(0), _dVdXModifier(0),
	_oneOverZStep(0.0f), _uOverZStep(0.0f), _vOverZStep(0.0f),
	_alphaMap(nullptr), _monoMap(nullptr), _rgbMap(nullptr), _rgbToPalette(nullptr),
	_RGBcolor(0), _color(0) {
	for (int i = 0; i < 32; i++) {
		_oneOverZ[i] = 0.0f;
		_uOverZ[i] = 0.0f;
		_vOverZ[i] = 0.0f;
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
		float *next = _vertices[vertC];
		float *minPointer = _vertices[_yMinVert];
		float *maxPointer = _vertices[_yMaxVert];

		int64 dxMax = ftol((double)next[0] - maxPointer[0]);
		if (dxMax < 0)
			dxMax = -dxMax;

		bool offLine = (double)dxMax > 1.0;
		if (!offLine) {
			int64 dxMin = ftol((double)next[0] - minPointer[0]);
			if (dxMin < 0)
				dxMin = -dxMin;

			offLine = (double)dxMin > 1.0;
		}

		if (offLine) {
			double expectedY = ((double)maxPointer[1] - minPointer[1]) / ((double)maxPointer[0] - minPointer[0]) * ((double)next[0] - minPointer[0]) + minPointer[1];
			double dist = next[1] - expectedY;
			if (dist < 0.0)
				dist = -dist;

			direction = (dist >= 1.0) ? 1 : -1;
		}
	}

	float bestDist = 0.0f;
	int cur = wrapVertex(direction + _yMinVert);
	while (cur != _yMaxVert) {
		float midX = ((double)_vertices[_yMinVert][0] + _vertices[_yMaxVert][0]) * 0.5f;
		double dist = _vertices[cur][0] - midX;
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
		if (z > (double)_rd->_mipDistances[1])
			_mipMapNum = 1;
		break;
	case 2:
		if (z <= (double)_rd->_mipDistances[0])
			_mipMapNum = 0;
		else if (z > (double)_rd->_mipDistances[1])
			_mipMapNum = 2;
		else
			_mipMapNum = 1;
		break;
	case 3:
		if (z <= (double)_rd->_mipDistances[0])
			_mipMapNum = 0;
		else if (z <= (double)_rd->_mipDistances[1])
			_mipMapNum = 1;
		else if (z > (double)_rd->_mipDistances[2])
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

	float *ref = _vertices[vertC];
	double dyA = _vertices[vertA][1] - ref[1];
	double dyB = _vertices[vertB][1] - ref[1];
	float dxA = (double)_vertices[vertA][0] - ref[0];
	float dxB = (double)_vertices[vertB][0] - ref[0];
	double duA = (double)_texVertices[vertA][0] - _texVertices[vertC][0];
	double duB = (double)_texVertices[vertB][0] - _texVertices[vertC][0];
	float duBStored = duB;
	float duAStored = duA;
	double dvB = (double)_texVertices[vertB][1] - _texVertices[vertC][1];
	double uCross = duB * dyA - duA * dyB;
	double invDet = 1.0 / (dyA * dxB - dyB * dxA);
	float invDetStored = invDet;
	float dvBStored = dvB;
	float dvA = (double)_texVertices[vertA][1] - _texVertices[vertC][1];
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
		double oneOverZ = 1.0 / _vertices[i][2];
		_oneOverZ[i] = oneOverZ;
		_uOverZ[i] = _texVertices[i][0] * oneOverZ;
		_vOverZ[i] = (double)_texVertices[i][1] * _oneOverZ[i];
	}

	float *a = _vertices[vertA];
	float *b = _vertices[vertB];
	float *c = _vertices[vertC];
	double dyA = (double)a[1] - c[1];
	double dyB = (double)b[1] - c[1];
	float dyAStored = dyA;
	float dyBStored = dyB;
	float dxA = (double)a[0] - c[0];
	float dxB = (double)b[0] - c[0];
	float dOneOverZA = (double)_oneOverZ[vertA] - _oneOverZ[vertC];
	float dUOverZB = (double)_uOverZ[vertB] - _uOverZ[vertC];
	float dUOverZA = (double)_uOverZ[vertA] - _uOverZ[vertC];
	float dVOverZB = (double)_vOverZ[vertB] - _vOverZ[vertC];
	double dOneOverZB = (double)_oneOverZ[vertB] - _oneOverZ[vertC];
	float dVOverZA = (double)_vOverZ[vertA] - _vOverZ[vertC];
	float invDet = 1.0 / (dyA * dxB - dyB * dxA);
	float negInvDet = -invDet;

	_dOneOverZdX = (dOneOverZB * dyAStored - (double)dOneOverZA * dyBStored) * invDet;
	_dOneOverZdY = (dOneOverZB * dxA - (double)dOneOverZA * dxB) * negInvDet;
	_dUOverZdX = ((double)dUOverZB * dyAStored - (double)dUOverZA * dyBStored) * invDet;
	_dUOverZdY = ((double)dUOverZB * dxA - (double)dUOverZA * dxB) * negInvDet;
	_dVOverZdX = ((double)dVOverZB * dyAStored - (double)dVOverZA * dyBStored) * invDet;
	_dVOverZdY = ((double)dVOverZB * dxA - (double)dVOverZA * dxB) * negInvDet;

	selectMipLevel(z);

	double uCrossX = (double)_oneOverZ[vertA] * _dUOverZdX - (double)_uOverZ[vertA] * _dOneOverZdX;
	if (uCrossX > 0.0)
		_dUdXModifier = 0x8000;
	else if (uCrossX < 0.0 || (double)_oneOverZ[vertA] * _dUOverZdY - (double)_uOverZ[vertA] * _dOneOverZdY < 0.0)
		_dUdXModifier = 0x7FFF;
	else
		_dUdXModifier = 0x8000;

	double vCrossX = (double)_oneOverZ[vertA] * _dVOverZdX - (double)_vOverZ[vertA] * _dOneOverZdX;
	if (vCrossX > 0.0)
		_dVdXModifier = 0x8000;
	else if (vCrossX < 0.0 || (double)_oneOverZ[vertA] * _dVOverZdY - (double)_vOverZ[vertA] * _dOneOverZdY < 0.0)
		_dVdXModifier = 0x7FFF;
	else
		_dVdXModifier = 0x8000;
}

int RenderDroid::NRasterModule::drawNGonLV(RdProcFace *procFace, RdMatCel *cel) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	int bpp = vBuffer->tex.format.bpp;
	_vertices = procFace->vertices;
	_numVertices = procFace->numVertices;

	if (bpp == 8) {
		_color = cel->colorIndex;
		for (int i = 0; i < (int)_numVertices; i++) {
			int y = floorToInt(_vertices[i][1]);
			int x = floorToInt(_vertices[i][0]);
			vBuffer->pixels[y * vBuffer->tex.pitchInBytes + x] = _color;
		}
	} else if (bpp == 16) {
		_RGBcolor = *(uint16 *)&procFace->colormap->rgb16[2 * (cel->colorIndex & 0xFF) + 32256];
		for (int i = 0; i < (int)_numVertices; i++) {
			int y = floorToInt(_vertices[i][1]);
			int x = floorToInt(_vertices[i][0]);
			*(uint16 *)&vBuffer->pixels[2 * x + 2 * y * (vBuffer->tex.pitchInBytes >> 1)] = _RGBcolor;
		}
	}

	return 0;
}

int RenderDroid::NRasterModule::drawNGonLW(RdProcFace *procFace, RdMatCel *cel) {
	RdCanvas *canvas = _rd->_camera.getCurrent()->canvas;
	int bpp = canvas->vBuffer->tex.format.bpp;
	_vertices = procFace->vertices;
	_numVertices = procFace->numVertices;

	if (_numVertices == 0) {
		warning("RenderDroid::NRasterModule::drawNGonLW(): face without vertices");
		return 0;
	}

	uint16 color;
	if (bpp == 8) {
		_color = cel->colorIndex;
		color = _color;
	} else if (bpp == 16) {
		_RGBcolor = *(uint16 *)&procFace->colormap->rgb16[2 * (cel->colorIndex & 0xFF) + 32256];
		color = _RGBcolor;
	} else {
		return 0;
	}

	int i = 0;
	for (; i < (int)_numVertices - 1; i++)
		_rd->_primit2.drawLine(canvas, floorToInt(_vertices[i][0]), floorToInt(_vertices[i][1]), floorToInt(_vertices[i + 1][0]), floorToInt(_vertices[i + 1][1]), color, 0xFFFFFFFF);

	_rd->_primit2.drawLine(canvas, floorToInt(_vertices[i][0]), floorToInt(_vertices[i][1]), floorToInt(_vertices[0][0]), floorToInt(_vertices[0][1]), color, 0xFFFFFFFF);
	return 0;
}

int RenderDroid::NRasterModule::setupEdge(RdNRasterEdge *edge, int step, EdgeKind kind) {
	bool gouraud = kind == kEdgeGouraud || kind == kEdgeAffineGouraud || kind == kEdgePerspectiveGouraud;
	bool affine = kind == kEdgeAffine || kind == kEdgeAffineFlat || kind == kEdgeAffineGouraud;
	bool perspective = kind == kEdgePerspective || kind == kEdgePerspectiveFlat || kind == kEdgePerspectiveGouraud;
	bool directSlope = kind == kEdgeFlat || kind == kEdgePerspective || kind == kEdgePerspectiveFlat;
	bool exactSlopeX = kind == kEdgeAffineFlat || kind == kEdgeAffineGouraud;

	int vertIdx = edge->vertIdx;
	if (vertIdx == _yMaxVert)
		return 1;

	int nextIdx;
	int numLines;
	float yStart;
	while (true) {
		nextIdx = wrapVertex(vertIdx + step);
		yStart = ceil(_vertices[vertIdx][1]);
		edge->y = floorToInt(yStart);
		numLines = floorToInt(ceil(_vertices[nextIdx][1])) - edge->y;
		edge->numLines = numLines;
		if (numLines > 0)
			break;

		vertIdx = nextIdx;
		if (vertIdx == _yMaxVert)
			return 1;
	}

	float *vert = _vertices[vertIdx];
	float *next = _vertices[nextIdx];
	float yDelta = (double)yStart - vert[1];
	float xPos = vert[0];
	float xFixed;
	float uFixed = 0.0f;
	float vFixed = 0.0f;
	float intensityFixed = 0.0f;

	if (numLines >= 2) {
		double deltaY = (double)next[1] - vert[1];
		double invDyExact = 1.0 / deltaY;
		float invDy = invDyExact;
		double slopeX;
		if (directSlope)
			slopeX = ((double)next[0] - vert[0]) / deltaY;
		else
			slopeX = ((double)next[0] - vert[0]) * (exactSlopeX ? invDyExact : (double)invDy);

		float slopeXStored = slopeX;
		float dxFixed = slopeX * 65536.0;
		edge->dx = floorToInt(dxFixed);
		double xExact = (double)slopeXStored * yDelta + vert[0];
		xPos = xExact;
		xFixed = xExact * 65536.0;

		if (affine) {
			double slopeU = ((double)_texVertices[nextIdx][0] - _texVertices[vertIdx][0]) * invDy;
			float slopeUStored = slopeU;
			float duFixed = slopeU * 65536.0;
			edge->du = floorToInt(duFixed);
			uFixed = ((double)slopeUStored * yDelta + _texVertices[vertIdx][0]) * 65536.0;

			double slopeV = ((double)_texVertices[nextIdx][1] - _texVertices[vertIdx][1]) * invDy;
			float slopeVStored = slopeV;
			float dvFixed = slopeV * 65536.0;
			edge->dv = floorToInt(dvFixed);
			vFixed = ((double)slopeVStored * yDelta + _texVertices[vertIdx][1]) * 65536.0;
		}

		if (perspective) {
			edge->dOneOverZ = (double)slopeXStored * _dOneOverZdX + _dOneOverZdY;
			edge->dUOverZ = (double)slopeXStored * _dUOverZdX + _dUOverZdY;
			edge->dVOverZ = (double)slopeXStored * _dVOverZdX + _dVOverZdY;
		}

		if (gouraud) {
			double slopeI = ((double)_intensities[nextIdx] - _intensities[vertIdx]) * invDy;
			float slopeIStored = slopeI;
			float dIntensityFixed = slopeI * 65536.0;
			edge->dIntensity = floorToInt(dIntensityFixed);
			intensityFixed = ((double)slopeIStored * yDelta + _intensities[vertIdx]) * 65536.0;
		}
	} else {
		xFixed = vert[0] * 65536.0;
		if (affine) {
			uFixed = _texVertices[vertIdx][0] * 65536.0;
			vFixed = _texVertices[vertIdx][1] * 65536.0;
		}

		if (gouraud)
			intensityFixed = _intensities[vertIdx] * 65536.0;
	}

	edge->x = floorToInt(xFixed);
	if (affine) {
		edge->u = floorToInt(uFixed);
		edge->v = floorToInt(vFixed);
	}

	if (gouraud)
		edge->intensity = floorToInt(intensityFixed);

	if (perspective) {
		double xOffset = (double)xPos - vert[0];
		double yOneOverZ = (double)yDelta * _dOneOverZdY;
		double yUOverZ = (double)yDelta * _dUOverZdY;
		double yVOverZ = (double)yDelta * _dVOverZdY;
		double xOneOverZ = xOffset * _dOneOverZdX;
		double xUOverZ = xOffset * _dUOverZdX;
		double xVOverZ = xOffset * _dVOverZdX;

		if (!_rd->isDemo()) {
			edge->oneOverZ = yOneOverZ + xOneOverZ + _oneOverZ[vertIdx];
			edge->uOverZ = yUOverZ + xUOverZ + _uOverZ[vertIdx];
			edge->vOverZ = yVOverZ + xVOverZ + _vOverZ[vertIdx];
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
				edge->oneOverZ = yOneOverZ + _oneOverZ[vertIdx] + xOneOverZ;
			else
				edge->oneOverZ = xOneOverZ + _oneOverZ[vertIdx] + yOneOverZ;

			if (uYFirst)
				edge->uOverZ = yUOverZ + _uOverZ[vertIdx] + xUOverZ;
			else
				edge->uOverZ = xUOverZ + _uOverZ[vertIdx] + yUOverZ;

			if (vYFirst)
				edge->vOverZ = yVOverZ + _vOverZ[vertIdx] + xVOverZ;
			else
				edge->vOverZ = xVOverZ + _vOverZ[vertIdx] + yVOverZ;
		}
	}

	edge->vertIdx = nextIdx;
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

void RenderDroid::NRasterModule::advanceEdge(RdNRasterEdge *edge, Geometry geometry, Lighting lighting, bool isLeft) {
	edge->x += edge->dx;

	if (geometry == kGeometryAffine) {
		edge->u += edge->du;
		edge->v += edge->dv;
	}

	if (geometry == kGeometryPerspective && isLeft) {
		edge->oneOverZ = edge->oneOverZ + edge->dOneOverZ;
		edge->uOverZ = edge->uOverZ + edge->dUOverZ;
		edge->vOverZ = edge->vOverZ + edge->dVOverZ;
	}

	if (lighting == kLightingGouraud)
		edge->intensity += edge->dIntensity;

	edge->y++;
}

void RenderDroid::NRasterModule::setupFace(RdProcFace *procFace) {
	_vertices = procFace->vertices;
	_numVertices = procFace->numVertices;
	_yMinVert = procFace->yMinIdx;
	_yMaxVert = procFace->yMaxIdx;

	if (procFace->backface) {
		_leftVertexStep = -1;
		_rightVertexStep = 1;
	} else {
		_leftVertexStep = 1;
		_rightVertexStep = -1;
	}
}

void RenderDroid::NRasterModule::setupSolidColors(RdProcFace *procFace, RdMatCel *cel, float light, int bpp, Lighting lighting, Blend blend) {
	RdColormap *colormap = procFace->colormap;
	uint16 *rgb16 = (uint16 *)colormap->rgb16;
	bool flat = lighting == kLightingFlat;
	bool gouraud = lighting == kLightingGouraud;
	bool transparent = blend == kBlendAlpha;
	int colorIndex = cel->colorIndex;
	int lightLevel = flat ? floorToInt(light) : 0;

	if (bpp == 8) {
		_color = flat ? colormap->lightLevel[256 * lightLevel + colorIndex] : colorIndex;
		if (gouraud)
			_monoMap = colormap->lightLevel;
	} else if (bpp == 16) {
		if (gouraud) {
			_rgbMap = rgb16;
			_color = colorIndex;
			if (transparent)
				_RGBcolor = rgb16[63 * 256 + colorIndex];
		} else if (transparent) {
			_rgbMap = rgb16 + 63 * 256;
			_RGBcolor = rgb16[256 * (flat ? lightLevel : 63) + colorIndex];
		} else if (flat) {
			_RGBcolor = rgb16[256 * lightLevel + colorIndex];
		} else {
			_RGBcolor = rgb16[63 * 256 + (colorIndex & 0xFF)];
		}

		if (transparent) {
			_rgbToPalette = colormap->rgb16Alpha;
			_color = _rgbToPalette[_RGBcolor];
		}
	}

	if (transparent)
		_alphaMap = _rd->_colormap.getIdentity()->transparency;

	setupFace(procFace);
	if (gouraud)
		_intensities = procFace->intensities;
}

bool RenderDroid::NRasterModule::setupTexture(RdProcFace *procFace, RdMatCel *cel, float light, int bpp, Geometry geometry, Lighting lighting, Blend blend) {
	RdColormap *colormap = procFace->colormap;
	RdTexture *texture = cel->texture;
	uint16 *rgb16 = (uint16 *)colormap->rgb16;
	bool transparent = blend == kBlendAlpha || blend == kBlendMaskedAlpha;
	bool perspective = geometry == kGeometryPerspective;
	int lightLevel = (lighting == kLightingFlat) ? floorToInt(light) : 0;

	_maxMipMap = texture->numMips - 1;
	_texVertices = procFace->texVertices;

	if (bpp == 8 && lighting == kLightingFlat)
		_monoMap = &colormap->lightLevel[256 * lightLevel];
	else if (bpp == 8 && lighting == kLightingGouraud)
		_monoMap = colormap->lightLevel;

	if (bpp == 16) {
		if (lighting == kLightingNone)
			_rgbMap = rgb16 + 63 * 256;
		else if (lighting == kLightingFlat)
			_rgbMap = rgb16 + 256 * lightLevel;
		else
			_rgbMap = rgb16;

		if (transparent)
			_rgbToPalette = colormap->rgb16Alpha;
	}

	if (transparent)
		_alphaMap = _rd->_colormap.getIdentity()->transparency;

	setupFace(procFace);
	if (lighting == kLightingGouraud)
		_intensities = procFace->intensities;

	if (perspective)
		calcPerspGradients(procFace->zMin);
	else
		calcAffineGradients(procFace->zMin);

	if (_mipMapNum < 0 || _mipMapNum >= (int)texture->numMips) {
		warning("RenderDroid::NRasterModule::setupTexture(): mip level %d is not available", _mipMapNum);
		return false;
	}

	_pixels = texture->mips[_mipMapNum]->pixels;
	_texRowShift = texture->widthLog2 - _mipMapNum;
	if (blend == kBlendOpaque)
		_texPitch = texture->mips[_mipMapNum]->tex.pitchInBytes;

	_uWrap = (uint)texture->widthMask >> _mipMapNum << 16;

	if (perspective) {
		_oneOverZStep = (double)_dOneOverZdX * 16.0;
		_uOverZStep = (double)_dUOverZdX * 16.0;
		_vOverZStep = (double)_dVOverZdX * 16.0;
	}

	_vWrap = (uint)texture->heightMask >> _mipMapNum << _texRowShift;
	return true;
}

int RenderDroid::NRasterModule::drawNGon(RdProcFace *procFace, RdMatCel *cel, float light, Geometry geometry, Lighting lighting, Blend blend) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (geometry == kGeometrySolid) {
		setupSolidColors(procFace, cel, light, bpp, lighting, blend);
	} else if (!setupTexture(procFace, cel, light, bpp, geometry, lighting, blend)) {
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

void RenderDroid::NRasterModule::drawScanLine(RdNRasterEdge *left, RdNRasterEdge *right, Geometry geometry, Lighting lighting, Blend blend, int bpp) {
	if (bpp != 8 && bpp != 16)
		return;

	if (bpp == 8 && blend == kBlendOpaque && (geometry == kGeometryPerspective || (geometry == kGeometryAffine && lighting != kLightingNone)))
		return;

	if (geometry == kGeometrySolid)
		drawScanLineSolid(left, right, (lighting == kLightingGouraud) ? kLightingGouraud : kLightingNone, blend, bpp);
	else if (geometry == kGeometryAffine)
		drawScanLineAffine(left, right, lighting, blend, bpp);
	else
		drawScanLinePerspective(left, right, lighting, blend, bpp);
}

int RenderDroid::NRasterModule::calcSpan(RdNRasterEdge *left, RdNRasterEdge *right, int *xStart) {
	*xStart = ceilToPixel(left->x);
	int xEnd = ceilToPixel(right->x);
	return (xEnd >> 16) - (*xStart >> 16);
}

void RenderDroid::NRasterModule::calcSpanIntensity(RdNRasterEdge *left, RdNRasterEdge *right, int width, int subPixel, int *intensity, int *dIntensityPointer) {
	int dIntensity = fixedMultiply(_rd->_raster.getOneOverNFixed(width), right->intensity - left->intensity);
	if ((uint)dIntensity > 0xFFFFFAE2)
		dIntensity = 0;

	*dIntensityPointer = dIntensity;
	*intensity = left->intensity + fixedMultiply(dIntensity, subPixel);
}

bool RenderDroid::NRasterModule::beginSpanPlot(SpanPlot *plot, int y, int xStart, Lighting lighting, Blend blend, int bpp, bool perspective, bool bufferedDestination) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	bool transparent = blend == kBlendAlpha || blend == kBlendMaskedAlpha;

	plot->lighting = lighting;
	plot->blend = blend;
	plot->bpp = bpp;
	plot->perspective = perspective;
	plot->backdrop = _dest;
	plot->dest8 = vBuffer->pixels + vBuffer->tex.pitchInBytes * y + (xStart >> 16);
	plot->dest16 = (uint16 *)(vBuffer->pixels + 2 * (xStart >> 16) + 2 * (vBuffer->tex.pitchInBytes >> 1) * y);

	if (bufferedDestination) {
		_dest = plot->dest8;
		plot->backdrop = plot->dest8;
		plot->dest16 = _wDest;
	}

	if (bpp == 16 && transparent && (bufferedDestination ? _wDest == nullptr : _dest == nullptr)) {
		warning("RenderDroid::NRasterModule::beginSpanPlot(): destination pointer was never set");
		return false;
	}

	return true;
}

byte RenderDroid::NRasterModule::lightTexel(Lighting lighting, byte texel, int intensity) {
	if (lighting == kLightingFlat)
		return _monoMap[texel];

	if (lighting == kLightingGouraud)
		return _monoMap[texel + ((intensity >> 8) & 0x3F00)];

	return texel;
}

void RenderDroid::NRasterModule::plotPixel(SpanPlot *plot, byte texel, int intensity) {
	Blend blend = plot->blend;
	bool masked = blend == kBlendMasked || blend == kBlendMaskedAlpha;

	if (plot->bpp == 8) {
		byte *dest = plot->dest8++;

		if (masked && texel == 0)
			return;

		if (blend == kBlendAlpha)
			*dest = _alphaMap[256 * lightTexel(plot->lighting, texel, intensity) + *dest];
		else if (blend == kBlendMaskedAlpha)
			*dest = lightTexel(plot->lighting, _alphaMap[256 * texel + *dest], intensity);
		else
			*dest = lightTexel(plot->lighting, texel, intensity);

		return;
	}

	uint16 *dest = plot->dest16++;

	if (masked && texel == 0)
		return;

	int shade = (plot->lighting == kLightingGouraud) ? 256 * ((intensity >> 16) & 0x3F) : 0;

	if (blend == kBlendOpaque || blend == kBlendMasked) {
		*dest = _rgbMap[shade + texel];
		return;
	}

	byte backdrop = _rgbToPalette[*plot->backdrop];

	if (blend == kBlendAlpha && plot->lighting == kLightingFlat && !plot->perspective) {
		*dest = _rgbMap[_alphaMap[256 * _rgbToPalette[_rgbMap[texel]] + backdrop]];
	} else if (blend == kBlendAlpha && plot->lighting != kLightingNone && plot->perspective) {
		uint16 color = _rgbMap[shade + texel];
		byte colorIndex = 0;
		if (color < 256)
			colorIndex = _rgbToPalette[color << 8];
		else
			warning("RenderDroid::NRasterModule::plotPixel(): palette index out of range");

		*dest = _rgbMap[_alphaMap[colorIndex + backdrop]];
	} else {
		*dest = _rgbMap[shade + _alphaMap[256 * texel + backdrop]];
	}
}

byte RenderDroid::NRasterModule::fetchTexel(int u, int v) {
	return _pixels[((u >> 16) & (_uWrap >> 16)) | (_vWrap & (v >> (16 - _texRowShift)))];
}

byte RenderDroid::NRasterModule::fetchTexelByRows(int u, int v) {
	int index = ((u >> 16) & (_uWrap >> 16)) + (((v >> 16) & _vWrap) << _texRowShift);

	if (index > _vWrap + (1 << _texRowShift) - 1) {
		warning("RenderDroid::NRasterModule::fetchTexelByRows(): texture index out of range");
		return 0;
	}

	return _pixels[index];
}

void RenderDroid::NRasterModule::drawScanLineSolid(RdNRasterEdge *left, RdNRasterEdge *right, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(left, right, &xStart);
	if (width <= 0)
		return;

	int intensity = 0;
	int dIntensity = 0;
	if (lighting == kLightingGouraud)
		calcSpanIntensity(left, right, width, xStart - left->x, &intensity, &dIntensity);

	SpanPlot plot;
	if (!beginSpanPlot(&plot, left->y, xStart, lighting, blend, bpp, false, false))
		return;

	if (lighting != kLightingGouraud && blend == kBlendOpaque) {
		if (bpp == 8) {
			memset(plot.dest8, _color, width);
		} else {
			for (int i = 0; i < width; i++)
				*plot.dest16++ = _RGBcolor;
		}
	} else {
		for (int i = 0; i < width; i++) {
			plotPixel(&plot, _color, intensity);
			intensity += dIntensity;
		}
	}

	if (bpp == 16)
		_wDest = plot.dest16;
}

void RenderDroid::NRasterModule::drawScanLineAffine(RdNRasterEdge *left, RdNRasterEdge *right, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(left, right, &xStart);
	if (width <= 0)
		return;

	int subPixel = xStart - left->x;
	int oneOverN = _rd->_raster.getOneOverNFixed(width);
	int uDelta = fixedMultiply(oneOverN, right->u - left->u);
	_uStep = uDelta >> _mipMapNum;
	_fixU = (left->u + fixedMultiply(uDelta, subPixel)) >> _mipMapNum;
	int vDelta = fixedMultiply(oneOverN, right->v - left->v);
	_vStep = vDelta >> _mipMapNum;
	int vStart = (left->v + fixedMultiply(vDelta, subPixel)) >> _mipMapNum;

	int intensity = 0;
	int dIntensity = 0;
	if (lighting == kLightingGouraud)
		calcSpanIntensity(left, right, width, subPixel, &intensity, &dIntensity);

	_fixU += _dUdXModifier;
	_fixV = _dVdXModifier + vStart;

	bool rowsByShift = lighting == kLightingNone && blend == kBlendMaskedAlpha;
	bool bufferedDestination = lighting == kLightingFlat && blend == kBlendMaskedAlpha && bpp == 16;

	SpanPlot plot;
	if (!beginSpanPlot(&plot, left->y, xStart, lighting, blend, bpp, false, bufferedDestination))
		return;

	for (int i = 0; i < width; i++) {
		byte texel = rowsByShift ? fetchTexelByRows(_fixU, _fixV) : fetchTexel(_fixU, _fixV);
		plotPixel(&plot, texel, intensity);
		_fixU += _uStep;
		_fixV += _vStep;
		intensity += dIntensity;
	}

	if (bpp == 8)
		_dest = plot.dest8;
	else
		_wDest = plot.dest16;
}

void RenderDroid::NRasterModule::startRun(SpanRun *run, float u, float v, float uStepFixed, float vStepFixed) {
	run->uStep = floorToInt(uStepFixed) >> _mipMapNum;
	run->vStep = floorToInt(vStepFixed) >> _mipMapNum;
	run->u = _dUdXModifier + (floorToInt(u * 65536.0f) >> _mipMapNum);
	run->v = _dVdXModifier + (floorToInt(v * 65536.0f) >> _mipMapNum);
}

void RenderDroid::NRasterModule::plotRun(SpanPlot *plot, SpanRun *run, int count) {
	for (int i = 0; i < count; i++) {
		plotPixel(plot, fetchTexel(run->u, run->v), run->intensity);
		run->u += run->uStep;
		run->v += run->vStep;
		run->intensity += run->dIntensity;
	}
}

void RenderDroid::NRasterModule::drawScanLinePerspective(RdNRasterEdge *left, RdNRasterEdge *right, Lighting lighting, Blend blend, int bpp) {
	int xStart;
	int width = calcSpan(left, right, &xStart);
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
		calcSpanIntensity(left, right, width, xStart - left->x, &run.intensity, &run.dIntensity);

	bool keepsState = lighting == kLightingGouraud && blend == kBlendOpaque && bpp == 16;
	bool divideLastRun = lighting == kLightingNone;

	float fraction = (float)(xStart - left->x) * 1.52587890625e-05f;
	float oneOverZ = lowAdd(lowMul(fraction, _dOneOverZdX), left->oneOverZ);
	float uOverZ = lowAdd(lowMul(fraction, _dUOverZdX), left->uOverZ);
	float vOverZ = lowAdd(lowMul(fraction, _dVOverZdX), left->vOverZ);
	float invZ = lowDiv(1.0f, oneOverZ);
	float u = lowMul(invZ, uOverZ);
	float v = lowMul(invZ, vOverZ);

	SpanPlot plot;
	if (!beginSpanPlot(&plot, left->y, xStart, lighting, blend, bpp, true, false))
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
		_wDest = plot.dest16;

	if (keepsState) {
		_fixU = run.u;
		_fixV = run.v;
		_uStep = run.uStep;
		_vStep = run.vStep;
	}
}

int RenderDroid::NRasterModule::drawNGonLS(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometrySolid, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLS(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometrySolid, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonFS(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometrySolid, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFS(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometrySolid, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonGS(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometrySolid, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGS(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometrySolid, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonLAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMLAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingNone, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTLAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingNone, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonFAT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryAffine, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFAT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryAffine, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMFAT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryAffine, kLightingFlat, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTFAT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryAffine, kLightingFlat, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonGAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMGAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTGAT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryAffine, kLightingGouraud, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonLIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingNone, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTLIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingNone, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMLIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingNone, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTLIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingNone, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonFIT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryPerspective, kLightingFlat, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTFIT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryPerspective, kLightingFlat, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMFIT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryPerspective, kLightingFlat, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTFIT(RdProcFace *procFace, RdMatCel *cel, float light) {
	return drawNGon(procFace, cel, light, kGeometryPerspective, kLightingFlat, kBlendMaskedAlpha);
}

int RenderDroid::NRasterModule::drawNGonGIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendOpaque);
}

int RenderDroid::NRasterModule::drawNGonTGIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendAlpha);
}

int RenderDroid::NRasterModule::drawNGonMGIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendMasked);
}

int RenderDroid::NRasterModule::drawNGonMTGIT(RdProcFace *procFace, RdMatCel *cel) {
	return drawNGon(procFace, cel, 0.0f, kGeometryPerspective, kLightingGouraud, kBlendMaskedAlpha);
}

void RenderDroid::NRasterModule::drawScanLineNGonLS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLS_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLS_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGS_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingGouraud, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonGS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGS_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGS_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineSolid(left, right, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendOpaque, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonLAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingNone, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingFlat, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGAT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGAT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLineAffine(left, right, kLightingGouraud, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonLIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTLIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMLIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTLIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingNone, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonFIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTFIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMFIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTFIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingFlat, kBlendMaskedAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonGIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendOpaque, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonTGIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendAlpha, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendMasked, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMGIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendMasked, 16);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGIT_8(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendMaskedAlpha, 8);
}

void RenderDroid::NRasterModule::drawScanLineNGonMTGIT_16(RdNRasterEdge *left, RdNRasterEdge *right) {
	drawScanLinePerspective(left, right, kLightingGouraud, kBlendMaskedAlpha, 16);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLS(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLS(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFS(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFS(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGS(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGS(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeGouraud);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeAffine);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeAffine);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeAffineFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeAffineFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgeAffineGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGAT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeAffineGouraud);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonLIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgePerspective);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonLIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonFIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgePerspectiveFlat);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonFIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeFlat);
}

int RenderDroid::NRasterModule::setupLeftEdgeNGonGIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _leftVertexStep, kEdgePerspectiveGouraud);
}

int RenderDroid::NRasterModule::setupRightEdgeNGonGIT(RdNRasterEdge *edge) {
	return setupEdge(edge, _rightVertexStep, kEdgeGouraud);
}

} // End of namespace Grim
