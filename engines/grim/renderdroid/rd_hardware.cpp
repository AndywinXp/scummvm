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

#include "engines/grim/std/std_3d_config.h"

#if defined(GRIM_STD3D)

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

static void scaleVertexLight(float *light, float extraLight, float ambientLight, double scale) {
	double sum = (double)extraLight + (double)*light;

	*light = (float)(clampLight(sum, (float)sum, ambientLight) * scale);
}

static void scaleFaceLight(RdProcFace *procFace, int lightingMode, float ambientLight, int vertexColorMode, double scale, int solid) {
	switch (lightingMode) {
	case 1:
		procFace->diffuseLight = (float)(clampLight(procFace->extraLight, procFace->extraLight, ambientLight) * scale);
		break;
	case 2: {
		double sum = (double)procFace->extraLight + (double)procFace->diffuseLight;

		procFace->diffuseLight = (float)(clampLight(sum, (float)sum, ambientLight) * scale);
		break;
	}
	case 3:
		if (vertexColorMode == 1) {
			for (uint i = 0; i < procFace->numVertices; i++) {
				if (solid)
					scaleVertexLight(&procFace->intensities[i], procFace->extraLight, ambientLight, scale);

				scaleVertexLight(&procFace->redIntensities[i], procFace->extraLight, ambientLight, scale);
				scaleVertexLight(&procFace->greenIntensities[i], procFace->extraLight, ambientLight, scale);
				scaleVertexLight(&procFace->blueIntensities[i], procFace->extraLight, ambientLight, scale);
			}
		} else {
			for (uint i = 0; i < procFace->numVertices; i++)
				scaleVertexLight(&procFace->intensities[i], procFace->extraLight, ambientLight, scale);
		}
		break;
	default:
		break;
	}
}

