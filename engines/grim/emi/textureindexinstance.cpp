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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/csprite.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/textureindexinstance.h"

namespace Grim {

TextureIndexInstance::TextureIndexInstance(const LString &componentName, Actor *actor) : ComponentInstance(componentName) {
	owner = actor;

	if (componentName.size() >= Component::kNamePrefixSize)
		filter = componentName.c_str() + Component::kNamePrefixSize;
	else
		warning("TextureIndexInstance::TextureIndexInstance(): invalid component name %s", componentName.c_str());
}

void TextureIndexInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!owner || !partial)
		return;

	for (uint i = 0; i < owner->instances.size(); i++) {
		RMeshInstance *meshInstance = dynamic_cast<RMeshInstance *>(owner->instances[i]);
		CSpriteInstance *spriteInstance = nullptr;
		int materialIndex = -1;

		if (meshInstance) {
			RMesh *mesh = static_cast<RMesh *>(meshInstance->prim);

			for (uint j = 0; j < mesh->matHandles.size(); j++) {
				LResource *meshMaterial = mesh->matHandles[j].resource;

				if (!meshMaterial) {
					warning("TextureIndexInstance::setKey(): missing material %u", j);
					continue;
				}

				if (meshMaterial->name == filter) {
					materialIndex = j;
					break;
				}
			}

			if (materialIndex < 0)
				continue;
		} else {
			spriteInstance = dynamic_cast<CSpriteInstance *>(owner->instances[i]);

			if (!spriteInstance)
				continue;

			LResource *spriteMaterial = static_cast<CSprite *>(spriteInstance->prim)->material.resource;

			if (!spriteMaterial) {
				warning("TextureIndexInstance::setKey(): the sprite has no material");
				continue;
			}

			if (!(spriteMaterial->name == filter))
				continue;
		}

		int textureIndex = (int)value;
		LResourceHandle<CMaterial> materialHandle(filter);
		CMaterial *material = materialHandle.lock() ? materialHandle.typed : nullptr;

		if (!material) {
			warning("TextureIndexInstance::setKey(): cannot lock the material %s", filter.c_str());
			materialHandle.unlock();
		} else if (textureIndex < 0 || textureIndex >= material->getNumTextures()) {
			materialHandle.unlock();
		} else {
			if (materialHandle.resource && materialHandle.resource->loaded) {
				materialHandle.lockCount--;
				materialHandle.resource->resUnlock();
			}

			if (meshInstance)
				meshInstance->setMaterialTextureIndex(materialIndex, textureIndex);
			else
				spriteInstance->textureIndex = textureIndex;
		}

		return;
	}
}

} // End of namespace Grim
