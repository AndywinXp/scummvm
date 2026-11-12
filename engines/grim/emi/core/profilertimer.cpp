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
	_startTicks = 0;

	for (int i = 0; i < kMaxContexts; i++) {
		_ticks[i] = 0;
		_ticksShown[i] = 0;
	}

	Common::strlcpy(_name, timerName, kNameSize);

	_next = _engine->_profilerNewest;

	if (_engine->_profilerNewest)
		_engine->_profilerNewest->_prev = this;

	_prev = nullptr;
	_engine->_profilerNewest = this;

	if (!_engine->_profilerOldest)
		_engine->_profilerOldest = this;

	_engine->_profilerTimerCount++;
	_flags = timerFlags;
	_numContexts = 0;
	_contextsWarned = false;
	_currentContext = -1;
}

ProfilerTimer::~ProfilerTimer() {
	bool unlinked = false;

	if (this == _engine->_profilerOldest) {
		ProfilerTimer *timer = _engine->_profilerOldest;

		_engine->_profilerOldest = _engine->_profilerOldest->_prev;

		if (_engine->_profilerOldest)
			_engine->_profilerOldest->_next = nullptr;
		else
			_engine->_profilerNewest = nullptr;

		timer->_next = nullptr;
		timer->_prev = nullptr;
		unlinked = true;
	} else if (this == _engine->_profilerNewest) {
		ProfilerTimer *timer = _engine->_profilerNewest;

		_engine->_profilerNewest = timer->_next;

		if (_engine->_profilerNewest)
			_engine->_profilerNewest->_prev = nullptr;
		else
			_engine->_profilerOldest = nullptr;

		timer->_next = nullptr;
		timer->_prev = nullptr;
		unlinked = true;
	} else if (_prev || _next) {
		_prev->_next = _next;
		_next->_prev = _prev;
		unlinked = true;
	}

	if (unlinked) {
		_engine->_profilerTimerCount--;
		_next = nullptr;
		_prev = nullptr;
	}
}

void ProfilerTimer::start() {
	// Not in the disasm, the timers only feed the statistics print of the debug build
	if (gDebugLevel < 1)
		return;

	if ((_flags & kTrackNesting) && _engine->_profilerCallStackDepth > 0) {
		ProfilerTimer *top = _engine->_profilerCallStack[_engine->_profilerCallStackDepth - 1];
		int context = 0;

		while (context < _numContexts) {
			if (top == _parent[context] && top->_currentContext == _parentContext[context])
				break;

			context++;
		}

		if (context == _numContexts) {
			if (_numContexts < kMaxContexts) {
				_parent[context] = top;
				_parentContext[context] = top->_currentContext;
				_ticks[context] = 0;
				_calls[context] = 0;
				_frames[context] = 0;
				_ticksShown[context] = 0;
				_callsShown[context] = 0;
				_framesShown[context] = 0;
				_numContexts++;
			} else {
				if (!_contextsWarned) {
					warning("ProfilerTimer::start(): too many contexts for '%s'", _name);
					_contextsWarned = true;
				}

				context = kMaxContexts - 1;
			}
		}

		_currentContext = context;
	} else {
		if (!_numContexts) {
			_ticks[0] = 0;
			_ticksShown[0] = 0;
			_parent[0] = &_engine->_rootTimer;
			_parentContext[0] = 0;
			_calls[0] = 0;
			_frames[0] = 0;
			_callsShown[0] = 0;
			_framesShown[0] = 0;
			_numContexts = 1;
		}

		_currentContext = 0;
	}

	if (_flags & kTrackNesting) {
		if (_engine->_profilerCallStackDepth < kMaxCallStack) {
			_engine->_profilerCallStack[_engine->_profilerCallStackDepth] = this;
			_engine->_profilerCallStackDepth++;
		}
	}

	_startTicks = g_system->getMillis();
}

void ProfilerTimer::stop() {
	uint64 now = g_system->getMillis();

	if (_currentContext != -1) {
		_ticks[_currentContext] += now - _startTicks;
		_calls[_currentContext]++;

		if (_flags & kTrackNesting)
			_engine->_profilerCallStackDepth--;

		_currentContext = -1;
	}
}

void ProfilerTimer::snapshot() {
	for (int i = 0; i < _numContexts; i++) {
		_ticksShown[i] = _ticks[i];
		_callsShown[i] = _calls[i];
		_framesShown[i] = _frames[i];
	}
}

