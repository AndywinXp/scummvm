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

#ifndef GRIM_EMI_RMESHPARSER_H
#define GRIM_EMI_RMESHPARSER_H

#include "engines/grim/emi/lformatparser.h"

namespace Grim {

class RMesh;

class RMeshParser : public LFormatParser {
public:
	enum {
		kKeywordNumVerts = 0,
		kKeywordVertFlags = 1,
		kKeywordNumFaces = 2,
		kKeywordNumFaceNormals = 3,
		kKeywordVector = 4,
		kKeywordNormal = 5,
		kKeywordTexCoord = 6,
		kKeywordColor = 7,
		kKeywordVertIndex = 8,
		kKeywordFace = 9,
		kKeywordNumTriSets = 10,
		kKeywordTsDrawFlags = 11,
		kKeywordTsNumMaterials = 12,
		kKeywordTsMaterialIdx = 13,
		kKeywordTsNumTris = 14,
		kKeywordTsTriangle = 15,
		kKeywordTotTxSets = 16,
		kKeywordBasePrim = 17,
		kKeywordDeformPrim = 19,
		kKeywordSetType = 20,
		kKeywordNumTextures = 21,
		kKeywordTextureName = 22
	};

	explicit RMeshParser(EmiEngine *engine);

	bool handleKeyword(int id, int numArgs, KeywordArg *args) override;
	bool parseBasePrim(int token, int numArgs, KeywordArg *args);
	bool parseDeformPrim(int token, int numArgs, KeywordArg *args);

	RMesh *mesh;
};

} // End of namespace Grim

#endif
