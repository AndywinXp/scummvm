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

#include "engines/grim/emi/costume/components/animinstance.h"
#include "engines/grim/emi/costume/components/animwgtinstance.h"

namespace Grim {

AnimWgtInstance::AnimWgtInstance(EmiEngine *engine, const LString &componentName) : ComponentInstance(engine, componentName) {
	_anim = nullptr;
}

AnimWgtInstance::~AnimWgtInstance() {
	_anim = nullptr;
}

void AnimWgtInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!partial || !_anim)
		return;

	double fraction = 0.0;
	double span = (double)elapsed + (double)remaining;

	// The original could divide by zero here, let's avoid that...
	if (span != 0.0)
		fraction = (double)elapsed / span;

	_anim->_weight = (float)(fraction * ((double)nextValue - (double)value) + (double)value);
}

bool AnimWgtInstance::setParent(ComponentInstance *parent) {
	_anim = dynamic_cast<AnimInstance *>(parent);

	return _anim != nullptr;
}

} // End of namespace Grim
