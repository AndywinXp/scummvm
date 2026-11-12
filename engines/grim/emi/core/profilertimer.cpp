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

#include "common/debug.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/ltimeservices.h"
#include "engines/grim/emi/core/profilertimer.h"

namespace Grim {

Common::String ProfilerTimer::padColumn(const char *text, int width) {
	Common::String column(text);

	while ((int)column.size() < width)
		column += ' ';

	return column;
}

ProfilerTimer::ProfilerTimer(EmiEngine *engine, const char *timerName, int timerFlags) {
	_engine = engine;
	startTicks = 0;

	for (int i = 0; i < kMaxContexts; i++) {
		ticks[i] = 0;
		ticksShown[i] = 0;
	}

	Common::strlcpy(name, timerName, kNameSize);

	next = _engine->_profilerNewest;

	if (_engine->_profilerNewest)
		_engine->_profilerNewest->prev = this;

	prev = nullptr;
	_engine->_profilerNewest = this;

	if (!_engine->_profilerOldest)
		_engine->_profilerOldest = this;

	_engine->_profilerTimerCount++;
	flags = timerFlags;
	numContexts = 0;
	contextsWarned = false;
	currentContext = -1;
}

ProfilerTimer::~ProfilerTimer() {
	bool unlinked = false;

	if (this == _engine->_profilerOldest) {
		ProfilerTimer *timer = _engine->_profilerOldest;

		_engine->_profilerOldest = _engine->_profilerOldest->prev;

		if (_engine->_profilerOldest)
			_engine->_profilerOldest->next = nullptr;
		else
			_engine->_profilerNewest = nullptr;

		timer->next = nullptr;
		timer->prev = nullptr;
		unlinked = true;
	} else if (this == _engine->_profilerNewest) {
		ProfilerTimer *timer = _engine->_profilerNewest;

		_engine->_profilerNewest = timer->next;

		if (_engine->_profilerNewest)
			_engine->_profilerNewest->prev = nullptr;
		else
			_engine->_profilerOldest = nullptr;

		timer->next = nullptr;
		timer->prev = nullptr;
		unlinked = true;
	} else if (prev || next) {
		prev->next = next;
		next->prev = prev;
		unlinked = true;
	}

	if (unlinked) {
		_engine->_profilerTimerCount--;
		next = nullptr;
		prev = nullptr;
	}
}

void ProfilerTimer::start() {
	// Not in the disasm, the timers only feed the statistics print of the debug build
	if (gDebugLevel < 1)
		return;

	if ((flags & kTrackNesting) && _engine->_profilerCallStackDepth > 0) {
		ProfilerTimer *top = _engine->_profilerCallStack[_engine->_profilerCallStackDepth - 1];
		int context = 0;

		while (context < numContexts) {
			if (top == parent[context] && top->currentContext == parentContext[context])
				break;

			context++;
		}

		if (context == numContexts) {
			if (numContexts < kMaxContexts) {
				parent[context] = top;
				parentContext[context] = top->currentContext;
				ticks[context] = 0;
				calls[context] = 0;
				frames[context] = 0;
				ticksShown[context] = 0;
				callsShown[context] = 0;
				framesShown[context] = 0;
				numContexts++;
			} else {
				if (!contextsWarned) {
					warning("ProfilerTimer::start(): too many contexts for '%s'", name);
					contextsWarned = true;
				}

				context = kMaxContexts - 1;
			}
		}

		currentContext = context;
	} else {
		if (!numContexts) {
			ticks[0] = 0;
			ticksShown[0] = 0;
			parent[0] = &_engine->_rootTimer;
			parentContext[0] = 0;
			calls[0] = 0;
			frames[0] = 0;
			callsShown[0] = 0;
			framesShown[0] = 0;
			numContexts = 1;
		}

		currentContext = 0;
	}

	if (flags & kTrackNesting) {
		if (_engine->_profilerCallStackDepth < kMaxCallStack) {
			_engine->_profilerCallStack[_engine->_profilerCallStackDepth] = this;
			_engine->_profilerCallStackDepth++;
		}
	}

	startTicks = g_system->getMillis();
}

void ProfilerTimer::stop() {
	uint64 now = g_system->getMillis();

	if (currentContext != -1) {
		ticks[currentContext] += now - startTicks;
		calls[currentContext]++;

		if (flags & kTrackNesting)
			_engine->_profilerCallStackDepth--;

		currentContext = -1;
	}
}

void ProfilerTimer::snapshot() {
	for (int i = 0; i < numContexts; i++) {
		ticksShown[i] = ticks[i];
		callsShown[i] = calls[i];
		framesShown[i] = frames[i];
	}
}

void ProfilerTimer::reset() {
	for (int i = 0; i < numContexts; i++) {
		ticks[i] = 0;
		calls[i] = 0;
		frames[i] = 0;
	}
}

void ProfilerTimer::printStats(ProfilerTimer *parentTimer, int parentContextIndex, double parentMillis, int depth) {
	int context = 0;

	while (context < numContexts) {
		if (parentTimer == parent[context] && parentContextIndex == parentContext[context])
			break;

		context++;
	}

	if (context == numContexts || !framesShown[context])
		return;

	double frameCount = (double)framesShown[context];
	double millis = getSecondsPerTick(_engine) * ((double)(int64)ticksShown[context] / frameCount) * 1000.0;
	double callsPerFrame = (double)callsShown[context] / frameCount;
	double percent = 100.0;

	if (parentMillis == 0.0)
		parentMillis = millis;

	if (parentTimer)
		percent = millis / parentMillis * 100.0;

	Common::String indent = padColumn("", 2 * depth);
	Common::String label = padColumn(name, 45 - 2 * depth);

	_engine->getConsoleServices()->logStatus("%s%s %6.02fms %4.1f %4.1f\n", indent.c_str(), label.c_str(), millis, percent, callsPerFrame);

	Common::List<ProfilerTimer *>::iterator child;

	for (child = children.begin(); child != children.end(); ++child)
		(*child)->printStats(this, context, parentMillis, depth + 1);

	if (flags & kPrintOther) {
		uint64 childTicks = 0;

		for (child = children.begin(); child != children.end(); ++child) {
			ProfilerTimer *childTimer = *child;

			for (int i = 0; i < childTimer->numContexts; i++) {
				if (this == childTimer->parent[i] && context == childTimer->parentContext[i])
					childTicks += childTimer->ticksShown[i];
			}
		}

		double otherMillis = millis - getSecondsPerTick(_engine) *((double)(int64)childTicks / (double)framesShown[context]) * 1000.0;

		if (parentTimer)
			percent = otherMillis / parentMillis * 100.0;

		Common::String otherIndent = padColumn("", 2 * depth + 2);
		Common::String otherLabel = padColumn("Other", 43 - 2 * depth);

		_engine->getConsoleServices()->logStatus("%s%s %6.02fms %4.1f\n", otherIndent.c_str(), otherLabel.c_str(), otherMillis, percent);
	}
}

void ProfilerTimer::endFrame(EmiEngine *engine) {
	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->prev) {
		for (int i = 0; i < timer->numContexts; i++)
			timer->frames[i]++;
	}