static uint32 packVertexColor(int &red, int &green, int &blue, uint alpha, int hasTint, int hasFilter, const float *filterGain, const RdColorEffects &effects) {
	if (hasTint) {
		if (!effects.tint[0])
			red = 0;

		if (!effects.tint[1])
			green = 0;

		if (!effects.tint[2])
			blue = 0;
	}

	if (hasFilter) {
		red += truncateToInt((double)red * (double)filterGain[0]);
		green += truncateToInt((double)green * (double)filterGain[1]);
		blue += truncateToInt((double)blue * (double)filterGain[2]);
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

static void addTriangles(Std3DTri *tris, uint *total, uint numVertices, uint firstVertex, uint flags, RdTexCacheEntry *texture) {
	if (numVertices < 3) {
		warning("RenderDroid::CacheModule::sendFaceListToHardware(): face with %u vertices", numVertices);
		return;
	}

	if (numVertices == 3) {
		Std3DTri *tri = &tris[*total];

		tri->idx[2] = firstVertex;
		tri->idx[1] = firstVertex + 1;
		tri->idx[0] = firstVertex + 2;
		tri->flags = flags;
		tri->texture = texture;
		(*total)++;
		return;
	}

	uint numTris = numVertices - 2;
	uint first = 0;
	uint second = 1;
	uint last = numVertices - 1;
	Std3DTri *tri = &tris[*total];

	*total += numTris;

	for (uint i = 0; i < numTris; i++) {
		tri->idx[2] = firstVertex + first;
		tri->idx[1] = firstVertex + second;
		tri->idx[0] = firstVertex + last;
		tri->flags = flags;
		tri->texture = texture;

		if (!(i & 1)) {
			first = second;
			second++;
		} else {
			first = last;
			last--;
		}

		tri++;
	}
}

int RenderDroid::CacheModule::sendFaceListToHardware() {
	Std3D *std3D = _rd->_std3D;
	Std3DDevice *device = std3D->getCurDevice();

	if (!device) {
		warning("RenderDroid::CacheModule::sendFaceListToHardware(): no 3D device");
		return 0;
	}

	const RdColorEffects &effects = _rd->_curColorEffects;
	int hasFilter = 0;
	int hasTint = 0;
	int alt = 0;
	int mipLevel = 0;
	int colorRed = 0;
	int colorGreen = 0;
	int colorBlue = 0;
	float filterGain[3] = { 0.0f, 0.0f, 0.0f };
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

		hasFilter = 1;
		filterGain[0] = (float)((double)effects.filter[0] - (halfBlue + halfGreen));
		filterGain[1] = (float)((double)effects.filter[1] - ((double)halfRed + halfBlue));
		filterGain[2] = (float)((double)effects.filter[2] - ((double)halfRed + halfGreen));
	}

	if (effects.tint[0] || effects.tint[1] || effects.tint[2])
		hasTint = 1;

	if (hasFilter || hasTint || _rd->_curVertexColorMode == 1)
		baseFlags |= 0x8000;

	resetRenderList();

	for (size_t faceIndex = 0; faceIndex < _numProcFaces; faceIndex++) {
		RdProcFace *procFace = &_procFaces[faceIndex];
		float ambientLight = 0.0f;

		if (_rd->_curRenderOptions & 2)
			ambientLight = procFace->ambientLight;

		uint numVertices = procFace->numVertices;
		uint numTris = numVertices > 3 ? numVertices - 2 : 1;

		if (numVertices + _totalVerts >= std3D->getMaxVertices()) {
			drawRenderList();
			resetRenderList();
		} else if (_totalVerts + numVertices > kMaxHWVertices || _totalSolidTris + numTris > kMaxHWTris || _totalNormalTris + numTris > kMaxHWTris) {
			warning("RenderDroid::CacheModule::sendFaceListToHardware(): render list full");
			drawRenderList();
			resetRenderList();
		}

		uint firstVertex = _totalVerts;
		int geometryMode = procFace->geometryMode;

		if (geometryMode >= _rd->_curGeometryMode)
			geometryMode = _rd->_curGeometryMode;

		int lightingMode = procFace->lightingMode;

		if (lightingMode >= _rd->_curLightingMode)
			lightingMode = _rd->_curLightingMode;

		uint flags = baseFlags;
		uint alpha = 255;

		if (procFace->type & 2) {
			flags |= 0x200;
			alpha = _rd->isDemo() ? 90 : 200;
		}

		if (alpha != 255 && !device->alphaBlend && !device->alphaStipple) {
			alpha = 255;
			alt = 1;
		}

		RdMaterial *material = procFace->material;

		if (!material)
			continue;

		RdMatCel *cel = selectCel(procFace);

		if (!cel && (geometryMode == 3 || geometryMode == 4)) {
			warning("RenderDroid::CacheModule::sendFaceListToHardware(): face without a material cel");
			continue;
		}

		if (cel && geometryMode == 4 && !(cel->flags & 8))
			geometryMode = 3;

		RdTexCacheEntry *cacheEntry = nullptr;
		float texWidth = 0.0f;
		float texHeight = 0.0f;

		if (cel && (cel->flags & 8)) {
			RdTexture *texture = cel->texture;

			if (!procFace->colormap) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): face without a colormap");
				continue;
			}

			switch (texture->numMips) {
			case 1:
				mipLevel = 0;
				break;
			case 2:
				if (procFace->zMin > (double)_rd->_mipDistances[1])
					mipLevel = 1;
				else
					mipLevel = 0;
				break;
			case 3:
				if (procFace->zMin > (double)_rd->_mipDistances[0]) {
					if (procFace->zMin <= (double)_rd->_mipDistances[1])
						mipLevel = 1;
					else
						mipLevel = 2;
				} else {
					mipLevel = 0;
				}
				break;
			case 4:
				if (procFace->zMin > (double)_rd->_mipDistances[0]) {
					if (procFace->zMin > (double)_rd->_mipDistances[1]) {
						if (procFace->zMin > (double)_rd->_mipDistances[2])
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

			if (mipLevel < 0 || mipLevel >= 4 || !texture->mips[mipLevel]) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): invalid mip level %d", mipLevel);
				continue;
			}

			if ((texture->flags & 1) && device->alphaTexture)
				flags |= 0x400;

			// The demo sets the palette only when the identity colormap changes
			if (!_rd->isDemo() && !texture->cache[0][mipLevel].inCache)
				std3D->setCurrentPalette(procFace->colormap->colors, 90);

			if (!_rd->_material.addToTextureCache(material, texture, mipLevel, alt)) {
				drawRenderList();
				resetRenderList();

				if (!_rd->isDemo())
					std3D->purgeTextureCache();

				firstVertex = _totalVerts;
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): texture cache full, render list flushed");

				if (!_rd->_material.addToTextureCache(material, texture, mipLevel, alt))
					return 0;
			}

			cacheEntry = &texture->cache[alt ? 1 : 0][mipLevel];

			uint validWidth;
			uint validHeight;

			std3D->getValidDimensions(texture->mips[mipLevel]->tex.width, texture->mips[mipLevel]->tex.height, &validWidth, &validHeight);
			texWidth = (float)(uint)(validWidth << mipLevel);
			texHeight = (float)(uint)(validHeight << mipLevel);
		}

		if (geometryMode == 3) {
			if (!procFace->colormap || cel->colorIndex < 0 || cel->colorIndex > 255) {
				warning("RenderDroid::CacheModule::sendFaceListToHardware(): invalid face colour");
				continue;
			}

			const byte *color = &procFace->colormap->colors[3 * cel->colorIndex];

			scaleFaceLight(procFace, lightingMode, ambientLight, _rd->_curVertexColorMode, 63.0, 1);

			for (uint i = 0; i < procFace->numVertices; i++) {
				Std3DVertex *vertex = &_hwVertices[_totalVerts];
				float *pos = procFace->vertices[i];

				vertex->sx = (float)ceil((double)pos[0]);
				vertex->sy = (float)ceil((double)pos[1]);

				double invZ;

				if (_rd->isDemo()) {
					// The demo uses 1 / (z + 1), reversed when the z buffer is not 16
					if (pos[2] == 0.0f)
						invZ = 0.0;
					else
						invZ = 1.0 / ((double)pos[2] - -1.0);

					vertex->sz = (float)(std3D->getZBufferMode() == 16 ? invZ : 1.0 - invZ);
				} else {
					if (!(pos[2] < 0.0f || pos[2] > 0.0f)) {
						invZ = 0.0;
					} else {
						int zBias = -1;

						if (std3D->getZBufferMode() == 16)
							zBias = 1;

						invZ = 1.0 / ((double)zBias + (double)pos[2]);
					}

					vertex->sz = (float)invZ;
				}

				vertex->rhw = (float)(invZ * 0.03125);
				vertex->specular = 0;

				if (_rd->isDemo()) {
					// The demo takes the colors from the palette through the light table of the colormap
					const byte *palette = procFace->colormap->colors;
					const byte *lightLevel = procFace->colormap->lightLevel;
					int paletteIndex = 3 * cel->colorIndex;
					bool colorMode = false;

					if (lightingMode == 3 && _rd->_curVertexColorMode == 1) {
						colorMode = true;
						paletteIndex = 3 * lightLevel[cel->colorIndex];
					} else if (lightingMode == 3) {
						int level = truncateToInt(procFace->intensities[i]) & 0x3F;

						paletteIndex = 3 * lightLevel[(level << 8) + cel->colorIndex];
					} else if (lightingMode != 0) {
						int level = truncateToInt(procFace->diffuseLight) & 0x3F;

						paletteIndex = 3 * lightLevel[(level << 8) + cel->colorIndex];
					}

					colorRed = palette[paletteIndex];
					colorGreen = palette[paletteIndex + 1];
					colorBlue = palette[paletteIndex + 2];

					if (colorMode) {
						colorRed += truncateToInt((double)procFace->redIntensities[i] * 255.0 - -0.5);
						colorGreen += truncateToInt((double)procFace->greenIntensities[i] * 255.0 - -0.5);
						colorBlue += truncateToInt((double)procFace->blueIntensities[i] * 255.0 - -0.5);

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
					colorRed = color[0];
					colorGreen = color[1];
					colorBlue = color[2];
				} else if (lightingMode == 3) {
					if (_rd->_curVertexColorMode != 1) {
						double scale = (double)procFace->intensities[i] * 0.015625;

						colorRed = truncateToInt((double)color[0] * scale);
						colorGreen = truncateToInt((double)color[1] * scale);
						colorBlue = truncateToInt((double)color[2] * scale);
					}
				} else {
					double scale = (double)procFace->diffuseLight * 0.015625;

					colorRed = truncateToInt((double)color[0] * scale);
					colorGreen = truncateToInt((double)color[1] * scale);
					colorBlue = truncateToInt((double)color[2] * scale);
				}

				if (procFace->colormap != _rd->_colormap.getIdentity()) {
					colorRed = truncateToInt((double)procFace->colormap->tint[0] * (double)colorRed) & 0xFF;
					colorGreen = truncateToInt((double)procFace->colormap->tint[1] * (double)colorGreen) & 0xFF;
					colorBlue = truncateToInt((double)procFace->colormap->tint[2] * (double)colorBlue) & 0xFF;
				}

				flags |= 0x8000;
				vertex->color = packVertexColor(colorRed, colorGreen, colorBlue, alpha, hasTint, hasFilter, filterGain, effects);
				vertex->tu = 0.0f;
				vertex->tv = 0.0f;
				_totalVerts++;
			}

			addTriangles(_hwSolidTris, &_totalSolidTris, procFace->numVertices, firstVertex, flags, nullptr);
		} else if (geometryMode == 4) {
			scaleFaceLight(procFace, lightingMode, ambientLight, _rd->_curVertexColorMode, 255.0, 0);

			for (uint i = 0; i < procFace->numVertices; i++) {
				Std3DVertex *vertex = &_hwVertices[_totalVerts];
				float *pos = procFace->vertices[i];

				vertex->sx = (float)ceil((double)pos[0]);
				vertex->sy = (float)ceil((double)pos[1]);

				double invZ;

				if (_rd->isDemo()) {
					// The demo uses 1 / (z + 1), reversed when the z buffer is not 16
					if (pos[2] == 0.0f)
						invZ = 0.0;
					else
						invZ = 1.0 / ((double)pos[2] - -1.0);

					vertex->sz = (float)(std3D->getZBufferMode() == 16 ? invZ : 1.0 - invZ);
				} else {
					if (!(pos[2] < 0.0f || pos[2] > 0.0f)) {
						invZ = 0.0;
					} else {
						int zBias = -1;

						if (std3D->getZBufferMode() == 16)
							zBias = 1;

						pos[2] = (float)((double)zBias + (double)pos[2]);
						invZ = 1.0 / (double)pos[2];
					}

					vertex->sz = (float)invZ;
				}

				vertex->rhw = (float)invZ;
				vertex->specular = 0;

				if (lightingMode == 0) {
					colorRed = 255;
					colorGreen = 255;
					colorBlue = 255;
				} else if (lightingMode == 3) {
					if (_rd->_curVertexColorMode == 1) {
						colorRed = truncateToInt(procFace->redIntensities[i]);
						colorGreen = truncateToInt(procFace->greenIntensities[i]);
						colorBlue = truncateToInt(procFace->blueIntensities[i]);
					} else {
						colorRed = truncateToInt(procFace->intensities[i]);
						colorGreen = colorRed;
						colorBlue = colorRed;
					}
				} else {
					colorRed = truncateToInt(procFace->diffuseLight);
					colorGreen = colorRed;
					colorBlue = colorRed;
				}

				if (procFace->colormap != _rd->_colormap.getIdentity()) {
					colorRed = truncateToInt((double)procFace->colormap->tint[0] * (double)colorRed) & 0xFF;
					colorGreen = truncateToInt((double)procFace->colormap->tint[1] * (double)colorGreen) & 0xFF;
					colorBlue = truncateToInt((double)procFace->colormap->tint[2] * (double)colorBlue) & 0xFF;
					flags |= 0x8000;
				}

				vertex->color = packVertexColor(colorRed, colorGreen, colorBlue, alpha, hasTint, hasFilter, filterGain, effects);
				vertex->tu = (float)((double)procFace->texVertices[i][0] / (double)texWidth);
				vertex->tv = (float)((double)procFace->texVertices[i][1] / (double)texHeight);
				_totalVerts++;
			}

			addTriangles(_hwNormalTris, &_totalNormalTris, procFace->numVertices, firstVertex, flags, cacheEntry);
		}
	}

	drawRenderList();

	return 1;
}

