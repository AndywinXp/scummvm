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

#include "engines/grim/emi/graphics/cmaterial.h"
#include "engines/grim/emi/nodes/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/resources/lresource.h"
#include "engines/grim/emi/nodes/rmesh.h"
#include "engines/grim/emi/nodes/rmeshparser.h"
#include "engines/grim/emi/nodes/triset.h"

namespace Grim {

static const int kTokenSize = 256;
static const int kDefaultDrawFlags = 772;
static const int kMaxTriangles = 0x7FFFFFFF / 3;

RMeshParser::RMeshParser(EmiEngine *engine) : LFormatParser(engine) {
	_mesh = nullptr;

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
	if (!_mesh) {
		warning("RMeshParser::parseBasePrim(): no mesh");

		return false;
	}

	bool result = readEntry(kKeywordTotTxSets, &_mesh->_numTextureSets);

	if (!result)
		return result;

	if (_mesh->_numTextureSets > 0) {
		int setType = 0;
		int numTextures = 0;

		readVersion(2, 0);
		readEntry(kKeywordSetType, &setType);
		readEntry(kKeywordNumTextures, &numTextures);

		if (numTextures < 0) {
			warning("RMeshParser::parseBasePrim(): invalid texture count %d", numTextures);
			numTextures = 0;
		}

		_mesh->_matHandles.resize(numTextures, LResourceHandle<CMaterial>(_engine));
		_mesh->_texFlags.resize(numTextures, 0);
		_mesh->_materials.resize(numTextures, nullptr);

		for (int i = 0; i < numTextures; i++) {
			char textureName[kTokenSize];
			LResourceHandle<CMaterial> *handle = &_mesh->_matHandles[i];

			textureName[0] = 0;
			readEntry(kKeywordTextureName, textureName, &_mesh->_texFlags[i]);

			handle->_resource = _engine->getResourceManager()->findResource(textureName);

			if (!handle->_resource) {
				handle->_resource = _engine->getResourceManager()->createResource(textureName);

				if (!handle->_resource)
					handle->allocateResource();

				if (handle->_resource) {
					handle->_resource->resSetName(textureName);
					_engine->getResourceManager()->addResource(handle->_resource);
				}
			}

			handle->setResource(handle->_resource);

			CMaterial *material = nullptr;

			if (handle->_resource && handle->lock())
				material = handle->_typed;

			_mesh->_materials[i] = material;

			if (material)
				material->addFlags(_mesh->_texFlags[i]);
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

	result = readEntry(kKeywordVertFlags, &_mesh->_vertFlags);

	if (!result)
		return result;

	int numVerts = 0;

	result = readEntry(kKeywordNumVerts, &numVerts);

	if (!result)
		return result;

	_mesh->allocVertexData(_mesh->_vertFlags, numVerts);

	for (int i = 0; i < _mesh->_numVerts; i++)
		readEntry(kKeywordVector, &_mesh->_pos[i * 3], &_mesh->_pos[i * 3 + 1], &_mesh->_pos[i * 3 + 2]);

	if (_mesh->_vertFlags & 1) {
		for (int i = 0; i < _mesh->_numVerts; i++)
			readEntry(kKeywordVector, &_mesh->_normals[i * 3], &_mesh->_normals[i * 3 + 1], &_mesh->_normals[i * 3 + 2]);
	}

	if (_mesh->_vertFlags & 2) {
		for (int i = 0; i < _mesh->_numVerts; i++) {
			int red = 0;
			int green = 0;
			int blue = 0;
			int alpha = 0;

			readEntry(kKeywordColor, &red, &green, &blue, &alpha);
			_mesh->_colors[i]._r = red;
			_mesh->_colors[i]._g = green;
			_mesh->_colors[i]._b = blue;
			_mesh->_colors[i]._a = alpha;
		}
	}

	if (_mesh->_vertFlags & 4) {
		for (int i = 0; i < _mesh->_numVerts; i++)
			readEntry(kKeywordVertIndex, &_mesh->_vertIndex[i]);
	}

	if (_mesh->_vertFlags & 0x10) {
		for (int i = 0; i < _mesh->_numVerts; i++)
			readEntry(kKeywordTexCoord, &_mesh->_uv[i * 2], &_mesh->_uv[i * 2 + 1]);
	}

	int numTriSets = 0;

	if (!readEntry(kKeywordNumTriSets, &numTriSets))
		return false;

	if (numTriSets < 0) {
		warning("RMeshParser::parseBasePrim(): invalid triangle set count %d", numTriSets);
		numTriSets = 0;
	}

	_mesh->_trisets.resize(numTriSets, nullptr);

	for (int i = 0; i < numTriSets; i++) {
		TriSet *triset = new TriSet();

		_mesh->_trisets[i] = triset;

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
	if (!_mesh) {
		warning("RMeshParser::parseDeformPrim(): no mesh");

		return false;
	}

	CWeightMap *weightMap = new CWeightMap(_engine);

	weightMap->restore(_file);
	_mesh->_weightMap = weightMap;

	return true;
}

} // End of namespace Grim
