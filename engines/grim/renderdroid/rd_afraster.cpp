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

int RenderDroid::AFRasterModule::selectRefVertex(RdProcFace *pProcFace, int *pYMinVert, int *pYMaxVert) {
	int numVertices = (int)pProcFace->numVertices;

	if (numVertices == 3) {
		*pYMinVert = 0;
		*pYMaxVert = 1;
		return 2;
	}

	int yMinVert = pProcFace->yMinIdx;
	int yMaxVert = pProcFace->yMaxIdx;
	int lastVertex = numVertices - 1;
	RdVector3 *aVertices = pProcFace->aVertices;

	*pYMinVert = yMinVert;
	*pYMaxVert = yMaxVert;

	int refVert = 0;
	bool bRefFound = true;
	int direction;

	int nextVert = wrapVertex(yMinVert + 1, lastVertex);
	int prevVert = wrapVertex(yMinVert - 1, lastVertex);

	if (nextVert == yMaxVert) {
		direction = -1;
		bRefFound = false;
	} else if (prevVert == yMaxVert) {
		direction = 1;
		bRefFound = false;
	} else {
		refVert = nextVert;

		int xDiffMax = abs((int)((double)aVertices[refVert][0] - aVertices[yMaxVert][0]));
		bool bUseSlope = (double)xDiffMax > 1.0;

		if (!bUseSlope) {
			int xDiffMin = abs((int)((double)aVertices[refVert][0] - aVertices[yMinVert][0]));
			bUseSlope = (double)xDiffMin > 1.0;
		}

		if (bUseSlope) {
			double yOnLine = ((double)aVertices[yMaxVert][1] - aVertices[yMinVert][1]) / ((double)aVertices[yMaxVert][0] - aVertices[yMinVert][0]) * ((double)aVertices[refVert][0] - aVertices[yMinVert][0]) + aVertices[yMinVert][1];
			double deviation = (double)aVertices[refVert][1] - yOnLine;
			if (deviation < 0.0)
				deviation = -deviation;

			direction = (deviation >= 1.0) ? 1 : -1;
		} else {
			direction = -1;
		}
	}

	float midX = (float)(((double)aVertices[yMaxVert][0] + aVertices[yMinVert][0]) * 0.5);
	float maxDistance = 0.0f;
	int firstWalk = wrapVertex(yMinVert + direction, lastVertex);
	int walk = firstWalk;

	while (walk != yMaxVert) {
		double distance = (double)aVertices[walk][0] - midX;
		if (distance < 0.0)
			distance = -distance;

		if (distance > maxDistance) {
			maxDistance = (float)distance;
			refVert = walk;
			bRefFound = true;
		}

		walk = wrapVertex(walk + direction, lastVertex);
	}

	if (!bRefFound) {
		warning("RenderDroid::AFRasterModule::selectRefVertex(): no reference vertex found");
		refVert = firstWalk;
	}

	return refVert;
}

void RenderDroid::AFRasterModule::selectMipLevel(RdActiveFace *pFace, int refVert) {
	float zMin = pFace->pProcFace->zMin;
	float *aMipDistances = _rd->_aMipDistances;

	switch (refVert) {
	case 0:
		pFace->mipLevel = 0;
		break;
	case 1:
		pFace->mipLevel = (zMin > aMipDistances[1]) ? 1 : 0;
		break;
	case 2:
		if (zMin > aMipDistances[0])
			pFace->mipLevel = (zMin > aMipDistances[1]) ? 2 : 1;
		else
			pFace->mipLevel = 0;
		break;
	case 3:
		if (zMin > aMipDistances[0]) {
			if (zMin > aMipDistances[1])
				pFace->mipLevel = (zMin > aMipDistances[2]) ? 3 : 2;
			else
				pFace->mipLevel = 1;
		} else {
			pFace->mipLevel = 0;
		}
		break;
	default:
		break;
	}
}

void RenderDroid::AFRasterModule::calcAffineGradients(RdActiveFace *pFace, int refVert) {
	RdProcFace *pProcFace = pFace->pProcFace;
	RdVector3 *aVertices = pProcFace->aVertices;
	RdVector2 *aTexVertices = pProcFace->aTexVertices;
	int yMinVert;
	int yMaxVert;
	int refIdx = selectRefVertex(pProcFace, &yMinVert, &yMaxVert);

	for (int i = 0; i < (int)pProcFace->numVertices; i++)
		pFace->aOneOverZ[i] = (float)(1.0 / aVertices[i][2]);

	double dAy = (double)aVertices[yMinVert][1] - aVertices[refIdx][1];
	double dBy = (double)aVertices[yMaxVert][1] - aVertices[refIdx][1];
	float dAx = (float)((double)aVertices[yMinVert][0] - aVertices[refIdx][0]);
	float dBx = (float)((double)aVertices[yMaxVert][0] - aVertices[refIdx][0]);

	double dAu = (double)aTexVertices[yMinVert][0] - aTexVertices[refIdx][0];
	double dBu = (double)aTexVertices[yMaxVert][0] - aTexVertices[refIdx][0];
	float dBuF = (float)dBu;
	float dAuF = (float)dAu;
	float dBvF = (float)((double)aTexVertices[yMaxVert][1] - aTexVertices[refIdx][1]);
	float dAvF = (float)((double)aTexVertices[yMinVert][1] - aTexVertices[refIdx][1]);

	double det = dAy * dBx - dBy * dAx;
	double inverse = 1.0 / det;
	float inverseF = (float)inverse;
	float negInverseF = (float)-inverse;

	float uX = (float)((dBu * dAy - dAu * dBy) * inverseF);
	float uY = (float)(((double)dBuF * dAx - (double)dAuF * dBx) * negInverseF);
	float vX = (float)(((double)dBvF * dAy - (double)dAvF * dBy) * inverseF);
	float vY = (float)(((double)dBvF * dAx - (double)dAvF * dBx) * negInverseF);

	selectMipLevel(pFace, refVert);

	if (uX <= 0.0f) {
		if (uX < 0.0f || uY < 0.0f)
			pFace->uModifier = 0x7FFF;
		else
			pFace->uModifier = 0x8000;
	} else {
		pFace->uModifier = 0x8000;
	}

	if (vX <= 0.0f) {
		if (vX < 0.0f || vY < 0.0f)
			pFace->vModifier = 0x7FFF;
		else
			pFace->vModifier = 0x8000;
	} else {
		pFace->vModifier = 0x8000;
	}
}

