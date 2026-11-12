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

#ifndef GRIM_EMI_ANIMATION_H
#define GRIM_EMI_ANIMATION_H

#include "common/array.h"

#include "engines/grim/emi/atrack.h"
#include "engines/grim/emi/lfile.h"

namespace Grim {

class EmiEngine;

struct AnimEntry {
	AnimEntry() : track(nullptr) {}

	LString name;
	ATrack *track;
};

class Animation {
public:
	explicit Animation(EmiEngine *engine);
	virtual ~Animation();

	virtual bool save(LFile *file);
	virtual bool restore(LFile *file);

	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	LString name;
	Common::Array<AnimEntry> entries;
	int trackCount;
	int entryCount;
	float length;

protected:
	EmiEngine *_engine;

private:
	bool failLoad(LFile *file, const char *reason);
};

} // End of namespace Grim

#endif
