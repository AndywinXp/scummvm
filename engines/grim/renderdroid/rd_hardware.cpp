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

#include "common/scummsys.h"

#if defined(USE_OPENGL_SHADERS)

#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

static const uint kMaxHWVertices = 32768;
static const uint kMaxHWTris = 1024;

static int truncateToInt(double value) {
	if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0)) {
		warning("RenderDroid::CacheModule::sendFaceListToHardware(): value out of range");
		return 0;
	}

	return (int)(int64)value;
}

static double clampLight(double sum, float stored, float ambientLight) {
	double clamped;

	if (!(sum >= 0.0))
		clamped = 0.0;
	else if (stored > 1.0f)
		clamped = 1.0;
	else
		clamped = stored;

	if (!(clamped > ambientLight))
		return ambientLight;

	if (!(stored >= 0.0f))
		return 0.0;

	if (stored > 1.0f)
		return 1.0;

	return stored;
}

static void scaleVertexLight(float *pLight, float extraLight, float ambientLight, double scale) {
	double sum = (double)extraLight + (double)*pLight;

	*pLight = (float)(clampLight(sum, (float)sum, ambientLight) * scale);
}

static void scaleFaceLight(RdProcFace *pProcFace, int lightingMode, float ambientLight, int vertexColorMode, double scale, int bSolid) {
	switch (lightingMode) {
	case 1:
		pProcFace->diffuseLight = (float)(clampLight(pProcFace->extraLight, pProcFace->extraLight, ambientLight) * scale);
		break;
	case 2: {
		double sum = (double)pProcFace->extraLight + (double)pProcFace->diffuseLight;

		pProcFace->diffuseLight = (float)(clampLight(sum, (float)sum, ambientLight) * scale);
		break;
	}
	case 3:
		if (vertexColorMode == 1) {
			for (uint i = 0; i < pProcFace->numVertices; i++) {
				if (bSolid)
					scaleVertexLight(&pProcFace->aIntensities[i], pProcFace->extraLight, ambientLight, scale);

				scaleVertexLight(&pProcFace->aRedIntensities[i], pProcFace->extraLight, ambientLight, scale);
				scaleVertexLight(&pProcFace->aGreenIntensities[i], pProcFace->extraLight, ambientLight, scale);
				scaleVertexLight(&pProcFace->aBlueIntensities[i], pProcFace->extraLight, ambientLight, scale);
			}
		} else {
			for (uint i = 0; i < pProcFace->numVertices; i++)
				scaleVertexLight(&pProcFace->aIntensities[i], pProcFace->extraLight, ambientLight, scale);
		}
		break;
	default:
		break;
	}
}

static uint32 packVertexColor(int &red, int &green, int &blue, uint alpha, int bHasTint, int bHasFilter, const float *aFilterGain, const RdColorEffects &effects) {
	if (bHasTint) {
		if (!effects.tint[0])
			red = 0;

		if (!effects.tint[1])
			green = 0;

		if (!effects.tint[2])
			blue = 0;
	}

	if (bHasFilter) {
		red += truncateToInt((double)red * (double)aFilterGain[0]);
		green += truncateToInt((double)green * (double)aFilterGain[1]);
		blue += truncateToInt((double)blue * (double)aFilterGain[2]);
	}

	if (!(effects.fade >= 1.0f)) {
		red = truncateToInt((double)red * (double)effects.fade);
		green = truncateToInt((double)green * (double)effects.fade);
		blue = truncateToInt((double)blue * (double)effects.fade);
	}

	if (red < 0)
		red = 0;
	else if (red > 255)
		red = 255;

	if (green < 0)
		green = 0;
	else if (green > 255)
		green = 255;

	if (blue < 0)
		blue = 0;
	else if (blue > 255)
		blue = 255;

	red &= 0xFF;
	green &= 0xFF;
	blue &= 0xFF;

	return (alpha << 24) | ((uint)red << 16) | ((uint)green << 8) | (uint)blue;
}

