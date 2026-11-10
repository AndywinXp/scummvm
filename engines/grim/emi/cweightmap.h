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

#ifndef GRIM_EMI_CWEIGHTMAP_H
#define GRIM_EMI_CWEIGHTMAP_H

#include "common/array.h"

#include "engines/grim/emi/cnode.h"
#include "engines/grim/emi/cprimitive.h"
#include "engines/grim/emi/ctransform.h"

namespace Grim {

struct VertexSource {
	int count;
	const float *positions;
	const float *normals;
};

struct WeightData {
	void *operator new(size_t size);
	void operator delete(void *block);

	float weight;
	int node;
	WeightData *next;
};

struct VertexWeight {
	VertexWeight() : first(0), node(0), weight(0.0f) {}

	byte first;
	int node;
	float weight;
};

class CWeightMap : public CPrimitive {
public:
	CWeightMap();
	virtual ~CWeightMap();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual bool save(LFile *file);
	virtual bool restore(LFile *file);

	int getSize();
	void addWeight(int vertex, float weight, int nodeIndex);
	uint findNodeIndex(const LString &nodeName);
	void remapVertexLists(int count, const int *vertexMap);
	void flattenWeights();
	void bindNodes(CNode *actorNode, CNode **outNodes);
	void findNodes(CNode *actorNode, CNode **outNodes);
	void computeTransforms(CNode *node, const CTransform *parentTransform);
	void deformVerts(const VertexSource *source, float *outPositions, float *outNormals, CNode **nodes);

	bool bound;
	Common::Array<LString> nodeNames;
	Common::Array<VertexWeight> weights;
	Common::Array<WeightData *> weightBlocks;
	Common::Array<WeightData *> vertexLists;
};

} // End of namespace Grim

#endif
