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

int RenderDroid::AFRasterModule::wrapVertex(int index, int lastVertex) {
	if (index < 0)
		return lastVertex;

	return (index > lastVertex) ? 0 : index;
}

int RenderDroid::AFRasterModule::selectRefVertex(RdProcFace *procFace, int *yMinVertPointer, int *yMaxVertPointer) {
	int numVertices = (int)procFace->numVertices;

	if (numVertices == 3) {
		*yMinVertPointer = 0;
		*yMaxVertPointer = 1;
		return 2;
	}

	int yMinVert = procFace->yMinIdx;
	int yMaxVert = procFace->yMaxIdx;
	int lastVertex = numVertices - 1;
	RdVector3 *vertices = procFace->vertices;

	*yMinVertPointer = yMinVert;
	*yMaxVertPointer = yMaxVert;

	int refVert = 0;
	bool refFound = true;
	int direction;

	int nextVert = wrapVertex(yMinVert + 1, lastVertex);
	int prevVert = wrapVertex(yMinVert - 1, lastVertex);

	if (nextVert == yMaxVert) {
		direction = -1;
		refFound = false;
	} else if (prevVert == yMaxVert) {
		direction = 1;
		refFound = false;
	} else {
		refVert = nextVert;

		int xDiffMax = abs((int)((double)vertices[refVert][0] - vertices[yMaxVert][0]));
		bool useSlope = (double)xDiffMax > 1.0;

		if (!useSlope) {
			int xDiffMin = abs((int)((double)vertices[refVert][0] - vertices[yMinVert][0]));
			useSlope = (double)xDiffMin > 1.0;
		}

		if (useSlope) {
			double yOnLine = ((double)vertices[yMaxVert][1] - vertices[yMinVert][1]) / ((double)vertices[yMaxVert][0] - vertices[yMinVert][0]) * ((double)vertices[refVert][0] - vertices[yMinVert][0]) + vertices[yMinVert][1];
			double deviation = (double)vertices[refVert][1] - yOnLine;
			if (deviation < 0.0)
				deviation = -deviation;

			direction = (deviation >= 1.0) ? 1 : -1;
		} else {
			direction = -1;
		}
	}

	float midX = (float)(((double)vertices[yMaxVert][0] + vertices[yMinVert][0]) * 0.5);
	float maxDistance = 0.0f;
	int firstWalk = wrapVertex(yMinVert + direction, lastVertex);
	int walk = firstWalk;

	while (walk != yMaxVert) {
		double distance = (double)vertices[walk][0] - midX;
		if (distance < 0.0)
			distance = -distance;

		if (distance > maxDistance) {
			maxDistance = (float)distance;
			refVert = walk;
			refFound = true;
		}

		walk = wrapVertex(walk + direction, lastVertex);
	}

	if (!refFound) {
		warning("RenderDroid::AFRasterModule::selectRefVertex(): no reference vertex found");
		refVert = firstWalk;
	}

	return refVert;
}

void RenderDroid::AFRasterModule::selectMipLevel(RdActiveFace *face, int refVert) {
	float zMin = face->procFace->zMin;
	float *mipDistances = _rd->_mipDistances;

	switch (refVert) {
	case 0:
		face->mipLevel = 0;
		break;
	case 1:
		face->mipLevel = (zMin > mipDistances[1]) ? 1 : 0;
		break;
	case 2:
		if (zMin > mipDistances[0])
			face->mipLevel = (zMin > mipDistances[1]) ? 2 : 1;
		else
			face->mipLevel = 0;
		break;
	case 3:
		if (zMin > mipDistances[0]) {
			if (zMin > mipDistances[1])
				face->mipLevel = (zMin > mipDistances[2]) ? 3 : 2;
			else
				face->mipLevel = 1;
		} else {
			face->mipLevel = 0;
		}
		break;
	default:
		break;
	}
}

void RenderDroid::AFRasterModule::calcAffineGradients(RdActiveFace *face, int refVert) {
	RdProcFace *procFace = face->procFace;
	RdVector3 *vertices = procFace->vertices;
	RdVector2 *texVertices = procFace->texVertices;
	int yMinVert;
	int yMaxVert;
	int refIdx = selectRefVertex(procFace, &yMinVert, &yMaxVert);

	for (int i = 0; i < (int)procFace->numVertices; i++)
		face->oneOverZ[i] = (float)(1.0 / vertices[i][2]);

	double dAy = (double)vertices[yMinVert][1] - vertices[refIdx][1];
	double dBy = (double)vertices[yMaxVert][1] - vertices[refIdx][1];
	float dAx = (float)((double)vertices[yMinVert][0] - vertices[refIdx][0]);
	float dBx = (float)((double)vertices[yMaxVert][0] - vertices[refIdx][0]);

	double dAu = (double)texVertices[yMinVert][0] - texVertices[refIdx][0];
	double dBu = (double)texVertices[yMaxVert][0] - texVertices[refIdx][0];
	float dBuF = (float)dBu;
	float dAuF = (float)dAu;
	float dBvF = (float)((double)texVertices[yMaxVert][1] - texVertices[refIdx][1]);
	float dAvF = (float)((double)texVertices[yMinVert][1] - texVertices[refIdx][1]);

	double det = dAy * dBx - dBy * dAx;
	double inverse = 1.0 / det;
	float inverseF = (float)inverse;
	float negInverseF = (float)-inverse;

	float uX = (float)((dBu * dAy - dAu * dBy) * inverseF);
	float uY = (float)(((double)dBuF * dAx - (double)dAuF * dBx) * negInverseF);
	float vX = (float)(((double)dBvF * dAy - (double)dAvF * dBy) * inverseF);
	float vY = (float)(((double)dBvF * dAx - (double)dAvF * dBx) * negInverseF);

	selectMipLevel(face, refVert);

	if (uX <= 0.0f) {
		if (uX < 0.0f || uY < 0.0f)
			face->uModifier = 0x7FFF;
		else
			face->uModifier = 0x8000;
	} else {
		face->uModifier = 0x8000;
	}

	if (vX <= 0.0f) {
		if (vX < 0.0f || vY < 0.0f)
			face->vModifier = 0x7FFF;
		else
			face->vModifier = 0x8000;
	} else {
		face->vModifier = 0x8000;
	}
}

void RenderDroid::AFRasterModule::calcPerspGradients(RdActiveFace *face, int refVert) {
	RdProcFace *procFace = face->procFace;
	RdVector3 *vertices = procFace->vertices;
	RdVector2 *texVertices = procFace->texVertices;
	int yMinVert;
	int yMaxVert;
	int refIdx = selectRefVertex(procFace, &yMinVert, &yMaxVert);

	for (int i = 0; i < (int)procFace->numVertices; i++) {
		double oneOverZ = 1.0 / vertices[i][2];
		face->oneOverZ[i] = (float)oneOverZ;
		face->uOverZ[i] = (float)(texVertices[i][0] * oneOverZ);
		face->vOverZ[i] = (float)(texVertices[i][1] * (double)face->oneOverZ[i]);
	}

	float dAy = (float)((double)vertices[yMinVert][1] - vertices[refIdx][1]);
	float dBy = (float)((double)vertices[yMaxVert][1] - vertices[refIdx][1]);
	float dAx = (float)((double)vertices[yMinVert][0] - vertices[refIdx][0]);
	float dBx = (float)((double)vertices[yMaxVert][0] - vertices[refIdx][0]);

	double dAyExact = (double)vertices[yMinVert][1] - vertices[refIdx][1];
	double dByExact = (double)vertices[yMaxVert][1] - vertices[refIdx][1];

	double det = dAyExact * dBx - dByExact * dAx;
	float inverseF = (float)(1.0 / det);
	float negInverseF = -inverseF;

	double dAz = (double)face->oneOverZ[yMinVert] - face->oneOverZ[refIdx];
	double dBz = (double)face->oneOverZ[yMaxVert] - face->oneOverZ[refIdx];
	float dBuF = (float)((double)face->uOverZ[yMaxVert] - face->uOverZ[refIdx]);
	float dAuF = (float)((double)face->uOverZ[yMinVert] - face->uOverZ[refIdx]);
	float dBvF = (float)((double)face->vOverZ[yMaxVert] - face->vOverZ[refIdx]);
	float dAvF = (float)((double)face->vOverZ[yMinVert] - face->vOverZ[refIdx]);

	double zX = (dBz * dAy - dAz * dBy) * inverseF;
	float zY = (float)((dBz * dAx - dAz * dBx) * negInverseF);
	face->dOneOverZdX = (float)zX;
	face->dOneOverZdY = zY;
	face->dOneOverZdXFixed = (int)(zX * 65536.0);

	double uX = ((double)dBuF * dAy - (double)dAuF * dBy) * inverseF;
	face->dUOverZdX = (float)uX;
	float uY = (float)(((double)dBuF * dAx - (double)dAuF * dBx) * negInverseF);
	face->dUOverZdY = uY;

	float vX = (float)(((double)dBvF * dAy - (double)dAvF * dBy) * inverseF);
	face->dVOverZdX = vX;
	float vY = (float)(((double)dBvF * dAx - (double)dAvF * dBx) * negInverseF);
	face->dVOverZdY = vY;

	selectMipLevel(face, refVert);

	double uCheck = uX * face->oneOverZ[yMinVert] - (double)face->uOverZ[yMinVert] * zX;
	if (uCheck <= 0.0) {
		if (uCheck < 0.0 || (double)face->oneOverZ[yMinVert] * uY - (double)face->uOverZ[yMinVert] * zY < 0.0)
			face->uModifier = 0x7FFF;
		else
			face->uModifier = 0x8000;
	} else {
		face->uModifier = 0x8000;
	}

	double vCheck = (double)face->oneOverZ[yMinVert] * vX - (double)face->vOverZ[yMinVert] * zX;
	if (vCheck <= 0.0) {
		if (vCheck < 0.0 || (double)face->oneOverZ[yMinVert] * vY - (double)face->vOverZ[yMinVert] * zY < 0.0)
			face->vModifier = 0x7FFF;
		else
			face->vModifier = 0x8000;
	} else {
		face->vModifier = 0x8000;
	}
}