void RenderDroid::AFRasterModule::calcPerspGradients(RdActiveFace *pFace, int refVert) {
	RdProcFace *pProcFace = pFace->pProcFace;
	RdVector3 *aVertices = pProcFace->aVertices;
	RdVector2 *aTexVertices = pProcFace->aTexVertices;
	int yMinVert;
	int yMaxVert;
	int refIdx = selectRefVertex(pProcFace, &yMinVert, &yMaxVert);

	for (int i = 0; i < (int)pProcFace->numVertices; i++) {
		double oneOverZ = 1.0 / aVertices[i][2];
		pFace->aOneOverZ[i] = (float)oneOverZ;
		pFace->aUOverZ[i] = (float)(aTexVertices[i][0] * oneOverZ);
		pFace->aVOverZ[i] = (float)(aTexVertices[i][1] * (double)pFace->aOneOverZ[i]);
	}

	float dAy = (float)((double)aVertices[yMinVert][1] - aVertices[refIdx][1]);
	float dBy = (float)((double)aVertices[yMaxVert][1] - aVertices[refIdx][1]);
	float dAx = (float)((double)aVertices[yMinVert][0] - aVertices[refIdx][0]);
	float dBx = (float)((double)aVertices[yMaxVert][0] - aVertices[refIdx][0]);

	double dAyExact = (double)aVertices[yMinVert][1] - aVertices[refIdx][1];
	double dByExact = (double)aVertices[yMaxVert][1] - aVertices[refIdx][1];

	double det = dAyExact * dBx - dByExact * dAx;
	float inverseF = (float)(1.0 / det);
	float negInverseF = -inverseF;

	double dAz = (double)pFace->aOneOverZ[yMinVert] - pFace->aOneOverZ[refIdx];
	double dBz = (double)pFace->aOneOverZ[yMaxVert] - pFace->aOneOverZ[refIdx];
	float dBuF = (float)((double)pFace->aUOverZ[yMaxVert] - pFace->aUOverZ[refIdx]);
	float dAuF = (float)((double)pFace->aUOverZ[yMinVert] - pFace->aUOverZ[refIdx]);
	float dBvF = (float)((double)pFace->aVOverZ[yMaxVert] - pFace->aVOverZ[refIdx]);
	float dAvF = (float)((double)pFace->aVOverZ[yMinVert] - pFace->aVOverZ[refIdx]);

	double zX = (dBz * dAy - dAz * dBy) * inverseF;
	float zY = (float)((dBz * dAx - dAz * dBx) * negInverseF);
	pFace->dOneOverZdX = (float)zX;
	pFace->dOneOverZdY = zY;
	pFace->dOneOverZdXFixed = (int)(zX * 65536.0);

	double uX = ((double)dBuF * dAy - (double)dAuF * dBy) * inverseF;
	pFace->dUOverZdX = (float)uX;
	float uY = (float)(((double)dBuF * dAx - (double)dAuF * dBx) * negInverseF);
	pFace->dUOverZdY = uY;

	float vX = (float)(((double)dBvF * dAy - (double)dAvF * dBy) * inverseF);
	pFace->dVOverZdX = vX;
	float vY = (float)(((double)dBvF * dAx - (double)dAvF * dBx) * negInverseF);
	pFace->dVOverZdY = vY;

	selectMipLevel(pFace, refVert);

	double uCheck = uX * pFace->aOneOverZ[yMinVert] - (double)pFace->aUOverZ[yMinVert] * zX;
	if (uCheck <= 0.0) {
		if (uCheck < 0.0 || (double)pFace->aOneOverZ[yMinVert] * uY - (double)pFace->aUOverZ[yMinVert] * zY < 0.0)
			pFace->uModifier = 0x7FFF;
		else
			pFace->uModifier = 0x8000;
	} else {
		pFace->uModifier = 0x8000;
	}

	double vCheck = (double)pFace->aOneOverZ[yMinVert] * vX - (double)pFace->aVOverZ[yMinVert] * zX;
	if (vCheck <= 0.0) {
		if (vCheck < 0.0 || (double)pFace->aOneOverZ[yMinVert] * vY - (double)pFace->aVOverZ[yMinVert] * zY < 0.0)
			pFace->vModifier = 0x7FFF;
		else
			pFace->vModifier = 0x8000;
	} else {
		pFace->vModifier = 0x8000;
	}
}

int RenderDroid::AFRasterModule::getZShift(RdCanvas *pCanvas) {
	int shift = pCanvas->zScaleShift;

	if (shift < 0 || shift > 31) {
		warning("RenderDroid::AFRasterModule::getZShift(): z scale shift %d out of range", shift);
		shift &= 31;
	}

	return shift;
}

void RenderDroid::AFRasterModule::drawZSpans(RdActiveFace *pFace) {
	RdActiveSpan *pSpan = pFace->pFirstSpan;

	if (_rd->_camera.getCurrent()->pCanvas->zScaleShift) {
		for (; pSpan; pSpan = pSpan->pNext)
			drawZSpanShifted(pSpan);
	} else {
		for (; pSpan; pSpan = pSpan->pNext)
			drawZSpan(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawTexturedSpans(RdActiveFace *pFace) {
	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (_pfDrawSpan)
			(this->*_pfDrawSpan)(pSpan);
	}

	drawZSpans(pFace);
}

RenderDroid::AFRasterModule::DrawSpanFunc RenderDroid::AFRasterModule::selectWidthSpan(int texPitch, const DrawSpanFunc *apWidthSpans, DrawSpanFunc pfDefault) {
	switch (texPitch) {
	case 8:
		return apWidthSpans[0];
	case 16:
		return apWidthSpans[1];
	case 32:
		return apWidthSpans[2];
	case 64:
		return apWidthSpans[3];
	case 128:
		return apWidthSpans[4];
	case 256:
		return apWidthSpans[5];
	default:
		return pfDefault;
	}
}

void RenderDroid::AFRasterModule::setupFlat(RdActiveFace *pFace, RdMatCel *pCel, DrawFaceFunc pfDrawSpans, SetupEdgeFunc pfSetupEdge) {
	RdProcFace *pProcFace = pFace->pProcFace;

	for (uint i = 0; i < pProcFace->numVertices; i++)
		pFace->aOneOverZ[i] = (float)(1.0 / pProcFace->aVertices[i][2]);

	pFace->pfDrawSpans = pfDrawSpans;
	pFace->colorIndex = pCel->colorIndex;
	pFace->pfSetupEdge = pfSetupEdge;
}

void RenderDroid::AFRasterModule::setupTextured(RdActiveFace *pFace, RdMatCel *pCel, bool bPersp, int lightKind, float light, DrawFaceFunc pfDrawSpans, SetupEdgeFunc pfSetupEdge) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;
	RdTexture *pTexture = pCel->pTexture;
	RdColormap *pColormap = pFace->pProcFace->pColormap;

	if (bPersp)
		calcPerspGradients(pFace, (int)pTexture->numMips - 1);
	else
		calcAffineGradients(pFace, (int)pTexture->numMips - 1);

	int mipLevel = pFace->mipLevel;

	if (bPersp) {
		pFace->dOneOverZdX16 = (float)((double)pFace->dOneOverZdX * 16.0);
		pFace->dUOverZdX16 = (float)((double)pFace->dUOverZdX * 16.0);
		pFace->dVOverZdX16 = (float)((double)pFace->dVOverZdX * 16.0);
	}

	if (lightKind == 1) {
		if (bpp == 8)
			pFace->pLightLevel = pColormap->pLightLevel + 256 * (int)floor(light);
		else if (bpp == 16)
			pFace->pRGB16 = pColormap->pRGB16 + 512 * (int)floor(light);
	} else if (lightKind == 2) {
		if (bpp == 8)
			pFace->pLightLevel = pColormap->pLightLevel;
		else if (bpp == 16)
			pFace->pRGB16 = pColormap->pRGB16;
	} else if (bpp == 16) {
		pFace->pRGB16 = pColormap->pRGB16 + 32256;
	}

	pFace->pTexPixels = pTexture->apMips[mipLevel]->pPixels;
	pFace->texPitch = pTexture->apMips[mipLevel]->tex.pitchInBytes;
	pFace->texShift = pTexture->widthLog2 - mipLevel;
	pFace->uWrap = (int)(((uint)pTexture->widthMask >> mipLevel) << 16);
	pFace->vWrap = (int)(((uint)pTexture->heightMask >> mipLevel) << pFace->texShift);
	pFace->pfDrawSpans = pfDrawSpans;
	pFace->pfSetupEdge = pfSetupEdge;
}

void RenderDroid::AFRasterModule::setupNGonLW(RdActiveFace *pFace, RdMatCel *pCel) {
	setupFlat(pFace, pCel, &AFRasterModule::drawNGonLW, &AFRasterModule::setupEdgeNGonLW);
}

void RenderDroid::AFRasterModule::drawNGonLW(RdActiveFace *pFace) {
	RdActiveSpan *pSpan = pFace->pFirstSpan;
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8) {
		_color = (byte)pFace->colorIndex;

		for (; pSpan; pSpan = pSpan->pNext)
			drawSpanNGonLW_8(pSpan);
	} else if (bpp == 16) {
		_RGBcolor = ((uint16 *)pFace->pProcFace->pColormap->pRGB16)[(pFace->colorIndex & 0xFF) + 16128];

		for (; pSpan; pSpan = pSpan->pNext)
			drawSpanNGonLW_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonLW_8(RdActiveSpan *pSpan) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	byte *pDst = pVBuffer->pPixels + pVBuffer->tex.pitchInBytes * pSpan->y + pSpan->x;

	pDst[0] = _color;
	pDst[pSpan->width - 1] = _color;
}

void RenderDroid::AFRasterModule::drawSpanNGonLW_16(RdActiveSpan *pSpan) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	uint16 *pDst = (uint16 *)(pVBuffer->pPixels + 2 * pSpan->x + 2 * (pVBuffer->tex.pitchInBytes >> 1) * pSpan->y);

	pDst[0] = _RGBcolor;
	pDst[pSpan->width - 1] = _RGBcolor;
}

