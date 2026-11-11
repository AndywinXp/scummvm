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

#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/ltimeservices.h"
#include "engines/grim/emi/profilertimer.h"

namespace Grim {

ProfilerTimer *ProfilerTimer::oldest = nullptr;
ProfilerTimer *ProfilerTimer::newest = nullptr;
int ProfilerTimer::timerCount = 0;
ProfilerTimer *ProfilerTimer::callStack[ProfilerTimer::kMaxCallStack];
int ProfilerTimer::callStackDepth = 0;
int ProfilerTimer::frameCounter = 0;
int ProfilerTimer::snapshotInterval = 5;
uint64 ProfilerTimer::calibrationTicks = 0;
double ProfilerTimer::calibrationSeconds = 0.0;
double ProfilerTimer::secondsPerTick = 0.0;
int ProfilerTimer::secondsPerTickCalls = 0;
ProfilerTimer ProfilerTimer::rootTimer("Root", 0);

static Common::String padColumn(const char *text, int width) {
	Common::String column(text);

	while ((int)column.size() < width)
		column += ' ';

	return column;
}

ProfilerTimer::ProfilerTimer(const char *timerName, int timerFlags) {
	startTicks = 0;

	for (int i = 0; i < kMaxContexts; i++) {
		ticks[i] = 0;
		ticksShown[i] = 0;
	}

	Common::strlcpy(name, timerName, kNameSize);

	next = newest;

	if (newest)
		newest->prev = this;

	prev = nullptr;
	newest = this;

	if (!oldest)
		oldest = this;

	timerCount++;
	flags = timerFlags;
	numContexts = 0;
	contextsWarned = false;
	currentContext = -1;
}

ProfilerTimer::~ProfilerTimer() {
	bool unlinked = false;

	if (this == oldest) {
		ProfilerTimer *timer = oldest;

		oldest = oldest->prev;

		if (oldest)
			oldest->next = nullptr;
		else
			newest = nullptr;

		timer->next = nullptr;
		timer->prev = nullptr;
		unlinked = true;
	} else if (this == newest) {
		ProfilerTimer *timer = newest;

		newest = timer->next;

		if (newest)
			newest->prev = nullptr;
		else
			oldest = nullptr;

		timer->next = nullptr;
		timer->prev = nullptr;
		unlinked = true;
	} else if (prev || next) {
		prev->next = next;
		next->prev = prev;
		unlinked = true;
	}

	if (unlinked) {
		timerCount--;
		next = nullptr;
		prev = nullptr;
	}
}

void ProfilerTimer::start() {
	// Not in the disasm, the timers only feed the statistics print of the debug build
	if (gDebugLevel < 1)
		return;

	if ((flags & kTrackNesting) && callStackDepth > 0) {
		ProfilerTimer *top = callStack[callStackDepth - 1];
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
			parent[0] = &rootTimer;
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
		if (callStackDepth < kMaxCallStack) {
			callStack[callStackDepth] = this;
			callStackDepth++;
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
			callStackDepth--;

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
	double millis = getSecondsPerTick() * ((double)(int64)ticksShown[context] / frameCount) * 1000.0;
	double callsPerFrame = (double)callsShown[context] / frameCount;
	double percent = 100.0;

	if (parentMillis == 0.0)
		parentMillis = millis;

	if (parentTimer)
		percent = millis / parentMillis * 100.0;

	Common::String indent = padColumn("", 2 * depth);
	Common::String label = padColumn(name, 45 - 2 * depth);

	gpConsoleServices->logStatus("%s%s %6.02fms %4.1f %4.1f\n", indent.c_str(), label.c_str(), millis, percent, callsPerFrame);

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

		double otherMillis = millis - getSecondsPerTick() * ((double)(int64)childTicks / (double)framesShown[context]) * 1000.0;

		if (parentTimer)
			percent = otherMillis / parentMillis * 100.0;

		Common::String otherIndent = padColumn("", 2 * depth + 2);
		Common::String otherLabel = padColumn("Other", 43 - 2 * depth);

		gpConsoleServices->logStatus("%s%s %6.02fms %4.1f\n", otherIndent.c_str(), otherLabel.c_str(), otherMillis, percent);
	}
}

void ProfilerTimer::endFrame() {
	for (ProfilerTimer *timer = oldest; timer; timer = timer->prev) {
		for (int i = 0; i < timer->numContexts; i++)
			timer->frames[i]++;
	}

	frameCounter++;

	if (frameCounter >= snapshotInterval) {
		for (ProfilerTimer *timer = oldest; timer; timer = timer->prev)
			timer->snapshot();

		if (snapshotInterval) {
			for (ProfilerTimer *timer = oldest; timer; timer = timer->prev)
				timer->reset();
		}

		frameCounter = 0;
	}
}

void ProfilerTimer::printAllStats() {
	gpConsoleServices->logStatus("===============PERFORMANCE STATISTICS===============================\n");
	gpConsoleServices->logStatus("Name                                             Time  Percent Count\n");

	linkChildren();

	Common::List<ProfilerTimer *>::iterator child;

	for (child = rootTimer.children.begin(); child != rootTimer.children.end(); ++child)
		(*child)->printStats(&rootTimer, 0, 0.0, 0);
}

void ProfilerTimer::linkChildren() {
	rootTimer.children.clear();

	for (ProfilerTimer *timer = oldest; timer; timer = timer->prev)
		timer->children.clear();

	for (ProfilerTimer *timer = oldest; timer; timer = timer->prev) {
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

double ProfilerTimer::getSecondsPerTick() {
	if (!(secondsPerTickCalls % 64)) {
		int64 elapsedTicks = (int64)(g_system->getMillis() - calibrationTicks);
		LTimeStamp now;

		now.stamp();

		double elapsedSeconds = now.seconds - calibrationSeconds;

		// The original could divide by zero here, let's avoid that...
		if ((double)elapsedTicks != 0.0)
			secondsPerTick = elapsedSeconds / (double)elapsedTicks;
	}

	secondsPerTickCalls++;

	return secondsPerTick;
}

} // End of namespace Grim