int RenderDroid::AFRasterModule::getZShift(RdCanvas *canvas) {
	int shift = canvas->zScaleShift;

	if (shift < 0 || shift > 31) {
		warning("RenderDroid::AFRasterModule::getZShift(): z scale shift %d out of range", shift);
		shift &= 31;
	}

	return shift;
}

void RenderDroid::AFRasterModule::drawZSpans(RdActiveFace *face) {
	RdActiveSpan *span = face->firstSpan;

	if (_rd->_camera.getCurrent()->canvas->zScaleShift) {
		for (; span; span = span->next)
			drawZSpanShifted(span);
	} else {
		for (; span; span = span->next)
			drawZSpan(span);
	}
}

void RenderDroid::AFRasterModule::drawTexturedSpans(RdActiveFace *face) {
	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (_drawSpan)
			(this->*_drawSpan)(span);
	}

	drawZSpans(face);
}

RenderDroid::AFRasterModule::DrawSpanFunc RenderDroid::AFRasterModule::selectWidthSpan(int texPitch, const DrawSpanFunc *widthSpans, DrawSpanFunc defaultFunction) {
	switch (texPitch) {
	case 8:
		return widthSpans[0];
	case 16:
		return widthSpans[1];
	case 32:
		return widthSpans[2];
	case 64:
		return widthSpans[3];
	case 128:
		return widthSpans[4];
	case 256:
		return widthSpans[5];
	default:
		return defaultFunction;
	}
}

void RenderDroid::AFRasterModule::setupFlat(RdActiveFace *face, RdMatCel *cel, DrawFaceFunc drawSpans, SetupEdgeFunc setupEdge) {
	RdProcFace *procFace = face->procFace;

	for (uint i = 0; i < procFace->numVertices; i++)
		face->oneOverZ[i] = (float)(1.0 / procFace->vertices[i][2]);

	face->drawSpans = drawSpans;
	face->colorIndex = cel->colorIndex;
	face->setupEdge = setupEdge;
}

void RenderDroid::AFRasterModule::setupTextured(RdActiveFace *face, RdMatCel *cel, bool persp, int lightKind, float light, DrawFaceFunc drawSpans, SetupEdgeFunc setupEdge) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;
	RdTexture *texture = cel->texture;
	RdColormap *colormap = face->procFace->colormap;

	if (persp)
		calcPerspGradients(face, (int)texture->numMips - 1);
	else
		calcAffineGradients(face, (int)texture->numMips - 1);

	int mipLevel = face->mipLevel;

	if (persp) {
		face->dOneOverZdX16 = (float)((double)face->dOneOverZdX * 16.0);
		face->dUOverZdX16 = (float)((double)face->dUOverZdX * 16.0);
		face->dVOverZdX16 = (float)((double)face->dVOverZdX * 16.0);
	}

	if (lightKind == 1) {
		if (bpp == 8)
			face->lightLevel = colormap->lightLevel + 256 * (int)floor(light);
		else if (bpp == 16)
			face->rgb16 = colormap->rgb16 + 512 * (int)floor(light);
	} else if (lightKind == 2) {
		if (bpp == 8)
			face->lightLevel = colormap->lightLevel;
		else if (bpp == 16)
			face->rgb16 = colormap->rgb16;
	} else if (bpp == 16) {
		face->rgb16 = colormap->rgb16 + 32256;
	}

	face->texPixels = texture->mips[mipLevel]->pixels;
	face->texPitch = texture->mips[mipLevel]->tex.pitchInBytes;
	face->texShift = texture->widthLog2 - mipLevel;
	face->uWrap = (int)(((uint)texture->widthMask >> mipLevel) << 16);
	face->vWrap = (int)(((uint)texture->heightMask >> mipLevel) << face->texShift);
	face->drawSpans = drawSpans;
	face->setupEdge = setupEdge;
}

void RenderDroid::AFRasterModule::setupNGonLW(RdActiveFace *face, RdMatCel *cel) {
	setupFlat(face, cel, &AFRasterModule::drawNGonLW, &AFRasterModule::setupEdgeNGonLW);
}

void RenderDroid::AFRasterModule::drawNGonLW(RdActiveFace *face) {
	RdActiveSpan *span = face->firstSpan;
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8) {
		_color = (byte)face->colorIndex;

		for (; span; span = span->next)
			drawSpanNGonLW_8(span);
	} else if (bpp == 16) {
		_RGBcolor = ((uint16 *)face->procFace->colormap->rgb16)[(face->colorIndex & 0xFF) + 16128];

		for (; span; span = span->next)
			drawSpanNGonLW_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonLW_8(RdActiveSpan *span) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	byte *dst = vBuffer->pixels + vBuffer->tex.pitchInBytes * span->y + span->x;

	dst[0] = _color;
	dst[span->width - 1] = _color;
}

void RenderDroid::AFRasterModule::drawSpanNGonLW_16(RdActiveSpan *span) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	uint16 *dst = (uint16 *)(vBuffer->pixels + 2 * span->x + 2 * (vBuffer->tex.pitchInBytes >> 1) * span->y);

	dst[0] = _RGBcolor;
	dst[span->width - 1] = _RGBcolor;
}