void RenderDroid::AFRasterModule::advanceEdgeNGonLS(RdActiveEdge *pEdge) {
	pEdge->x += pEdge->dx;
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeAffine(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1, bool bTexture, bool bIntensity, StepEdgeFunc pfStep) {
	RdProcFace *pProcFace = pFace->pProcFace;
	RdVector3 *aVertices = pProcFace->aVertices;
	RdVector2 *aTexVertices = pProcFace->aTexVertices;
	float *aIntensities = pProcFace->aIntensities;

	float y0 = (float)ceil(aVertices[v0][1]);
	float y1 = (float)ceil(aVertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	pEdge->numLines = numLines;
	if (numLines == 0)
		return 0;

	int top;
	int bottom;
	float yTop;

	if (numLines > 0) {
		pEdge->bLeft = 1;
		pEdge->yStart = yStart0;
		top = v0;
		bottom = v1;
		yTop = y0;
	} else {
		pEdge->bLeft = 0;
		numLines = -numLines;
		pEdge->yStart = yStart1;
		pEdge->numLines = numLines;
		top = v1;
		bottom = v0;
		yTop = y1;
	}

	pEdge->pfStep = pfStep;

	if (numLines < 2) {
		pEdge->x = (int)floor((float)(aVertices[top][0] * 65536.0));

		if (bTexture) {
			pEdge->u = (int)floor((float)(aTexVertices[top][0] * 65536.0));
			pEdge->v = (int)floor((float)(aTexVertices[top][1] * 65536.0));
		}

		if (bIntensity)
			pEdge->intensity = (int)floor((float)(aIntensities[top] * 65536.0));

		pEdge->oneOverZ = pFace->aOneOverZ[top];
		return 1;
	}

	float dy = (float)(yTop - aVertices[top][1]);
	float inverseDy = (float)(1.0 / ((double)aVertices[bottom][1] - aVertices[top][1]));

	double slopeX = ((double)aVertices[bottom][0] - aVertices[top][0]) * inverseDy;
	float slopeXF = (float)slopeX;
	pEdge->dx = (int)floor((float)(slopeX * 65536.0));
	pEdge->x = (int)floor((float)(((double)slopeXF * dy + aVertices[top][0]) * 65536.0));

	if (bTexture) {
		double slopeU = ((double)aTexVertices[bottom][0] - aTexVertices[top][0]) * inverseDy;
		float slopeUF = (float)slopeU;
		pEdge->du = (int)floor((float)(slopeU * 65536.0));
		pEdge->u = (int)floor((float)(((double)slopeUF * dy + aTexVertices[top][0]) * 65536.0));

		double slopeV = ((double)aTexVertices[bottom][1] - aTexVertices[top][1]) * inverseDy;
		float slopeVF = (float)slopeV;
		pEdge->dv = (int)floor((float)(slopeV * 65536.0));
		pEdge->v = (int)floor((float)(((double)slopeVF * dy + aTexVertices[top][1]) * 65536.0));
	}

	if (bIntensity) {
		double slopeI = ((double)aIntensities[bottom] - aIntensities[top]) * inverseDy;
		float slopeIF = (float)slopeI;
		pEdge->dIntensity = (int)floor((float)(slopeI * 65536.0));
		pEdge->intensity = (int)floor((float)(((double)slopeIF * dy + aIntensities[top]) * 65536.0));
	}

	double slopeZ = ((double)pFace->aOneOverZ[bottom] - pFace->aOneOverZ[top]) * inverseDy;
	pEdge->dOneOverZ = (float)slopeZ;
	pEdge->oneOverZ = (float)((double)dy * slopeZ + pFace->aOneOverZ[top]);

	return 1;
}

int RenderDroid::AFRasterModule::setupEdgeNGonLW(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeAffine(pEdge, pFace, v0, v1, false, false, &AFRasterModule::advanceEdgeNGonLS);
}

void RenderDroid::AFRasterModule::setupNGonLS(RdActiveFace *pFace, RdMatCel *pCel) {
	setupFlat(pFace, pCel, &AFRasterModule::drawNGonLS, &AFRasterModule::setupEdgeNGonLS);
}

void RenderDroid::AFRasterModule::drawNGonLS(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8)
		_color = (byte)pFace->colorIndex;
	else if (bpp == 16)
		_RGBcolor = ((uint16 *)pFace->pProcFace->pColormap->pRGB16)[(pFace->colorIndex & 0xFF) + 16128];

	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	if (bpp == 8) {
		for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
			StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
			memset(pVBuffer->pPixels + pVBuffer->tex.pitchInBytes * pSpan->y + pSpan->x, _color, pSpan->width);
		}
	} else if (bpp == 16) {
		for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
			StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
			uint16 *pDst = (uint16 *)(pVBuffer->pPixels + 2 * pSpan->x + 2 * (pVBuffer->tex.pitchInBytes >> 1) * pSpan->y);

			for (int i = 0; i < pSpan->width; i++)
				pDst[i] = _RGBcolor;
		}
	}

	drawZSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeNGonLW(pEdge, pFace, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonFS(RdActiveFace *pFace, RdMatCel *pCel, float light) {
	RdProcFace *pProcFace = pFace->pProcFace;

	for (uint i = 0; i < pProcFace->numVertices; i++)
		pFace->aOneOverZ[i] = (float)(1.0 / pProcFace->aVertices[i][2]);

	pFace->pfDrawSpans = &AFRasterModule::drawNGonFS;
	pFace->colorIndex = pProcFace->pColormap->pLightLevel[pCel->colorIndex + 256 * (int)floor(light)];
	pFace->pfSetupEdge = &AFRasterModule::setupEdgeNGonFS;
}

void RenderDroid::AFRasterModule::drawNGonFS(RdActiveFace *pFace) {
	_color = (byte)pFace->colorIndex;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
		memset(pVBuffer->pPixels + pVBuffer->tex.pitchInBytes * pSpan->y + pSpan->x, _color, pSpan->width);
	}

	drawZSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeNGonLW(pEdge, pFace, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonGS(RdActiveFace *pFace, RdMatCel *pCel) {
	RdProcFace *pProcFace = pFace->pProcFace;
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	for (uint i = 0; i < pProcFace->numVertices; i++)
		pFace->aOneOverZ[i] = (float)(1.0 / pProcFace->aVertices[i][2]);

	pFace->colorIndex = pCel->colorIndex;

	if (bpp == 8)
		pFace->pLightLevel = pProcFace->pColormap->pLightLevel;
	else if (bpp == 16)
		pFace->pRGB16 = pProcFace->pColormap->pRGB16;

	pFace->pfDrawSpans = &AFRasterModule::drawNGonGS;
	pFace->pfSetupEdge = &AFRasterModule::setupEdgeNGonGS;
}

void RenderDroid::AFRasterModule::drawNGonGS(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	_color = (byte)pFace->colorIndex;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	if (bpp == 8) {
		for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
			StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
			int intensity = pSpan->intensity;
			byte *pDst = pVBuffer->pPixels + pVBuffer->tex.pitchInBytes * pSpan->y + pSpan->x;

			for (int i = 0; i < pSpan->width; i++) {
				pDst[i] = _aMonoMap[((intensity >> 8) & 0x3F00) + _color];
				intensity += pSpan->dIntensity;
			}
		}
	} else if (bpp == 16) {
		for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
			StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
			int intensity = pSpan->intensity;
			uint16 *pDst = (uint16 *)(pVBuffer->pPixels + 2 * pSpan->x + 2 * (pVBuffer->tex.pitchInBytes >> 1) * pSpan->y);

			for (int i = 0; i < pSpan->width; i++) {
				pDst[i] = ((uint16 *)_aRGBMap)[_color + (((intensity >> 16) & 0x3F) << 8)];
				intensity += pSpan->dIntensity;
			}
		}
	}

	drawZSpans(pFace);
}

