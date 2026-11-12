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
#include "engines/grim/emi/costume/components/spritecomponent.h"
#include "engines/grim/emi/costume/components/spriteinstance.h"

namespace Grim {

SpriteInstance::SpriteInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_owner = actor;
	_spriteInstance = nullptr;

	SpriteComponent *component = nullptr;

	if (_handle.lock())
		component = dynamic_cast<SpriteComponent *>(_handle._typed);

	if (!component) {
		warning("SpriteInstance::SpriteInstance(): cannot get the sprite component %s", componentName.c_str());
	} else if (!_owner) {
		warning("SpriteInstance::SpriteInstance(): no owner");
	} else {
		_spriteInstance = static_cast<CSpriteInstance *>(component->CSprite::createInstance());
		_owner->_instances.push_back(_spriteInstance);
	}

	_handle.unlock();

	if (_spriteInstance)
		_spriteInstance->_visible = true;
}

SpriteInstance::~SpriteInstance() {
	if (_owner && _spriteInstance) {
		int index = _owner->findPrimitiveInstance(_spriteInstance);

		_owner->removePrimitiveInstance(index);
		delete _spriteInstance;
		_spriteInstance = nullptr;
	}
}

void SpriteInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (_spriteInstance)
		_spriteInstance->_visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