int RenderDroid::CacheModule::resetRenderList() {
	_rd->_std3D->resetRenderList();
	_totalNormalTris = 0;
	_totalSolidTris = 0;
	_totalVerts = 0;

	return 0;
}

uint RenderDroid::CacheModule::drawRenderList() {
	Std3D *std3D = _rd->_std3D;

	if (!_totalVerts)
		return _totalVerts;

	if (!std3D->addRenderListVertices(_hwVertices, _totalVerts)) {
		std3D->drawRenderList();
		std3D->addRenderListVertices(_hwVertices, _totalVerts);
	}

	std3D->beginRenderList();

	if (_rd->_curZBufferMethod == 2)
		sort(_hwNormalTris, _totalNormalTris, sizeof(Std3DTri), hwTriCompare);

	if (_totalSolidTris)
		std3D->addRenderListTris(_hwSolidTris, _totalSolidTris);

	if (_totalNormalTris)
		std3D->addRenderListTris(_hwNormalTris, _totalNormalTris);

	return std3D->drawRenderList();
}

int RenderDroid::CacheModule::hwTriCompare(const void *a, const void *b) {
	const RdTexCacheEntry *textureA = ((const Std3DTri *)a)->texture;
	const RdTexCacheEntry *textureB = ((const Std3DTri *)b)->texture;

	if (textureA->colorKey != textureB->colorKey) {
		if (textureA->colorKey)
			return 1;

		return -1;
	}

	if ((uintptr)textureA < (uintptr)textureB)
		return -1;

	if ((uintptr)textureA > (uintptr)textureB)
		return 1;

	return 0;
}

} // End of namespace Grim

#endif
