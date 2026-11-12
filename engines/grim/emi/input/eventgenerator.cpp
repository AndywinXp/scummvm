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
#include "engines/grim/emi/input/eventgenerator.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

EventGenerator::EventGenerator(EmiEngine *engine, int generatorPriority) : _priority(generatorPriority), _engine(engine) {
	Common::List<EventGenerator *>::iterator it = _engine->_eventGenerators.begin();

	while (it != _engine->_eventGenerators.end() && (*it)->_priority >= generatorPriority)
		++it;

	_engine->_eventGenerators.insert(it, this);
}

EventGenerator::~EventGenerator() {
	Common::List<EventGenerator *>::iterator it = _engine->_eventGenerators.begin();

	while (it != _engine->_eventGenerators.end()) {
		if (*it == this)
			it = _engine->_eventGenerators.erase(it);
		else
			++it;
	}
}

void *EventGenerator::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void EventGenerator::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void EventGenerator::clearEvents() {
	clearEventQueue(_engine);
}

void EventGenerator::postEvent(EmiEngine *engine, Event *event) {
	engine->_eventQueue.push_back(event);
}

void EventGenerator::dispatchEvents(EmiEngine *engine) {
	Common::List<EventGenerator *>::iterator generatorIt;

	for (generatorIt = engine->_eventGenerators.begin(); generatorIt != engine->_eventGenerators.end(); ++generatorIt)
		(*generatorIt)->poll();

	if (engine->_eventQueue.empty())
		return;

	bool handled;

	do {
		Event *event = engine->_eventQueue.front();

		engine->_eventQueue.pop_front();

		handled = false;

		Common::List<EventHandler *>::iterator handlerIt;

		for (handlerIt = engine->_eventHandlers.begin(); handlerIt != engine->_eventHandlers.end(); ++handlerIt) {
			EventHandler *handler = *handlerIt;

			if ((handler->_typeMask & (uint)event->type) != 0 && handler->handleEvent(event))
				handled = true;
		}

		delete event;
	} while (!handled && !engine->_eventQueue.empty());
}

void EventGenerator::clearEventQueue(EmiEngine *engine) {
	Common::List<Event *>::iterator it;

	for (it = engine->_eventQueue.begin(); it != engine->_eventQueue.end(); ++it)
		delete *it;

	engine->_eventQueue.clear();
}

} // End of namespace Grim
