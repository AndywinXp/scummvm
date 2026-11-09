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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

RdSprite *RenderDroid::SpriteModule::create(int type, char *name, char *matFilename, float width, float height, int geometryMode, int lightingMode, int textureMode, float extraLight, RdVector3 *pOffset) {
	RdSprite *pSprite = new RdSprite;
	memset(pSprite, 0, sizeof(RdSprite));

	newEntry(pSprite, name, type, matFilename, width, height, geometryMode, lightingMode, textureMode, extraLight, pOffset);

	return pSprite;
}

int RenderDroid::SpriteModule::newEntry(RdSprite *pSprite, char *name, int type, char *matFilename, float width, float height, int geometryMode, int lightingMode, int textureMode, float extraLight, RdVector3 *pOffset) {
	if (name)
		Common::strlcpy(pSprite->aName, name, 32);

	pSprite->width = width;
	pSprite->type = type;
	pSprite->height = height;
	pSprite->offset[0] = (*pOffset)[0];
	pSprite->offset[1] = (*pOffset)[1];
	pSprite->offset[2] = (*pOffset)[2];
	pSprite->face.lightingMode = lightingMode;
	pSprite->face.geometryMode = geometryMode;
	pSprite->face.type = 1;
	pSprite->face.textureMode = textureMode;
	pSprite->face.extraLight = extraLight;

	pSprite->face.pMaterial = _rd->_material.load(matFilename, 0, 0);

	if (!pSprite->face.pMaterial)
		return 0;

	pSprite->face.numVertices = 4;
	pSprite->face.aVertices = new int[4];

	if (pSprite->face.geometryMode <= 3) {
		for (uint i = 0; i < pSprite->face.numVertices; i++)
			pSprite->face.aVertices[i] = i;
	} else {
		pSprite->face.aTexVertices = new int[pSprite->face.numVertices];

		for (uint i = 0; i < pSprite->face.numVertices; i++) {
			pSprite->face.aVertices[i] = i;
			pSprite->face.aTexVertices[i] = i;
		}

		if (!pSprite->face.pMaterial->apCels[0] || !pSprite->face.pMaterial->apCels[0]->pTexture || !pSprite->face.pMaterial->apCels[0]->pTexture->apMips[0]) {
			warning("RenderDroid::SpriteModule::newEntry(): material %s has no texture", pSprite->face.pMaterial->aName);
			return 0;
		}

		pSprite->aTexVerts = new float[2 * pSprite->face.numVertices];

		StdVBuffer *pTexture = pSprite->face.pMaterial->apCels[0]->pTexture->apMips[0];
		double texHeight = (double)(uint)pTexture->tex.height;
		double texWidth = (double)(uint)pTexture->tex.width;

		pSprite->aTexVerts[0] = 0.5f;
		pSprite->aTexVerts[1] = (float)(texHeight - 0.5);
		pSprite->aTexVerts[2] = (float)(texWidth - 0.5);
		pSprite->aTexVerts[3] = (float)(texHeight - 0.5);
		pSprite->aTexVerts[4] = (float)(texWidth - 0.5);
		pSprite->aTexVerts[5] = 0.5f;
		pSprite->aTexVerts[6] = 0.5f;
		pSprite->aTexVerts[7] = 0.5f;
	}

	pSprite->halfWidth = (float)(pSprite->width * 0.5);
	pSprite->halfHeight = (float)(pSprite->height * 0.5);
	pSprite->face.uvDelta[0] = 0.0f;
	pSprite->face.uvDelta[1] = 0.0f;
	pSprite->radius = (float)sqrt((double)pSprite->halfWidth * pSprite->halfWidth + (double)pSprite->halfHeight * pSprite->halfHeight);

	return 1;
}

