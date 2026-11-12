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

#ifndef GRIM_EMI_RMESH_H
#define GRIM_EMI_RMESH_H

#include "common/array.h"

#include "engines/grim/emi/cdcvertexarray.h"
#include "engines/grim/emi/clight.h"
#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/ctransform.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/rprimitive.h"
#include "engines/grim/emi/triset.h"

namespace Grim {

class CWeightMap;
class EmiEngine;

class RMesh : public RPrimitive {
public:
	enum {
		kMaxLights = 32,
		kNumCubeFaces = 6
	};

	explicit RMesh(EmiEngine *engine);
	~RMesh() override;

	EmiEngine *getEngine() const;

	RPrimitiveInstance *createInstance() override;
	bool save(LFile *file) override;
	bool restore(LFile *file) override;
	void computeBounds() override;

	void allocVertexData(int flags, int count);
	void freeVertexData();
	void lightVertices(const CTransform *transform, int count, CDCVertex *vertices, const float *vertexNormals, int maxLights);
	void lightVerticesAmbientCube(const CTransform *transform, int count, CDCVertex *vertices, const float *vertexNormals, int maxLights);
	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	int vertFlags;
	int numVerts;
	float *pos;
	float *normals;
	int *vertIndex;
	PackedColor *colors;
	float *uv;
	int numTextureSets;
	Common::Array<int> texFlags;
	Common::Array<LResourceHandle<CMaterial> > matHandles;
	Common::Array<CMaterial *> materials;
	CWeightMap *weightMap;
	Common::Array<TriSet *> trisets;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
