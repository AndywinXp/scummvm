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

#include "engines/grim/emi/eventgenerator.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

Common::List<EventGenerator *> EventGenerator::generators;
Common::List<Event *> EventGenerator::eventQueue;

EventGenerator::EventGenerator(int generatorPriority) : priority(generatorPriority) {
	Common::List<EventGenerator *>::iterator it = generators.begin();

	while (it != generators.end() && (*it)->priority >= generatorPriority)
		++it;

	generators.insert(it, this);
}

EventGenerator::~EventGenerator() {
	Common::List<EventGenerator *>::iterator it = generators.begin();

	while (it != generators.end()) {
		if (*it == this)
			it = generators.erase(it);
		else
			++it;
	}
}

void *EventGenerator::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void EventGenerator::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void EventGenerator::clearEvents() {
	clearEventQueue();
}

void EventGenerator::postEvent(Event *event) {
	eventQueue.push_back(event);
}

void EventGenerator::dispatchEvents() {
	Common::List<EventGenerator *>::iterator generatorIt;

	for (generatorIt = generators.begin(); generatorIt != generators.end(); ++generatorIt)
		(*generatorIt)->poll();

	if (eventQueue.empty())
		return;

	bool handled;

	do {
		Event *event = eventQueue.front();

		eventQueue.pop_front();

		handled = false;

		Common::List<EventHandler *>::iterator handlerIt;

		for (handlerIt = EventHandler::handlers.begin(); handlerIt != EventHandler::handlers.end(); ++handlerIt) {
			EventHandler *handler = *handlerIt;

			if ((handler->typeMask & (uint)event->type) != 0 && handler->handleEvent(event))
				handled = true;
		}

		delete event;
	} while (!handled && !eventQueue.empty());
}

void EventGenerator::clearEventQueue() {
	Common::List<Event *>::iterator it;

	for (it = eventQueue.begin(); it != eventQueue.end(); ++it)
		delete *it;

	eventQueue.clear();
}

} // End of namespace Grim