int RenderDroid::SpriteModule::draw(RdThing *pThing, RdMatrix34 mat) {
	RdSprite *pSprite = pThing->data.pSprite;
	RdCamera *pCamera = _rd->_camera.getCurrent();
	RdVector3 center;
	RdVector3 offset;

	_rd->transformPoint34(center, mat[3], pCamera->viewMatrix);
	_rd->transformVector34(offset, pSprite->offset, mat);
	_rd->transformVector34Acc(offset, pCamera->viewMatrix);

	float halfWidth;

	if (offset[0] >= 0.0)
		halfWidth = -pSprite->halfWidth;
	else
		halfWidth = pSprite->halfWidth;

	int frustrumCull;

	if (_rd->_curCullFlags & 2)
		frustrumCull = _rd->_clip.sphereInFrustrum(pCamera->pClipFrustum, center, pSprite->radius);
	else
		frustrumCull = pThing->frustrumCull;

	if (frustrumCull == 2)
		return 0;

	RdProcFace *pProcFace = _rd->_cache.getProcEntry();

	if (!pProcFace)
		return 0;

	float left = (float)((double)offset[0] - halfWidth);
	float right = (float)((double)halfWidth + offset[0]);
	float y = (float)((double)offset[1] + center[1]);
	float bottom = (float)((double)offset[2] + center[2]);
	float top = (float)(((double)center[2] + pSprite->halfHeight + pSprite->halfHeight) + offset[2]);
	float topSecond = (float)((double)center[2] + (((double)pSprite->halfHeight + pSprite->halfHeight) + offset[2]));

	_spriteVerts[0] = (float)((double)center[0] + left);
	_spriteVerts[1] = y;
	_spriteVerts[2] = bottom;
	_spriteVerts[3] = (float)((double)center[0] + right);
	_spriteVerts[4] = y;
	_spriteVerts[5] = bottom;
	_spriteVerts[6] = (float)((double)center[0] + right);
	_spriteVerts[7] = y;
	_spriteVerts[8] = top;
	_spriteVerts[9] = (float)((double)center[0] + left);
	_spriteVerts[10] = y;
	_spriteVerts[11] = topSecond;

	RdVertexLists src;
	RdVertexLists dst;

	memset(&src, 0, sizeof(RdVertexLists));
	memset(&dst, 0, sizeof(RdVertexLists));

	src.numVertices = pSprite->face.numVertices;
	src.aVertIndices = pSprite->face.aVertices;
	src.aTexIndices = pSprite->face.aTexVertices;
	src.aVertices = (RdVector3 *)_spriteVerts;
	src.aIntensities = nullptr;
	src.aTexVertices = (RdVector2 *)pSprite->aTexVerts;
	src.aAddedIntensities = nullptr;

	dst.aVertices = _spriteWork;
	dst.aScreen = pProcFace->aVertices;
	dst.aTexVertices = pProcFace->aTexVertices;
	dst.aIntensities = pProcFace->aIntensities;

	int geometryMode = _rd->_curGeometryMode;

	if (geometryMode >= pSprite->face.geometryMode)
		geometryMode = pSprite->face.geometryMode;

	if (geometryMode >= pThing->curGeometryMode)
		geometryMode = pThing->curGeometryMode;

	pProcFace->geometryMode = geometryMode;

	if ((_rd->_curRenderOptions & 2) && pCamera->ambientLight >= 1.0f) {
		pProcFace->lightingMode = 0;
	} else {
		int lightingMode = _rd->_curLightingMode;

		if (lightingMode >= pSprite->face.lightingMode)
			lightingMode = pSprite->face.lightingMode;

		if (lightingMode >= pThing->curLightingMode)
			lightingMode = pThing->curLightingMode;

		pProcFace->lightingMode = lightingMode;
	}

	int textureMode = _rd->_curTextureMode;

	if (textureMode >= pSprite->face.textureMode)
		textureMode = pSprite->face.textureMode;

	if (textureMode >= pThing->curTextureMode)
		textureMode = pThing->curTextureMode;

	pProcFace->textureMode = textureMode;

	_rd->_primit3.clipFace(pCamera->pClipFrustum, geometryMode, pProcFace->lightingMode, textureMode, &src, &dst, pSprite->face.uvDelta);

	if (dst.numVertices < 3)
		return 0;

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
		} else {
			pProcFace->lightingMode = pCurMap == pIdentityMap ? 0 : 2;
		}
	}

	char bufferFlags = 1;

	if (pProcFace->geometryMode >= 4)
		bufferFlags = 3;

	if (pProcFace->lightingMode >= 3)
		bufferFlags |= 4;

	pProcFace->curCel = pThing->curCel;
	pProcFace->bBackface = halfWidth < 0.0f;
	pProcFace->type = pSprite->face.type;
	pProcFace->extraLight = pSprite->face.extraLight;
	pProcFace->pMaterial = pSprite->face.pMaterial;
	_rd->_cache.addProcFace(0, dst.numVertices, bufferFlags);

	return 1;
}

void RenderDroid::SpriteModule::destroy(RdSprite *&pSprite) {
	if (pSprite) {
		freeEntry(pSprite);
		delete pSprite;
		pSprite = nullptr;
	}
}

void RenderDroid::SpriteModule::freeEntry(RdSprite *pSprite) {
	if (pSprite->aTexVerts) {
		delete[] pSprite->aTexVerts;
		pSprite->aTexVerts = nullptr;
	}

	if (pSprite->face.aVertices) {
		delete[] pSprite->face.aVertices;
		pSprite->face.aVertices = nullptr;
	}

	if (pSprite->face.aTexVertices) {
		delete[] pSprite->face.aTexVertices;
		pSprite->face.aTexVertices = nullptr;
	}
}

} // End of namespace Grim
