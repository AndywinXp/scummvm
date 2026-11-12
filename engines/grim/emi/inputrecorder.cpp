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

#include "common/fs.h"
#include "common/path.h"
#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/inputrecorder.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/ltimeservices.h"
#include "engines/grim/emi/lua_v2.h"

namespace Grim {

static const char *const kReplayFileName = "Replay.log";

InputRecorder::InputRecorder(EmiEngine *engine, int recorderMode) : mode(recorderMode), dt(0.0f), keyCode(0), character(0),
		lastRecordType(kRecordNone), _engine(engine), _timeServices(new LTimeServices()), _writeFile(nullptr), _readFile(nullptr) {
	if (mode == kModeRecord) {
		_writeFile = new Common::DumpFile();

		if (!_writeFile->open(Common::Path(kReplayFileName, '/'))) {
			warning("InputRecorder::InputRecorder(): could not create %s", kReplayFileName);

			delete _writeFile;
			_writeFile = nullptr;
			mode = kModeOff;
		}
	} else if (mode == kModeReplay) {
		_readFile = Common::FSNode(Common::Path(kReplayFileName, '/')).createReadStream();

		if (!_readFile) {
			warning("InputRecorder::InputRecorder(): could not open %s", kReplayFileName);

			mode = kModeOff;
		} else {
			readNext();
		}
	}
}

InputRecorder::~InputRecorder() {
	delete _timeServices;
	_timeServices = nullptr;
	delete _writeFile;
	_writeFile = nullptr;
	delete _readFile;
	_readFile = nullptr;
}

void *InputRecorder::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void InputRecorder::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void InputRecorder::recordKey(const ControlEntry *control, bool down) {
	if (mode == kModeOff)
		return;

	if (mode == kModeReplay) {
		readNext();

		return;
	}

	_writeFile->writeByte(down ? 'D' : 'U');
	_writeFile->writeUint32LE(control->code);
	_writeFile->flush();
}

void InputRecorder::recordChar(byte value) {
	if (mode == kModeReplay) {
		readNext();
	} else if (mode == kModeRecord) {
		_writeFile->writeByte('C');
		_writeFile->writeByte(value);
		_writeFile->flush();
	}
}

void InputRecorder::recordRoom(const char *roomName) {
	if (mode == kModeOff)
		return;

	if (mode == kModeReplay) {
		readNext();

		return;
	}

	_writeFile->writeByte('R');
	_writeFile->flush();
}

void InputRecorder::replayKeys() {
	if (mode != kModeReplay)
		return;

	while (lastRecordType == kRecordKeyUp || lastRecordType == kRecordKeyDown)
		replayKey();
}

bool InputRecorder::replayKey() {
	return _engine->getLua()->buttonHandler(keyCode, lastRecordType == kRecordKeyDown);
}

bool InputRecorder::replayChar() {
	return _engine->onCharacter(character);
}

void InputRecorder::readNext() {
	if (!_readFile) {
		warning("InputRecorder::readNext(): the file is not open");

		return;
	}

	if (_readFile->eos()) {
		mode = kModeOff;
		lastRecordType = kRecordNone;

		return;
	}

	char tag = 0;

	// The original uses an uninitialized tag when the file ends here
	if (_readFile->read(&tag, 1) != 1) {
		warning("InputRecorder::readNext(): the file ended");

		mode = kModeOff;
		lastRecordType = kRecordNone;

		return;
	}

	switch (tag) {
	case 'C':
		lastRecordType = kRecordChar;
		character = _readFile->readByte();

		return;
	case 'D':
		lastRecordType = kRecordKeyDown;

		break;
	case 'F': {
		uint32 bits = _readFile->readUint32LE();

		memcpy(&dt, &bits, sizeof(dt));
		lastRecordType = kRecordFrame;

		return;
	}
	case 'R':
		lastRecordType = kRecordRoom;

		return;
	case 'U':
		lastRecordType = kRecordKeyUp;

		break;
	default:
		break;
	}

	keyCode = (int)_readFile->readUint32LE();
}

float InputRecorder::replayFrame() {
	while (lastRecordType == kRecordChar)
		replayChar();

	float frameTime = dt;

	readNext();

	return frameTime;
}

void InputRecorder::recordFrame(float frameTime) {
	if (mode != kModeRecord)
		return;

	uint32 bits;

	memcpy(&bits, &frameTime, sizeof(bits));

	_writeFile->writeByte('F');
	_writeFile->writeUint32LE(bits);
	_writeFile->flush();
}

} // End of namespace Grim
