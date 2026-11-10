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

#include "common/array.h"
#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/csprite.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/rnode.h"

namespace Grim {

enum {
	kKeywordTextureName = 0,
	kKeywordDrawFlags = 1,
	kKeywordDimensions = 2,
	kKeywordOffset = 3,
	kKeywordColor = 4,
	kKeywordTexCoord = 5
};

static const int kTokenSize = 256;
static const int kBinaryHeaderSize = 40;
static const int kBinaryTailSize = 120;
static const float kCornerX[CSprite::kNumCorners] = { -1.0f, 1.0f, 1.0f, -1.0f };
static const float kCornerY[CSprite::kNumCorners] = { 1.0f, 1.0f, -1.0f, -1.0f };
static const float kAlphaScale = 255.0f;

class CSpriteParser : public LFormatParser {
public:
	CSpriteParser() {
		addKeyword("TextureName", kKeywordTextureName, 7);
		addKeyword("Drawflags", kKeywordDrawFlags, 1);
		addKeyword("Dimensions", kKeywordDimensions, 10);
		addKeyword("Offset", kKeywordOffset, 42);
		addKeyword("Color", kKeywordColor, 85);
		addKeyword("TxCoord", kKeywordTexCoord, 10);
	}
};

static bool readBytes(LFile *file, void *buffer, uint size) {
	return file->read(buffer, size) == size;
}

static bool readString(LFile *file, int fileSize, LString *result) {
	byte lengthBytes[4];

	if (!readBytes(file, lengthBytes, sizeof(lengthBytes)))
		return false;

	int length = READ_LE_INT32(lengthBytes);

	if (length < 0 || (fileSize > 0 && length > fileSize)) {
		warning("CSprite::loadBinary(): invalid string length %d", length);
		return false;
	}

	Common::Array<char> buffer(length + 1);

	if (length > 0 && !readBytes(file, &buffer[0], length))
		return false;

	buffer[length] = 0;
	*result = &buffer[0];

	return true;
}

static void writeInt(LFile *file, int value) {
	byte bytes[4];

	WRITE_LE_INT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeFloat(LFile *file, float value) {
	byte bytes[4];

	WRITE_LE_FLOAT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeString(LFile *file, const LString &text) {
	writeInt(file, text.size() + 1);
	file->write(text.c_str(), text.size() + 1);
}

CSprite::CSprite() {
	width = 1.0f;
	height = 1.0f;
	drawFlags = 0;

	for (int i = 0; i < kNumCorners; i++) {
		corners[i] = Color::kWhite;
		uv[i * 2] = (i == 0 || i == 3) ? 0.0f : 1.0f;
		uv[i * 2 + 1] = (i == 2 || i == 3) ? 0.0f : 1.0f;
	}
}

CSprite::~CSprite() {
	material.unlock();
	material.setResource(nullptr);
}

RPrimitiveInstance *CSprite::createInstance() {
	return new CSpriteInstance(this);
}

bool CSprite::restore(LFile *file) {
	CPrimitive::restore(file);

	CSpriteParser parser;
	char textureName[kTokenSize];
	int textureFlags = 0;

	parser.setFile(file);
	parser.readVersion(1, 0);

	if (!parser.readEntry(kKeywordTextureName, textureName, &textureFlags))
		return false;

	material.resource = gpResourceManager->findResource(textureName);

	if (!material.resource) {
		material.resource = gpResourceManager->createResource(textureName);

		if (!material.resource)
			material.allocateResource();

		if (material.resource) {
			material.resource->resSetName(textureName);
			gpResourceManager->addResource(material.resource);
		}
	}

	material.setResource(material.resource);

	if (!parser.readEntry(kKeywordDrawFlags, &drawFlags))
		return false;

	if (!parser.readEntry(kKeywordDimensions, &width, &height))
		return false;

	// The disasm reads the offset line with the id of the dimensions keyword
	if (!parser.readEntry(kKeywordDimensions, &offset.x, &offset.y, &offset.z))
		return false;

	for (int i = 0; i < kNumCorners; i++) {
		if (!parser.readEntry(kKeywordColor, &corners[i].r, &corners[i].g, &corners[i].b, &corners[i].a))
			return false;
	}

	for (int i = 0; i < kNumCorners; i++) {
		if (!parser.readEntry(kKeywordTexCoord, &uv[i * 2], &uv[i * 2 + 1]))
			return false;
	}

	parser.readEndOfSection();

	if (material.resource && material.lock() && material.typed)
		material.typed->addFlags(textureFlags);

	return true;
}

bool CSprite::loadBinary(const char *path) {
	LFile *file = gpFileServices->openAndLoad(path, "rb");

	if (!file)
		return false;

	int fileSize = gpFileServices->getSize(path);
	LString spriteName;
	LString materialName;
	byte header[kBinaryHeaderSize];
	byte flagBytes[4];
	byte tail[kBinaryTailSize];
	bool valid = readString(file, fileSize, &spriteName);

	valid = valid && readBytes(file, header, sizeof(header));
	valid = valid && readString(file, fileSize, &materialName);
	valid = valid && readBytes(file, flagBytes, sizeof(flagBytes));
	valid = valid && readBytes(file, tail, sizeof(tail));

	if (!valid) {
		warning("CSprite::loadBinary(): unexpected end of file");
		gpFileServices->close(file, true);

		return false;
	}

	name = spriteName;
	radius = READ_LE_FLOAT32(header);
	center.x = READ_LE_FLOAT32(header + 4);
	center.y = READ_LE_FLOAT32(header + 8);
	center.z = READ_LE_FLOAT32(header + 12);
	box.min.x = READ_LE_FLOAT32(header + 16);
	box.min.y = READ_LE_FLOAT32(header + 20);
	box.min.z = READ_LE_FLOAT32(header + 24);
	box.max.x = READ_LE_FLOAT32(header + 28);
	box.max.y = READ_LE_FLOAT32(header + 32);
	box.max.z = READ_LE_FLOAT32(header + 36);

	LResourceHandle<CMaterial> foundMaterial;

	foundMaterial.resource = gpResourceManager->findResource(materialName);

	if (!foundMaterial.resource) {
		foundMaterial.resource = gpResourceManager->createResource(materialName);

		if (!foundMaterial.resource)
			foundMaterial.allocateResource();

		if (foundMaterial.resource) {
			foundMaterial.resource->resSetName(materialName.c_str());
			gpResourceManager->addResource(foundMaterial.resource);
		}
	}

	foundMaterial.setResource(foundMaterial.resource);
	material.setResource(foundMaterial.resource);

	CMaterial *typedMaterial = nullptr;

	if (material.resource && material.lock())
		typedMaterial = material.typed;

	if (typedMaterial)
		typedMaterial->setFlags(READ_LE_INT32(flagBytes));
	else
		warning("CSprite::loadBinary(): cannot lock the material");

	width = READ_LE_FLOAT32(tail);
	height = READ_LE_FLOAT32(tail + 4);
	offset.x = READ_LE_FLOAT32(tail + 8);
	offset.y = READ_LE_FLOAT32(tail + 12);
	offset.z = READ_LE_FLOAT32(tail + 16);

	for (int i = 0; i < kNumCorners; i++) {
		corners[i].a = READ_LE_INT32(tail + 20 + i * 16);
		corners[i].r = READ_LE_INT32(tail + 24 + i * 16);
		corners[i].g = READ_LE_INT32(tail + 28 + i * 16);
		corners[i].b = READ_LE_INT32(tail + 32 + i * 16);
	}

	for (int i = 0; i < kNumCorners * 2; i++)
		uv[i] = READ_LE_FLOAT32(tail + 84 + i * 4);

	drawFlags = READ_LE_INT32(tail + 116);

	gpFileServices->close(file, true);

	return true;
}

bool CSprite::saveBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "wb");

	if (!file)
		return false;

	writeString(file, name);
	writeFloat(file, radius);
	writeFloat(file, center.x);
	writeFloat(file, center.y);
	writeFloat(file, center.z);
	writeFloat(file, box.min.x);
	writeFloat(file, box.min.y);
	writeFloat(file, box.min.z);
	writeFloat(file, box.max.x);
	writeFloat(file, box.max.y);
	writeFloat(file, box.max.z);

	LString materialName;

	if (material.resource)
		materialName = material.resource->name;
	else
		warning("CSprite::saveBinary(): the sprite has no material");

	writeString(file, materialName);

	CMaterial *typedMaterial = nullptr;

	if (material.resource && material.lock())
		typedMaterial = material.typed;

	writeInt(file, typedMaterial ? typedMaterial->flags : 0);
	material.unlock();

	writeFloat(file, width);
	writeFloat(file, height);
	writeFloat(file, offset.x);
	writeFloat(file, offset.y);
	writeFloat(file, offset.z);

	for (int i = 0; i < kNumCorners; i++) {
		writeInt(file, corners[i].a);
		writeInt(file, corners[i].r);
		writeInt(file, corners[i].g);
		writeInt(file, corners[i].b);
	}

	for (int i = 0; i < kNumCorners * 2; i++)
		writeFloat(file, uv[i]);

	writeInt(file, drawFlags);

	gpFileServices->close(file, true);

	return true;
}

CSpriteInstance::CSpriteInstance(CSprite *spritePrimitive) : RPrimitiveInstance(spritePrimitive) {
	textureIndex = 0;
}

CSpriteInstance::~CSpriteInstance() {
}

int CSpriteInstance::getSaveSize() {
	return RPrimitiveInstance::getSaveSize() + (int)sizeof(textureIndex);
}

bool CSpriteInstance::saveState(tSaveRestoreAccess *access) {
	RPrimitiveInstance::saveState(access);
	access(&textureIndex, sizeof(textureIndex));

	return true;
}

bool CSpriteInstance::restoreState(tSaveRestoreAccess *access) {
	RPrimitiveInstance::restoreState(access);
	access(&textureIndex, sizeof(textureIndex));

	return true;
}

void CSpriteInstance::output(RNode *node, Camera *camera) {
	CDrawContext *ctx = CDrawContext::context;

	if (!visible)
		return;

	if (!ctx) {
		warning("CSpriteInstance::output(): no draw context");
		return;
	}

	CSprite *sprite = static_cast<CSprite *>(prim);

	update(node, camera);
	ctx->resetCameraMatrix();

	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	const float *v = camera->view.rot.m;
	const CVector &nodePos = node->world.pos;
	const CVector &viewPos = camera->view.pos;

	double pendingZ = (double)v[8] * nodePos.z + (double)v[2] * nodePos.x + (double)v[5] * nodePos.y;
	float rotatedY = (float)((double)v[7] * nodePos.z + (double)v[1] * nodePos.x + (double)v[4] * nodePos.y);
	float rotatedX = (float)((double)v[6] * nodePos.z + (double)v[3] * nodePos.y + (double)v[0] * nodePos.x);
	float nodeViewZ = (float)(pendingZ + viewPos.z);
	float nodeViewY = (float)((double)rotatedY + viewPos.y);
	float nodeViewX = (float)((double)rotatedX + viewPos.x);

	if ((node->nodeFlags & CNode::kEulerValid) == 0)
		node->computeLocalMatrixFromWorld();

	CVector billboardEuler;

	billboardEuler.x = 0.0f;
	billboardEuler.y = 0.0f;
	billboardEuler.z = node->eulerDeg.y;

	CMatrix rotation;

	rotation.setFromEuler(billboardEuler);

	CDrawList *drawList = ctx->getDrawList(0);
	DrawPoly *poly = drawList->newPoly(4);

	poly->flags = sprite->drawFlags;

	if (sprite->material.resource) {
		CMaterial *spriteMaterial = nullptr;

		if (sprite->material.lock())
			spriteMaterial = sprite->material.typed;

		if (spriteMaterial) {
			poly->tex = spriteMaterial->getTexture(textureIndex);
			sprite->material.unlock();
		} else {
			warning("CSpriteInstance::output(): cannot lock the material");
			poly->tex = nullptr;
		}
	} else {
		poly->tex = nullptr;
	}

	const float *m = rotation.m;

	for (int i = 0; i < CSprite::kNumCorners; i++) {
		double halfWidth = (double)kCornerX[i] * sprite->width * 0.5;
		float halfHeight = (float)((double)kCornerY[i] * sprite->height * 0.5);
		double localX = halfWidth + sprite->offset.x;
		double localY = (double)halfHeight + sprite->offset.y;
		double localZ = sprite->offset.z;
		float spriteX = (float)((double)m[0] * localX + (double)m[6] * localZ + (double)m[3] * localY);
		float spriteY = (float)((double)m[1] * localX + (double)m[7] * localZ + (double)m[4] * localY);
		double pendingSpriteZ = (double)m[2] * localX + (double)m[8] * localZ + (double)m[5] * localY;
		DrawVertex *vertex = poly->verts[i];

		vertex->x = (float)((double)spriteX + nodeViewX);
		vertex->y = (float)((double)spriteY + nodeViewY);
		vertex->z = (float)(pendingSpriteZ + nodeViewZ);
		vertex->u = sprite->uv[i * 2];
		vertex->v = sprite->uv[i * 2 + 1];
		vertex->color = ((uint32)(sprite->corners[i].a & 0xFF) << 24) | ((uint32)(sprite->corners[i].r & 0xFF) << 16) |
				((uint32)(sprite->corners[i].g & 0xFF) << 8) | (uint32)(sprite->corners[i].b & 0xFF);
	}

	if (globalAlphaMode == kAlphaSet) {
		for (int i = 0; i < CSprite::kNumCorners; i++) {
			uint32 alpha = (uint32)(int)((double)globalAlpha * kAlphaScale) & 0xFF;

			poly->verts[i]->color = (poly->verts[i]->color & 0x00FFFFFF) | (alpha << 24);
		}
	} else if (globalAlphaMode == kAlphaModulate) {
		for (int i = 0; i < CSprite::kNumCorners; i++) {
			int scale = (int)((double)globalAlpha * kAlphaScale);
			int vertexAlpha = poly->verts[i]->color >> 24;
			int product = (int)((uint32)scale * (uint32)vertexAlpha);
			uint32 alpha = (uint32)(product / 255) & 0xFF;

			poly->verts[i]->color = (poly->verts[i]->color & 0x00FFFFFF) | (alpha << 24);
		}
	}

	if (matAlphaMode == kAlphaSet || matAlphaMode == kAlphaModulate) {
		for (Common::HashMap<int, float>::const_iterator it = matAlpha.begin(); it != matAlpha.end(); ++it) {
			if (it->_key < 0 || it->_key >= poly->numVerts) {
				warning("CSpriteInstance::output(): invalid vertex index %d", it->_key);
				continue;
			}

			DrawVertex *vertex = poly->verts[it->_key];
			int scale = (int)((double)it->_value * kAlphaScale);
			uint32 alpha;

			if (matAlphaMode == kAlphaSet) {
				alpha = (uint32)scale & 0xFF;
			} else {
				int vertexAlpha = vertex->color >> 24;
				int product = (int)((uint32)scale * (uint32)vertexAlpha);

				alpha = (uint32)(product / 255) & 0xFF;
			}

			vertex->color = (vertex->color & 0x00FFFFFF) | (alpha << 24);
		}
	}

	drawList->addPoly(poly);
	ctx->flushDrawLists();
}

} // End of namespace Grim
