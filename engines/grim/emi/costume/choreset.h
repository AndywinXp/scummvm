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

#ifndef GRIM_EMI_CHORESET_H
#define GRIM_EMI_CHORESET_H

#include "common/array.h"
#include "common/list.h"

#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/resources/lresource.h"

namespace Grim {

class EmiEngine;

struct ChoreKey {
	void *operator new(size_t size);
	void operator delete(void *block);

	float time;
	float value;
};

class ChoreTrack {
public:
	ChoreTrack();
	~ChoreTrack();

	void *operator new(size_t size);
	void operator delete(void *block);

	int getSize();
	void addKey(ChoreKey *key);

	int _id;
	int _parentId;
	LString _resourceName;
	Common::List<ChoreKey *> _keys;
};

class Chore {
public:
	explicit Chore(EmiEngine *engine);
	~Chore();

	void *operator new(size_t size);
	void operator delete(void *block);

	int getSize();
	ChoreTrack *addTrack(const LString &trackResourceName);
	int findTrackIndex(int trackId);
	void lockComponents();
	void unlockComponents();

	LString _name;
	Common::Array<ChoreTrack *> _tracks;
	float _length;

private:
	EmiEngine *_engine;
};

class ChoreSet : public LResource {
public:
	enum {
		kLineBufferSize = 256
	};

	ChoreSet();
	virtual ~ChoreSet();

	virtual uint resSizeOfItem();

	Chore *findChore(const LString &choreName);
	bool loadText(const char *path);
	bool parseSection(LFile *file, const char *sectionName);
	bool parseChores(LFile *file);
	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	Common::Array<Chore *> _chores;

protected:
	virtual bool resLoadItem(const char *itemName);

private:
	void logChoreError(const char *format, const char *argument);
};

} // End of namespace Grim

#endif
