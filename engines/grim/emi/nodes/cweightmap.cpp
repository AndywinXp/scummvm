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

#include "engines/grim/emi/nodes/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lformatparser.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/core/profilertimer.h"

namespace Grim {

enum {
	kKeywordNumNodes = 0,
	kKeywordNodeName = 1,
	kKeywordNumVerts = 2,
	kKeywordNumWeights = 3,
	kKeywordVertWgt = 4
};

static const int kTokenSize = 256;

class CWeightMapParser : public LFormatParser {
public:
	explicit CWeightMapParser(EmiEngine *engine) : LFormatParser(engine) {
		addKeyword("NUMNODES", kKeywordNumNodes, 1);
		addKeyword("NODENAME", kKeywordNodeName, 3);
		addKeyword("NUMVERTS", kKeywordNumVerts, 1);
		addKeyword("NUMWEIGHTS", kKeywordNumWeights, 1);
		addKeyword("VERTWGT", kKeywordVertWgt, 14);
	}
};

void *WeightData::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void WeightData::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

CWeightMap::CWeightMap(EmiEngine *engine) : _engine(engine) {
	_bound = false;
}

CWeightMap::~CWeightMap() {
	for (uint i = 0; i < _weightBlocks.size(); i++) {
		delete _weightBlocks[i];
		_weightBlocks[i] = nullptr;
	}

	_weightBlocks.clear();
	_vertexLists.clear();
	_weights.clear();
	_nodeNames.clear();
}

void *CWeightMap::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CWeightMap::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int CWeightMap::getSize() {
	int size = sizeof(CWeightMap);

	for (uint i = 0; i < _nodeNames.size(); i++)
		size += _nodeNames[i].size();

	return size + _weights.size() * sizeof(VertexWeight);
}

bool CWeightMap::save(LFile *file) {
	if (!file)
		return false;

	CWeightMapParser parser(_engine);

	parser.setFile(file);
	parser.writeVersion(2, 0);
	parser.writeComment("CWeightMap primitive");
	parser.writeEntry(kKeywordNumNodes, (int)_nodeNames.size());

	for (uint i = 0; i < _nodeNames.size(); i++)
		parser.writeEntry(kKeywordNodeName, _nodeNames[i].c_str());

	parser.writeEntry(kKeywordNumVerts, (int)_vertexLists.size());

	for (uint i = 0; i < _vertexLists.size(); i++) {
		int weightCount = 0;

		for (WeightData *data = _vertexLists[i]; data; data = data->next)
			weightCount++;

		parser.writeEntry(kKeywordNumWeights, weightCount);

		for (WeightData *data = _vertexLists[i]; data; data = data->next) {
			const char *nodeName = "";

			if (data->node >= 0 && data->node < (int)_nodeNames.size())
				nodeName = _nodeNames[data->node].c_str();
			else
				warning("CWeightMap::save(): invalid node index %d", data->node);

			parser.writeEntry(kKeywordVertWgt, data->weight, nodeName);
		}
	}

	parser.writeEndOfSection();

	return true;
}

bool CWeightMap::restore(LFile *file) {
	if (!file)
		return false;

	CWeightMapParser parser(_engine);
	char token[kTokenSize];
	int nodeCount = 0;
	int vertexCount = 0;

	parser.setFile(file);
	parser.readVersion(2, 0);
	parser.readEntry(kKeywordNumNodes, &nodeCount);

	if (nodeCount < 0) {
		warning("CWeightMap::restore(): invalid node count %d", nodeCount);
		nodeCount = 0;
	}

	for (int i = 0; i < nodeCount; i++) {
		token[0] = 0;
		parser.readEntry(kKeywordNodeName, token);
		_nodeNames.push_back(LString(token));
	}

	parser.readEntry(kKeywordNumVerts, &vertexCount);

	if (vertexCount < 0) {
		warning("CWeightMap::restore(): invalid vertex count %d", vertexCount);
		vertexCount = 0;
	}

	_vertexLists.resize(vertexCount);

	for (int i = 0; i < vertexCount; i++) {
		int weightCount = 0;

		parser.readEntry(kKeywordNumWeights, &weightCount);
		_vertexLists[i] = nullptr;

		if (weightCount < 0) {
			warning("CWeightMap::restore(): invalid weight count %d", weightCount);
			weightCount = 0;
		}

		for (int j = 0; j < weightCount; j++) {
			float weight = 0.0f;

			token[0] = 0;
			parser.readEntry(kKeywordVertWgt, &weight, token);
			addWeight(i, weight, findNodeIndex(LString(token)));
		}
	}

	parser.readEndOfSection();

	return true;
}

void CWeightMap::addWeight(int vertex, float weight, int nodeIndex) {
	if (vertex < 0 || vertex >= (int)_vertexLists.size()) {
		warning("CWeightMap::addWeight(): invalid vertex %d", vertex);

		return;
	}

	WeightData *data = new WeightData();

	data->weight = weight;
	data->node = nodeIndex;
	data->next = _vertexLists[vertex];
	_vertexLists[vertex] = data;
	_weightBlocks.push_back(data);
}

uint CWeightMap::findNodeIndex(const LString &nodeName) {
	for (uint i = 0; i < _nodeNames.size(); i++) {
		if (_nodeNames[i] == nodeName)
			return i;
	}

	return 0;
}

void CWeightMap::remapVertexLists(int count, const int *vertexMap) {
	Common::Array<WeightData *> remapped;

	if (count < 0) {
		warning("CWeightMap::remapVertexLists(): invalid vertex count %d", count);
		count = 0;
	}

	remapped.resize(count);

	for (int i = 0; i < count; i++) {
		int source = vertexMap[i];

		if (source >= 0 && source < (int)_vertexLists.size())
			remapped[i] = _vertexLists[source];
		else
			warning("CWeightMap::remapVertexLists(): invalid source vertex %d", source);
	}

	_vertexLists = remapped;
}

void CWeightMap::flattenWeights() {
	int total = 0;

	for (uint i = 0; i < _vertexLists.size(); i++) {
		for (WeightData *data = _vertexLists[i]; data; data = data->next)
			total++;
	}

	_weights.clear();
	_weights.resize(total);

	int position = 0;

	for (uint i = 0; i < _vertexLists.size(); i++) {
		if (position < total)
			_weights[position].first = 1;
		else
			warning("CWeightMap::flattenWeights(): vertex %u has no weights", i);

		for (WeightData *data = _vertexLists[i]; data; data = data->next) {
			_weights[position].node = data->node;
			_weights[position].weight = data->weight;
			position++;
		}
	}

	for (uint i = 0; i < _weightBlocks.size(); i++) {
		delete _weightBlocks[i];
		_weightBlocks[i] = nullptr;
	}

	_weightBlocks.clear();
	_vertexLists.clear();
}

void CWeightMap::bindNodes(CNode *actorNode, CNode **outNodes) {
	CNode *root = actorNode->_firstChild;

	if (!root)
		return;

	root->detachFromParent();

	for (uint i = 0; i < _nodeNames.size(); i++) {
		CNode *node = root->findNode(_nodeNames[i].c_str());

		if (node) {
			if ((node->_nodeFlags & CNode::kWorldValid) == 0)
				node->updateWorldTransform();

			node->_bindRot = node->_world._rot;
			node->_bindPos = node->_world._pos;
			outNodes[i] = node;
		}
	}

	root->setParent(actorNode);
	_bound = true;
}

void CWeightMap::findNodes(CNode *actorNode, CNode **outNodes) {
	CNode *root = actorNode->_firstChild;

	if (!root)
		return;

	for (uint i = 0; i < _nodeNames.size(); i++) {
		CNode *node = root->findNode(_nodeNames[i].c_str());

		if (node)
			outNodes[i] = node;
	}

	_bound = true;
}

void CWeightMap::computeTransforms(CNode *node, const CTransform *parentTransform) {
	const float *b = node->_bindRot._m;
	const float *r = parentTransform->_rot._m;
	double negX = -(double)node->_bindPos._x;
	double negY = -(double)node->_bindPos._y;
	double negZ = -(double)node->_bindPos._z;
	float inverseX = (float)(negZ * b[2] + negY * b[1] + negX * b[0]);
	float inverseY = (float)(negZ * b[5] + negX * b[3] + negY * b[4]);
	float inverseZ = (float)(negZ * b[8] + negX * b[6] + negY * b[7]);
	CMatrix transposed;

	node->_bindRot.transpose(&transposed);

	double sumX = (double)inverseY * r[3] + (double)inverseX * r[0] + (double)inverseZ * r[6];
	float sumY = (float)((double)inverseX * r[1] + (double)inverseZ * r[7] + (double)inverseY * r[4]);
	float sumZ = (float)((double)inverseX * r[2] + (double)inverseZ * r[8] + (double)inverseY * r[5]);
	CVector skinTranslation;

	skinTranslation._x = (float)(sumX + parentTransform->_pos._x);
	skinTranslation._y = (float)((double)sumY + parentTransform->_pos._y);
	skinTranslation._z = (float)((double)sumZ + parentTransform->_pos._z);

	CMatrix skin;

	transposed.multiply(&skin, &parentTransform->_rot);
	node->_skinRot = skin;
	node->_skinPos = skinTranslation;

	for (CNode *child = node->_firstChild; child; child = child->_nextSibling) {
		if ((child->_nodeFlags & CNode::kPositionValid) == 0)
			child->computeLocalPositionFromWorld();

		if ((child->_nodeFlags & CNode::kQuaternionValid) == 0)
			child->updateLocalQuaternion();

		CMatrix childRotation;

		childRotation.setFromQuaternion(child->_quat);

		const float *c = childRotation._m;
		float childX = child->_pos._x;
		float childY = child->_pos._y;
		float childZ = child->_pos._z;
		double partialX = (double)childZ * r[6] + (double)childY * r[3] + (double)childX * r[0];
		float partialY = (float)((double)childX * r[1] + (double)childY * r[4] + (double)childZ * r[7]);
		float partialZ = (float)((double)childX * r[2] + (double)childY * r[5] + (double)childZ * r[8]);
		CTransform childWorld;

		childWorld._pos._x = (float)(partialX + parentTransform->_pos._x);
		childWorld._pos._y = (float)((double)partialY + parentTransform->_pos._y);
		childWorld._pos._z = (float)((double)partialZ + parentTransform->_pos._z);
		childWorld._rot._m[0] = (float)((double)c[0] * r[0] + (double)c[2] * r[6] + (double)c[1] * r[3]);
		childWorld._rot._m[1] = (float)((double)c[0] * r[1] + (double)c[2] * r[7] + (double)c[1] * r[4]);
		childWorld._rot._m[2] = (float)((double)c[2] * r[8] + (double)c[1] * r[5] + (double)c[0] * r[2]);
		childWorld._rot._m[3] = (float)((double)c[3] * r[0] + (double)c[5] * r[6] + (double)c[4] * r[3]);
		childWorld._rot._m[4] = (float)((double)c[3] * r[1] + (double)c[5] * r[7] + (double)c[4] * r[4]);
		childWorld._rot._m[5] = (float)((double)c[5] * r[8] + (double)c[4] * r[5] + (double)c[3] * r[2]);
		childWorld._rot._m[6] = (float)((double)c[6] * r[0] + (double)c[8] * r[6] + (double)c[7] * r[3]);
		childWorld._rot._m[7] = (float)((double)c[6] * r[1] + (double)c[8] * r[7] + (double)c[7] * r[4]);
		childWorld._rot._m[8] = (float)((double)c[8] * r[8] + (double)c[7] * r[5] + (double)c[6] * r[2]);

		computeTransforms(child, &childWorld);
	}
}

void CWeightMap::deformVerts(const VertexSource *source, float *outPositions, float *outNormals, CNode **nodes) {
	_engine->_weightMapDeformTimer.start();

	if (!_bound) {
		_engine->_weightMapDeformTimer.stop();

		return;
	}

	int vertex = -1;

	for (uint i = 0; i < _weights.size(); i++) {
		const VertexWeight &entry = _weights[i];

		if (entry.first)
			vertex++;

		if (vertex < 0 || entry.node < 0 || entry.node >= (int)_nodeNames.size() || !nodes[entry.node]) {
			warning("CWeightMap::deformVerts(): invalid weight entry %u", i);

			continue;
		}

		const float *m = nodes[entry.node]->_skinRot._m;
		const CVector &t = nodes[entry.node]->_skinPos;
		const float *p = source->positions + vertex * 3;
		float *o = outPositions + vertex * 8;
		float w = entry.weight;
		double x = p[0];
		double y = p[1];
		double z = p[2];

		if (source->normals) {
			const float *n = source->normals + vertex * 3;
			float *no = outNormals + vertex * 3;
			double nx = n[0];
			double ny = n[1];
			double nz = n[2];

			if (entry.first) {
				float py = (float)(m[4] * y + m[1] * x + m[7] * z);
				float px = (float)(m[3] * y + m[6] * z + m[0] * x);
				float sy = (float)((double)py + t._y);
				float sx = (float)((double)px + t._x);
				double sz = (m[5] * y + m[2] * x + m[8] * z + t._z) * w;
				float ny1 = (float)(m[4] * ny + m[7] * nz + m[1] * nx);
				float nx1 = (float)(m[3] * ny + m[6] * nz + nx * m[0]);
				double nz1 = (m[5] * ny + m[8] * nz + m[2] * nx) * w;

				o[0] = (float)((double)sx * w);
				o[1] = (float)((double)sy * w);
				o[2] = (float)sz;
				no[0] = (float)((double)nx1 * w);
				no[1] = (float)((double)ny1 * w);
				no[2] = (float)nz1;
			} else {
				float py = (float)(m[4] * y + m[7] * z + m[1] * x);
				float px = (float)(m[3] * y + m[6] * z + m[0] * x);
				float sy = (float)((double)py + t._y);
				float sx = (float)((double)px + t._x);
				double sz = (m[5] * y + m[8] * z + m[2] * x + t._z) * w;
				double nz1 = m[8] * nz + m[2] * nx + m[5] * ny;
				float ny1 = (float)(m[7] * nz + m[1] * nx + m[4] * ny);
				float nx1 = (float)(m[6] * nz + m[3] * ny + nx * m[0]);

				o[0] = (float)((double)sx * w + o[0]);
				o[1] = (float)((double)sy * w + o[1]);
				o[2] = (float)(sz + o[2]);
				no[0] = (float)((double)nx1 * w + no[0]);
				no[1] = (float)((double)ny1 * w + no[1]);
				no[2] = (float)(nz1 * w + no[2]);
			}
		} else if (entry.first) {
			float py = (float)(m[4] * y + m[1] * x + m[7] * z);
			float px = (float)(m[3] * y + m[6] * z + m[0] * x);
			float sy = (float)((double)py + t._y);
			float sx = (float)((double)px + t._x);
			double sz = (m[5] * y + m[2] * x + m[8] * z + t._z) * w;

			o[0] = (float)((double)sx * w);
			o[1] = (float)((double)sy * w);
			o[2] = (float)sz;
		} else {
			float py = (float)(m[7] * z + m[1] * x + m[4] * y);
			float px = (float)(m[6] * z + m[3] * y + x * m[0]);
			float sy = (float)((double)py + t._y);
			float sx = (float)((double)px + t._x);
			double sz = (m[8] * z + m[2] * x + m[5] * y + t._z) * w;

			o[0] = (float)((double)sx * w + o[0]);
			o[1] = (float)((double)sy * w + o[1]);
			o[2] = (float)(sz + o[2]);
		}
	}

	_engine->_weightMapDeformTimer.stop();
}

} // End of namespace Grim
