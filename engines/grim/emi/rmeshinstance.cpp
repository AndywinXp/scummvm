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

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/rnode.h"

namespace Grim {

static const int kStaticLights = 32;
static const int kDynamicLights = 4;
static const int kStaticLightingMask = RNode::kDynamicLighting | RNode::kNoLighting;
static const int kBlendFlag = 0x10000;
static const float kAlphaScale = 255.0f;
static const int kAlphaShift = 24;
static const uint32 kAlphaMask = 0x00FFFFFF;

RMeshInstance::RMeshInstance(RMesh *meshPrimitive) : RPrimitiveInstance(meshPrimitive), _engine(meshPrimitive->getEngine()) {
	CDrawContext *ctx = _engine->getDrawContext();
	Common::Array<uint32> packedColors;
	const uint32 *colorData = nullptr;

	vertexArray = nullptr;
	ownsVertexBuf = false;
	vertexBuf = nullptr;
	boneNodes = nullptr;
	staticLightingDone = false;
	lastFrame = 0;

	if (meshPrimitive->colors && meshPrimitive->numVerts > 0) {
		packedColors.resize(meshPrimitive->numVerts);

		for (int i = 0; i < meshPrimitive->numVerts; i++)
			packedColors[i] = meshPrimitive->colors[i].pack();

		colorData = &packedColors[0];
	}

	if (ctx)
		vertexArray = ctx->createVertexArray(meshPrimitive->numVerts, meshPrimitive->pos, colorData, meshPrimitive->uv);
	else
		warning("RMeshInstance::RMeshInstance(): no draw context");

	for (uint i = 0; i < meshPrimitive->matHandles.size(); i++)
		texIndex.push_back(0);

	if (!meshPrimitive->weightMap) {
		ownsVertexBuf = false;
		vertexBuf = meshPrimitive->normals;
	} else {
		int numVerts = vertexArray ? vertexArray->numVerts : 0;

		vertexBuf = (float *)_engine->getMemoryServices()->mallocImpl(numVerts * 3 * sizeof(float));

		if (vertexBuf)
			memset(vertexBuf, 0, numVerts * 3 * sizeof(float));

		ownsVertexBuf = true;
	}
}

RMeshInstance::~RMeshInstance() {
	delete vertexArray;
	vertexArray = nullptr;

	if (ownsVertexBuf && vertexBuf) {
		_engine->getMemoryServices()->freeImpl(vertexBuf);
		vertexBuf = nullptr;
	}

	if (boneNodes) {
		_engine->getMemoryServices()->freeImpl(boneNodes);
		boneNodes = nullptr;
	}

	texIndex.clear();
}

void RMeshInstance::setMaterialTextureIndex(int materialIndex, int textureIndex) {
	if (materialIndex < 0 || materialIndex >= (int)texIndex.size()) {
		warning("RMeshInstance::setMaterialTextureIndex(): invalid material index %d", materialIndex);

		return;
	}

	texIndex[materialIndex] = textureIndex;
}

void RMeshInstance::update(RNode *node, Camera *camera) {
	_engine->_rmeshUpdateTimer.start();

	RMesh *mesh = static_cast<RMesh *>(prim);
	CDrawContext *ctx = camera->getDrawContext();

	if (!ctx || !vertexArray) {
		warning("RMeshInstance::update(): no draw context or vertex array");
		_engine->_rmeshUpdateTimer.stop();

		return;
	}

	int frameCounter = ctx->frameCounter;
	CDCVertex *vertices = static_cast<CDCVertex *>(vertexArray->lock());

	if (!staticLightingDone) {
		if (mesh->normals && (node->drawFlags & kStaticLightingMask) == 0) {
			if ((node->nodeFlags & CNode::kWorldValid) == 0)
				node->updateWorldTransform();

			mesh->lightVertices(&node->world, vertexArray->numVerts, vertices, vertexBuf, kStaticLights);
		}

		staticLightingDone = true;
	}

	if (lastFrame != frameCounter) {
		lastFrame = frameCounter;

		if (mesh->weightMap) {
			_engine->_rmeshComputeTransformsTimer.start();

			CTransform identity;

			identity.rot.setIdentity();
			mesh->weightMap->computeTransforms(node, &identity);
			_engine->_rmeshComputeTransformsTimer.stop();

			VertexSource source;

			source.count = mesh->numVerts;
			source.positions = mesh->pos;
			source.normals = mesh->normals;

			if (boneNodes)
				mesh->weightMap->deformVerts(&source, &vertices->x, vertexBuf, boneNodes);
			else
				warning("RMeshInstance::update(): the bone nodes are not bound");
		}

		if (mesh->normals) {
			uint flags = node->drawFlags;

			if ((flags & RNode::kNoLighting) == 0 && (flags & RNode::kDynamicLighting) != 0) {
				if ((node->nodeFlags & CNode::kWorldValid) == 0)
					node->updateWorldTransform();

				if ((flags & RNode::kAmbientCube) == 0)
					mesh->lightVertices(&node->world, vertexArray->numVerts, vertices, vertexBuf, kDynamicLights);
				else
					mesh->lightVerticesAmbientCube(&node->world, vertexArray->numVerts, vertices, vertexBuf, kDynamicLights);
			}
		}

		if (globalAlphaMode == kAlphaSet) {
			for (int i = 0; i < vertexArray->numVerts; i++) {
				uint32 alpha = (uint32)(int)((double)globalAlpha * kAlphaScale) & 0xFF;

				vertices[i].color = (vertices[i].color & kAlphaMask) | (alpha << kAlphaShift);
			}
		} else if (globalAlphaMode == kAlphaModulate) {
			for (int i = 0; i < vertexArray->numVerts; i++) {
				int scale = (int)((double)globalAlpha * kAlphaScale);
				int vertexAlpha = vertices[i].color >> kAlphaShift;
				int product = (int)((uint32)scale * (uint32)vertexAlpha);
				uint32 alpha = (uint32)(product / 255) & 0xFF;

				vertices[i].color = (vertices[i].color & kAlphaMask) | (alpha << kAlphaShift);
			}
		}

		if (matAlphaMode == kAlphaSet || matAlphaMode == kAlphaModulate) {
			for (Common::HashMap<int, float>::const_iterator it = matAlpha.begin(); it != matAlpha.end(); ++it) {
				if (it->_key < 0 || it->_key >= vertexArray->numVerts) {
					warning("RMeshInstance::update(): invalid vertex index %d", it->_key);
					continue;
				}

				CDCVertex *vertex = &vertices[it->_key];
				int scale = (int)((double)it->_value * kAlphaScale);
				uint32 alpha;

				if (matAlphaMode == kAlphaSet) {
					alpha = (uint32)scale & 0xFF;
				} else {
					int vertexAlpha = vertex->color >> kAlphaShift;
					int product = (int)((uint32)scale * (uint32)vertexAlpha);

					alpha = (uint32)(product / 255) & 0xFF;
				}

				vertex->color = (vertex->color & kAlphaMask) | (alpha << kAlphaShift);
			}
		}
	}

	vertexArray->unlock();
	_engine->_rmeshUpdateTimer.stop();
}

void RMeshInstance::output(RNode *node, Camera *camera) {
	_engine->_rmeshOutputTimer.start();

	if (!visible) {
		_engine->_rmeshOutputTimer.stop();

		return;
	}

	CDrawContext *ctx = camera->getDrawContext();
	RMesh *mesh = static_cast<RMesh *>(prim);

	if (!ctx || !vertexArray) {
		warning("RMeshInstance::output(): no draw context or vertex array");
		_engine->_rmeshOutputTimer.stop();

		return;
	}

	CTexture *texture = nullptr;

	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	const float *v = camera->view.rot.m;
	const CVector &nodePos = node->world.pos;
	const CVector &viewPos = camera->view.pos;

	double pendingZ = (double)v[2] * nodePos.x + (double)v[8] * nodePos.z + (double)v[5] * nodePos.y;
	float rotatedY = (float)((double)v[1] * nodePos.x + (double)v[7] * nodePos.z + (double)v[4] * nodePos.y);
	float rotatedX = (float)((double)v[6] * nodePos.z + (double)v[3] * nodePos.y + (double)nodePos.x * v[0]);
	float viewX = (float)((double)rotatedX + viewPos.x);
	float viewY = (float)((double)rotatedY + viewPos.y);
	float viewZ = (float)(pendingZ + viewPos.z);
	CMatrix rotation;

	node->world.rot.multiply(&rotation, &camera->view.rot);

	float matrix[12];

	for (int i = 0; i < 9; i++)
		matrix[i] = rotation.m[i];

	matrix[9] = viewX;
	matrix[10] = viewY;
	matrix[11] = viewZ;

	ctx->setCameraMatrix(matrix);
	update(node, camera);
	ctx->submitVertices(vertexArray);

	for (uint i = 0; i < mesh->trisets.size(); i++) {
		_engine->_rmeshFutzingTimer.start();

		TriSet *triset = mesh->trisets[i];
		int flags = triset->drawFlags;

		if (globalAlphaMode || matAlphaMode)
			flags |= kBlendFlag;

		// The disasm keeps the texture of the previous triangle set when the material index is -1
		if (!mesh->matHandles.empty() && triset->materialIdx != -1) {
			int materialIdx = triset->materialIdx;

			if (materialIdx < 0 || materialIdx >= (int)mesh->materials.size() || materialIdx >= (int)texIndex.size() || !mesh->materials[materialIdx])
				warning("RMeshInstance::output(): invalid material index %d", materialIdx);
			else
				texture = mesh->materials[materialIdx]->getTexture(texIndex[materialIdx]);
		}

		_engine->_rmeshFutzingTimer.stop();
		ctx->drawTriangles(flags, texture, (int)triset->indices.size(), triset->indices.data());
	}

	ctx->releaseVertices();
	ctx->resetCameraMatrix();
	_engine->_rmeshOutputTimer.stop();
}

int RMeshInstance::getSaveSize() {
	return RPrimitiveInstance::getSaveSize() + (int)sizeof(int) + (int)(texIndex.size() * sizeof(int));
}

bool RMeshInstance::saveState(tSaveRestoreAccess *access) {
	RPrimitiveInstance::saveState(access);

	int count = (int)texIndex.size();

	access(&count, sizeof(count));

	for (int i = 0; i < count; i++)
		access(&texIndex[i], sizeof(int));

	return true;
}

bool RMeshInstance::restoreState(tSaveRestoreAccess *access) {
	RPrimitiveInstance::restoreState(access);

	int count = 0;

	access(&count, sizeof(count));

	if (count < 0) {
		warning("RMeshInstance::restoreState(): invalid texture index count %d", count);
		count = 0;
	}

	texIndex.resize(count, 0);

	for (int i = 0; i < count; i++)
		access(&texIndex[i], sizeof(int));

	return true;
}

} // End of namespace Grim
