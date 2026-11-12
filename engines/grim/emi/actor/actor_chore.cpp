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
	Common::List<uint>::iterator it = _choreIds.begin();

	while (it != _choreIds.end()) {
		uint choreId = *it;
		Common::List<uint>::iterator next = it;

		++next;

		if (ChoreInstance::isInstanceValid(_engine, choreId)) {
			if (!ChoreInstance::isInstanceValid(_engine, choreId) || ChoreInstance::getInstance(_engine, choreId)->isFinished()) {
				Common::List<uint>::iterator removeIt = _choreIds.begin();

				while (removeIt != _choreIds.end()) {
					if (*removeIt == choreId) {
						if (removeIt == next) {
							warning("Actor::removeFinishedChores(): the chore id is listed twice");
							++next;
						}

						removeIt = _choreIds.erase(removeIt);
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
			Common::List<uint>::iterator removeIt = _choreIds.begin();

			while (removeIt != _choreIds.end()) {
				if (*removeIt == choreId) {
					if (removeIt == next) {
						warning("Actor::removeFinishedChores(): the chore id is listed twice");
						++next;
					}

					removeIt = _choreIds.erase(removeIt);
				} else {
					++removeIt;
				}
			}
		}

		it = next;
	}
}

uint *Actor::findChoreInstance(uint *outId, uint previousId) {
	Common::List<uint>::iterator it = _choreIds.begin();

	if (previousId && it != _choreIds.end()) {
		uint value;

		do {
			value = *it;
			++it;
		} while (previousId != value && it != _choreIds.end());
	}

	if (it == _choreIds.end())
		*outId = 0;
	else
		*outId = *it;

	return outId;
}

uint *Actor::startChore(uint *outId, const LString &choreName, const LString &choreSetName) {
	uint newId = ChoreInstance::newInstance(_engine, this, choreName, choreSetName);

	if (ChoreInstance::isInstanceValid(_engine, newId))
		_choreIds.push_back(newId);

	*outId = newId;

	return outId;
}

void Actor::stopChores(bool skipLooping) {
	Common::List<uint>::iterator it;

	for (it = _choreIds.begin(); it != _choreIds.end(); ++it) {
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

	if (!_choreSlotNames[slot].empty()) {
		if (choreName == _choreSlotNames[slot] && choreSetName == _choreSlotSetNames[slot])
			return;

		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[slot])) {
			ChoreInstance::getInstance(_engine, _choreSlotIds[slot])->stop();

			uint startedId;

			startChore(&startedId, choreName, choreSetName);
			_choreSlotIds[slot] = startedId;

			if (ChoreInstance::isInstanceValid(_engine, startedId))
				ChoreInstance::getInstance(_engine, _choreSlotIds[slot])->setPlaying();

			if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[slot]))
				ChoreInstance::getInstance(_engine, _choreSlotIds[slot])->setLooping(true);
		}
	}

	_choreSlotNames[slot] = choreName;
	_choreSlotSetNames[slot] = choreSetName;
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

	if (!ChoreInstance::isInstanceValid(_engine, _choreSlotIds[0])) {
		startChore(&startedId, _choreSlotNames[0], _choreSlotSetNames[0]);
		_choreSlotIds[0] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, _choreSlotIds[0])->setPlaying();

		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[0]))
			ChoreInstance::getInstance(_engine, _choreSlotIds[0])->setLooping(true);
	}

	if (!_extra)
		return;

	if (_extra->_walking) {
		_extra->_turnDir = 0;

		if (!ChoreInstance::isInstanceValid(_engine, _choreSlotIds[1])) {
			_extra->_resting = false;
			startChore(&startedId, _choreSlotNames[1], _choreSlotSetNames[1]);
			_choreSlotIds[1] = startedId;

			if (ChoreInstance::isInstanceValid(_engine, startedId))
				ChoreInstance::getInstance(_engine, _choreSlotIds[1])->fadeIn(0.25f);

			if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[1]))
				ChoreInstance::getInstance(_engine, _choreSlotIds[1])->setLooping(true);
		}
	} else if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[1]) && (_mode & 1) == 0) {
		ChoreInstance::getInstance(_engine, _choreSlotIds[1])->fadeOut(0.25f);
		_choreSlotIds[1] = 0;
	}

	if (_extra->_turnDir <= 0) {
		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[2]))
			ChoreInstance::getInstance(_engine, _choreSlotIds[2])->fadeOut(0.25f);

		_choreSlotIds[2] = 0;
	} else if (!ChoreInstance::isInstanceValid(_engine, _choreSlotIds[2])) {
		_extra->_resting = false;
		startChore(&startedId, _choreSlotNames[2], _choreSlotSetNames[2]);
		_choreSlotIds[2] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, _choreSlotIds[2])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[2]))
			ChoreInstance::getInstance(_engine, _choreSlotIds[2])->setLooping(true);
	}

	if (_extra->_turnDir >= 0) {
		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[3]))
			ChoreInstance::getInstance(_engine, _choreSlotIds[3])->fadeOut(0.25f);

		_choreSlotIds[3] = 0;
	} else if (!ChoreInstance::isInstanceValid(_engine, _choreSlotIds[3])) {
		_extra->_resting = false;
		startChore(&startedId, _choreSlotNames[3], _choreSlotSetNames[3]);
		_choreSlotIds[3] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, _choreSlotIds[3])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(_engine, _choreSlotIds[3]))
			ChoreInstance::getInstance(_engine, _choreSlotIds[3])->setLooping(true);
	}

	if (!_extra->_walking && !_extra->_turnDir)
		_extra->_resting = true;

	_extra->_walking = false;
	_extra->_turnDir = 0;
}

} // End of namespace Grim