void ProfilerTimer::reset() {
	for (int i = 0; i < _numContexts; i++) {
		_ticks[i] = 0;
		_calls[i] = 0;
		_frames[i] = 0;
	}
}

void ProfilerTimer::printStats(ProfilerTimer *parentTimer, int parentContextIndex, double parentMillis, int depth) {
	int context = 0;

	while (context < _numContexts) {
		if (parentTimer == _parent[context] && parentContextIndex == _parentContext[context])
			break;

		context++;
	}

	if (context == _numContexts || !_framesShown[context])
		return;

	double frameCount = (double)_framesShown[context];
	double millis = getSecondsPerTick(_engine) * ((double)(int64)_ticksShown[context] / frameCount) * 1000.0;
	double callsPerFrame = (double)_callsShown[context] / frameCount;
	double percent = 100.0;

	if (parentMillis == 0.0)
		parentMillis = millis;

	if (parentTimer)
		percent = millis / parentMillis * 100.0;

	Common::String indent = padColumn("", 2 * depth);
	Common::String label = padColumn(_name, 45 - 2 * depth);

	_engine->getConsoleServices()->logStatus("%s%s %6.02fms %4.1f %4.1f\n", indent.c_str(), label.c_str(), millis, percent, callsPerFrame);

	Common::List<ProfilerTimer *>::iterator child;

	for (child = _children.begin(); child != _children.end(); ++child)
		(*child)->printStats(this, context, parentMillis, depth + 1);

	if (_flags & kPrintOther) {
		uint64 childTicks = 0;

		for (child = _children.begin(); child != _children.end(); ++child) {
			ProfilerTimer *childTimer = *child;

			for (int i = 0; i < childTimer->_numContexts; i++) {
				if (this == childTimer->_parent[i] && context == childTimer->_parentContext[i])
					childTicks += childTimer->_ticksShown[i];
			}
		}

		double otherMillis = millis - getSecondsPerTick(_engine) *((double)(int64)childTicks / (double)_framesShown[context]) * 1000.0;

		if (parentTimer)
			percent = otherMillis / parentMillis * 100.0;

		Common::String otherIndent = padColumn("", 2 * depth + 2);
		Common::String otherLabel = padColumn("Other", 43 - 2 * depth);

		_engine->getConsoleServices()->logStatus("%s%s %6.02fms %4.1f\n", otherIndent.c_str(), otherLabel.c_str(), otherMillis, percent);
	}
}

void ProfilerTimer::endFrame(EmiEngine *engine) {
	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->_prev) {
		for (int i = 0; i < timer->_numContexts; i++)
			timer->_frames[i]++;
	}

	engine->_profilerFrameCounter++;

	if (engine->_profilerFrameCounter >= engine->_profilerSnapshotInterval) {
		for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->_prev)
			timer->snapshot();

		if (engine->_profilerSnapshotInterval) {
			for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->_prev)
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

	for (child = engine->_rootTimer._children.begin(); child != engine->_rootTimer._children.end(); ++child)
		(*child)->printStats(&engine->_rootTimer, 0, 0.0, 0);
}

void ProfilerTimer::linkChildren(EmiEngine *engine) {
	engine->_rootTimer._children.clear();

	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->_prev)
		timer->_children.clear();

	for (ProfilerTimer *timer = engine->_profilerOldest; timer; timer = timer->_prev) {
		for (int i = 0; i < timer->_numContexts; i++) {
			ProfilerTimer *parentTimer = timer->_parent[i];
			Common::List<ProfilerTimer *>::iterator child = parentTimer->_children.begin();

			while (child != parentTimer->_children.end() && *child != timer)
				++child;

			if (child == parentTimer->_children.end())
				parentTimer->_children.push_back(timer);
		}
	}
}

double ProfilerTimer::getSecondsPerTick(EmiEngine *engine) {
	if (!(engine->_profilerSecondsPerTickCalls % 64)) {
		int64 elapsedTicks = (int64)(g_system->getMillis() - engine->_profilerCalibrationTicks);
		LTimeStamp now;

		now.stamp();

		double elapsedSeconds = now._seconds - engine->_profilerCalibrationSeconds;

		// The original could divide by zero here, let's avoid that...
		if ((double)elapsedTicks != 0.0)
			engine->_profilerSecondsPerTick = elapsedSeconds / (double)elapsedTicks;
	}

	engine->_profilerSecondsPerTickCalls++;

	return engine->_profilerSecondsPerTick;
}

} // End of namespace Grim
