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

#include "engines/grim/emi/sndinstance.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

SndInstance::SndInstance(const LString &componentName, Actor *actor) : ComponentInstance(componentName) {
	owner = actor;
	soundHandle = 0;

	if (componentName.size() >= Component::kNamePrefixSize)
		soundHandle = Sounds::loadSound(componentName.c_str() + Component::kNamePrefixSize, 1);
	else
		warning("SndInstance::SndInstance(): invalid component name %s", componentName.c_str());
}

SndInstance::~SndInstance() {
	Sounds::stopSound(soundHandle);
}

bool SndInstance::isReplayable() {
	return false;
}

void SndInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!soundHandle)
		return;

	bool loop = false;

	switch ((int)value) {
	case 0:
		Sounds::stopSoundPlayback(soundHandle);
		break;
	case 1:
		loop = true;
		// Fall through
	case 2:
		if (Sounds::isSoundPaused(soundHandle))
			Sounds::resumeSound(soundHandle);
		else if (!Sounds::isSoundPlaying(soundHandle))
			Sounds::startSound(soundHandle, 0, loop, 100);

		break;
	case 3:
		Sounds::pauseSound(soundHandle);
		break;
	default:
		break;
	}
}

} // End of namespace Grim
