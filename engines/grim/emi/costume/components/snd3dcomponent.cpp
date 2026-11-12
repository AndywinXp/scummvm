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

#include "engines/grim/emi/costume/components/snd3dcomponent.h"
#include "engines/grim/emi/costume/components/sndinstance.h"

namespace Grim {

Snd3DComponent::Snd3DComponent() {
}

Snd3DComponent::~Snd3DComponent() {
}

uint Snd3DComponent::resSizeOfItem() {
	return sizeof(Snd3DComponent);
}

ComponentInstance *Snd3DComponent::createInstance(Actor *owner) {
	return new SndInstance(_engine, name, owner);
}

bool Snd3DComponent::resLoadItem(const char *itemName) {
	return true;
}

} // End of namespace Grim