void RenderDroid::AFRasterModule::advanceEdgeNGonGS(RdActiveEdge *pEdge) {
	pEdge->x += pEdge->dx;
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
	pEdge->intensity += pEdge->dIntensity;
}

int RenderDroid::AFRasterModule::setupEdgeNGonGS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeAffine(pEdge, pFace, v0, v1, false, true, &AFRasterModule::advanceEdgeNGonGS);
}

void RenderDroid::AFRasterModule::setupNGonLAT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, false, 0, 0.0f, &AFRasterModule::drawNGonLAT, &AFRasterModule::setupEdgeNGonLAT);
}

void RenderDroid::AFRasterModule::drawNGonLAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;
	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonLAT_16_W8,
		&AFRasterModule::drawSpanNGonLAT_16_W16,
		&AFRasterModule::drawSpanNGonLAT_16_W32,
		&AFRasterModule::drawSpanNGonLAT_16_W64,
		&AFRasterModule::drawSpanNGonLAT_16_W128,
		&AFRasterModule::drawSpanNGonLAT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonLAT_8_W8,
		&AFRasterModule::drawSpanNGonLAT_8_W16,
		&AFRasterModule::drawSpanNGonLAT_8_W32,
		&AFRasterModule::drawSpanNGonLAT_8_W64,
		&AFRasterModule::drawSpanNGonLAT_8_W128,
		&AFRasterModule::drawSpanNGonLAT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonLAT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonLAT_16);

	drawTexturedSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeAffine(pEdge, pFace, v0, v1, true, false, &AFRasterModule::advanceLeftEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonFAT(RdActiveEdge *pEdge) {
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
	pEdge->x += pEdge->dx;
	pEdge->u += pEdge->du;
	pEdge->v += pEdge->dv;
}

void RenderDroid::AFRasterModule::setupNGonMLAT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, false, 0, 0.0f, &AFRasterModule::drawNGonMLAT, &AFRasterModule::setupEdgeNGonLAT);
}

void RenderDroid::AFRasterModule::drawNGonMLAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;
	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMLAT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMLAT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::plotMaskedRun(SpanRun &run, int count, bool bpp16, int lightKind) {
	for (int i = 0; i < count; i++) {
		int texel = _pPixels[(_vWrap & (run.v >> (16 - _texRowShift))) + ((_uWrap >> 16) & (run.u >> 16))];

		if (texel) {
			if (bpp16) {
				int index = texel;
				if (lightKind == 2)
					index += ((run.intensity >> 16) & 0x3F) << 8;

				*(uint16 *)run.pDst = ((uint16 *)_aRGBMap)[index];
			} else if (lightKind == 0) {
				*run.pDst = (byte)texel;
			} else {
				int index = texel;
				if (lightKind == 2)
					index += (run.intensity >> 8) & 0x3F00;

				*run.pDst = _aMonoMap[index];
			}

			*run.pZ = (uint16)(run.z >> run.zShift);
		}

		run.z += run.dz;
		run.u += run.du;
		run.v += run.dv;

		if (lightKind == 2)
			run.intensity += run.dIntensity;

		run.pDst += bpp16 ? 2 : 1;
		run.pZ++;
	}
}

void RenderDroid::AFRasterModule::drawSpanMaskedAffine(RdActiveSpan *pSpan, bool bpp16, int lightKind) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	StdVBuffer *pVBuffer = pCanvas->pVBuffer;
	StdVBuffer *pZBuffer = pCanvas->pZBuffer;
	SpanRun run;

	run.zShift = getZShift(pCanvas);
	run.u = pSpan->u;
	run.du = pSpan->du;
	run.dv = pSpan->dv;
	run.v = pSpan->v;
	run.dz = _dOneOverZdXFixed;
	run.z = (int)floor((float)(pSpan->oneOverZ * 65536.0));
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = pSpan->intensity;
		run.dIntensity = pSpan->dIntensity;
	}

	if (bpp16)
		run.pDst = pVBuffer->pPixels + 2 * pSpan->x + 2 * pSpan->y * (pVBuffer->tex.pitchInBytes >> 1);
	else
		run.pDst = pVBuffer->pPixels + pSpan->y * pVBuffer->tex.pitchInBytes + pSpan->x;

	run.pZ = (uint16 *)(pZBuffer->pPixels + 2 * pSpan->x + pSpan->y * pZBuffer->tex.pitchInBytes);

	plotMaskedRun(run, pSpan->width, bpp16, lightKind);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLAT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedAffine(pSpan, false, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLAT_16(RdActiveSpan *pSpan) {
	drawSpanMaskedAffine(pSpan, true, 0);
}

void RenderDroid::AFRasterModule::setupNGonFAT(RdActiveFace *pFace, RdMatCel *pCel, float light) {
	setupTextured(pFace, pCel, false, 1, light, &AFRasterModule::drawNGonFAT, &AFRasterModule::setupEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::drawNGonFAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonFAT_16_W8,
		&AFRasterModule::drawSpanNGonFAT_16_W16,
		&AFRasterModule::drawSpanNGonFAT_16_W32,
		&AFRasterModule::drawSpanNGonFAT_16_W64,
		&AFRasterModule::drawSpanNGonFAT_16_W128,
		&AFRasterModule::drawSpanNGonFAT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonFAT_8_W8,
		&AFRasterModule::drawSpanNGonFAT_8_W16,
		&AFRasterModule::drawSpanNGonFAT_8_W32,
		&AFRasterModule::drawSpanNGonFAT_8_W64,
		&AFRasterModule::drawSpanNGonFAT_8_W128,
		&AFRasterModule::drawSpanNGonFAT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonFAT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonFAT_16);

	drawTexturedSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeNGonLAT(pEdge, pFace, v0, v1);
}

void RenderDroid::AFRasterModule::setupNGonMFAT(RdActiveFace *pFace, RdMatCel *pCel, float light) {
	setupTextured(pFace, pCel, false, 1, light, &AFRasterModule::drawNGonMFAT, &AFRasterModule::setupEdgeNGonFAT);
}

void RenderDroid::AFRasterModule::drawNGonMFAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMFAT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMFAT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMFAT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedAffine(pSpan, false, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonMFAT_16(RdActiveSpan *pSpan) {
	drawSpanNGonMLAT_16(pSpan);
}

void RenderDroid::AFRasterModule::setupNGonGAT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, false, 2, 0.0f, &AFRasterModule::drawNGonGAT, &AFRasterModule::setupEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::drawNGonGAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonGAT_16_W8,
		&AFRasterModule::drawSpanNGonGAT_16_W16,
		&AFRasterModule::drawSpanNGonGAT_16_W32,
		&AFRasterModule::drawSpanNGonGAT_16_W64,
		&AFRasterModule::drawSpanNGonGAT_16_W128,
		&AFRasterModule::drawSpanNGonGAT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonGAT_8_W8,
		&AFRasterModule::drawSpanNGonGAT_8_W16,
		&AFRasterModule::drawSpanNGonGAT_8_W32,
		&AFRasterModule::drawSpanNGonGAT_8_W64,
		&AFRasterModule::drawSpanNGonGAT_8_W128,
		&AFRasterModule::drawSpanNGonGAT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonGAT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonGAT_16);

	drawTexturedSpans(pFace);
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonGAT(RdActiveEdge *pEdge) {
	pEdge->x += pEdge->dx;
	pEdge->u += pEdge->du;
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
	pEdge->v += pEdge->dv;
	pEdge->intensity += pEdge->dIntensity;
}

int RenderDroid::AFRasterModule::setupEdgeNGonGAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeAffine(pEdge, pFace, v0, v1, true, true, &AFRasterModule::advanceLeftEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::setupNGonMGAT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, false, 2, 0.0f, &AFRasterModule::drawNGonMGAT, &AFRasterModule::setupEdgeNGonGAT);
}

