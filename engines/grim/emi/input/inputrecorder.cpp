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
#include "engines/grim/emi/input/inputrecorder.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/core/ltimeservices.h"
#include "engines/grim/emi/lua_v2_opcodes/lua_v2.h"

namespace Grim {

static const char *const kReplayFileName = "Replay.log";

InputRecorder::InputRecorder(EmiEngine *engine, int recorderMode) : _mode(recorderMode), _dt(0.0f), _keyCode(0), _character(0),
		_lastRecordType(kRecordNone), _engine(engine), _timeServices(new LTimeServices()), _writeFile(nullptr), _readFile(nullptr) {
	if (_mode == kModeRecord) {
		_writeFile = new Common::DumpFile();

		if (!_writeFile->open(Common::Path(kReplayFileName, '/'))) {
			warning("InputRecorder::InputRecorder(): could not create %s", kReplayFileName);

			delete _writeFile;
			_writeFile = nullptr;
			_mode = kModeOff;
		}
	} else if (_mode == kModeReplay) {
		_readFile = Common::FSNode(Common::Path(kReplayFileName, '/')).createReadStream();

		if (!_readFile) {
			warning("InputRecorder::InputRecorder(): could not open %s", kReplayFileName);

			_mode = kModeOff;
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
	if (_mode == kModeOff)
		return;

	if (_mode == kModeReplay) {
		readNext();

		return;
	}

	_writeFile->writeByte(down ? 'D' : 'U');
	_writeFile->writeUint32LE(control->code);
	_writeFile->flush();
}

void InputRecorder::recordChar(byte value) {
	if (_mode == kModeReplay) {
		readNext();
	} else if (_mode == kModeRecord) {
		_writeFile->writeByte('C');
		_writeFile->writeByte(value);
		_writeFile->flush();
	}
}

void InputRecorder::recordRoom(const char *roomName) {
	if (_mode == kModeOff)
		return;

	if (_mode == kModeReplay) {
		readNext();

		return;
	}

	_writeFile->writeByte('R');
	_writeFile->flush();
}

void InputRecorder::replayKeys() {
	if (_mode != kModeReplay)
		return;

	while (_lastRecordType == kRecordKeyUp || _lastRecordType == kRecordKeyDown)
		replayKey();
}

bool InputRecorder::replayKey() {
	return _engine->getLua()->buttonHandler(_keyCode, _lastRecordType == kRecordKeyDown);
}

bool InputRecorder::replayChar() {
	return _engine->onCharacter(_character);
}

void InputRecorder::readNext() {
	if (!_readFile) {
		warning("InputRecorder::readNext(): the file is not open");

		return;
	}

	if (_readFile->eos()) {
		_mode = kModeOff;
		_lastRecordType = kRecordNone;

		return;
	}

	char tag = 0;

	// The original uses an uninitialized tag when the file ends here
	if (_readFile->read(&tag, 1) != 1) {
		warning("InputRecorder::readNext(): the file ended");

		_mode = kModeOff;
		_lastRecordType = kRecordNone;

		return;
	}

	switch (tag) {
	case 'C':
		_lastRecordType = kRecordChar;
		_character = _readFile->readByte();

		return;
	case 'D':
		_lastRecordType = kRecordKeyDown;

		break;
	case 'F': {
		uint32 bits = _readFile->readUint32LE();

		memcpy(&_dt, &bits, sizeof(_dt));
		_lastRecordType = kRecordFrame;

		return;
	}
	case 'R':
		_lastRecordType = kRecordRoom;

		return;
	case 'U':
		_lastRecordType = kRecordKeyUp;

		break;
	default:
		break;
	}

	_keyCode = (int)_readFile->readUint32LE();
}

float InputRecorder::replayFrame() {
	while (_lastRecordType == kRecordChar)
		replayChar();

	float frameTime = _dt;

	readNext();

	return frameTime;
}

void InputRecorder::recordFrame(float frameTime) {
	if (_mode != kModeRecord)
		return;

	uint32 bits;

	memcpy(&bits, &frameTime, sizeof(bits));

	_writeFile->writeByte('F');
	_writeFile->writeUint32LE(bits);
	_writeFile->flush();
}

} // End of namespace Grim
