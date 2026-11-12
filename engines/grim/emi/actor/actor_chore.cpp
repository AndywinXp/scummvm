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
#include "engines/grim/emi/costume/choreinstance.h"

namespace Grim {

void Actor::removeFinishedChores() {
	Common::List<uint>::iterator it = choreIds.begin();

	while (it != choreIds.end()) {
		uint choreId = *it;
		Common::List<uint>::iterator next = it;

		++next;

		if (ChoreInstance::isInstanceValid(_engine, choreId)) {
			if (!ChoreInstance::isInstanceValid(_engine, choreId) || ChoreInstance::getInstance(_engine, choreId)->isFinished()) {
				Common::List<uint>::iterator removeIt = choreIds.begin();

				while (removeIt != choreIds.end()) {
					if (*removeIt == choreId) {
						if (removeIt == next) {
							warning("Actor::removeFinishedChores(): the chore id is listed twice");
							++next;
						}

						removeIt = choreIds.erase(removeIt);
					} else {
						++removeIt;
					}
				}

				if (ChoreInstance::isInstanceValid(_engine, choreId)) {
					ChoreInstance *instance = ChoreInstance::getInstance(_engine, choreId);

					if (instance)
						delete instance;
				}
			}
		} else {
			Common::List<uint>::iterator removeIt = choreIds.begin();

			while (removeIt != choreIds.end()) {
				if (*removeIt == choreId) {
					if (removeIt == next) {
						warning("Actor::removeFinishedChores(): the chore id is listed twice");
						++next;
					}

					removeIt = choreIds.erase(removeIt);
				} else {
					++removeIt;
				}
			}
		}

		it = next;
	}
}

uint *Actor::findChoreInstance(uint *outId, uint previousId) {
	Common::List<uint>::iterator it = choreIds.begin();

	if (previousId && it != choreIds.end()) {
		uint value;

		do {
			value = *it;
			++it;
		} while (previousId != value && it != choreIds.end());
	}

	if (it == choreIds.end())
		*outId = 0;
	else
		*outId = *it;

	return outId;
}

uint *Actor::startChore(uint *outId, const LString &choreName, const LString &choreSetName) {
	uint newId = ChoreInstance::newInstance(_engine, this, choreName, choreSetName);

	if (ChoreInstance::isInstanceValid(_engine, newId))
		choreIds.push_back(newId);

	*outId = newId;

	return outId;
}

void Actor::stopChores(bool skipLooping) {
	Common::List<uint>::iterator it;

	for (it = choreIds.begin(); it != choreIds.end(); ++it) {
		if (!skipLooping || !ChoreInstance::isInstanceValid(_engine, *it) || !ChoreInstance::getInstance(_engine, *it)->isLooping()) {
			if (ChoreInstance::isInstanceValid(_engine, *it))
				ChoreInstance::getInstance(_engine, *it)->stop();
		}
	}
}

void Actor::setChore(int slot, LString choreName, LString choreSetName) {
	if (slot < 0 || slot >= kNumChoreSlots) {
		warning("Actor::setChore(): invalid slot %d", slot);

		return;
	}

	if (!choreSlotNames[slot].empty()) {
		if (choreName == choreSlotNames[slot] && choreSetName == choreSlotSetNames[slot])
			return;

		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[slot])) {
			ChoreInstance::getInstance(_engine, choreSlotIds[slot])->stop();

			uint startedId;

			startChore(&startedId, choreName, choreSetName);
			choreSlotIds[slot] = startedId;

			if (ChoreInstance::isInstanceValid(_engine, startedId))
				ChoreInstance::getInstance(_engine, choreSlotIds[slot])->setPlaying();

			if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[slot]))
				ChoreInstance::getInstance(_engine, choreSlotIds[slot])->setLooping(true);
		}
	}

	choreSlotNames[slot] = choreName;
	choreSlotSetNames[slot] = choreSetName;
}

void Actor::setWalkChore(LString choreName, LString choreSetName) {
	setChore(1, choreName, choreSetName);
}

void Actor::setTurnChores(LString leftChoreName, LString leftChoreSetName, LString rightChoreName, LString rightChoreSetName) {
	setChore(2, leftChoreName, leftChoreSetName);
	setChore(3, rightChoreName, rightChoreSetName);
}

void Actor::setRestChore(LString choreName, LString choreSetName) {
	setChore(0, choreName, choreSetName);
}

void Actor::setMumbleChore(LString choreName, LString choreSetName) {
	setChore(4, choreName, choreSetName);
}

void Actor::setTalkChore(int index, LString choreName, LString choreSetName) {
	setChore(index + 5, choreName, choreSetName);
}

void Actor::updateWalkChores() {
	uint startedId;

	if (!ChoreInstance::isInstanceValid(_engine, choreSlotIds[0])) {
		startChore(&startedId, choreSlotNames[0], choreSlotSetNames[0]);
		choreSlotIds[0] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, choreSlotIds[0])->setPlaying();

		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[0]))
			ChoreInstance::getInstance(_engine, choreSlotIds[0])->setLooping(true);
	}

	if (!extra)
		return;

	if (extra->walking) {
		extra->turnDir = 0;

		if (!ChoreInstance::isInstanceValid(_engine, choreSlotIds[1])) {
			extra->resting = false;
			startChore(&startedId, choreSlotNames[1], choreSlotSetNames[1]);
			choreSlotIds[1] = startedId;

			if (ChoreInstance::isInstanceValid(_engine, startedId))
				ChoreInstance::getInstance(_engine, choreSlotIds[1])->fadeIn(0.25f);

			if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[1]))
				ChoreInstance::getInstance(_engine, choreSlotIds[1])->setLooping(true);
		}
	} else if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[1]) && (mode & 1) == 0) {
		ChoreInstance::getInstance(_engine, choreSlotIds[1])->fadeOut(0.25f);
		choreSlotIds[1] = 0;
	}

	if (extra->turnDir <= 0) {
		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[2]))
			ChoreInstance::getInstance(_engine, choreSlotIds[2])->fadeOut(0.25f);

		choreSlotIds[2] = 0;
	} else if (!ChoreInstance::isInstanceValid(_engine, choreSlotIds[2])) {
		extra->resting = false;
		startChore(&startedId, choreSlotNames[2], choreSlotSetNames[2]);
		choreSlotIds[2] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, choreSlotIds[2])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[2]))
			ChoreInstance::getInstance(_engine, choreSlotIds[2])->setLooping(true);
	}

	if (extra->turnDir >= 0) {
		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[3]))
			ChoreInstance::getInstance(_engine, choreSlotIds[3])->fadeOut(0.25f);

		choreSlotIds[3] = 0;
	} else if (!ChoreInstance::isInstanceValid(_engine, choreSlotIds[3])) {
		extra->resting = false;
		startChore(&startedId, choreSlotNames[3], choreSlotSetNames[3]);
		choreSlotIds[3] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, choreSlotIds[3])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[3]))
			ChoreInstance::getInstance(_engine, choreSlotIds[3])->setLooping(true);
	}

	if (!extra->walking && !extra->turnDir)
		extra->resting = true;

	extra->walking = false;
	extra->turnDir = 0;
}

} // End of namespace Grim