	engine->_profilerFrameCounter++;

	if (engine->_profilerFrameCounter >= engine->_profilerSnapshotInterval) {
		for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->prev)
			timer->snapshot();

		if (engine->_profilerSnapshotInterval) {
			for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->prev)
				timer->reset();
		}

		engine->_profilerFrameCounter = 0;
	}
}

void ProfilerTimer::printAllStats(EmiEngine *engine) {
	engine->getConsoleServices()->logStatus("===============PERFORMANCE STATISTICS===============================\n");
	engine->getConsoleServices()->logStatus("Name                                             Time  Percent Count\n");

	linkChildren(engine);

	Common::List<ProfilerTimer *>::iterator child;

	for (child = engine->_rootTimer.children.begin(); child != engine->_rootTimer.children.end(); ++child)
		(*child)->printStats(&engine->_rootTimer, 0, 0.0, 0);
}

void ProfilerTimer::linkChildren(EmiEngine *engine) {
	engine->_rootTimer.children.clear();

	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->prev)
		timer->children.clear();

	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->prev) {
		for (int i = 0; i < timer->numContexts; i++) {
			ProfilerTimer *parentTimer = timer->parent[i];
			Common::List<ProfilerTimer *>::iterator child = parentTimer->children.begin();

			while (child != parentTimer->children.end() && *child != timer)
				++child;

			if (child == parentTimer->children.end())
				parentTimer->children.push_back(timer);
		}
	}
}

double ProfilerTimer::getSecondsPerTick(EmiEngine *engine) {
	if (!(engine->_profilerSecondsPerTickCalls % 64)) {
		int64 elapsedTicks = (int64)(g_system->getMillis() - engine->_profilerCalibrationTicks);
		LTimeStamp now;

		now.stamp();

		double elapsedSeconds = now.seconds - engine->_profilerCalibrationSeconds;

		// The original could divide by zero here, let's avoid that...
		if ((double)elapsedTicks != 0.0)
			engine->_profilerSecondsPerTick = elapsedSeconds / (double)elapsedTicks;
	}

	engine->_profilerSecondsPerTickCalls++;

	return engine->_profilerSecondsPerTick;
}

} // End of namespace Grim
