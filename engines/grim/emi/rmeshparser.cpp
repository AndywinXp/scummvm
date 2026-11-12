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

#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshparser.h"
#include "engines/grim/emi/triset.h"

namespace Grim {

static const int kTokenSize = 256;
static const int kDefaultDrawFlags = 772;
static const int kMaxTriangles = 0x7FFFFFFF / 3;

RMeshParser::RMeshParser(EmiEngine *engine) : LFormatParser(engine) {
	mesh = nullptr;

	addKeyword("NumVerts", kKeywordNumVerts, 1);
	addKeyword("VertFlags", kKeywordVertFlags, 1);
	addKeyword("NumFaces", kKeywordNumFaces, 1);
	addKeyword("NumFacesNorms", kKeywordNumFaceNormals, 1);
	addKeyword("Vector", kKeywordVector, 42);
	addKeyword("Normal", kKeywordNormal, 42);
	addKeyword("TxCoord", kKeywordTexCoord, 10);
	addKeyword("Color", kKeywordColor, 85);
	addKeyword("VertIndex", kKeywordVertIndex, 1);
	addKeyword("Face", kKeywordFace, 1365);
	addKeyword("NumTriSets", kKeywordNumTriSets, 1);
	addKeyword("TsDrawFlags", kKeywordTsDrawFlags, 1);
	addKeyword("TsNumMaterials", kKeywordTsNumMaterials, 1);
	addKeyword("TsMaterialIdx", kKeywordTsMaterialIdx, 1);
	addKeyword("TsNumTris", kKeywordTsNumTris, 1);
	addKeyword("TsTriangle", kKeywordTsTriangle, 21);
	addKeyword("TotTxSets", kKeywordTotTxSets, 1);
	addKeyword("BASEPRIM", kKeywordBasePrim, 0);
	addKeyword("DEFORMPRIM", kKeywordDeformPrim, 0);
	addKeyword("SetType", kKeywordSetType, 1);
	addKeyword("NumTextures", kKeywordNumTextures, 1);
	addKeyword("TextureName", kKeywordTextureName, 7);
}

bool RMeshParser::handleKeyword(int id, int numArgs, KeywordArg *args) {
	if (id == kKeywordBasePrim)
		return parseBasePrim(id, numArgs, args);

	if (id == kKeywordDeformPrim)
		return parseDeformPrim(id, numArgs, args);

	return true;
}

bool RMeshParser::parseBasePrim(int token, int numArgs, KeywordArg *args) {
	if (!mesh) {
		warning("RMeshParser::parseBasePrim(): no mesh");

		return false;
	}

	bool result = readEntry(kKeywordTotTxSets, &mesh->numTextureSets);

	if (!result)
		return result;

	if (mesh->numTextureSets > 0) {
		int setType = 0;
		int numTextures = 0;

		readVersion(2, 0);
		readEntry(kKeywordSetType, &setType);
		readEntry(kKeywordNumTextures, &numTextures);

		if (numTextures < 0) {
			warning("RMeshParser::parseBasePrim(): invalid texture count %d", numTextures);
			numTextures = 0;
		}

		mesh->matHandles.resize(numTextures, LResourceHandle<CMaterial>(_engine));
		mesh->texFlags.resize(numTextures, 0);
		mesh->materials.resize(numTextures, nullptr);

		for (int i = 0; i < numTextures; i++) {
			char textureName[kTokenSize];
			LResourceHandle<CMaterial> *handle = &mesh->matHandles[i];

			textureName[0] = 0;
			readEntry(kKeywordTextureName, textureName, &mesh->texFlags[i]);

			handle->resource = _engine->getResourceManager()->findResource(textureName);

			if (!handle->resource) {
				handle->resource = _engine->getResourceManager()->createResource(textureName);

				if (!handle->resource)
					handle->allocateResource();

				if (handle->resource) {
					handle->resource->resSetName(textureName);
					_engine->getResourceManager()->addResource(handle->resource);
				}
			}

			handle->setResource(handle->resource);

			CMaterial *material = nullptr;

			if (handle->resource && handle->lock())
				material = handle->typed;

			mesh->materials[i] = material;

			if (material)
				material->addFlags(mesh->texFlags[i]);
		}

		readEndOfSection();
	}

	int numFaces = 0;

	result = readEntry(kKeywordNumFaces, &numFaces);

	if (!result)
		return result;

	int numFaceNormals = 0;

	result = readEntry(kKeywordNumFaceNormals, &numFaceNormals);

	if (!result)
		return result;

	if (numFaces < 0) {
		warning("RMeshParser::parseBasePrim(): invalid face count %d", numFaces);
		numFaces = 0;
	}

	if (numFaceNormals < 0) {
		warning("RMeshParser::parseBasePrim(): invalid face normal count %d", numFaceNormals);
		numFaceNormals = 0;
	}

	for (int i = 0; i < numFaceNormals; i++) {
		float normal[3];

		if (!readEntry(kKeywordNormal, &normal[0], &normal[1], &normal[2]))
			return false;
	}

	for (int i = 0; i < numFaces; i++) {
		int face[6];

		if (!readEntry(kKeywordFace, &face[0], &face[1], &face[2], &face[3], &face[4], &face[5]))
			return false;
	}

	result = readEntry(kKeywordVertFlags, &mesh->vertFlags);

	if (!result)
		return result;

	int numVerts = 0;

	result = readEntry(kKeywordNumVerts, &numVerts);

	if (!result)
		return result;

	mesh->allocVertexData(mesh->vertFlags, numVerts);

	for (int i = 0; i < mesh->numVerts; i++)
		readEntry(kKeywordVector, &mesh->pos[i * 3], &mesh->pos[i * 3 + 1], &mesh->pos[i * 3 + 2]);

	if (mesh->vertFlags & 1) {
		for (int i = 0; i < mesh->numVerts; i++)
			readEntry(kKeywordVector, &mesh->normals[i * 3], &mesh->normals[i * 3 + 1], &mesh->normals[i * 3 + 2]);
	}

	if (mesh->vertFlags & 2) {
		for (int i = 0; i < mesh->numVerts; i++) {
			int red = 0;
			int green = 0;
			int blue = 0;
			int alpha = 0;

			readEntry(kKeywordColor, &red, &green, &blue, &alpha);
			mesh->colors[i].r = red;
			mesh->colors[i].g = green;
			mesh->colors[i].b = blue;
			mesh->colors[i].a = alpha;
		}
	}

	if (mesh->vertFlags & 4) {
		for (int i = 0; i < mesh->numVerts; i++)
			readEntry(kKeywordVertIndex, &mesh->vertIndex[i]);
	}

	if (mesh->vertFlags & 0x10) {
		for (int i = 0; i < mesh->numVerts; i++)
			readEntry(kKeywordTexCoord, &mesh->uv[i * 2], &mesh->uv[i * 2 + 1]);
	}

	int numTriSets = 0;

	if (!readEntry(kKeywordNumTriSets, &numTriSets))
		return false;

	if (numTriSets < 0) {
		warning("RMeshParser::parseBasePrim(): invalid triangle set count %d", numTriSets);
		numTriSets = 0;
	}

	mesh->trisets.resize(numTriSets, nullptr);

	for (int i = 0; i < numTriSets; i++) {
		TriSet *triset = new TriSet();

		mesh->trisets[i] = triset;

		readEntry(kKeywordTsDrawFlags, &triset->drawFlags);

		if (!triset->drawFlags)
			triset->drawFlags = kDefaultDrawFlags;

		readEntry(kKeywordTsNumMaterials, &triset->numMaterials);

		if (triset->numMaterials > 0)
			readEntry(kKeywordTsMaterialIdx, &triset->materialIdx);

		int numTriangles = 0;

		readEntry(kKeywordTsNumTris, &numTriangles);

		if (numTriangles < 0 || numTriangles > kMaxTriangles) {
			warning("RMeshParser::parseBasePrim(): invalid triangle count %d", numTriangles);
			numTriangles = 0;
		}

		triset->indices.resize(3 * numTriangles);

		for (int j = 0; j < 3 * numTriangles; j += 3) {
			int first = 0;
			int second = 0;
			int third = 0;

			readEntry(kKeywordTsTriangle, &first, &second, &third);
			triset->indices[j] = (uint16)first;
			triset->indices[j + 1] = (uint16)second;
			triset->indices[j + 2] = (uint16)third;
		}
	}

	return true;
}

bool RMeshParser::parseDeformPrim(int token, int numArgs, KeywordArg *args) {
	if (!mesh) {
		warning("RMeshParser::parseDeformPrim(): no mesh");

		return false;
	}

	CWeightMap *weightMap = new CWeightMap(_engine);

	weightMap->restore(file);
	mesh->weightMap = weightMap;

	return true;
}

} // End of namespace Grim