void RenderDroid::AFRasterModule::advanceEdgeNGonLS(RdActiveEdge *edge) {
	edge->x += edge->dx;
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeAffine(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1, bool texture, bool intensity, StepEdgeFunc step) {
	RdProcFace *procFace = face->procFace;
	RdVector3 *vertices = procFace->vertices;
	RdVector2 *texVertices = procFace->texVertices;
	float *intensities = procFace->intensities;

	float y0 = (float)ceil(vertices[v0][1]);
	float y1 = (float)ceil(vertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	edge->numLines = numLines;
	if (numLines == 0)
		return 0;

	int top;
	int bottom;
	float yTop;

	if (numLines > 0) {
		edge->left = 1;
		edge->yStart = yStart0;
		top = v0;
		bottom = v1;
		yTop = y0;
	} else {
		edge->left = 0;
		numLines = -numLines;
		edge->yStart = yStart1;
		edge->numLines = numLines;
		top = v1;
		bottom = v0;
		yTop = y1;
	}

	edge->step = step;

	if (numLines < 2) {
		edge->x = (int)floor((float)(vertices[top][0] * 65536.0));

		if (texture) {
			edge->u = (int)floor((float)(texVertices[top][0] * 65536.0));
			edge->v = (int)floor((float)(texVertices[top][1] * 65536.0));
		}

		if (intensity)
			edge->intensity = (int)floor((float)(intensities[top] * 65536.0));

		edge->oneOverZ = face->oneOverZ[top];
		return 1;
	}

	float dy = (float)(yTop - vertices[top][1]);
	float inverseDy = (float)(1.0 / ((double)vertices[bottom][1] - vertices[top][1]));

	double slopeX = ((double)vertices[bottom][0] - vertices[top][0]) * inverseDy;
	float slopeXF = (float)slopeX;
	edge->dx = (int)floor((float)(slopeX * 65536.0));
	edge->x = (int)floor((float)(((double)slopeXF * dy + vertices[top][0]) * 65536.0));

	if (texture) {
		double slopeU = ((double)texVertices[bottom][0] - texVertices[top][0]) * inverseDy;
		float slopeUF = (float)slopeU;
		edge->du = (int)floor((float)(slopeU * 65536.0));
		edge->u = (int)floor((float)(((double)slopeUF * dy + texVertices[top][0]) * 65536.0));

		double slopeV = ((double)texVertices[bottom][1] - texVertices[top][1]) * inverseDy;
		float slopeVF = (float)slopeV;
		edge->dv = (int)floor((float)(slopeV * 65536.0));
		edge->v = (int)floor((float)(((double)slopeVF * dy + texVertices[top][1]) * 65536.0));
	}

	if (intensity) {
		double slopeI = ((double)intensities[bottom] - intensities[top]) * inverseDy;
		float slopeIF = (float)slopeI;
		edge->dIntensity = (int)floor((float)(slopeI * 65536.0));
		edge->intensity = (int)floor((float)(((double)slopeIF * dy + intensities[top]) * 65536.0));
	}

	double slopeZ = ((double)face->oneOverZ[bottom] - face->oneOverZ[top]) * inverseDy;
	edge->dOneOverZ = (float)slopeZ;
	edge->oneOverZ = (float)((double)dy * slopeZ + face->oneOverZ[top]);

	return 1;
}

int RenderDroid::AFRasterModule::setupEdgeNGonLW(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeAffine(edge, face, v0, v1, false, false, &AFRasterModule::advanceEdgeNGonLS);
}

void RenderDroid::AFRasterModule::setupNGonLS(RdActiveFace *face, RdMatCel *cel) {
	setupFlat(face, cel, &AFRasterModule::drawNGonLS, &AFRasterModule::setupEdgeNGonLS);
}

void RenderDroid::AFRasterModule::drawNGonLS(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8)
		_color = (byte)face->colorIndex;
	else if (bpp == 16)
		_RGBcolor = ((uint16 *)face->procFace->colormap->rgb16)[(face->colorIndex & 0xFF) + 16128];

	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	if (bpp == 8) {
		for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
			StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
			memset(vBuffer->pixels + vBuffer->tex.pitchInBytes * span->y + span->x, _color, span->width);
		}
	} else if (bpp == 16) {
		for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
			StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
			uint16 *dst = (uint16 *)(vBuffer->pixels + 2 * span->x + 2 * (vBuffer->tex.pitchInBytes >> 1) * span->y);

			for (int i = 0; i < span->width; i++)
				dst[i] = _RGBcolor;
		}
	}

	drawZSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLS(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeNGonLW(edge, face, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonFS(RdActiveFace *face, RdMatCel *cel, float light) {
	RdProcFace *procFace = face->procFace;

	for (uint i = 0; i < procFace->numVertices; i++)
		face->oneOverZ[i] = (float)(1.0 / procFace->vertices[i][2]);

	face->drawSpans = &AFRasterModule::drawNGonFS;
	face->colorIndex = procFace->colormap->lightLevel[cel->colorIndex + 256 * (int)floor(light)];
	face->setupEdge = &AFRasterModule::setupEdgeNGonFS;
}

void RenderDroid::AFRasterModule::drawNGonFS(RdActiveFace *face) {
	_color = (byte)face->colorIndex;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
		memset(vBuffer->pixels + vBuffer->tex.pitchInBytes * span->y + span->x, _color, span->width);
	}

	drawZSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFS(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeNGonLW(edge, face, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonGS(RdActiveFace *face, RdMatCel *cel) {
	RdProcFace *procFace = face->procFace;
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	for (uint i = 0; i < procFace->numVertices; i++)
		face->oneOverZ[i] = (float)(1.0 / procFace->vertices[i][2]);

	face->colorIndex = cel->colorIndex;

	if (bpp == 8)
		face->lightLevel = procFace->colormap->lightLevel;
	else if (bpp == 16)
		face->rgb16 = procFace->colormap->rgb16;

	face->drawSpans = &AFRasterModule::drawNGonGS;
	face->setupEdge = &AFRasterModule::setupEdgeNGonGS;
}

void RenderDroid::AFRasterModule::drawNGonGS(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	_color = (byte)face->colorIndex;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	if (bpp == 8) {
		for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
			StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
			int intensity = span->intensity;
			byte *dst = vBuffer->pixels + vBuffer->tex.pitchInBytes * span->y + span->x;

			for (int i = 0; i < span->width; i++) {
				dst[i] = _monoMap[((intensity >> 8) & 0x3F00) + _color];
				intensity += span->dIntensity;
			}
		}
	} else if (bpp == 16) {
		for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
			StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
			int intensity = span->intensity;
			uint16 *dst = (uint16 *)(vBuffer->pixels + 2 * span->x + 2 * (vBuffer->tex.pitchInBytes >> 1) * span->y);

			for (int i = 0; i < span->width; i++) {
				dst[i] = ((uint16 *)_rgbMap)[_color + (((intensity >> 16) & 0x3F) << 8)];
				intensity += span->dIntensity;
			}
		}
	}

	drawZSpans(face);
}

void RenderDroid::AFRasterModule::advanceEdgeNGonGS(RdActiveEdge *edge) {
	edge->x += edge->dx;
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
	edge->intensity += edge->dIntensity;
}

int RenderDroid::AFRasterModule::setupEdgeNGonGS(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeAffine(edge, face, v0, v1, false, true, &AFRasterModule::advanceEdgeNGonGS);
}

void RenderDroid::AFRasterModule::setupNGonLAT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, false, 0, 0.0f, &AFRasterModule::drawNGonLAT, &AFRasterModule::setupEdgeNGonLAT);
}

void RenderDroid::AFRasterModule::drawNGonLAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 16)
		_rgbMap = face->rgb16;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;
	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonLAT_16_W8,
		&AFRasterModule::drawSpanNGonLAT_16_W16,
		&AFRasterModule::drawSpanNGonLAT_16_W32,
		&AFRasterModule::drawSpanNGonLAT_16_W64,
		&AFRasterModule::drawSpanNGonLAT_16_W128,
		&AFRasterModule::drawSpanNGonLAT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonLAT_8_W8,
		&AFRasterModule::drawSpanNGonLAT_8_W16,
		&AFRasterModule::drawSpanNGonLAT_8_W32,
		&AFRasterModule::drawSpanNGonLAT_8_W64,
		&AFRasterModule::drawSpanNGonLAT_8_W128,
		&AFRasterModule::drawSpanNGonLAT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonLAT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonLAT_16);

	drawTexturedSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLAT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeAffine(edge, face, v0, v1, true, false, &AFRasterModule::advanceLeftEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonFAT(RdActiveEdge *edge) {
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
	edge->x += edge->dx;
	edge->u += edge->du;
	edge->v += edge->dv;
}

void RenderDroid::AFRasterModule::setupNGonMLAT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, false, 0, 0.0f, &AFRasterModule::drawNGonMLAT, &AFRasterModule::setupEdgeNGonLAT);
}

void RenderDroid::AFRasterModule::drawNGonMLAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 16)
		_rgbMap = face->rgb16;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;
	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMLAT_8(span);
		else if (bpp == 16)
			drawSpanNGonMLAT_16(span);
	}
}

void RenderDroid::AFRasterModule::plotMaskedRun(SpanRun &run, int count, bool bpp16, int lightKind) {
	for (int i = 0; i < count; i++) {
		int texel = _pixels[(_vWrap & (run.v >> (16 - _texRowShift))) + ((_uWrap >> 16) & (run.u >> 16))];

		if (texel) {
			if (bpp16) {
				int index = texel;
				if (lightKind == 2)
					index += ((run.intensity >> 16) & 0x3F) << 8;

				*(uint16 *)run.dst = ((uint16 *)_rgbMap)[index];
			} else if (lightKind == 0) {
				*run.dst = (byte)texel;
			} else {
				int index = texel;
				if (lightKind == 2)
					index += (run.intensity >> 8) & 0x3F00;

				*run.dst = _monoMap[index];
			}

			*run.zPointer = (uint16)(run.z >> run.zShift);
		}

		run.z += run.dz;
		run.u += run.du;
		run.v += run.dv;

		if (lightKind == 2)
			run.intensity += run.dIntensity;

		run.dst += bpp16 ? 2 : 1;
		run.zPointer++;
	}
}