void RenderDroid::AFRasterModule::drawNGonMGAT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMGAT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMGAT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMGAT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedAffine(pSpan, false, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonMGAT_16(RdActiveSpan *pSpan) {
	drawSpanMaskedAffine(pSpan, true, 2);
}

void RenderDroid::AFRasterModule::setupNGonLIT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, true, 0, 0.0f, &AFRasterModule::drawNGonLIT, &AFRasterModule::setupEdgeNGonLIT);
}

void RenderDroid::AFRasterModule::loadPerspState(RdActiveFace *pFace) {
	_mipMapNum = pFace->mipLevel;
	_pPixels = pFace->pTexPixels;
	_texRowShift = pFace->texShift;
	_texPitch = pFace->texPitch;
	_uWrap = pFace->uWrap;
	_vWrap = pFace->vWrap;
	_oneOverZStep = pFace->dOneOverZdX16;
	_uOverZStep = pFace->dUOverZdX16;
	_vOverZStep = pFace->dVOverZdX16;
	_dOneOverZdXFixed = pFace->dOneOverZdXFixed;
	_dOneOverZdX = pFace->dOneOverZdX;
	_dUOverZdX = pFace->dUOverZdX;
	_dVOverZdX = pFace->dVOverZdX;
	_dUdXModifier = pFace->uModifier;
	_dVdXModifier = pFace->vModifier;
}

void RenderDroid::AFRasterModule::drawNGonLIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonLIT_16_W8,
		&AFRasterModule::drawSpanNGonLIT_16_W16,
		&AFRasterModule::drawSpanNGonLIT_16_W32,
		&AFRasterModule::drawSpanNGonLIT_16_W64,
		&AFRasterModule::drawSpanNGonLIT_16_W128,
		&AFRasterModule::drawSpanNGonLIT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonLIT_8_W8,
		&AFRasterModule::drawSpanNGonLIT_8_W16,
		&AFRasterModule::drawSpanNGonLIT_8_W32,
		&AFRasterModule::drawSpanNGonLIT_8_W64,
		&AFRasterModule::drawSpanNGonLIT_8_W128,
		&AFRasterModule::drawSpanNGonLIT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonLIT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonLIT_16);

	drawTexturedSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonLIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeNGonPerspective(pEdge, pFace, v0, v1, 0);
}