static void addTriangles(Std3DTri *aTris, uint *pTotal, uint numVertices, uint firstVertex, uint flags, RdTexCacheEntry *pTexture) {
	if (numVertices < 3) {
		warning("RenderDroid::CacheModule::sendFaceListToHardware(): face with %u vertices", numVertices);
		return;
	}

	if (numVertices == 3) {
		Std3DTri *pTri = &aTris[*pTotal];

		pTri->aIdx[2] = firstVertex;
		pTri->aIdx[1] = firstVertex + 1;
		pTri->aIdx[0] = firstVertex + 2;
		pTri->flags = flags;
		pTri->pTexture = pTexture;
		(*pTotal)++;
		return;
	}

	uint numTris = numVertices - 2;
	uint first = 0;
	uint second = 1;
	uint last = numVertices - 1;
	Std3DTri *pTri = &aTris[*pTotal];

	*pTotal += numTris;

	for (uint i = 0; i < numTris; i++) {
		pTri->aIdx[2] = firstVertex + first;
		pTri->aIdx[1] = firstVertex + second;
		pTri->aIdx[0] = firstVertex + last;
		pTri->flags = flags;
		pTri->pTexture = pTexture;

		if (!(i & 1)) {
			first = second;
			second++;
		} else {
			first = last;
			last--;
		}

		pTri++;
	}
}