void RenderDroid::AFRasterModule::drawSpanMaskedAffine(RdActiveSpan *span, bool bpp16, int lightKind) {
	RdCanvas *canvas = _rd->_camera.getCurrent()->canvas;
	StdVBuffer *vBuffer = canvas->vBuffer;
	StdVBuffer *zBuffer = canvas->zBuffer;
	SpanRun run;

	run.zShift = getZShift(canvas);
	run.u = span->u;
	run.du = span->du;
	run.dv = span->dv;
	run.v = span->v;
	run.dz = _dOneOverZdXFixed;
	run.z = (int)floor((float)(span->oneOverZ * 65536.0));
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = span->intensity;
		run.dIntensity = span->dIntensity;
	}

	if (bpp16)
		run.dst = vBuffer->pixels + 2 * span->x + 2 * span->y * (vBuffer->tex.pitchInBytes >> 1);
	else
		run.dst = vBuffer->pixels + span->y * vBuffer->tex.pitchInBytes + span->x;

	run.zPointer = (uint16 *)(zBuffer->pixels + 2 * span->x + span->y * zBuffer->tex.pitchInBytes);

	plotMaskedRun(run, span->width, bpp16, lightKind);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLAT_8(RdActiveSpan *span) {
	drawSpanMaskedAffine(span, false, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLAT_16(RdActiveSpan *span) {
	drawSpanMaskedAffine(span, true, 0);
}

void RenderDroid::AFRasterModule::setupNGonFAT(RdActiveFace *face, RdMatCel *cel, float light) {
	setupTextured(face, cel, false, 1, light, &AFRasterModule::drawNGonFAT, &AFRasterModule::setupEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::drawNGonFAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonFAT_16_W8,
		&AFRasterModule::drawSpanNGonFAT_16_W16,
		&AFRasterModule::drawSpanNGonFAT_16_W32,
		&AFRasterModule::drawSpanNGonFAT_16_W64,
		&AFRasterModule::drawSpanNGonFAT_16_W128,
		&AFRasterModule::drawSpanNGonFAT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonFAT_8_W8,
		&AFRasterModule::drawSpanNGonFAT_8_W16,
		&AFRasterModule::drawSpanNGonFAT_8_W32,
		&AFRasterModule::drawSpanNGonFAT_8_W64,
		&AFRasterModule::drawSpanNGonFAT_8_W128,
		&AFRasterModule::drawSpanNGonFAT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonFAT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonFAT_16);

	drawTexturedSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFAT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeNGonLAT(edge, face, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonMFAT(RdActiveFace *face, RdMatCel *cel, float light) {
	setupTextured(face, cel, false, 1, light, &AFRasterModule::drawNGonMFAT, &AFRasterModule::setupEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::drawNGonMFAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMFAT_8(span);
		else if (bpp == 16)
			drawSpanNGonMFAT_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMFAT_8(RdActiveSpan *span) {
	drawSpanMaskedAffine(span, false, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonMFAT_16(RdActiveSpan *span) {
	drawSpanNGonMLAT_16(span);
}

void RenderDroid::AFRasterModule::setupNGonGAT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, false, 2, 0.0f, &AFRasterModule::drawNGonGAT, &AFRasterModule::setupEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::drawNGonGAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonGAT_16_W8,
		&AFRasterModule::drawSpanNGonGAT_16_W16,
		&AFRasterModule::drawSpanNGonGAT_16_W32,
		&AFRasterModule::drawSpanNGonGAT_16_W64,
		&AFRasterModule::drawSpanNGonGAT_16_W128,
		&AFRasterModule::drawSpanNGonGAT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonGAT_8_W8,
		&AFRasterModule::drawSpanNGonGAT_8_W16,
		&AFRasterModule::drawSpanNGonGAT_8_W32,
		&AFRasterModule::drawSpanNGonGAT_8_W64,
		&AFRasterModule::drawSpanNGonGAT_8_W128,
		&AFRasterModule::drawSpanNGonGAT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonGAT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonGAT_16);

	drawTexturedSpans(face);
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonGAT(RdActiveEdge *edge) {
	edge->x += edge->dx;
	edge->u += edge->du;
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
	edge->v += edge->dv;
	edge->intensity += edge->dIntensity;
}

int RenderDroid::AFRasterModule::setupEdgeNGonGAT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeAffine(edge, face, v0, v1, true, true, &AFRasterModule::advanceLeftEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::setupNGonMGAT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, false, 2, 0.0f, &AFRasterModule::drawNGonMGAT, &AFRasterModule::setupEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::drawNGonMGAT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMGAT_8(span);
		else if (bpp == 16)
			drawSpanNGonMGAT_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMGAT_8(RdActiveSpan *span) {
	drawSpanMaskedAffine(span, false, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonMGAT_16(RdActiveSpan *span) {
	drawSpanMaskedAffine(span, true, 2);
}

void RenderDroid::AFRasterModule::setupNGonLIT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, true, 0, 0.0f, &AFRasterModule::drawNGonLIT, &AFRasterModule::setupEdgeNGonLIT);
}

void RenderDroid::AFRasterModule::loadPerspState(RdActiveFace *face) {
	_mipMapNum = face->mipLevel;
	_pixels = face->texPixels;
	_texRowShift = face->texShift;
	_texPitch = face->texPitch;
	_uWrap = face->uWrap;
	_vWrap = face->vWrap;
	_oneOverZStep = face->dOneOverZdX16;
	_uOverZStep = face->dUOverZdX16;
	_vOverZStep = face->dVOverZdX16;
	_dOneOverZdXFixed = face->dOneOverZdXFixed;
	_dOneOverZdX = face->dOneOverZdX;
	_dUOverZdX = face->dUOverZdX;
	_dVOverZdX = face->dVOverZdX;
	_dUdXModifier = face->uModifier;
	_dVdXModifier = face->vModifier;
}

void RenderDroid::AFRasterModule::drawNGonLIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonLIT_16_W8,
		&AFRasterModule::drawSpanNGonLIT_16_W16,
		&AFRasterModule::drawSpanNGonLIT_16_W32,
		&AFRasterModule::drawSpanNGonLIT_16_W64,
		&AFRasterModule::drawSpanNGonLIT_16_W128,
		&AFRasterModule::drawSpanNGonLIT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonLIT_8_W8,
		&AFRasterModule::drawSpanNGonLIT_8_W16,
		&AFRasterModule::drawSpanNGonLIT_8_W32,
		&AFRasterModule::drawSpanNGonLIT_8_W64,
		&AFRasterModule::drawSpanNGonLIT_8_W128,
		&AFRasterModule::drawSpanNGonLIT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonLIT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonLIT_16);

	drawTexturedSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLIT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeNGonPerspective(edge, face, v0, v1, 0);
}

void RenderDroid::AFRasterModule::setEdgePerspectiveValues(RdActiveEdge *edge, RdActiveFace *face, int v0, float dy, double xOffset, int variant) {
	double yOneOverZ = (double)face->dOneOverZdY * dy;
	double yUOverZ = (double)face->dUOverZdY * dy;
	double yVOverZ = (double)face->dVOverZdY * dy;
	double xOneOverZ = face->dOneOverZdX * xOffset;
	double xUOverZ = face->dUOverZdX * xOffset;
	double xVOverZ = face->dVOverZdX * xOffset;
	bool oneYFirst = true;
	bool vYFirst = false;

	// The demo adds the vertex value to one of the two terms first, in a different order for each variant
	if (_rd->isDemo()) {
		if (variant == 0)
			vYFirst = true;
		else
			oneYFirst = false;
	}

	if (oneYFirst)
		edge->oneOverZ = (float)((yOneOverZ + face->oneOverZ[v0]) + xOneOverZ);
	else
		edge->oneOverZ = (float)((xOneOverZ + face->oneOverZ[v0]) + yOneOverZ);

	edge->uOverZ = (float)((yUOverZ + face->uOverZ[v0]) + xUOverZ);

	if (vYFirst)
		edge->vOverZ = (float)((yVOverZ + face->vOverZ[v0]) + xVOverZ);
	else
		edge->vOverZ = (float)((xVOverZ + face->vOverZ[v0]) + yVOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeNGonPerspective(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1, int variant) {
	RdProcFace *procFace = face->procFace;
	RdVector3 *vertices = procFace->vertices;

	float y0 = (float)ceil(vertices[v0][1]);
	float y1 = (float)ceil(vertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	edge->numLines = numLines;
	if (numLines == 0)
		return 0;

	if (numLines <= 0) {
		edge->left = 0;
		numLines = -numLines;
		edge->yStart = yStart1;
		edge->numLines = numLines;
		edge->step = &AFRasterModule::advanceRightEdgeNGonLIT;

		if (numLines >= 2) {
			float dy = (float)(y1 - vertices[v1][1]);
			double slopeX = ((double)vertices[v0][0] - vertices[v1][0]) / ((double)vertices[v0][1] - vertices[v1][1]);
			float slopeXF = (float)slopeX;
			edge->dx = (int)floor((float)(slopeX * 65536.0));
			edge->x = (int)floor((float)(((double)slopeXF * dy + vertices[v1][0]) * 65536.0));
		} else {
			edge->x = (int)floor((float)(vertices[v1][0] * 65536.0));
		}

		return 1;
	}

	edge->left = 1;
	edge->yStart = yStart0;
	edge->step = &AFRasterModule::advanceLeftEdgeNGonFIT;

	float dy = (float)(y0 - vertices[v0][1]);
	float startX;

	if (numLines >= 2) {
		double slopeX = ((double)vertices[v1][0] - vertices[v0][0]) / ((double)vertices[v1][1] - vertices[v0][1]);
		float slopeXF = (float)slopeX;
		edge->dx = (int)floor((float)(slopeX * 65536.0));

		double xSum = (double)slopeXF * dy + vertices[v0][0];
		startX = (float)xSum;
		edge->x = (int)floor((float)(xSum * 65536.0));

		edge->dOneOverZ = (float)((double)face->dOneOverZdX * slopeXF + face->dOneOverZdY);
		edge->dUOverZ = (float)((double)face->dUOverZdX * slopeXF + face->dUOverZdY);
		edge->dVOverZ = (float)((double)face->dVOverZdX * slopeXF + face->dVOverZdY);
	} else {
		startX = vertices[v0][0];
		edge->x = (int)floor((float)(startX * 65536.0));
	}

	setEdgePerspectiveValues(edge, face, v0, dy, (double)startX - vertices[v0][0], variant);

	return 1;
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonFIT(RdActiveEdge *edge) {
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
	edge->uOverZ = (float)((double)edge->dUOverZ + edge->uOverZ);
	edge->vOverZ = (float)((double)edge->dVOverZ + edge->vOverZ);
	edge->x += edge->dx;
}

void RenderDroid::AFRasterModule::advanceRightEdgeNGonLIT(RdActiveEdge *edge) {
	edge->x += edge->dx;
}

void RenderDroid::AFRasterModule::setupNGonMLIT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, true, 0, 0.0f, &AFRasterModule::drawNGonMLIT, &AFRasterModule::setupEdgeNGonLIT);
}

void RenderDroid::AFRasterModule::drawNGonMLIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMLIT_8(span);
		else if (bpp == 16)
			drawSpanNGonMLIT_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanMaskedPersp(RdActiveSpan *span, bool bpp16, int lightKind) {
	RdCanvas *canvas = _rd->_camera.getCurrent()->canvas;
	StdVBuffer *vBuffer = canvas->vBuffer;
	StdVBuffer *zBuffer = canvas->zBuffer;
	SpanRun run;

	run.zShift = getZShift(canvas);

	int xAlign = span->x & 0xF;
	int width = 0;

	if (xAlign) {
		width = span->width;
		if (width >= 16 - xAlign)
			width = 16 - xAlign;
	}

	int remaining = span->width - width;
	int numChunks = 0;
	int tail = 0;

	if (remaining) {
		numChunks = remaining / 16;
		tail = remaining % 16;
		if (!tail) {
			tail = 16;
			numChunks--;
		}
	}

	float oneOverZ = span->oneOverZ;
	float uOverZ = span->uOverZ;
	float vOverZ = span->vOverZ;

	run.dz = _dOneOverZdXFixed;
	run.z = (int)floor((float)(oneOverZ * 65536.0));
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = span->intensity;
		run.dIntensity = span->dIntensity;
	}

	// The EXE switches the FPU to 24-bit precision rounding toward zero for the whole span
	_lowPrecision = true;

	double inverse = fpuDiv(1.0, oneOverZ);

	if (bpp16)
		run.dst = vBuffer->pixels + 2 * span->x + 2 * span->y * (vBuffer->tex.pitchInBytes >> 1);
	else
		run.dst = vBuffer->pixels + span->y * vBuffer->tex.pitchInBytes + span->x;

	run.zPointer = (uint16 *)(zBuffer->pixels + 2 * span->x + span->y * zBuffer->tex.pitchInBytes);

	float uStart = (float)fpuMul(inverse, uOverZ);
	float vStart = (float)fpuMul(inverse, vOverZ);

	run.du = 0;
	run.dv = 0;
	run.u = _dUdXModifier + ((int)floor((float)(uStart * 65536.0)) >> _mipMapNum);
	run.v = _dVdXModifier + ((int)floor((float)(vStart * 65536.0)) >> _mipMapNum);

	float nextZ;
	float nextU;
	float nextV;

	if (width) {
		float inverseWidth = _rd->_raster.getOneOverNFlex(width);

		nextZ = (float)fpuAdd(fpuMul(width, _dOneOverZdX), oneOverZ);
		nextU = (float)fpuAdd(fpuMul(width, _dUOverZdX), uOverZ);
		nextV = (float)fpuAdd(fpuMul(width, _dVOverZdX), vOverZ);

		double inverseEnd = fpuDiv(1.0, nextZ);
		float uEnd = (float)fpuMul(inverseEnd, nextU);
		float vEnd = (float)fpuMul(inverseEnd, nextV);

		run.du = (int)floor((float)fpuMul(fpuMul(fpuSub(uEnd, uStart), inverseWidth), 65536.0)) >> _mipMapNum;
		run.dv = (int)floor((float)fpuMul(fpuMul(fpuSub(vEnd, vStart), inverseWidth), 65536.0)) >> _mipMapNum;

		plotMaskedRun(run, width, bpp16, lightKind);

		uStart = uEnd;
		vStart = vEnd;
		nextZ = (float)fpuAdd(nextZ, _oneOverZStep);
		nextU = (float)fpuAdd(nextU, _uOverZStep);
		nextV = (float)fpuAdd(nextV, _vOverZStep);
	} else {
		nextZ = (float)fpuAdd(oneOverZ, _oneOverZStep);
		nextU = (float)fpuAdd(uOverZ, _uOverZStep);
		nextV = (float)fpuAdd(vOverZ, _vOverZStep);
	}

	for (int chunk = 0; chunk < numChunks; chunk++) {
		double inverseEnd = fpuDiv(1.0, nextZ);
		float uEnd = (float)fpuMul(inverseEnd, nextU);
		float vEnd = (float)fpuMul(inverseEnd, nextV);

		run.du = (int)floor((float)fpuMul(fpuMul(fpuSub(uEnd, uStart), 0.0625), 65536.0)) >> _mipMapNum;
		run.dv = (int)floor((float)fpuMul(fpuMul(fpuSub(vEnd, vStart), 0.0625), 65536.0)) >> _mipMapNum;
		run.u = _dUdXModifier + ((int)floor((float)(uStart * 65536.0)) >> _mipMapNum);
		run.v = _dVdXModifier + ((int)floor((float)(vStart * 65536.0)) >> _mipMapNum);

		plotMaskedRun(run, 16, bpp16, lightKind);

		vStart = vEnd;
		uStart = uEnd;
		nextZ = (float)fpuAdd(nextZ, _oneOverZStep);
		nextU = (float)fpuAdd(nextU, _uOverZStep);
		nextV = (float)fpuAdd(nextV, _vOverZStep);
	}

	if (tail) {
		int count = tail - 1;

		if (count != 0) {
			double steps = (double)count;
			float inverseCount = _rd->_raster.getOneOverNFlex(count);
			double inverseEnd = fpuDiv(1.0, fpuAdd(fpuSub(fpuMul(steps, _dOneOverZdX), _oneOverZStep), nextZ));
			float uEnd = (float)fpuMul(fpuAdd(fpuSub(fpuMul(steps, _dUOverZdX), _uOverZStep), nextU), inverseEnd);
			float vEnd = (float)fpuMul(fpuAdd(fpuSub(fpuMul(steps, _dVOverZdX), _vOverZStep), nextV), inverseEnd);

			run.du = (int)floor((float)fpuMul(fpuMul(fpuSub(uEnd, uStart), inverseCount), 65536.0)) >> _mipMapNum;
			run.dv = (int)floor((float)fpuMul(fpuMul(fpuSub(vEnd, vStart), inverseCount), 65536.0)) >> _mipMapNum;
			run.u = _dUdXModifier + ((int)floor((float)(uStart * 65536.0)) >> _mipMapNum);
			run.v = _dVdXModifier + ((int)floor((float)(vStart * 65536.0)) >> _mipMapNum);
		}

		plotMaskedRun(run, tail, bpp16, lightKind);
	}

	_lowPrecision = false;
}

void RenderDroid::AFRasterModule::drawSpanNGonMLIT_8(RdActiveSpan *span) {
	drawSpanMaskedPersp(span, false, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLIT_16(RdActiveSpan *span) {
	drawSpanMaskedPersp(span, true, 0);
}

void RenderDroid::AFRasterModule::setupNGonFIT(RdActiveFace *face, RdMatCel *cel, float light) {
	setupTextured(face, cel, true, 1, light, &AFRasterModule::drawNGonFIT, &AFRasterModule::setupEdgeNGonFIT);
}

void RenderDroid::AFRasterModule::drawNGonFIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonFIT_16_W8,
		&AFRasterModule::drawSpanNGonFIT_16_W16,
		&AFRasterModule::drawSpanNGonFIT_16_W32,
		&AFRasterModule::drawSpanNGonFIT_16_W64,
		&AFRasterModule::drawSpanNGonFIT_16_W128,
		&AFRasterModule::drawSpanNGonFIT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonFIT_8_W8,
		&AFRasterModule::drawSpanNGonFIT_8_W16,
		&AFRasterModule::drawSpanNGonFIT_8_W32,
		&AFRasterModule::drawSpanNGonFIT_8_W64,
		&AFRasterModule::drawSpanNGonFIT_8_W128,
		&AFRasterModule::drawSpanNGonFIT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonFIT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonFIT_16);

	drawTexturedSpans(face);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFIT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	return setupEdgeNGonPerspective(edge, face, v0, v1, 1);
}

void RenderDroid::AFRasterModule::setupNGonMFIT(RdActiveFace *face, RdMatCel *cel, float light) {
	setupTextured(face, cel, true, 1, light, &AFRasterModule::drawNGonMFIT, &AFRasterModule::setupEdgeNGonFIT);
}

void RenderDroid::AFRasterModule::drawNGonMFIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMFIT_8(span);
		else if (bpp == 16)
			drawSpanNGonMFIT_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMFIT_8(RdActiveSpan *span) {
	drawSpanMaskedPersp(span, false, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonMFIT_16(RdActiveSpan *span) {
	drawSpanNGonMLIT_16(span);
}

void RenderDroid::AFRasterModule::setupNGonGIT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, true, 2, 0.0f, &AFRasterModule::drawNGonGIT, &AFRasterModule::setupEdgeNGonGIT);
}

void RenderDroid::AFRasterModule::drawNGonGIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	DrawSpanFunc widthSpans[6] = {
		&AFRasterModule::drawSpanNGonGIT_16_W8,
		&AFRasterModule::drawSpanNGonGIT_16_W16,
		&AFRasterModule::drawSpanNGonGIT_16_W32,
		&AFRasterModule::drawSpanNGonGIT_16_W64,
		&AFRasterModule::drawSpanNGonGIT_16_W128,
		&AFRasterModule::drawSpanNGonGIT_16_W256
	};

	DrawSpanFunc widthSpans8[6] = {
		&AFRasterModule::drawSpanNGonGIT_8_W8,
		&AFRasterModule::drawSpanNGonGIT_8_W16,
		&AFRasterModule::drawSpanNGonGIT_8_W32,
		&AFRasterModule::drawSpanNGonGIT_8_W64,
		&AFRasterModule::drawSpanNGonGIT_8_W128,
		&AFRasterModule::drawSpanNGonGIT_8_W256
	};

	if (bpp == 8)
		_drawSpan = _rd->isDemo() ? selectWidthSpan(face->texPitch, widthSpans8, &AFRasterModule::drawSpanNGonGIT_8) : nullptr;
	else if (bpp == 16)
		_drawSpan = selectWidthSpan(face->texPitch, widthSpans, &AFRasterModule::drawSpanNGonGIT_16);

	drawTexturedSpans(face);
}

void RenderDroid::AFRasterModule::advanceEdgeNGonGIT(RdActiveEdge *edge) {
	edge->x += edge->dx;
	edge->intensity += edge->dIntensity;
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonGIT(RdActiveEdge *edge) {
	edge->oneOverZ = (float)((double)edge->dOneOverZ + edge->oneOverZ);
	edge->x += edge->dx;
	edge->uOverZ = (float)((double)edge->dUOverZ + edge->uOverZ);
	edge->intensity += edge->dIntensity;
	edge->vOverZ = (float)((double)edge->dVOverZ + edge->vOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeNGonGIT(RdActiveEdge *edge, RdActiveFace *face, int v0, int v1) {
	RdProcFace *procFace = face->procFace;
	RdVector3 *vertices = procFace->vertices;
	float *intensities = procFace->intensities;

	float y0 = (float)ceil(vertices[v0][1]);
	float y1 = (float)ceil(vertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	edge->numLines = numLines;
	if (numLines == 0)
		return 0;

	if (numLines <= 0) {
		edge->left = 0;
		numLines = -numLines;
		edge->yStart = yStart1;
		edge->numLines = numLines;
		edge->step = &AFRasterModule::advanceEdgeNGonGIT;

		if (numLines >= 2) {
			float dy = (float)(y1 - vertices[v1][1]);
			float inverseDy = (float)(1.0 / ((double)vertices[v0][1] - vertices[v1][1]));

			double slopeX = ((double)vertices[v0][0] - vertices[v1][0]) * inverseDy;
			float slopeXF = (float)slopeX;
			edge->dx = (int)floor((float)(slopeX * 65536.0));
			edge->x = (int)floor((float)(((double)slopeXF * dy + vertices[v1][0]) * 65536.0));

			double slopeI = ((double)intensities[v0] - intensities[v1]) * inverseDy;
			float slopeIF = (float)slopeI;
			edge->dIntensity = (int)floor((float)(slopeI * 65536.0));
			edge->intensity = (int)floor((float)(((double)slopeIF * dy + intensities[v1]) * 65536.0));
		} else {
			edge->x = (int)floor((float)(vertices[v1][0] * 65536.0));
			edge->intensity = (int)floor((float)(intensities[v1] * 65536.0));
		}

		return 1;
	}

	edge->left = 1;
	edge->yStart = yStart0;
	edge->step = &AFRasterModule::advanceLeftEdgeNGonGIT;

	float dy = (float)(y0 - vertices[v0][1]);
	float startX;

	if (numLines >= 2) {
		float inverseDy = (float)(1.0 / ((double)vertices[v1][1] - vertices[v0][1]));

		double slopeX = ((double)vertices[v1][0] - vertices[v0][0]) * inverseDy;
		float slopeXF = (float)slopeX;
		edge->dx = (int)floor((float)(slopeX * 65536.0));

		double xSum = (double)slopeXF * dy + vertices[v0][0];
		startX = (float)xSum;
		edge->x = (int)floor((float)(xSum * 65536.0));

		edge->dOneOverZ = (float)((double)face->dOneOverZdX * slopeXF + face->dOneOverZdY);
		edge->dUOverZ = (float)((double)face->dUOverZdX * slopeXF + face->dUOverZdY);
		edge->dVOverZ = (float)((double)face->dVOverZdX * slopeXF + face->dVOverZdY);

		double slopeI = ((double)intensities[v1] - intensities[v0]) * inverseDy;
		float slopeIF = (float)slopeI;
		edge->dIntensity = (int)floor((float)(slopeI * 65536.0));
		edge->intensity = (int)floor((float)(((double)slopeIF * dy + intensities[v0]) * 65536.0));
	} else {
		startX = vertices[v0][0];
		edge->x = (int)floor((float)(startX * 65536.0));
		edge->intensity = (int)floor((float)(intensities[v0] * 65536.0));
	}

	setEdgePerspectiveValues(edge, face, v0, dy, (double)startX - vertices[v0][0], 2);

	return 1;
}

void RenderDroid::AFRasterModule::setupNGonMGIT(RdActiveFace *face, RdMatCel *cel) {
	setupTextured(face, cel, true, 2, 0.0f, &AFRasterModule::drawNGonMGIT, &AFRasterModule::setupEdgeNGonGIT);
}

void RenderDroid::AFRasterModule::drawNGonMGIT(RdActiveFace *face) {
	int bpp = _rd->_camera.getCurrent()->canvas->vBuffer->tex.format.bpp;

	if (bpp == 8)
		_monoMap = face->lightLevel;
	else if (bpp == 16)
		_rgbMap = face->rgb16;

	loadPerspState(face);

	for (RdActiveSpan *span = face->firstSpan; span; span = span->next) {
		if (bpp == 8)
			drawSpanNGonMGIT_8(span);
		else if (bpp == 16)
			drawSpanNGonMGIT_16(span);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMGIT_8(RdActiveSpan *span) {
	drawSpanMaskedPersp(span, false, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonMGIT_16(RdActiveSpan *span) {
	drawSpanMaskedPersp(span, true, 2);
}

void RenderDroid::AFRasterModule::drawZSpan(RdActiveSpan *span) {
	StdVBuffer *zBuffer = _rd->_camera.getCurrent()->canvas->zBuffer;
	uint16 *dst = (uint16 *)(zBuffer->pixels + 2 * span->x + zBuffer->tex.pitchInBytes * span->y);
	uint32 z = (uint32)(int)floor((float)(span->oneOverZ * 65536.0));
	uint32 dz = (uint32)_dOneOverZdXFixed;

	for (int i = 0; i < span->width; i++) {
		dst[i] = (uint16)z;
		z += dz;
	}
}

void RenderDroid::AFRasterModule::drawZSpanShifted(RdActiveSpan *span) {
	RdCanvas *canvas = _rd->_camera.getCurrent()->canvas;
	StdVBuffer *zBuffer = canvas->zBuffer;
	uint16 *dst = (uint16 *)(zBuffer->pixels + 2 * span->x + zBuffer->tex.pitchInBytes * span->y);
	int shift = getZShift(canvas);
	uint32 z = (uint32)(int)floor((float)(span->oneOverZ * 65536.0));
	uint32 dz = (uint32)_dOneOverZdXFixed;

	for (int i = 0; i < span->width; i++) {
		dst[i] = (uint16)(z >> shift);
		z += dz;
	}
}

void RenderDroid::AFRasterModule::plotRun16(SpanRun &run, int count, bool intensity) {
	const uint16 *rgb = (const uint16 *)_rgbMap;
	uint16 *dst = (uint16 *)run.dst;

	for (int i = 0; i < count; i++) {
		int index = _pixels[((run.uMask & run.u) >> 16) | (_vWrap & (run.v >> run.vShift))];

		if (intensity)
			index += (run.intensity & 0x3F0000) >> 8;

		dst[i] = rgb[index];

		run.u += run.du;
		run.v += run.dv;

		if (intensity)
			run.intensity += run.dIntensity;
	}

	run.dst += 2 * count;
}

void RenderDroid::AFRasterModule::plotRun8(SpanRun &run, int count, int lightKind, bool pairedIntensity) {
	int pairIntensity = run.intensity;

	for (int i = 0; i < count; i++) {
		uint index = (uint)_pixels[((run.uMask & (uint)run.u) >> 16) + (_vWrap & ((uint)run.v >> run.vShift))];

		// The gouraud blocks of 16 pixels use one intensity for each pair of pixels
		if (!pairedIntensity || !(i & 1))
			pairIntensity = run.intensity;

		if (lightKind == 1)
			index = _monoMap[index];
		else if (lightKind == 2)
			index = _monoMap[index + ((pairIntensity & 0x3F0000) >> 8)];

		run.dst[i] = (byte)index;

		run.u += run.du;
		run.v += run.dv;

		if (lightKind == 2)
			run.intensity += run.dIntensity;
	}

	run.dst += count;
}

void RenderDroid::AFRasterModule::plotPerspRun(SpanRun &run, int count, int lightKind, bool bpp16, bool chunk) {
	if (bpp16)
		plotRun16(run, count, lightKind == 2);
	else
		plotRun8(run, count, lightKind, chunk && lightKind == 2);
}

void RenderDroid::AFRasterModule::drawSpanAffine8(RdActiveSpan *span, int vShift, int lightKind) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	SpanRun run;

	run.dst = vBuffer->pixels + span->x + vBuffer->tex.pitchInBytes * span->y;
	run.u = span->u + _dUdXModifier;
	run.du = span->du;
	run.v = span->v + _dVdXModifier;
	run.dv = span->dv;
	run.uMask = _uWrap;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = span->intensity;
		run.dIntensity = span->dIntensity;
	}

	plotRun8(run, span->width, lightKind, false);
}

void RenderDroid::AFRasterModule::drawSpanAffine16(RdActiveSpan *span, int uMask, int vShift, bool intensity) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	SpanRun run;

	run.dst = vBuffer->pixels + 2 * span->x + 2 * (vBuffer->tex.pitchInBytes >> 1) * span->y;
	run.u = span->u + _dUdXModifier;
	run.du = span->du;
	run.v = span->v + _dVdXModifier;
	run.dv = span->dv;
	run.uMask = uMask;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (intensity) {
		run.intensity = span->intensity;
		run.dIntensity = span->dIntensity;
	}

	plotRun16(run, span->width, intensity);

	_spanU = run.u;
	_spanV = run.v;
	_spanDU = run.du;
	_spanDV = run.dv;
}

void RenderDroid::AFRasterModule::drawSpanPersp16(RdActiveSpan *span, int uMask, int vShift, bool intensity) {
	drawSpanPersp(span, uMask, vShift, intensity ? 2 : 0, true);
}

void RenderDroid::AFRasterModule::drawSpanPersp(RdActiveSpan *span, int uMask, int vShift, int lightKind, bool bpp16) {
	StdVBuffer *vBuffer = _rd->_camera.getCurrent()->canvas->vBuffer;
	SpanRun run;
	bool intensity = (lightKind == 2);

	int xAlign = span->x & 0xF;
	int width = 0;

	if (xAlign) {
		width = span->width;
		if (width >= 16 - xAlign)
			width = 16 - xAlign;
	}

	int remaining = span->width - width;
	int numChunks = 0;
	int tail = 0;

	if (remaining) {
		numChunks = remaining / 16;
		tail = remaining % 16;
		if (!tail) {
			tail = 16;
			numChunks--;
		}
	}

	float oneOverZ = span->oneOverZ;
	float uOverZ = span->uOverZ;
	float vOverZ = span->vOverZ;

	_lowPrecision = true;

	double inverse = fpuDiv(1.0, oneOverZ);
	float uStart = (float)fpuMul(inverse, uOverZ);
	float vStart = (float)fpuMul(inverse, vOverZ);

	if (bpp16)
		run.dst = vBuffer->pixels + 2 * span->x + 2 * (vBuffer->tex.pitchInBytes >> 1) * span->y;
	else
		run.dst = vBuffer->pixels + span->x + vBuffer->tex.pitchInBytes * span->y;

	run.uMask = uMask;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (intensity) {
		run.intensity = span->intensity;
		run.dIntensity = span->dIntensity;
	}

	run.u = _spanU;
	run.v = _spanV;
	run.du = _spanDU;
	run.dv = _spanDV;

	float nextZ;
	float nextU;
	float nextV;

	if (width) {
		float inverseWidth = _rd->_raster.getOneOverNFlex(width);

		nextZ = (float)fpuAdd(fpuMul(width, _dOneOverZdX), oneOverZ);
		nextU = (float)fpuAdd(fpuMul(width, _dUOverZdX), uOverZ);
		nextV = (float)fpuAdd(fpuMul(width, _dVOverZdX), vOverZ);

		double inverseEnd = fpuDiv(1.0, nextZ);
		float inverseEndF = (float)inverseEnd;
		float uEnd = (float)fpuMul(inverseEnd, nextU);
		float vEnd = (float)fpuMul(inverseEndF, nextV);

		run.du = (int)(float)fpuMul(fpuMul(fpuSub(uEnd, uStart), inverseWidth), 65536.0) >> _mipMapNum;
		run.dv = (int)(float)fpuMul(fpuMul(fpuSub(vEnd, vStart), inverseWidth), 65536.0) >> _mipMapNum;
		run.u = _dUdXModifier + ((int)(float)(uStart * 65536.0) >> _mipMapNum);
		run.v = _dVdXModifier + ((int)(float)(vStart * 65536.0) >> _mipMapNum);

		plotPerspRun(run, width, lightKind, bpp16, false);

		nextZ = (float)fpuAdd(nextZ, _oneOverZStep);
		nextU = (float)fpuAdd(nextU, _uOverZStep);
		nextV = (float)fpuAdd(nextV, _vOverZStep);
		uStart = uEnd;
		vStart = vEnd;
	} else {
		nextZ = (float)fpuAdd(oneOverZ, _oneOverZStep);
		nextU = (float)fpuAdd(uOverZ, _uOverZStep);
		nextV = (float)fpuAdd(vOverZ, _vOverZStep);
	}

	for (int chunk = 0; chunk < numChunks; chunk++) {
		double inverseEnd = fpuDiv(1.0, nextZ);
		float inverseEndF = (float)inverseEnd;
		float uEnd = (float)fpuMul(inverseEnd, nextU);
		float vEnd = (float)fpuMul(inverseEndF, nextV);

		run.du = (int)(float)fpuMul(fpuMul(fpuSub(uEnd, uStart), 0.0625), 65536.0) >> _mipMapNum;
		run.dv = (int)(float)fpuMul(fpuMul(fpuSub(vEnd, vStart), 0.0625), 65536.0) >> _mipMapNum;
		run.u = _dUdXModifier + ((int)(float)(uStart * 65536.0) >> _mipMapNum);
		run.v = _dVdXModifier + ((int)(float)(vStart * 65536.0) >> _mipMapNum);

		plotPerspRun(run, 16, lightKind, bpp16, true);

		nextZ = (float)fpuAdd(nextZ, _oneOverZStep);
		nextU = (float)fpuAdd(nextU, _uOverZStep);
		nextV = (float)fpuAdd(nextV, _vOverZStep);
		uStart = uEnd;
		vStart = vEnd;
	}

	if (tail) {
		int count = tail - 1;

		if (count != 0) {
			double steps = (double)count;
			float inverseCount = _rd->_raster.getOneOverNFlex(count);

			nextZ = (float)fpuAdd(fpuSub(fpuMul(steps, _dOneOverZdX), _oneOverZStep), nextZ);
			nextU = (float)fpuAdd(fpuSub(fpuMul(steps, _dUOverZdX), _uOverZStep), nextU);
			nextV = (float)fpuAdd(fpuSub(fpuMul(steps, _dVOverZdX), _vOverZStep), nextV);

			double inverseEnd = fpuDiv(1.0, nextZ);
			float inverseEndF = (float)inverseEnd;
			float uEnd = (float)fpuMul(inverseEnd, nextU);
			float vEnd = (float)fpuMul(inverseEndF, nextV);

			run.du = (int)(float)fpuMul(fpuMul(fpuSub(uEnd, uStart), inverseCount), 65536.0) >> _mipMapNum;
			run.dv = (int)(float)fpuMul(fpuMul(fpuSub(vEnd, vStart), inverseCount), 65536.0) >> _mipMapNum;
			run.u = _dUdXModifier + ((int)(float)(uStart * 65536.0) >> _mipMapNum);
			run.v = _dVdXModifier + ((int)(float)(vStart * 65536.0) >> _mipMapNum);
		}

		plotPerspRun(run, tail, lightKind, bpp16, false);
	}

	_lowPrecision = false;
	_spanU = run.u;
	_spanV = run.v;
	_spanDU = run.du;
	_spanDV = run.dv;
}

double RenderDroid::AFRasterModule::fpuRound(double value) {
	if (!_lowPrecision)
		return value;

	float result = (float)value;
	if (result != 0.0f && fabs((double)result) > fabs(value)) {
		uint32 bits;
		memcpy(&bits, &result, sizeof(bits));
		bits--;
		memcpy(&result, &bits, sizeof(bits));
	}

	return result;
}

double RenderDroid::AFRasterModule::fpuAdd(double a, double b) {
	return fpuRound(a + b);
}

double RenderDroid::AFRasterModule::fpuSub(double a, double b) {
	return fpuRound(a - b);
}

double RenderDroid::AFRasterModule::fpuMul(double a, double b) {
	return fpuRound(a * b);
}

double RenderDroid::AFRasterModule::fpuDiv(double a, double b) {
	return fpuRound(a / b);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16(RdActiveSpan *span) {
	drawSpanAffine16(span, _uWrap, 16 - _texRowShift, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W8(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x70000, 13, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W16(RdActiveSpan *span) {
	drawSpanAffine16(span, 0xF0000, 12, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W32(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x1F0000, 11, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W64(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x3F0000, 10, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W128(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x7F0000, 9, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W256(RdActiveSpan *span) {
	drawSpanAffine16(span, 0xFF0000, 8, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16(RdActiveSpan *span) {
	drawSpanNGonLAT_16(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W8(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W8(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W16(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W16(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W32(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W32(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W64(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W64(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W128(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W128(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W256(RdActiveSpan *span) {
	drawSpanNGonLAT_16_W256(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16(RdActiveSpan *span) {
	drawSpanAffine16(span, _uWrap, 16 - _texRowShift, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W8(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x70000, 13, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W16(RdActiveSpan *span) {
	drawSpanAffine16(span, 0xF0000, 12, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W32(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x1F0000, 11, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W64(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x3F0000, 10, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W128(RdActiveSpan *span) {
	drawSpanAffine16(span, 0x7F0000, 9, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W256(RdActiveSpan *span) {
	drawSpanAffine16(span, 0xFF0000, 8, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16(RdActiveSpan *span) {
	drawSpanPersp16(span, _uWrap, 16 - _texRowShift, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W8(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x70000, 13, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W16(RdActiveSpan *span) {
	drawSpanPersp16(span, 0xF0000, 12, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W32(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x1F0000, 11, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W64(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x3F0000, 10, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W128(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x7F0000, 9, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W256(RdActiveSpan *span) {
	drawSpanPersp16(span, 0xFF0000, 8, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16(RdActiveSpan *span) {
	drawSpanNGonLIT_16(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W8(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W8(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W16(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W16(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W32(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W32(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W64(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W64(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W128(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W128(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W256(RdActiveSpan *span) {
	drawSpanNGonLIT_16_W256(span);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16(RdActiveSpan *span) {
	drawSpanPersp16(span, _uWrap, 16 - _texRowShift, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W8(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x70000, 13, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W16(RdActiveSpan *span) {
	drawSpanPersp16(span, 0xF0000, 12, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W32(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x1F0000, 11, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W64(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x3F0000, 10, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W128(RdActiveSpan *span) {
	drawSpanPersp16(span, 0x7F0000, 9, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W256(RdActiveSpan *span) {
	drawSpanPersp16(span, 0xFF0000, 8, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8(RdActiveSpan *span) {
	drawSpanAffine8(span, 16 - _texRowShift, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W8(RdActiveSpan *span) {
	drawSpanAffine8(span, 13, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W16(RdActiveSpan *span) {
	drawSpanAffine8(span, 12, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W32(RdActiveSpan *span) {
	drawSpanAffine8(span, 11, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W64(RdActiveSpan *span) {
	drawSpanAffine8(span, 10, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W128(RdActiveSpan *span) {
	drawSpanAffine8(span, 9, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W256(RdActiveSpan *span) {
	drawSpanAffine8(span, 8, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8(RdActiveSpan *span) {
	drawSpanAffine8(span, 16 - _texRowShift, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W8(RdActiveSpan *span) {
	drawSpanAffine8(span, 13, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W16(RdActiveSpan *span) {
	drawSpanAffine8(span, 12, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W32(RdActiveSpan *span) {
	drawSpanAffine8(span, 11, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W64(RdActiveSpan *span) {
	drawSpanAffine8(span, 10, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W128(RdActiveSpan *span) {
	drawSpanAffine8(span, 9, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W256(RdActiveSpan *span) {
	drawSpanAffine8(span, 8, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8(RdActiveSpan *span) {
	drawSpanAffine8(span, 16 - _texRowShift, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W8(RdActiveSpan *span) {
	drawSpanAffine8(span, 13, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W16(RdActiveSpan *span) {
	drawSpanAffine8(span, 12, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W32(RdActiveSpan *span) {
	drawSpanAffine8(span, 11, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W64(RdActiveSpan *span) {
	drawSpanAffine8(span, 10, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W128(RdActiveSpan *span) {
	drawSpanAffine8(span, 9, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W256(RdActiveSpan *span) {
	drawSpanAffine8(span, 8, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 16 - _texRowShift, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 13, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W16(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 12, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W32(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 11, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W64(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 10, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W128(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 9, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W256(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 8, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 16 - _texRowShift, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 13, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W16(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 12, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W32(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 11, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W64(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 10, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W128(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 9, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W256(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 8, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 16 - _texRowShift, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W8(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 13, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W16(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 12, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W32(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 11, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W64(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 10, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W128(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 9, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W256(RdActiveSpan *span) {
	drawSpanPersp(span, _uWrap, 8, 2, false);
}

} // End of namespace Grim