void RenderDroid::AFRasterModule::setEdgePerspectiveValues(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, float dy, double xOffset, int variant) {
	double yOneOverZ = (double)pFace->dOneOverZdY * dy;
	double yUOverZ = (double)pFace->dUOverZdY * dy;
	double yVOverZ = (double)pFace->dVOverZdY * dy;
	double xOneOverZ = pFace->dOneOverZdX * xOffset;
	double xUOverZ = pFace->dUOverZdX * xOffset;
	double xVOverZ = pFace->dVOverZdX * xOffset;
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
		pEdge->oneOverZ = (float)((yOneOverZ + pFace->aOneOverZ[v0]) + xOneOverZ);
	else
		pEdge->oneOverZ = (float)((xOneOverZ + pFace->aOneOverZ[v0]) + yOneOverZ);

	pEdge->uOverZ = (float)((yUOverZ + pFace->aUOverZ[v0]) + xUOverZ);

	if (vYFirst)
		pEdge->vOverZ = (float)((yVOverZ + pFace->aVOverZ[v0]) + xVOverZ);
	else
		pEdge->vOverZ = (float)((xVOverZ + pFace->aVOverZ[v0]) + yVOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeNGonPerspective(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1, int variant) {
	RdProcFace *pProcFace = pFace->pProcFace;
	RdVector3 *aVertices = pProcFace->aVertices;

	float y0 = (float)ceil(aVertices[v0][1]);
	float y1 = (float)ceil(aVertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	pEdge->numLines = numLines;
	if (numLines == 0)
		return 0;

	if (numLines <= 0) {
		pEdge->bLeft = 0;
		numLines = -numLines;
		pEdge->yStart = yStart1;
		pEdge->numLines = numLines;
		pEdge->pfStep = &AFRasterModule::advanceRightEdgeNGonLIT;

		if (numLines >= 2) {
			float dy = (float)(y1 - aVertices[v1][1]);
			double slopeX = ((double)aVertices[v0][0] - aVertices[v1][0]) / ((double)aVertices[v0][1] - aVertices[v1][1]);
			float slopeXF = (float)slopeX;
			pEdge->dx = (int)floor((float)(slopeX * 65536.0));
			pEdge->x = (int)floor((float)(((double)slopeXF * dy + aVertices[v1][0]) * 65536.0));
		} else {
			pEdge->x = (int)floor((float)(aVertices[v1][0] * 65536.0));
		}

		return 1;
	}

	pEdge->bLeft = 1;
	pEdge->yStart = yStart0;
	pEdge->pfStep = &AFRasterModule::advanceLeftEdgeNGonFIT;

	float dy = (float)(y0 - aVertices[v0][1]);
	float startX;

	if (numLines >= 2) {
		double slopeX = ((double)aVertices[v1][0] - aVertices[v0][0]) / ((double)aVertices[v1][1] - aVertices[v0][1]);
		float slopeXF = (float)slopeX;
		pEdge->dx = (int)floor((float)(slopeX * 65536.0));

		double xSum = (double)slopeXF * dy + aVertices[v0][0];
		startX = (float)xSum;
		pEdge->x = (int)floor((float)(xSum * 65536.0));

		pEdge->dOneOverZ = (float)((double)pFace->dOneOverZdX * slopeXF + pFace->dOneOverZdY);
		pEdge->dUOverZ = (float)((double)pFace->dUOverZdX * slopeXF + pFace->dUOverZdY);
		pEdge->dVOverZ = (float)((double)pFace->dVOverZdX * slopeXF + pFace->dVOverZdY);
	} else {
		startX = aVertices[v0][0];
		pEdge->x = (int)floor((float)(startX * 65536.0));
	}

	setEdgePerspectiveValues(pEdge, pFace, v0, dy, (double)startX - aVertices[v0][0], variant);

	return 1;
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonFIT(RdActiveEdge *pEdge) {
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
	pEdge->uOverZ = (float)((double)pEdge->dUOverZ + pEdge->uOverZ);
	pEdge->vOverZ = (float)((double)pEdge->dVOverZ + pEdge->vOverZ);
	pEdge->x += pEdge->dx;
}

void RenderDroid::AFRasterModule::advanceRightEdgeNGonLIT(RdActiveEdge *pEdge) {
	pEdge->x += pEdge->dx;
}

void RenderDroid::AFRasterModule::setupNGonMLIT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, true, 0, 0.0f, &AFRasterModule::drawNGonMLIT, &AFRasterModule::setupEdgeNGonLIT);
}

void RenderDroid::AFRasterModule::drawNGonMLIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMLIT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMLIT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanMaskedPersp(RdActiveSpan *pSpan, bool bpp16, int lightKind) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	StdVBuffer *pVBuffer = pCanvas->pVBuffer;
	StdVBuffer *pZBuffer = pCanvas->pZBuffer;
	SpanRun run;

	run.zShift = getZShift(pCanvas);

	int xAlign = pSpan->x & 0xF;
	int width = 0;

	if (xAlign) {
		width = pSpan->width;
		if (width >= 16 - xAlign)
			width = 16 - xAlign;
	}

	int remaining = pSpan->width - width;
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

	float oneOverZ = pSpan->oneOverZ;
	float uOverZ = pSpan->uOverZ;
	float vOverZ = pSpan->vOverZ;

	run.dz = _dOneOverZdXFixed;
	run.z = (int)floor((float)(oneOverZ * 65536.0));
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = pSpan->intensity;
		run.dIntensity = pSpan->dIntensity;
	}

	// The EXE switches the FPU to 24-bit precision rounding toward zero for the whole span
	_bLowPrecision = true;

	double inverse = fpuDiv(1.0, oneOverZ);

	if (bpp16)
		run.pDst = pVBuffer->pPixels + 2 * pSpan->x + 2 * pSpan->y * (pVBuffer->tex.pitchInBytes >> 1);
	else
		run.pDst = pVBuffer->pPixels + pSpan->y * pVBuffer->tex.pitchInBytes + pSpan->x;

	run.pZ = (uint16 *)(pZBuffer->pPixels + 2 * pSpan->x + pSpan->y * pZBuffer->tex.pitchInBytes);

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

	_bLowPrecision = false;
}

void RenderDroid::AFRasterModule::drawSpanNGonMLIT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedPersp(pSpan, false, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonMLIT_16(RdActiveSpan *pSpan) {
	drawSpanMaskedPersp(pSpan, true, 0);
}

void RenderDroid::AFRasterModule::setupNGonFIT(RdActiveFace *pFace, RdMatCel *pCel, float light) {
	setupTextured(pFace, pCel, true, 1, light, &AFRasterModule::drawNGonFIT, &AFRasterModule::setupEdgeNGonFIT);
}

void RenderDroid::AFRasterModule::drawNGonFIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonFIT_16_W8,
		&AFRasterModule::drawSpanNGonFIT_16_W16,
		&AFRasterModule::drawSpanNGonFIT_16_W32,
		&AFRasterModule::drawSpanNGonFIT_16_W64,
		&AFRasterModule::drawSpanNGonFIT_16_W128,
		&AFRasterModule::drawSpanNGonFIT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonFIT_8_W8,
		&AFRasterModule::drawSpanNGonFIT_8_W16,
		&AFRasterModule::drawSpanNGonFIT_8_W32,
		&AFRasterModule::drawSpanNGonFIT_8_W64,
		&AFRasterModule::drawSpanNGonFIT_8_W128,
		&AFRasterModule::drawSpanNGonFIT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonFIT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonFIT_16);

	drawTexturedSpans(pFace);
}

int RenderDroid::AFRasterModule::setupEdgeNGonFIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	return setupEdgeNGonPerspective(pEdge, pFace, v0, v1, 1);
}

void RenderDroid::AFRasterModule::setupNGonMFIT(RdActiveFace *pFace, RdMatCel *pCel, float light) {
	setupTextured(pFace, pCel, true, 1, light, &AFRasterModule::drawNGonMFIT, &AFRasterModule::setupEdgeNGonFIT);
}

void RenderDroid::AFRasterModule::drawNGonMFIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMFIT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMFIT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMFIT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedPersp(pSpan, false, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonMFIT_16(RdActiveSpan *pSpan) {
	drawSpanNGonMLIT_16(pSpan);
}

void RenderDroid::AFRasterModule::setupNGonGIT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, true, 2, 0.0f, &AFRasterModule::drawNGonGIT, &AFRasterModule::setupEdgeNGonGIT);
}

void RenderDroid::AFRasterModule::drawNGonGIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	DrawSpanFunc apWidthSpans[6] = {
		&AFRasterModule::drawSpanNGonGIT_16_W8,
		&AFRasterModule::drawSpanNGonGIT_16_W16,
		&AFRasterModule::drawSpanNGonGIT_16_W32,
		&AFRasterModule::drawSpanNGonGIT_16_W64,
		&AFRasterModule::drawSpanNGonGIT_16_W128,
		&AFRasterModule::drawSpanNGonGIT_16_W256
	};

	DrawSpanFunc apWidthSpans8[6] = {
		&AFRasterModule::drawSpanNGonGIT_8_W8,
		&AFRasterModule::drawSpanNGonGIT_8_W16,
		&AFRasterModule::drawSpanNGonGIT_8_W32,
		&AFRasterModule::drawSpanNGonGIT_8_W64,
		&AFRasterModule::drawSpanNGonGIT_8_W128,
		&AFRasterModule::drawSpanNGonGIT_8_W256
	};

	if (bpp == 8)
		_pfDrawSpan = _rd->isDemo() ? selectWidthSpan(pFace->texPitch, apWidthSpans8, &AFRasterModule::drawSpanNGonGIT_8) : nullptr;
	else if (bpp == 16)
		_pfDrawSpan = selectWidthSpan(pFace->texPitch, apWidthSpans, &AFRasterModule::drawSpanNGonGIT_16);

	drawTexturedSpans(pFace);
}

void RenderDroid::AFRasterModule::advanceEdgeNGonGIT(RdActiveEdge *pEdge) {
	pEdge->x += pEdge->dx;
	pEdge->intensity += pEdge->dIntensity;
}

void RenderDroid::AFRasterModule::advanceLeftEdgeNGonGIT(RdActiveEdge *pEdge) {
	pEdge->oneOverZ = (float)((double)pEdge->dOneOverZ + pEdge->oneOverZ);
	pEdge->x += pEdge->dx;
	pEdge->uOverZ = (float)((double)pEdge->dUOverZ + pEdge->uOverZ);
	pEdge->intensity += pEdge->dIntensity;
	pEdge->vOverZ = (float)((double)pEdge->dVOverZ + pEdge->vOverZ);
}

