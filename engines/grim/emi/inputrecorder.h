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

#ifndef GRIM_EMI_INPUTRECORDER_H
#define GRIM_EMI_INPUTRECORDER_H

#include "common/file.h"
#include "common/scummsys.h"
#include "common/stream.h"

#include "engines/grim/emi/sputcontrol_v2.h"

namespace Grim {

class InputRecorder {
public:
	enum {
		kModeOff = 0,
		kModeRecord = 1,
		kModeReplay = 2
	};

	enum {
		kRecordNone = 0,
		kRecordKeyUp = 1,
		kRecordKeyDown = 2,
		kRecordRoom = 3,
		kRecordFrame = 4,
		kRecordChar = 5
	};

	InputRecorder(int recorderMode);
	~InputRecorder();

	void *operator new(size_t size);
	void operator delete(void *block);

	void recordKey(const ControlEntry *control, bool down);
	void recordChar(byte character);
	void recordRoom(const char *roomName);
	void replayKeys();
	bool replayKey();
	bool replayChar();
	void readNext();
	float replayFrame();
	void recordFrame(float frameTime);

	static void createInputRecorder(int recorderMode);
	static void destroyInputRecorder();

	int mode;
	float dt;
	int keyCode;
	byte character;
	int lastRecordType;

	static InputRecorder *instance;

private:
	Common::DumpFile *_writeFile;
	Common::SeekableReadStream *_readFile;
};

} // End of namespace Grim

#endif
