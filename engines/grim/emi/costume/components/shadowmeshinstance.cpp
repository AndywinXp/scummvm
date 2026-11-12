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

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/nodes/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/nodes/rmeshinstance.h"
#include "engines/grim/emi/costume/components/shadowmeshcomponent.h"
#include "engines/grim/emi/costume/components/shadowmeshinstance.h"

namespace Grim {

ShadowMeshInstance::ShadowMeshInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_owner = actor;
	_meshInstance = nullptr;

	if (_handle.lock()) {
		ShadowMeshComponent *component = dynamic_cast<ShadowMeshComponent *>(_handle._typed);

		if (_handle._typed && !component)
			warning("ShadowMeshInstance::ShadowMeshInstance(): %s is not a shadow mesh component", componentName.c_str());

		if (component && !_owner) {
			warning("ShadowMeshInstance::ShadowMeshInstance(): no owner");
			component = nullptr;
		}

		if (component) {
			_meshInstance = static_cast<RMeshInstance *>(component->RMesh::createInstance());
			_meshInstance->_visible = false;

			if (component->_weightMap) {
				uint numNodes = component->_weightMap->_nodeNames.size();

				_meshInstance->_boneNodes = (CNode **)_engine->getMemoryServices()->mallocImpl(numNodes * sizeof(CNode *));

				// The disasm leaves the bones it cannot find uninitialized
				if (_meshInstance->_boneNodes)
					memset(_meshInstance->_boneNodes, 0, numNodes * sizeof(CNode *));

				component->_weightMap->findNodes(_owner, _meshInstance->_boneNodes);
			}

			// The disasm frees the normals of the shared mesh
			if (component->_normals) {
				_engine->getMemoryServices()->freeImpl(component->_normals);
				component->_normals = nullptr;
			}

			component->_vertFlags &= ~1;
			_owner->_shadowMeshes.push_back(_meshInstance);
		}
	}

	_handle.unlock();
}

ShadowMeshInstance::~ShadowMeshInstance() {
	if (_owner && _meshInstance) {
		uint index = _owner->findShadowPrimitive(_meshInstance);

		_owner->removeShadowPrimitive((int)index);
		delete _meshInstance;
		_meshInstance = nullptr;
	}
}

void ShadowMeshInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (_meshInstance)
		_meshInstance->_visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
