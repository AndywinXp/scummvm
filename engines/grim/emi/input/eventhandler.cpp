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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/input/eventhandler.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

void *Event::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Event::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

EventHandler::EventHandler(EmiEngine *engine, int handlerPriority) : _typeMask(0xFFFFFFFF), _priority(handlerPriority), _engine(engine) {
	Common::List<EventHandler *>::iterator it = _engine->_eventHandlers.begin();

	while (it != _engine->_eventHandlers.end() && (*it)->_priority >= handlerPriority)
		++it;

	_engine->_eventHandlers.insert(it, this);
}

EventHandler::~EventHandler() {
	Common::List<EventHandler *>::iterator it = _engine->_eventHandlers.begin();

	while (it != _engine->_eventHandlers.end()) {
		if (*it == this)
			it = _engine->_eventHandlers.erase(it);
		else
			++it;
	}
}

void *EventHandler::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void EventHandler::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

} // End of namespace Grim
