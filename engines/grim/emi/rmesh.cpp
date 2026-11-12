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

#include <math.h>

#include "common/endian.h"
#include "common/list.h"
#include "common/textconsole.h"

#include "engines/grim/emi/clightnode.h"
#include "engines/grim/emi/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/rmeshparser.h"

namespace Grim {

static const int kBoundsSize = 40;
static const int kDefaultDrawFlags = 772;
static const int kDefaultAmbient = 128;
static const double kHalf = 0.5;
static const char kSourceExtension[] = "PCX";
static const CVector kZeroVector;
static const float kCubeDirections[RMesh::kNumCubeFaces][3] = {
	{ 1.0f, 0.0f, 0.0f },
	{ -1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f },
	{ 0.0f, -1.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, -1.0f }
};

static bool readBytes(LFile *file, void *buffer, uint size) {
	return file->read(buffer, size) == size;
}

static bool readInt(LFile *file, int *value) {
	byte bytes[4];

	if (!readBytes(file, bytes, sizeof(bytes)))
		return false;

	*value = READ_LE_INT32(bytes);

	return true;
}

static bool readString(LFile *file, int fileSize, LString *result) {
	int length;

	if (!readInt(file, &length))
		return false;

	if (length < 0 || (fileSize > 0 && length > fileSize)) {
		warning("RMesh::loadBinary(): invalid string length %d", length);

		return false;
	}

	Common::Array<char> buffer(length + 1);

	if (length > 0 && !readBytes(file, &buffer[0], length))
		return false;

	buffer[length] = 0;
	*result = &buffer[0];

	return true;
}

static bool readFloats(LFile *file, float *dest, int count) {
	if (count <= 0)
		return true;

	Common::Array<byte> bytes(count * sizeof(float));

	if (!readBytes(file, &bytes[0], bytes.size()))
		return false;

	for (int i = 0; i < count; i++)
		dest[i] = READ_LE_FLOAT32(&bytes[i * sizeof(float)]);

	return true;
}

static bool readInts(LFile *file, int *dest, int count) {
	if (count <= 0)
		return true;

	Common::Array<byte> bytes(count * sizeof(int));

	if (!readBytes(file, &bytes[0], bytes.size()))
		return false;

	for (int i = 0; i < count; i++)
		dest[i] = READ_LE_INT32(&bytes[i * sizeof(int)]);

	return true;
}

static bool isCountValid(int count, int elementSize, int fileSize) {
	return count >= 0 && (fileSize <= 0 || (int64)count * elementSize <= fileSize);
}

static void writeBytes(LFile *file, const void *buffer, uint size) {
	file->write(buffer, size);
}

static void writeInt(LFile *file, int value) {
	byte bytes[4];

	WRITE_LE_INT32(bytes, value);
	writeBytes(file, bytes, sizeof(bytes));
}

static void writeFloat(LFile *file, float value) {
	byte bytes[4];

	WRITE_LE_FLOAT32(bytes, value);
	writeBytes(file, bytes, sizeof(bytes));
}

static void writeString(LFile *file, const LString &text) {
	writeInt(file, text.size() + 1);
	writeBytes(file, text.c_str(), text.size() + 1);
}

RMesh::RMesh(EmiEngine *engine) : _engine(engine) {
	vertFlags = 0;
	numTextureSets = 0;
	numVerts = 0;
	pos = nullptr;
	normals = nullptr;
	vertIndex = nullptr;
	colors = nullptr;
	uv = nullptr;
	weightMap = nullptr;
}

RMesh::~RMesh() {
	freeVertexData();

	for (uint i = 0; i < matHandles.size(); i++)
		matHandles[i].unlock();

	matHandles.clear();
	materials.clear();
	texFlags.clear();

	for (uint i = 0; i < trisets.size(); i++) {
		delete trisets[i];
		trisets[i] = nullptr;
	}

	trisets.clear();

	delete weightMap;
	weightMap = nullptr;
}

EmiEngine *RMesh::getEngine() const {
	return _engine;
}

RPrimitiveInstance *RMesh::createInstance() {
	return new RMeshInstance(this);
}

void RMesh::allocVertexData(int flags, int count) {
	if (count < 0) {
		warning("RMesh::allocVertexData(): invalid vertex count %d", count);
		count = 0;
	}

	vertFlags = flags;
	numVerts = count;

	pos = (float *)_engine->getMemoryServices()->mallocImpl(count * 3 * sizeof(float));

	if (pos)
		memset(pos, 0, count * 3 * sizeof(float));

	if (flags & 1) {
		normals = (float *)_engine->getMemoryServices()->mallocImpl(numVerts * 3 * sizeof(float));

		if (normals)
			memset(normals, 0, numVerts * 3 * sizeof(float));
	}

	if (flags & 2) {
		colors = (PackedColor *)_engine->getMemoryServices()->mallocImpl(numVerts * sizeof(PackedColor));

		if (colors)
			memset(colors, 0, numVerts * sizeof(PackedColor));
	}

	if (flags & 4)
		vertIndex = (int *)_engine->getMemoryServices()->mallocImpl(numVerts * sizeof(int));

	if (flags & 0x10) {
		uv = (float *)_engine->getMemoryServices()->mallocImpl(numVerts * 2 * sizeof(float));

		if (uv)
			memset(uv, 0, numVerts * 2 * sizeof(float));
	}
}

void RMesh::freeVertexData() {
	numVerts = 0;

	if (pos) {
		_engine->getMemoryServices()->freeImpl(pos);
		pos = nullptr;
	}

	if (normals) {
		_engine->getMemoryServices()->freeImpl(normals);
		normals = nullptr;
	}

	if (vertIndex) {
		_engine->getMemoryServices()->freeImpl(vertIndex);
		vertIndex = nullptr;
	}

	if (colors) {
		_engine->getMemoryServices()->freeImpl(colors);
		colors = nullptr;
	}

	if (uv) {
		_engine->getMemoryServices()->freeImpl(uv);
		uv = nullptr;
	}
}

void RMesh::lightVertices(const CTransform *transform, int count, CDCVertex *vertices, const float *vertexNormals, int maxLights) {
	_engine->_rmeshLightingTimer.start();

	if (maxLights > kMaxLights) {
		warning("RMesh::lightVertices(): too many lights requested (%d)", maxLights);
		maxLights = kMaxLights;
	}

	int numLights = 0;
	float lightDistances[kMaxLights] = { 0.0f };
	CLight *lightList[kMaxLights];
	float lightTransforms[kMaxLights][12];

	CLight *ambientLight = nullptr;
	const Common::List<CLightNode *> &activeLights = _engine->getActiveLights();

	for (Common::List<CLightNode *>::const_iterator it = activeLights.begin(); it != activeLights.end(); ++it) {
		CLightNode *lightNode = *it;

		if (!lightNode)
			continue;

		CLight *light = lightNode->light;

		if (!light) {
			warning("RMesh::lightVertices(): light node without a light");
			continue;
		}

		if (light->type == CLight::kTypeAmbient) {
			ambientLight = light;
			continue;
		}

		if ((lightNode->nodeFlags & CNode::kWorldValid) == 0)
			lightNode->updateWorldTransform();

		const float *r = transform->rot.m;
		double deltaX = (double)lightNode->world.pos.x - transform->pos.x;
		double deltaY = (double)lightNode->world.pos.y - transform->pos.y;
		double deltaZ = (double)lightNode->world.pos.z - transform->pos.z;
		float localX = (float)(deltaY * r[1] + deltaZ * r[2] + deltaX * r[0]);
		float localY = (float)(deltaY * r[4] + deltaZ * r[5] + deltaX * r[3]);
		float localZ = (float)(deltaY * r[7] + deltaZ * r[8] + deltaX * r[6]);
		CMatrix transposed;
		CMatrix product;
		float lightTransform[12];

		transform->rot.transpose(&transposed);
		lightNode->world.rot.multiply(&product, &transposed);

		for (int i = 0; i < 9; i++)
			lightTransform[i] = product.m[i];

		lightTransform[9] = localX;
		lightTransform[10] = localY;
		lightTransform[11] = localZ;

		if (light->isOutOfRange(lightTransform, &box))
			continue;

		double sumX = (double)box.max.x + box.min.x;
		float sumY = (float)((double)box.max.y + box.min.y);
		float sumZ = (float)((double)box.max.z + box.min.z);
		double centerX = sumX * kHalf;
		float centerY = (float)((double)sumY * kHalf);
		float centerZ = (float)((double)sumZ * kHalf);
		double distanceX = centerX - lightTransform[9];
		double distanceY = (double)centerY - lightTransform[10];
		double distanceZ = (double)centerZ - lightTransform[11];
		double distance = distanceZ * distanceZ + distanceY * distanceY + distanceX * distanceX;
		int slot = numLights;

		if (numLights == maxLights) {
			slot = 0;

			for (int i = 1; i < numLights; i++) {
				if (lightDistances[i] > lightDistances[slot])
					slot = i;
			}

			if (!(distance < lightDistances[slot]))
				continue;
		} else {
			numLights++;
		}

		lightDistances[slot] = (float)distance;
		lightList[slot] = light;

		for (int i = 0; i < 12; i++)
			lightTransforms[slot][i] = lightTransform[i];
	}

	float actorTransform[12];

	for (int i = 0; i < 9; i++)
		actorTransform[i] = transform->rot.m[i];

	actorTransform[9] = transform->pos.x;
	actorTransform[10] = transform->pos.y;
	actorTransform[11] = transform->pos.z;

	Color ambient;

	ambient.setRGB(0, 0, 0);

	if (ambientLight)
		ambientLight->addLighting(&ambient, actorTransform, &kZeroVector.x, &kZeroVector.x);
	else
		ambient.setRGB(kDefaultAmbient, kDefaultAmbient, kDefaultAmbient);

	Color color;
	PackedColor lit;

	lit.b = 0;
	lit.g = 0;
	lit.r = 0;
	lit.a = 0;

	for (int i = 0; i < count; i++) {
		color = ambient;

		for (int j = 0; j < numLights; j++)
			lightList[j]->addLighting(&color, lightTransforms[j], &vertices[i].x, vertexNormals + i * 3);

		color.clamp();

		lit.r = 255;

		if ((uint)color.r <= 0xFF)
			lit.r = color.r;

		lit.g = 255;

		if ((uint)color.g <= 0xFF)
			lit.g = color.g;

		lit.b = 255;

		if ((uint)color.b <= 0xFF)
			lit.b = color.b;

		lit.a = 255;

		if (colors) {
			PackedColor modulated;

			colors[i].modulate(&modulated, &lit);
			vertices[i].color = modulated.pack();
		} else {
			vertices[i].color = lit.pack();
		}
	}

	_engine->_rmeshLightingTimer.stop();
}

void RMesh::lightVerticesAmbientCube(const CTransform *transform, int count, CDCVertex *vertices, const float *vertexNormals, int maxLights) {
	_engine->_rmeshLightingTimer.start();

	if (maxLights > kMaxLights) {
		warning("RMesh::lightVerticesAmbientCube(): too many lights requested (%d)", maxLights);
		maxLights = kMaxLights;
	}

	int numLights = 0;
	float lightDistances[kMaxLights] = { 0.0f };
	CLight *lightList[kMaxLights];
	float lightTransforms[kMaxLights][12];

	CLight *ambientLight = nullptr;
	const Common::List<CLightNode *> &activeLights = _engine->getActiveLights();

	for (Common::List<CLightNode *>::const_iterator it = activeLights.begin(); it != activeLights.end(); ++it) {
		CLightNode *lightNode = *it;

		if (!lightNode)
			continue;

		CLight *light = lightNode->light;

		if (!light) {
			warning("RMesh::lightVerticesAmbientCube(): light node without a light");
			continue;
		}

		if (light->type == CLight::kTypeAmbient) {
			ambientLight = light;
			continue;
		}

		if ((lightNode->nodeFlags & CNode::kWorldValid) == 0)
			lightNode->updateWorldTransform();

		const float *r = transform->rot.m;
		double deltaX = (double)lightNode->world.pos.x - transform->pos.x;
		double deltaY = (double)lightNode->world.pos.y - transform->pos.y;
		double deltaZ = (double)lightNode->world.pos.z - transform->pos.z;
		float localX = (float)(deltaY * r[1] + deltaZ * r[2] + deltaX * r[0]);
		float localY = (float)(deltaY * r[4] + deltaZ * r[5] + deltaX * r[3]);
		float localZ = (float)(deltaY * r[7] + deltaZ * r[8] + deltaX * r[6]);
		CMatrix transposed;
		CMatrix product;
		float lightTransform[12];

		transform->rot.transpose(&transposed);
		lightNode->world.rot.multiply(&product, &transposed);

		for (int i = 0; i < 9; i++)
			lightTransform[i] = product.m[i];

		lightTransform[9] = localX;
		lightTransform[10] = localY;
		lightTransform[11] = localZ;

		if (light->isOutOfRange(lightTransform, &box))
			continue;

		double sumX = (double)box.min.x + box.max.x;
		float sumY = (float)((double)box.max.y + box.min.y);
		float sumZ = (float)((double)box.max.z + box.min.z);
		double centerX = sumX * kHalf;
		float centerY = (float)((double)sumY * kHalf);
		float centerZ = (float)((double)sumZ * kHalf);
		double distanceX = centerX - lightTransform[9];
		double distanceY = (double)centerY - lightTransform[10];
		double distanceZ = (double)centerZ - lightTransform[11];
		double distance = distanceX * distanceX + distanceZ * distanceZ + distanceY * distanceY;
		int slot = numLights;

		if (numLights == maxLights) {
			slot = 0;

			for (int i = 1; i < numLights; i++) {
				if (lightDistances[i] > lightDistances[slot])
					slot = i;
			}

			if (!(distance < lightDistances[slot]))
				continue;
		} else {
			numLights++;
		}

		lightDistances[slot] = (float)distance;
		lightList[slot] = light;

		for (int i = 0; i < 12; i++)
			lightTransforms[slot][i] = lightTransform[i];
	}

	float actorTransform[12];

	for (int i = 0; i < 9; i++)
		actorTransform[i] = transform->rot.m[i];

	actorTransform[9] = transform->pos.x;
	actorTransform[10] = transform->pos.y;
	actorTransform[11] = transform->pos.z;

	Color ambient;

	ambient.setRGB(0, 0, 0);

	if (ambientLight)
		ambientLight->addLighting(&ambient, actorTransform, &kZeroVector.x, &kZeroVector.x);
	else
		ambient.setRGB(kDefaultAmbient, kDefaultAmbient, kDefaultAmbient);

	Color cube[kNumCubeFaces];

	for (int face = 0; face < kNumCubeFaces; face++) {
		cube[face] = ambient;

		for (int j = 0; j < numLights; j++) {
			float factor = lightList[j]->evaluate(lightTransforms[j], &center.x, kCubeDirections[face]);

			if (factor > 0.0f) {
				const Color &lightColor = lightList[j]->color;

				// The disasm rounds the products to the nearest integer here
				float addedR = (float)((double)(uint)lightColor.r * factor);
				cube[face].r += (int)lrint(addedR);

				float addedG = (float)((double)(uint)lightColor.g * factor);
				cube[face].g += (int)lrint(addedG);

				float addedB = (float)((double)(uint)lightColor.b * factor);
				cube[face].b += (int)lrint(addedB);
			}
		}

		cube[face].clamp();
	}

	PackedColor lit;

	lit.b = 0;
	lit.g = 0;
	lit.r = 0;
	lit.a = 0;

	for (int i = 0; i < count; i++) {
		float normalX = vertexNormals[i * 3];
		float normalY = vertexNormals[i * 3 + 1];
		float normalZ = vertexNormals[i * 3 + 2];
		float weightX = (float)((double)normalX * normalX);
		float weightY = (float)((double)normalY * normalY);
		float weightZ = (float)((double)normalZ * normalZ);
		uint32 bitsX;
		uint32 bitsY;
		uint32 bitsZ;

		memcpy(&bitsX, &normalX, sizeof(bitsX));
		memcpy(&bitsY, &normalY, sizeof(bitsY));
		memcpy(&bitsZ, &normalZ, sizeof(bitsZ));

		Color sum;
		const Color &faceX = cube[bitsX >> 31];
		const Color &faceY = cube[2 + (bitsY >> 31)];
		const Color &faceZ = cube[4 + (bitsZ >> 31)];

		sum.r += (int)lrint((float)((double)(uint)faceX.r * weightX));
		sum.g += (int)lrint((float)((double)(uint)faceX.g * weightX));
		sum.b += (int)lrint((float)((double)(uint)faceX.b * weightX));
		sum.r += (int)lrint((float)((double)(uint)faceY.r * weightY));
		sum.g += (int)lrint((float)((double)(uint)faceY.g * weightY));
		sum.b += (int)lrint((float)((double)(uint)faceY.b * weightY));
		sum.r += (int)lrint((float)((double)(uint)faceZ.r * weightZ));
		sum.g += (int)lrint((float)((double)(uint)faceZ.g * weightZ));
		sum.b += (int)lrint((float)((double)(uint)faceZ.b * weightZ));

		lit.r = 255;

		if ((uint)sum.r <= 0xFF)
			lit.r = sum.r;

		lit.g = 255;

		if ((uint)sum.g <= 0xFF)
			lit.g = sum.g;

		lit.b = 255;

		if ((uint)sum.b <= 0xFF)
			lit.b = sum.b;

		lit.a = 255;

		if (colors) {
			PackedColor modulated;

			colors[i].modulate(&modulated, &lit);
			vertices[i].color = modulated.pack();
		} else {
			vertices[i].color = lit.pack();
		}
	}

	_engine->_rmeshLightingTimer.stop();
}

void RMesh::computeBounds() {
	if (numVerts) {
		box.init(pos);

		for (int i = 1; i < numVerts; i++)
			box.expand(&pos[i * 3]);

		double sumX = (double)box.max.x + box.min.x;
		float sumY = (float)((double)box.min.y + box.max.y);
		float sumZ = (float)((double)box.min.z + box.max.z);

		center.y = (float)((double)sumY * kHalf);
		center.x = (float)(sumX * kHalf);
		center.z = (float)((double)sumZ * kHalf);

		double sizeZ = (double)box.max.z - box.min.z;
		double sizeY = (double)box.max.y - box.min.y;
		double sizeX = (double)box.max.x - box.min.x;

		radius = (float)(sqrt(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ) * kHalf);
	}
}

bool RMesh::save(LFile *file) {
	CPrimitive::save(file);

	RMeshParser parser(_engine);

	parser.setFile(file);
	parser.writeVersion(2, 1);
	parser.writeComment("RMesh primitive");
	parser.writeEntry(RMeshParser::kKeywordBasePrim);
	parser.writeComment("Texture sets");
	parser.writeEntry(RMeshParser::kKeywordTotTxSets, numTextureSets);

	if (numTextureSets) {
		parser.writeVersion(2, 0);
		parser.writeComment("Material Set");
		parser.writeEntry(RMeshParser::kKeywordSetType, 0);
		parser.writeEntry(RMeshParser::kKeywordNumTextures, (int)matHandles.size());

		for (uint i = 0; i < matHandles.size(); i++) {
			LString materialName;

			if (matHandles[i].resource)
				materialName = matHandles[i].resource->name;
			else
				warning("RMesh::save(): material %u has no resource", i);

			if (!materialName.contains(".sur") && !materialName.contains(".SUR")) {
				uint dot = materialName.find('.');

				if (dot != LString::npos) {
					materialName.erase(dot + 1);
					materialName += kSourceExtension;
				}
			}

			int flags = 0;

			if (i < texFlags.size())
				flags = texFlags[i];
			else
				warning("RMesh::save(): material %u has no texture flags", i);

			parser.writeEntry(RMeshParser::kKeywordTextureName, materialName.c_str(), flags);
		}

		parser.writeEndOfSection();
	}

	parser.writeComment("Faces");
	parser.writeEntry(RMeshParser::kKeywordNumFaces, 0);
	parser.writeEntry(RMeshParser::kKeywordNumFaceNormals, 0);
	parser.writeComment("Vertex arrays");
	parser.writeEntry(RMeshParser::kKeywordVertFlags, vertFlags);
	parser.writeEntry(RMeshParser::kKeywordNumVerts, numVerts);
	parser.writeComment("Vert x y z");

	for (int i = 0; i < numVerts; i++)
		parser.writeEntry(RMeshParser::kKeywordVector, pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2]);

