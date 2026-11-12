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
#include "engines/grim/emi/animation/anode.h"
#include "engines/grim/emi/costume/components/animcomponent.h"
#include "engines/grim/emi/costume/components/animinstance.h"

namespace Grim {

AnimInstance::AnimInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName), AInst(engine), _engine(engine) {
	owner = actor;
	bound = false;
	bindAnimation();
}

AnimInstance::~AnimInstance() {
	if (bound)
		unbindAnimation();
}

void AnimInstance::update(float deltaTime) {
	if ((flags & AInst::kAttached) != 0)
		AInst::update(deltaTime);
}

void AnimInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!bound)
		return;

	switch ((int)value) {
	case 0:
		AInst::stop();
		break;
	case 1:
		AInst::play();
		break;
	case 2:
		AInst::pause();
		break;
	case 3:
		AInst::setLoop(true);
		break;
	case 4:
		AInst::setLoop(false);
		break;
	case 5:
		AInst::fadeIn(1.0f);
		break;
	case 6:
		AInst::fadeIn(0.75f);
		break;
	case 7:
		AInst::fadeIn(0.5f);
		break;
	case 8:
		AInst::fadeIn(0.25f);
		break;
	case 9:
		AInst::fadeIn(0.125f);
		break;
	case 10:
		AInst::fadeOut(1.0f);
		break;
	case 11:
		AInst::fadeOut(0.75f);
		break;
	case 12:
		AInst::fadeOut(0.5f);
		break;
	case 13:
		AInst::fadeOut(0.25f);
		break;
	case 14:
		AInst::fadeOut(0.125f);
		break;
	default:
		break;
	}
}

void AnimInstance::fadeIn(float seconds) {
	AInst::fadeIn(seconds);
	AInst::play();
}

void AnimInstance::fadeOut(float seconds) {
	AInst::fadeOut(seconds);
	AInst::play();
}

void AnimInstance::reset() {
	unbindAnimation();
}

void AnimInstance::bindAnimation() {
	if (bound)
		return;

	AnimComponent *component = nullptr;

	if (handle.lock())
		component = dynamic_cast<AnimComponent *>(handle.typed);

	if (component) {
		length = component->Animation::length;

		for (int i = 0; i < component->entryCount; i++) {
			if (!owner) {
				warning("AnimInstance::bindAnimation(): no owner");
				break;
			}

			if ((uint)i >= component->entries.size()) {
				warning("AnimInstance::bindAnimation(): entry %d is missing", i);
				break;
			}

			ANode *animNode = dynamic_cast<ANode *>(owner->findNode(component->entries[i].name));

			if (animNode && !component->entries[i].track)
				warning("AnimInstance::bindAnimation(): entry %d has no track", i);
			else if (animNode)
				animNode->bindTrack(this, component->entries[i].track);
		}
	}

	handle.unlock();
	bound = true;
}

void AnimInstance::unbindAnimation() {
	if (!bound)
		return;

	if (!owner) {
		bound = false;

		return;
	}

	AnimComponent *component = nullptr;

	if (handle.lock())
		component = dynamic_cast<AnimComponent *>(handle.typed);

	if (component) {
		for (int i = 0; i < component->entryCount; i++) {
			if ((uint)i >= component->entries.size()) {
				warning("AnimInstance::unbindAnimation(): entry %d is missing", i);
				break;
			}

			ANode *animNode = dynamic_cast<ANode *>(owner->findNode(component->entries[i].name));

			if (animNode && !component->entries[i].track)
				warning("AnimInstance::unbindAnimation(): entry %d has no track", i);
			else if (animNode)
				animNode->unbindTrack(this, component->entries[i].track);
		}
	}

	handle.unlock();
	bound = false;
}

} // End of namespace Grim
