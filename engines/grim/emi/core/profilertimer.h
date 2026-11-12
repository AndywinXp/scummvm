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
#include "common/str.h"

namespace Grim {

class EmiEngine;

class ProfilerTimer {
public:
	enum {
		kMaxContexts = 32,
		kMaxCallStack = 64,
		kNameSize = 32
	};

	enum {
		kTrackNesting = 1,
		kPrintOther = 2
	};

	ProfilerTimer(EmiEngine *engine, const char *timerName, int timerFlags);
	~ProfilerTimer();

	void start();
	void stop();
	void snapshot();
	void reset();
	void printStats(ProfilerTimer *parentTimer, int parentContextIndex, double parentMillis, int depth);

	static void endFrame(EmiEngine *engine);
	static void printAllStats(EmiEngine *engine);
	static void linkChildren(EmiEngine *engine);
	static double getSecondsPerTick(EmiEngine *engine);

	uint64 _startTicks;
	uint64 _ticks[kMaxContexts];
	uint64 _ticksShown[kMaxContexts];
	ProfilerTimer *_parent[kMaxContexts];
	int _parentContext[kMaxContexts];
	int _calls[kMaxContexts];
	int _frames[kMaxContexts];
	int _callsShown[kMaxContexts];
	int _framesShown[kMaxContexts];
	int _numContexts;
	bool _contextsWarned;
	int _currentContext;
	int _flags;
	char _name[kNameSize];
	Common::List<ProfilerTimer *> _children;

	// next points to the older timer, prev to the newer one
	ProfilerTimer *_next;
	ProfilerTimer *_prev;

private:
	static Common::String padColumn(const char *text, int width);

	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
