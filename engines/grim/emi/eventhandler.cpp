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

#include "engines/grim/emi/eventhandler.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

Common::List<EventHandler *> EventHandler::handlers;

void *Event::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void Event::operator delete(void *block) {
	gpMemoryServices->free(block);
}

EventHandler::EventHandler(int handlerPriority) : typeMask(0xFFFFFFFF), priority(handlerPriority) {
	Common::List<EventHandler *>::iterator it = handlers.begin();

	while (it != handlers.end() && (*it)->priority >= handlerPriority)
		++it;

	handlers.insert(it, this);
}

EventHandler::~EventHandler() {
	Common::List<EventHandler *>::iterator it = handlers.begin();

	while (it != handlers.end()) {
		if (*it == this)
			it = handlers.erase(it);
		else
			++it;
	}
}

void *EventHandler::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void EventHandler::operator delete(void *block) {
	gpMemoryServices->free(block);
}

} // End of namespace Grim