int RenderDroid::CacheModule::sendFaceListToHardware() {
	Std3D *pStd3D = _rd->_pStd3D;
	Std3DDevice *pDevice = pStd3D->getCurDevice();

	if (!pDevice) {
		warning("RenderDroid::CacheModule::sendFaceListToHardware(): no 3D device");
		return 0;
	}

	const RdColorEffects &effects = _rd->_curColorEffects;
	int bHasFilter = 0;
	int bHasTint = 0;
	int alt = 0;
	int mipLevel = 0;
	int colorRed = 0;
	int colorGreen = 0;
	int colorBlue = 0;
	float aFilterGain[3] = { 0.0f, 0.0f, 0.0f };
	uint baseFlags = 0x33;

	switch (_rd->_curZBufferMethod) {
	case 1:
		baseFlags = 0x1033;
		break;
	case 2:
		baseFlags = 0x1833;
		break;
	case 3:
		baseFlags = 0x833;
		break;
	default:
		break;
	}

	if (effects.filter[0] > 0.0f || effects.filter[1] > 0.0f || effects.filter[2] > 0.0f) {
		double halfGreen = (double)effects.filter[1] * 0.5;
		double halfBlue = (double)effects.filter[2] * 0.5;
		float halfRed = (float)((double)effects.filter[0] * 0.5);

		bHasFilter = 1;
		aFilterGain[0] = (float)((double)effects.filter[0] - (halfBlue + halfGreen));
		aFilterGain[1] = (float)((double)effects.filter[1] - ((double)halfRed + halfBlue));
		aFilterGain[2] = (float)((double)effects.filter[2] - ((double)halfRed + halfGreen));
	}

	if (effects.tint[0] || effects.tint[1] || effects.tint[2])
		bHasTint = 1;

	if (bHasFilter || bHasTint || _rd->_curVertexColorMode == 1)
		baseFlags |= 0x8000;

	resetRenderList();

	for (size_t faceIndex = 0; faceIndex < _numProcFaces; faceIndex++) {
		RdProcFace *pProcFace = &_aProcFaces[faceIndex];
		float ambientLight = 0.0f;

		if (_rd->_curRenderOptions & 2)
			ambientLight = pProcFace->ambientLight;

		uint numVertices = pProcFace->numVertices;
		uint numTris = numVertices > 3 ? numVertices - 2 : 1;

		if (numVertices + _totalVerts >= pStd3D->getMaxVertices()) {
			drawRenderList();
			resetRenderList();
		} else if (_totalVerts + numVertices > kMaxHWVertices || _totalSolidTris + numTris > kMaxHWTris || _totalNormalTris + numTris > kMaxHWTris) {
			warning("RenderDroid::CacheModule::sendFaceListToHardware(): render list full");
			drawRenderList();
			resetRenderList();
		}

		uint firstVertex = _totalVerts;
		int geometryMode = pProcFace->geometryMode;

		if (geometryMode >= _rd->_curGeometryMode)
			geometryMode = _rd->_curGeometryMode;

		int lightingMode = pProcFace->lightingMode;

		if (lightingMode >= _rd->_curLightingMode)
			lightingMode = _rd->_curLightingMode;

		uint flags = baseFlags;
		uint alpha = 255;

		if (pProcFace->type & 2) {
			flags |= 0x200;
			alpha = _rd->isDemo() ? 90 : 200;
		}

		if (alpha != 255 && !pDevice->bAlphaBlend && !pDevice->bAlphaStipple) {
			alpha = 255;
			alt = 1;
		}

		RdMaterial *pMaterial = pProcFace->pMaterial;

		if (!pMaterial)
			continue;

		RdMatCel *pCel = selectCel(pProcFace);

		if (!pCel && (geometryMode == 3 || geometryMode == 4)) {
			warning("RenderDroid::CacheModule::sendFaceListToHardware(): face without a material cel");
			continue;
		}

		if (pCel && geometryMode == 4 && !(pCel->flags & 8))
			geometryMode = 3;

		RdTexCacheEntry *pCacheEntry = nullptr;
		float texWidth = 0.0f;
		float texHeight = 0.0f;

		if (pCel && (pCel->flags & 8)) {
			RdTexture *pTexture = pCel->pTexture;

			if (!pProcFace->pColormap) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): face without a colormap");
				continue;
			}

			switch (pTexture->numMips) {
			case 1:
				mipLevel = 0;
				break;
			case 2:
				if (pProcFace->zMin > (double)_rd->_aMipDistances[1])
					mipLevel = 1;
				else
					mipLevel = 0;
				break;
			case 3:
				if (pProcFace->zMin > (double)_rd->_aMipDistances[0]) {
					if (pProcFace->zMin <= (double)_rd->_aMipDistances[1])
						mipLevel = 1;
					else
						mipLevel = 2;
				} else {
					mipLevel = 0;
				}
				break;
			case 4:
				if (pProcFace->zMin > (double)_rd->_aMipDistances[0]) {
					if (pProcFace->zMin > (double)_rd->_aMipDistances[1]) {
						if (pProcFace->zMin > (double)_rd->_aMipDistances[2])
							mipLevel = 3;
						else
							mipLevel = 2;
					} else {
						mipLevel = 1;
					}
				} else {
					mipLevel = 0;
				}
				break;
			default:
				break;
			}

			if (mipLevel < 0 || mipLevel >= 4 || !pTexture->apMips[mipLevel]) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): invalid mip level %d", mipLevel);
				continue;
			}

			if ((pTexture->flags & 1) && pDevice->bAlphaTexture)
				flags |= 0x400;

			// The demo sets the palette only when the identity colormap changes
			if (!_rd->isDemo() && !pTexture->aCache[0][mipLevel].bInCache)
				pStd3D->setCurrentPalette(pProcFace->pColormap->aColors, 90);

			if (!_rd->_material.addToTextureCache(pMaterial, pTexture, mipLevel, alt)) {
				drawRenderList();
				resetRenderList();

				if (!_rd->isDemo())
					pStd3D->purgeTextureCache();

				firstVertex = _totalVerts;
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): texture cache full, render list flushed");

				if (!_rd->_material.addToTextureCache(pMaterial, pTexture, mipLevel, alt))
					return 0;
			}

			pCacheEntry = &pTexture->aCache[alt ? 1 : 0][mipLevel];

			uint validWidth;
			uint validHeight;

			pStd3D->getValidDimensions(pTexture->apMips[mipLevel]->tex.width, pTexture->apMips[mipLevel]->tex.height, &validWidth, &validHeight);
			texWidth = (float)(uint)(validWidth << mipLevel);
			texHeight = (float)(uint)(validHeight << mipLevel);
		}

		if (geometryMode == 3) {
			if (!pProcFace->pColormap || pCel->colorIndex < 0 || pCel->colorIndex > 255) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): invalid face colour");
				continue;
			}

			const byte *pColor = &pProcFace->pColormap->aColors[3 * pCel->colorIndex];

			scaleFaceLight(pProcFace, lightingMode, ambientLight, _rd->_curVertexColorMode, 63.0, 1);

			for (uint i = 0; i < pProcFace->numVertices; i++) {
				Std3DVertex *pVertex = &_aHWVertices[_totalVerts];
				float *pPos = pProcFace->aVertices[i];

				pVertex->sx = (float)ceil((double)pPos[0]);
				pVertex->sy = (float)ceil((double)pPos[1]);

				double invZ;

				if (_rd->isDemo()) {
					// The demo uses 1 / (z + 1), reversed when the z buffer is not 16
					if (pPos[2] == 0.0f)
						invZ = 0.0;
					else
						invZ = 1.0 / ((double)pPos[2] - -1.0);

					pVertex->sz = (float)(pStd3D->getZBufferMode() == 16 ? invZ : 1.0 - invZ);
				} else {
					if (!(pPos[2] < 0.0f || pPos[2] > 0.0f)) {
						invZ = 0.0;
					} else {
						int zBias = -1;

						if (pStd3D->getZBufferMode() == 16)
							zBias = 1;

						invZ = 1.0 / ((double)zBias + (double)pPos[2]);
					}

					pVertex->sz = (float)invZ;
				}

				pVertex->rhw = (float)(invZ * 0.03125);
				pVertex->specular = 0;

				if (_rd->isDemo()) {
					// The demo takes the colors from the palette through the light table of the colormap
					const byte *pPalette = pProcFace->pColormap->aColors;
					const byte *pLightLevel = pProcFace->pColormap->pLightLevel;
					int paletteIndex = 3 * pCel->colorIndex;
					bool bColorMode = false;

					if (lightingMode == 3 && _rd->_curVertexColorMode == 1) {
						bColorMode = true;
						paletteIndex = 3 * pLightLevel[pCel->colorIndex];
					} else if (lightingMode == 3) {
						int level = truncateToInt(pProcFace->aIntensities[i]) & 0x3F;

						paletteIndex = 3 * pLightLevel[(level << 8) + pCel->colorIndex];
					} else if (lightingMode != 0) {
						int level = truncateToInt(pProcFace->diffuseLight) & 0x3F;

						paletteIndex = 3 * pLightLevel[(level << 8) + pCel->colorIndex];
					}

					colorRed = pPalette[paletteIndex];
					colorGreen = pPalette[paletteIndex + 1];
					colorBlue = pPalette[paletteIndex + 2];

					if (bColorMode) {
						colorRed += truncateToInt((double)pProcFace->aRedIntensities[i] * 255.0 - -0.5);
						colorGreen += truncateToInt((double)pProcFace->aGreenIntensities[i] * 255.0 - -0.5);
						colorBlue += truncateToInt((double)pProcFace->aBlueIntensities[i] * 255.0 - -0.5);

						if (colorRed < 0)
							colorRed = 0;
						else if (colorRed > 255)
							colorRed = 255;

						if (colorGreen < 0)
							colorGreen = 0;
						else if (colorGreen > 255)
							colorGreen = 255;

						if (colorBlue < 0)
							colorBlue = 0;
						else if (colorBlue > 255)
							colorBlue = 255;
					}
				} else if (lightingMode == 0) {
					colorRed = pColor[0];
					colorGreen = pColor[1];
					colorBlue = pColor[2];
				} else if (lightingMode == 3) {
					if (_rd->_curVertexColorMode != 1) {
						double scale = (double)pProcFace->aIntensities[i] * 0.015625;

						colorRed = truncateToInt((double)pColor[0] * scale);
						colorGreen = truncateToInt((double)pColor[1] * scale);
						colorBlue = truncateToInt((double)pColor[2] * scale);
					}
				} else {
					double scale = (double)pProcFace->diffuseLight * 0.015625;

					colorRed = truncateToInt((double)pColor[0] * scale);
					colorGreen = truncateToInt((double)pColor[1] * scale);
					colorBlue = truncateToInt((double)pColor[2] * scale);
				}

				if (pProcFace->pColormap != _rd->_colormap.getIdentity()) {
					colorRed = truncateToInt((double)pProcFace->pColormap->tint[0] * (double)colorRed) & 0xFF;
					colorGreen = truncateToInt((double)pProcFace->pColormap->tint[1] * (double)colorGreen) & 0xFF;
					colorBlue = truncateToInt((double)pProcFace->pColormap->tint[2] * (double)colorBlue) & 0xFF;
				}

				flags |= 0x8000;
				pVertex->color = packVertexColor(colorRed, colorGreen, colorBlue, alpha, bHasTint, bHasFilter, aFilterGain, effects);
				pVertex->tu = 0.0f;
				pVertex->tv = 0.0f;
				_totalVerts++;
			}

			addTriangles(_aHWSolidTris, &_totalSolidTris, pProcFace->numVertices, firstVertex, flags, nullptr);
		} else if (geometryMode == 4) {
			scaleFaceLight(pProcFace, lightingMode, ambientLight, _rd->_curVertexColorMode, 255.0, 0);

			for (uint i = 0; i < pProcFace->numVertices; i++) {
				Std3DVertex *pVertex = &_aHWVertices[_totalVerts];
				float *pPos = pProcFace->aVertices[i];

				pVertex->sx = (float)ceil((double)pPos[0]);
				pVertex->sy = (float)ceil((double)pPos[1]);

				double invZ;

				if (_rd->isDemo()) {
					// The demo uses 1 / (z + 1), reversed when the z buffer is not 16
					if (pPos[2] == 0.0f)
						invZ = 0.0;
					else
						invZ = 1.0 / ((double)pPos[2] - -1.0);

					pVertex->sz = (float)(pStd3D->getZBufferMode() == 16 ? invZ : 1.0 - invZ);
				} else {
					if (!(pPos[2] < 0.0f || pPos[2] > 0.0f)) {
						invZ = 0.0;
					} else {
						int zBias = -1;

						if (pStd3D->getZBufferMode() == 16)
							zBias = 1;

						pPos[2] = (float)((double)zBias + (double)pPos[2]);
						invZ = 1.0 / (double)pPos[2];
					}

					pVertex->sz = (float)invZ;
				}

				pVertex->rhw = (float)invZ;
				pVertex->specular = 0;

				if (lightingMode == 0) {
					colorRed = 255;
					colorGreen = 255;
					colorBlue = 255;
				} else if (lightingMode == 3) {
					if (_rd->_curVertexColorMode == 1) {
						colorRed = truncateToInt(pProcFace->aRedIntensities[i]);
						colorGreen = truncateToInt(pProcFace->aGreenIntensities[i]);
						colorBlue = truncateToInt(pProcFace->aBlueIntensities[i]);
					} else {
						colorRed = truncateToInt(pProcFace->aIntensities[i]);
						colorGreen = colorRed;
						colorBlue = colorRed;
					}
				} else {
					colorRed = truncateToInt(pProcFace->diffuseLight);
					colorGreen = colorRed;
					colorBlue = colorRed;
				}

				if (pProcFace->pColormap != _rd->_colormap.getIdentity()) {
					colorRed = truncateToInt((double)pProcFace->pColormap->tint[0] * (double)colorRed) & 0xFF;
					colorGreen = truncateToInt((double)pProcFace->pColormap->tint[1] * (double)colorGreen) & 0xFF;
					colorBlue = truncateToInt((double)pProcFace->pColormap->tint[2] * (double)colorBlue) & 0xFF;
					flags |= 0x8000;
				}

				pVertex->color = packVertexColor(colorRed, colorGreen, colorBlue, alpha, bHasTint, bHasFilter, aFilterGain, effects);
				pVertex->tu = (float)((double)pProcFace->aTexVertices[i][0] / (double)texWidth);
				pVertex->tv = (float)((double)pProcFace->aTexVertices[i][1] / (double)texHeight);
				_totalVerts++;
			}

			addTriangles(_aHWNormalTris, &_totalNormalTris, pProcFace->numVertices, firstVertex, flags, pCacheEntry);
		}
	}

	drawRenderList();

	return 1;
}