int RenderDroid::AFRasterModule::setupEdgeNGonGIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1) {
	RdProcFace *pProcFace = pFace->pProcFace;
	RdVector3 *aVertices = pProcFace->aVertices;
	float *aIntensities = pProcFace->aIntensities;

	float y0 = (float)ceil(aVertices[v0][1]);
	float y1 = (float)ceil(aVertices[v1][1]);
	int yStart0 = (int)floor(y0);
	int yStart1 = (int)floor(y1);
	int numLines = yStart1 - yStart0;

	pEdge->numLines = numLines;
	if (numLines == 0)
		return 0;

	if (numLines <= 0) {
		pEdge->bLeft = 0;
		numLines = -numLines;
		pEdge->yStart = yStart1;
		pEdge->numLines = numLines;
		pEdge->pfStep = &AFRasterModule::advanceEdgeNGonGIT;

		if (numLines >= 2) {
			float dy = (float)(y1 - aVertices[v1][1]);
			float inverseDy = (float)(1.0 / ((double)aVertices[v0][1] - aVertices[v1][1]));

			double slopeX = ((double)aVertices[v0][0] - aVertices[v1][0]) * inverseDy;
			float slopeXF = (float)slopeX;
			pEdge->dx = (int)floor((float)(slopeX * 65536.0));
			pEdge->x = (int)floor((float)(((double)slopeXF * dy + aVertices[v1][0]) * 65536.0));

			double slopeI = ((double)aIntensities[v0] - aIntensities[v1]) * inverseDy;
			float slopeIF = (float)slopeI;
			pEdge->dIntensity = (int)floor((float)(slopeI * 65536.0));
			pEdge->intensity = (int)floor((float)(((double)slopeIF * dy + aIntensities[v1]) * 65536.0));
		} else {
			pEdge->x = (int)floor((float)(aVertices[v1][0] * 65536.0));
			pEdge->intensity = (int)floor((float)(aIntensities[v1] * 65536.0));
		}

		return 1;
	}

	pEdge->bLeft = 1;
	pEdge->yStart = yStart0;
	pEdge->pfStep = &AFRasterModule::advanceLeftEdgeNGonGIT;

	float dy = (float)(y0 - aVertices[v0][1]);
	float startX;

	if (numLines >= 2) {
		float inverseDy = (float)(1.0 / ((double)aVertices[v1][1] - aVertices[v0][1]));

		double slopeX = ((double)aVertices[v1][0] - aVertices[v0][0]) * inverseDy;
		float slopeXF = (float)slopeX;
		pEdge->dx = (int)floor((float)(slopeX * 65536.0));

		double xSum = (double)slopeXF * dy + aVertices[v0][0];
		startX = (float)xSum;
		pEdge->x = (int)floor((float)(xSum * 65536.0));

		pEdge->dOneOverZ = (float)((double)pFace->dOneOverZdX * slopeXF + pFace->dOneOverZdY);
		pEdge->dUOverZ = (float)((double)pFace->dUOverZdX * slopeXF + pFace->dUOverZdY);
		pEdge->dVOverZ = (float)((double)pFace->dVOverZdX * slopeXF + pFace->dVOverZdY);

		double slopeI = ((double)aIntensities[v1] - aIntensities[v0]) * inverseDy;
		float slopeIF = (float)slopeI;
		pEdge->dIntensity = (int)floor((float)(slopeI * 65536.0));
		pEdge->intensity = (int)floor((float)(((double)slopeIF * dy + aIntensities[v0]) * 65536.0));
	} else {
		startX = aVertices[v0][0];
		pEdge->x = (int)floor((float)(startX * 65536.0));
		pEdge->intensity = (int)floor((float)(aIntensities[v0] * 65536.0));
	}

	setEdgePerspectiveValues(pEdge, pFace, v0, dy, (double)startX - aVertices[v0][0], 2);

	return 1;
}

void RenderDroid::AFRasterModule::setupNGonMGIT(RdActiveFace *pFace, RdMatCel *pCel) {
	setupTextured(pFace, pCel, true, 2, 0.0f, &AFRasterModule::drawNGonMGIT, &AFRasterModule::setupEdgeNGonGIT);
}

void RenderDroid::AFRasterModule::drawNGonMGIT(RdActiveFace *pFace) {
	int bpp = _rd->_camera.getCurrent()->pCanvas->pVBuffer->tex.format.bpp;

	if (bpp == 8)
		_aMonoMap = pFace->pLightLevel;
	else if (bpp == 16)
		_aRGBMap = pFace->pRGB16;

	loadPerspState(pFace);

	for (RdActiveSpan *pSpan = pFace->pFirstSpan; pSpan; pSpan = pSpan->pNext) {
		if (bpp == 8)
			drawSpanNGonMGIT_8(pSpan);
		else if (bpp == 16)
			drawSpanNGonMGIT_16(pSpan);
	}
}

void RenderDroid::AFRasterModule::drawSpanNGonMGIT_8(RdActiveSpan *pSpan) {
	drawSpanMaskedPersp(pSpan, false, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonMGIT_16(RdActiveSpan *pSpan) {
	drawSpanMaskedPersp(pSpan, true, 2);
}

void RenderDroid::AFRasterModule::drawZSpan(RdActiveSpan *pSpan) {
	StdVBuffer *pZBuffer = _rd->_camera.getCurrent()->pCanvas->pZBuffer;
	uint16 *pDst = (uint16 *)(pZBuffer->pPixels + 2 * pSpan->x + pZBuffer->tex.pitchInBytes * pSpan->y);
	uint32 z = (uint32)(int)floor((float)(pSpan->oneOverZ * 65536.0));
	uint32 dz = (uint32)_dOneOverZdXFixed;

	for (int i = 0; i < pSpan->width; i++) {
		pDst[i] = (uint16)z;
		z += dz;
	}
}

void RenderDroid::AFRasterModule::drawZSpanShifted(RdActiveSpan *pSpan) {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	StdVBuffer *pZBuffer = pCanvas->pZBuffer;
	uint16 *pDst = (uint16 *)(pZBuffer->pPixels + 2 * pSpan->x + pZBuffer->tex.pitchInBytes * pSpan->y);
	int shift = getZShift(pCanvas);
	uint32 z = (uint32)(int)floor((float)(pSpan->oneOverZ * 65536.0));
	uint32 dz = (uint32)_dOneOverZdXFixed;

	for (int i = 0; i < pSpan->width; i++) {
		pDst[i] = (uint16)(z >> shift);
		z += dz;
	}
}

void RenderDroid::AFRasterModule::plotRun16(SpanRun &run, int count, bool bIntensity) {
	const uint16 *pRGB = (const uint16 *)_aRGBMap;
	uint16 *pDst = (uint16 *)run.pDst;

	for (int i = 0; i < count; i++) {
		int index = _pPixels[((run.uMask & run.u) >> 16) | (_vWrap & (run.v >> run.vShift))];

		if (bIntensity)
			index += (run.intensity & 0x3F0000) >> 8;

		pDst[i] = pRGB[index];

		run.u += run.du;
		run.v += run.dv;

		if (bIntensity)
			run.intensity += run.dIntensity;
	}

	run.pDst += 2 * count;
}

void RenderDroid::AFRasterModule::plotRun8(SpanRun &run, int count, int lightKind, bool bPairedIntensity) {
	int pairIntensity = run.intensity;

	for (int i = 0; i < count; i++) {
		uint index = (uint)_pPixels[((run.uMask & (uint)run.u) >> 16) + (_vWrap & ((uint)run.v >> run.vShift))];

		// The gouraud blocks of 16 pixels use one intensity for each pair of pixels
		if (!bPairedIntensity || !(i & 1))
			pairIntensity = run.intensity;

		if (lightKind == 1)
			index = _aMonoMap[index];
		else if (lightKind == 2)
			index = _aMonoMap[index + ((pairIntensity & 0x3F0000) >> 8)];

		run.pDst[i] = (byte)index;

		run.u += run.du;
		run.v += run.dv;

		if (lightKind == 2)
			run.intensity += run.dIntensity;
	}

	run.pDst += count;
}

void RenderDroid::AFRasterModule::plotPerspRun(SpanRun &run, int count, int lightKind, bool bpp16, bool bChunk) {
	if (bpp16)
		plotRun16(run, count, lightKind == 2);
	else
		plotRun8(run, count, lightKind, bChunk && lightKind == 2);
}

void RenderDroid::AFRasterModule::drawSpanAffine8(RdActiveSpan *pSpan, int vShift, int lightKind) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	SpanRun run;

	run.pDst = pVBuffer->pPixels + pSpan->x + pVBuffer->tex.pitchInBytes * pSpan->y;
	run.u = pSpan->u + _dUdXModifier;
	run.du = pSpan->du;
	run.v = pSpan->v + _dVdXModifier;
	run.dv = pSpan->dv;
	run.uMask = _uWrap;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (lightKind == 2) {
		run.intensity = pSpan->intensity;
		run.dIntensity = pSpan->dIntensity;
	}

	plotRun8(run, pSpan->width, lightKind, false);
}

void RenderDroid::AFRasterModule::drawSpanAffine16(RdActiveSpan *pSpan, int uMask, int vShift, bool bIntensity) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	SpanRun run;

	run.pDst = pVBuffer->pPixels + 2 * pSpan->x + 2 * (pVBuffer->tex.pitchInBytes >> 1) * pSpan->y;
	run.u = pSpan->u + _dUdXModifier;
	run.du = pSpan->du;
	run.v = pSpan->v + _dVdXModifier;
	run.dv = pSpan->dv;
	run.uMask = uMask;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (bIntensity) {
		run.intensity = pSpan->intensity;
		run.dIntensity = pSpan->dIntensity;
	}

	plotRun16(run, pSpan->width, bIntensity);

	_spanU = run.u;
	_spanV = run.v;
	_spanDU = run.du;
	_spanDV = run.dv;
}