	if (vertFlags & 1) {
		parser.writeComment("Normal x y z");

		for (int i = 0; i < numVerts; i++)
			parser.writeEntry(RMeshParser::kKeywordNormal, normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
	}

	if (vertFlags & 2) {
		parser.writeComment("Color r g b");

		for (int i = 0; i < numVerts; i++)
			parser.writeEntry(RMeshParser::kKeywordColor, colors[i].r, colors[i].g, colors[i].b, colors[i].a);
	}

	if (vertFlags & 4) {
		parser.writeComment("VertIndex");

		for (int i = 0; i < numVerts; i++)
			parser.writeEntry(RMeshParser::kKeywordVertIndex, vertIndex[i]);
	}

	if (vertFlags & 0x10) {
		parser.writeComment("TextCoord u v");

		for (int i = 0; i < numVerts; i++)
			parser.writeEntry(RMeshParser::kKeywordTexCoord, uv[i * 2], uv[i * 2 + 1]);
	}

	parser.writeComment("Triangle sets");
	parser.writeEntry(RMeshParser::kKeywordNumTriSets, (int)trisets.size());

	for (uint i = 0; i < trisets.size(); i++) {
		TriSet *triset = trisets[i];
		int numIndices = (int)triset->indices.size();

		parser.writeEntry(RMeshParser::kKeywordTsDrawFlags, triset->drawFlags);
		parser.writeEntry(RMeshParser::kKeywordTsMaterialIdx, triset->materialIdx);
		parser.writeEntry(RMeshParser::kKeywordTsNumTris, numIndices / 3);

		for (int j = 0; j + 2 < numIndices; j += 3)
			parser.writeEntry(RMeshParser::kKeywordTsTriangle, triset->indices[j], triset->indices[j + 1], triset->indices[j + 2]);
	}

	if (weightMap) {
		parser.writeEntry(RMeshParser::kKeywordDeformPrim);
		weightMap->save(file);
	}

	parser.writeEndOfSection();

	return true;
}

bool RMesh::restore(LFile *file) {
	CPrimitive::restore(file);

	RMeshParser parser(_engine);

	parser.setFile(file);
	parser.mesh = this;

	bool result = parser.parseRemaining(2, 1);

	if (result && weightMap && vertIndex) {
		weightMap->remapVertexLists(numVerts, vertIndex);
		weightMap->flattenWeights();
	}

	if (vertIndex) {
		_engine->getMemoryServices()->freeImpl(vertIndex);
		vertIndex = nullptr;
		vertFlags &= ~4;
	}

	return result;
}

bool RMesh::loadBinary(const char *path) {
	LFile *file = _engine->getFileServices()->openAndLoad(path, "rb");

	if (!file)
		return false;

	int fileSize = _engine->getFileServices()->getSize(path);
	byte bounds[kBoundsSize];
	bool valid = readString(file, fileSize, &name);

	valid = valid && readBytes(file, bounds, sizeof(bounds));

	if (valid) {
		radius = READ_LE_FLOAT32(bounds);
		center.x = READ_LE_FLOAT32(bounds + 4);
		center.y = READ_LE_FLOAT32(bounds + 8);
		center.z = READ_LE_FLOAT32(bounds + 12);
		box.min.x = READ_LE_FLOAT32(bounds + 16);
		box.min.y = READ_LE_FLOAT32(bounds + 20);
		box.min.z = READ_LE_FLOAT32(bounds + 24);
		box.max.x = READ_LE_FLOAT32(bounds + 28);
		box.max.y = READ_LE_FLOAT32(bounds + 32);
		box.max.z = READ_LE_FLOAT32(bounds + 36);
	}

	valid = valid && readInt(file, &numTextureSets);

	if (valid && numTextureSets > 0) {
		int setType = 0;
		int numTextures = 0;

		valid = readInt(file, &setType) && readInt(file, &numTextures);

		if (valid && !isCountValid(numTextures, 8, fileSize)) {
			warning("RMesh::loadBinary(): invalid texture count %d", numTextures);
			valid = false;
		}

		if (valid) {
			matHandles.resize(numTextures, LResourceHandle<CMaterial>(_engine));
			texFlags.resize(numTextures, 0);
			materials.resize(numTextures, nullptr);
		}

		for (int i = 0; valid && i < numTextures; i++) {
			LString materialName;

			valid = readString(file, fileSize, &materialName) && readInt(file, &texFlags[i]);

			if (!valid)
				break;

			LResourceHandle<CMaterial> foundMaterial(_engine);

			foundMaterial.resource = _engine->getResourceManager()->findResource(materialName);

			if (!foundMaterial.resource) {
				foundMaterial.resource = _engine->getResourceManager()->createResource(materialName);

				if (!foundMaterial.resource)
					foundMaterial.allocateResource();

				if (foundMaterial.resource) {
					foundMaterial.resource->resSetName(materialName.c_str());
					_engine->getResourceManager()->addResource(foundMaterial.resource);
				}
			}

			foundMaterial.setResource(foundMaterial.resource);
			matHandles[i].setResource(foundMaterial.resource);

			if (matHandles[i].lock() && matHandles[i].typed) {
				materials[i] = matHandles[i].typed;
				materials[i]->addFlags(texFlags[i]);
			} else {
				materials[i] = nullptr;
			}
		}
	}

	int vertexCount = 0;

	valid = valid && readInt(file, &vertFlags) && readInt(file, &vertexCount);

	if (valid && !isCountValid(vertexCount, 12, fileSize)) {
		warning("RMesh::loadBinary(): invalid vertex count %d", vertexCount);
		valid = false;
	}

	if (valid) {
		allocVertexData(vertFlags, vertexCount);
		valid = readFloats(file, pos, 3 * numVerts);
	}

	if (valid && (vertFlags & 1))
		valid = readFloats(file, normals, 3 * numVerts);

	if (valid && (vertFlags & 2) && numVerts > 0) {
		Common::Array<byte> rgba(numVerts * sizeof(PackedColor));

		valid = readBytes(file, &rgba[0], rgba.size());

		for (int i = 0; valid && i < numVerts; i++) {
			colors[i].r = rgba[i * 4];
			colors[i].g = rgba[i * 4 + 1];
			colors[i].b = rgba[i * 4 + 2];
			colors[i].a = rgba[i * 4 + 3];
		}
	}

	if (valid && (vertFlags & 4))
		valid = readInts(file, vertIndex, numVerts);

	if (valid && (vertFlags & 0x10))
		valid = readFloats(file, uv, 2 * numVerts);

	int numTriSets = 0;
	bool isPS2 = _engine->getGamePlatform() == Common::kPlatformPS2;
	int indexSize = isPS2 ? (int)sizeof(uint32) : (int)sizeof(uint16);

	valid = valid && readInt(file, &numTriSets);

	if (valid && !isCountValid(numTriSets, 12, fileSize)) {
		warning("RMesh::loadBinary(): invalid triangle set count %d", numTriSets);
		valid = false;
	}

	if (valid)
		trisets.resize(numTriSets, nullptr);

	for (int i = 0; valid && i < numTriSets; i++) {
		TriSet *triset = new TriSet();
		int numIndices = 0;

		trisets[i] = triset;
		valid = readInt(file, &triset->drawFlags);

		if (valid && !triset->drawFlags)
			triset->drawFlags = kDefaultDrawFlags;

		valid = valid && readInt(file, &triset->numMaterials);

		if (valid && triset->numMaterials > 0)
			valid = readInt(file, &triset->materialIdx);

		valid = valid && readInt(file, &numIndices);

		if (valid && !isCountValid(numIndices, indexSize, fileSize)) {
			warning("RMesh::loadBinary(): invalid index count %d", numIndices);
			valid = false;
		}

		if (valid && numIndices > 0) {
			Common::Array<byte> bytes(numIndices * indexSize);

			valid = readBytes(file, &bytes[0], bytes.size());

			if (valid) {
				triset->indices.resize(numIndices);

				for (int j = 0; j < numIndices; j++) {
					if (isPS2) {
						uint32 index = READ_LE_UINT32(&bytes[j * indexSize]);

						if (index > 0xFFFF)
							warning("RMesh::loadBinary(): index %u does not fit in 16 bits in %s", index, path);

						triset->indices[j] = (uint16)index;
					} else {
						triset->indices[j] = READ_LE_UINT16(&bytes[j * indexSize]);
					}
				}
			}
		}
	}

	int hasWeightMap = 0;

	valid = valid && readInt(file, &hasWeightMap);

	if (valid && hasWeightMap) {
		int numNodes = 0;
		int numWeights = 0;

		weightMap = new CWeightMap(_engine);
		valid = readInt(file, &numNodes);

		if (valid && !isCountValid(numNodes, 4, fileSize)) {
			warning("RMesh::loadBinary(): invalid node count %d", numNodes);
			valid = false;
		}

		if (valid)
			weightMap->nodeNames.resize(numNodes);

		for (int i = 0; valid && i < numNodes; i++)
			valid = readString(file, fileSize, &weightMap->nodeNames[i]);

		valid = valid && readInt(file, &numWeights);

		if (valid && !isCountValid(numWeights, 12, fileSize)) {
			warning("RMesh::loadBinary(): invalid weight count %d", numWeights);
			valid = false;
		}

		if (valid && numWeights > 0) {
			Common::Array<byte> bytes(numWeights * 12);

			valid = readBytes(file, &bytes[0], bytes.size());

			if (valid) {
				weightMap->weights.resize(numWeights);

				for (int i = 0; i < numWeights; i++) {
					weightMap->weights[i].first = (byte)READ_LE_UINT32(&bytes[i * 12]);
					weightMap->weights[i].node = READ_LE_INT32(&bytes[i * 12 + 4]);
					weightMap->weights[i].weight = READ_LE_FLOAT32(&bytes[i * 12 + 8]);
				}
			}
		}
	}

	if (!valid)
		warning("RMesh::loadBinary(): unexpected end of file in %s", path);

	_engine->getFileServices()->close(file, true);

	return valid;
}

bool RMesh::saveBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "wb");

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
	writeInt(file, numTextureSets);

	if (numTextureSets > 0) {
		// The disasm writes an uninitialised stack value here
		writeInt(file, 0);
		writeInt(file, (int)matHandles.size());

		for (uint i = 0; i < matHandles.size(); i++) {
			LString materialName;

			if (matHandles[i].resource)
				materialName = matHandles[i].resource->name;
			else
				warning("RMesh::saveBinary(): material %u has no resource", i);

			writeString(file, materialName);

			if (i < texFlags.size()) {
				writeInt(file, texFlags[i]);
			} else {
				warning("RMesh::saveBinary(): material %u has no texture flags", i);
				writeInt(file, 0);
			}
		}
	}

	writeInt(file, vertFlags);
	writeInt(file, numVerts);

	for (int i = 0; i < 3 * numVerts; i++)
		writeFloat(file, pos[i]);

	if (vertFlags & 1) {
		for (int i = 0; i < 3 * numVerts; i++)
			writeFloat(file, normals[i]);
	}

	if (vertFlags & 2) {
		Common::Array<byte> rgba(numVerts * sizeof(PackedColor));

		for (int i = 0; i < numVerts; i++) {
			rgba[i * 4] = colors[i].r;
			rgba[i * 4 + 1] = colors[i].g;
			rgba[i * 4 + 2] = colors[i].b;
			rgba[i * 4 + 3] = colors[i].a;
		}

		if (numVerts > 0)
			writeBytes(file, &rgba[0], rgba.size());
	}

	if (vertFlags & 4) {
		for (int i = 0; i < numVerts; i++)
			writeInt(file, vertIndex[i]);
	}

	if (vertFlags & 0x10) {
		for (int i = 0; i < 2 * numVerts; i++)
			writeFloat(file, uv[i]);
	}

	writeInt(file, (int)trisets.size());

	for (uint i = 0; i < trisets.size(); i++) {
		TriSet *triset = trisets[i];

		writeInt(file, triset->drawFlags);
		writeInt(file, triset->numMaterials);

		if (triset->numMaterials > 0)
			writeInt(file, triset->materialIdx);

		writeInt(file, (int)triset->indices.size());

		for (uint j = 0; j < triset->indices.size(); j++) {
			byte bytes[2];

			WRITE_LE_UINT16(bytes, triset->indices[j]);
			writeBytes(file, bytes, sizeof(bytes));
		}
	}

	if (weightMap) {
		writeInt(file, 1);
		writeInt(file, (int)weightMap->nodeNames.size());

		for (uint i = 0; i < weightMap->nodeNames.size(); i++)
			writeString(file, weightMap->nodeNames[i]);

		writeInt(file, (int)weightMap->weights.size());

		for (uint i = 0; i < weightMap->weights.size(); i++) {
			writeInt(file, weightMap->weights[i].first);
			writeInt(file, weightMap->weights[i].node);
			writeFloat(file, weightMap->weights[i].weight);
		}
	} else {
		writeInt(file, 0);
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

} // End of namespace Grim
