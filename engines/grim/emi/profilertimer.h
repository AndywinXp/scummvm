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

#ifndef GRIM_EMI_PROFILERTIMER_H
#define GRIM_EMI_PROFILERTIMER_H

#include "common/list.h"

namespace Grim {

class ProfilerTimer {
public:
	enum {
		kMaxContexts = 8,
		kMaxCallStack = 64,
		kNameSize = 32
	};

	enum {
		kTrackNesting = 1,
		kPrintOther = 2
	};

	ProfilerTimer(const char *timerName, int timerFlags);
	~ProfilerTimer();

	void start();
	void stop();
	void snapshot();
	void reset();
	void printStats(ProfilerTimer *parentTimer, int parentContextIndex, double parentMillis, int depth);

	static void endFrame();
	static void printAllStats();
	static void linkChildren();
	static double getSecondsPerTick();

	uint64 startTicks;
	uint64 ticks[kMaxContexts];
	uint64 ticksShown[kMaxContexts];
	ProfilerTimer *parent[kMaxContexts];
	int parentContext[kMaxContexts];
	int calls[kMaxContexts];
	int frames[kMaxContexts];
	int callsShown[kMaxContexts];
	int framesShown[kMaxContexts];
	int numContexts;
	int currentContext;
	int flags;
	char name[kNameSize];
	Common::List<ProfilerTimer *> children;

	// next points to the older timer, prev to the newer one
	ProfilerTimer *next;
	ProfilerTimer *prev;

private:
	static ProfilerTimer *oldest;
	static ProfilerTimer *newest;
	static int timerCount;
	static ProfilerTimer *callStack[kMaxCallStack];
	static int callStackDepth;
	static int frameCounter;
	static int snapshotInterval;
	static ProfilerTimer rootTimer;
	static uint64 calibrationTicks;
	static double calibrationSeconds;
	static double secondsPerTick;
	static int secondsPerTickCalls;
};

} // End of namespace Grim

#endif