void RenderDroid::AFRasterModule::drawSpanPersp16(RdActiveSpan *pSpan, int uMask, int vShift, bool bIntensity) {
	drawSpanPersp(pSpan, uMask, vShift, bIntensity ? 2 : 0, true);
}

void RenderDroid::AFRasterModule::drawSpanPersp(RdActiveSpan *pSpan, int uMask, int vShift, int lightKind, bool bpp16) {
	StdVBuffer *pVBuffer = _rd->_camera.getCurrent()->pCanvas->pVBuffer;
	SpanRun run;
	bool bIntensity = (lightKind == 2);

	int xAlign = pSpan->x & 0xF;
	int width = 0;

	if (xAlign) {
		width = pSpan->width;
		if (width >= 16 - xAlign)
			width = 16 - xAlign;
	}

	int remaining = pSpan->width - width;
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

	float oneOverZ = pSpan->oneOverZ;
	float uOverZ = pSpan->uOverZ;
	float vOverZ = pSpan->vOverZ;

	_bLowPrecision = true;

	double inverse = fpuDiv(1.0, oneOverZ);
	float uStart = (float)fpuMul(inverse, uOverZ);
	float vStart = (float)fpuMul(inverse, vOverZ);

	if (bpp16)
		run.pDst = pVBuffer->pPixels + 2 * pSpan->x + 2 * (pVBuffer->tex.pitchInBytes >> 1) * pSpan->y;
	else
		run.pDst = pVBuffer->pPixels + pSpan->x + pVBuffer->tex.pitchInBytes * pSpan->y;

	run.uMask = uMask;
	run.vShift = vShift;
	run.intensity = 0;
	run.dIntensity = 0;

	if (bIntensity) {
		run.intensity = pSpan->intensity;
		run.dIntensity = pSpan->dIntensity;
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

	_bLowPrecision = false;
	_spanU = run.u;
	_spanV = run.v;
	_spanDU = run.du;
	_spanDV = run.dv;
}

double RenderDroid::AFRasterModule::fpuRound(double value) {
	if (!_bLowPrecision)
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

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, _uWrap, 16 - _texRowShift, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W8(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x70000, 13, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W16(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0xF0000, 12, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W32(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x1F0000, 11, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W64(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x3F0000, 10, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W128(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x7F0000, 9, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_16_W256(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0xFF0000, 8, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W8(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W8(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W16(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W16(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W32(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W32(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W64(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W64(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W128(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W128(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_16_W256(RdActiveSpan *pSpan) {
	drawSpanNGonLAT_16_W256(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, _uWrap, 16 - _texRowShift, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W8(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x70000, 13, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W16(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0xF0000, 12, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W32(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x1F0000, 11, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W64(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x3F0000, 10, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W128(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0x7F0000, 9, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_16_W256(RdActiveSpan *pSpan) {
	drawSpanAffine16(pSpan, 0xFF0000, 8, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, _uWrap, 16 - _texRowShift, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W8(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x70000, 13, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W16(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0xF0000, 12, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W32(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x1F0000, 11, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W64(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x3F0000, 10, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W128(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x7F0000, 9, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_16_W256(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0xFF0000, 8, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W8(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W8(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W16(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W16(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W32(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W32(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W64(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W64(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W128(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W128(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_16_W256(RdActiveSpan *pSpan) {
	drawSpanNGonLIT_16_W256(pSpan);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, _uWrap, 16 - _texRowShift, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W8(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x70000, 13, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W16(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0xF0000, 12, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W32(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x1F0000, 11, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W64(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x3F0000, 10, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W128(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0x7F0000, 9, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_16_W256(RdActiveSpan *pSpan) {
	drawSpanPersp16(pSpan, 0xFF0000, 8, true);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 16 - _texRowShift, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 13, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W16(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 12, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W32(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 11, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W64(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 10, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W128(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 9, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonLAT_8_W256(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 8, 0);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 16 - _texRowShift, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 13, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W16(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 12, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W32(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 11, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W64(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 10, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W128(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 9, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonFAT_8_W256(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 8, 1);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 16 - _texRowShift, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W8(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 13, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W16(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 12, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W32(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 11, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W64(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 10, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W128(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 9, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonGAT_8_W256(RdActiveSpan *pSpan) {
	drawSpanAffine8(pSpan, 8, 2);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 16 - _texRowShift, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 13, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W16(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 12, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W32(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 11, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W64(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 10, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W128(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 9, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonLIT_8_W256(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 8, 0, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 16 - _texRowShift, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 13, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W16(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 12, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W32(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 11, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W64(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 10, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W128(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 9, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonFIT_8_W256(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 8, 1, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 16 - _texRowShift, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W8(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 13, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W16(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 12, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W32(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 11, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W64(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 10, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W128(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 9, 2, false);
}

void RenderDroid::AFRasterModule::drawSpanNGonGIT_8_W256(RdActiveSpan *pSpan) {
	drawSpanPersp(pSpan, _uWrap, 8, 2, false);
}

} // End of namespace Grim