int RenderDroid::CacheModule::resetRenderList() {
	_rd->_pStd3D->resetRenderList();
	_totalNormalTris = 0;
	_totalSolidTris = 0;
	_totalVerts = 0;

	return 0;
}

uint RenderDroid::CacheModule::drawRenderList() {
	Std3D *pStd3D = _rd->_pStd3D;

	if (!_totalVerts)
		return _totalVerts;

	if (!pStd3D->addRenderListVertices(_aHWVertices, _totalVerts)) {
		pStd3D->drawRenderList();
		pStd3D->addRenderListVertices(_aHWVertices, _totalVerts);
	}

	pStd3D->beginRenderList();

	if (_rd->_curZBufferMethod == 2)
		sort(_aHWNormalTris, _totalNormalTris, sizeof(Std3DTri), hwTriCompare);

	if (_totalSolidTris)
		pStd3D->addRenderListTris(_aHWSolidTris, _totalSolidTris);

	if (_totalNormalTris)
		pStd3D->addRenderListTris(_aHWNormalTris, _totalNormalTris);

	return pStd3D->drawRenderList();
}

int RenderDroid::CacheModule::hwTriCompare(const void *pA, const void *pB) {
	const RdTexCacheEntry *pTextureA = ((const Std3DTri *)pA)->pTexture;
	const RdTexCacheEntry *pTextureB = ((const Std3DTri *)pB)->pTexture;

	if (pTextureA->bColorKey != pTextureB->bColorKey) {
		if (pTextureA->bColorKey)
			return 1;

		return -1;
	}

	if ((uintptr)pTextureA < (uintptr)pTextureB)
		return -1;

	if ((uintptr)pTextureA > (uintptr)pTextureB)
		return 1;

	return 0;
}

} // End of namespace Grim

#endif
