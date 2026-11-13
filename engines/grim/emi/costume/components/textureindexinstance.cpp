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
#include "engines/grim/emi/graphics/cmaterial.h"
#include "engines/grim/emi/nodes/csprite.h"
#include "engines/grim/emi/nodes/rmesh.h"
#include "engines/grim/emi/nodes/rmeshinstance.h"
#include "engines/grim/emi/costume/components/textureindexinstance.h"
#include "engines/grim/emi/emiengine.h"

namespace Grim {

TextureIndexInstance::TextureIndexInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_owner = actor;

	if (componentName.size() >= Component::kNamePrefixSize)
		_filter = componentName.c_str() + Component::kNamePrefixSize;
	else
		warning("TextureIndexInstance::TextureIndexInstance(): invalid component name %s", componentName.c_str());
}

void TextureIndexInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!_owner || !partial)
		return;

	for (uint i = 0; i < _owner->_instances.size(); i++) {
		RMeshInstance *meshInstance = dynamic_cast<RMeshInstance *>(_owner->_instances[i]);
		CSpriteInstance *spriteInstance = nullptr;
		int materialIndex = -1;

		if (meshInstance) {
			RMesh *mesh = static_cast<RMesh *>(meshInstance->_prim);

			for (uint j = 0; j < mesh->_matHandles.size(); j++) {
				LResource *meshMaterial = mesh->_matHandles[j]._resource;

				if (!meshMaterial) {
					warning("TextureIndexInstance::setKey(): missing material %u", j);
					continue;
				}

				if (meshMaterial->_name == _filter) {
					materialIndex = j;
					break;
				}
			}

			if (materialIndex < 0)
				continue;
		} else {
			spriteInstance = dynamic_cast<CSpriteInstance *>(_owner->_instances[i]);

			if (!spriteInstance)
				continue;

			LResource *spriteMaterial = static_cast<CSprite *>(spriteInstance->_prim)->_material._resource;

			if (!spriteMaterial) {
				warning("TextureIndexInstance::setKey(): the sprite has no material");
				continue;
			}

			if (!(spriteMaterial->_name == _filter))
				continue;
		}

		int textureIndex = (int)value;
		LResourceHandle<CMaterial> materialHandle(_engine, _filter);
		CMaterial *material = materialHandle.lock() ? materialHandle._typed : nullptr;

		if (!material) {
			warning("TextureIndexInstance::setKey(): cannot lock the material %s", _filter.c_str());
			materialHandle.unlock();
		} else if (textureIndex < 0 || textureIndex >= material->getNumTextures()) {
			if (_engine->isDemo())
				warning("TextureIndexInstance::setKey(): index %d is out of range for the material %s", textureIndex, _filter.c_str());

			materialHandle.unlock();
		} else {
			if (materialHandle._resource && materialHandle._resource->_loaded) {
				materialHandle._lockCount--;
				materialHandle._resource->resUnlock();
			}

			if (meshInstance)
				meshInstance->setMaterialTextureIndex(materialIndex, textureIndex);
			else
				spriteInstance->_textureIndex = textureIndex;
		}

		return;
	}

	if (_engine->isDemo())
		warning("TextureIndexInstance::setKey(): no material named %s was found on any primitive the actor is wearing", _filter.c_str());
}

} // End of namespace Grim
