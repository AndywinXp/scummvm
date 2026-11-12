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

#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/miles_audio/driver.h"
#include "engines/grim/emi/musicstream.h"
#include "engines/grim/emi/streamscript.h"

namespace Grim {

MusicStream *MusicStream::open(const LString &fileName) {
	EMIMilesDriver *driver = CWaveAudioDevice::getMilesDriver();

	if (!driver)
		return nullptr;

	Common::SeekableReadStream *file = StreamScript::openStream(fileName.c_str());

	if (!file)
		return nullptr;

	EMIMilesStream *stream = driver->openStream(file);

	if (!stream) {
		warning("MusicStream::open(): cannot open <%s>", fileName.c_str());

		return nullptr;
	}

	return new MusicStream(driver, stream);
}

MusicStream::MusicStream(EMIMilesDriver *driver, EMIMilesStream *stream) {
	_driver = driver;
	_stream = stream;
}

MusicStream::~MusicStream() {
	if (_driver == CWaveAudioDevice::getMilesDriver())
		_driver->closeStream(_stream);

	_stream = nullptr;
}

void MusicStream::start() {
	_driver->startStream(_stream);
}

void MusicStream::pause(bool paused) {
	_driver->pauseStream(_stream, paused);
}

int MusicStream::getStatus() {
	return _driver->getStreamStatus(_stream);
}

void MusicStream::setVolume(int volume) {
	_driver->setStreamVolume(_stream, volume);
}

void MusicStream::setPan(int pan) {
	_driver->setStreamPan(_stream, pan);
}

void MusicStream::setLoopCount(int loopCount) {
	_driver->setStreamLoopCount(_stream, loopCount);
}

void MusicStream::setLoopBlock(int startByte, int endByte) {
	_driver->setStreamLoopBlock(_stream, startByte, endByte);
}

void MusicStream::setPosition(int bytePosition) {
	_driver->setStreamPosition(_stream, bytePosition);
}

void MusicStream::setPositionSynced(int bytePosition, uint stamp) {
	_driver->setStreamPositionSynced(_stream, bytePosition, stamp);
}

int MusicStream::getPosition() {
	return _driver->getStreamPosition(_stream);
}

int MusicStream::getPositionStamped(uint *stamp) {
	return _driver->getStreamPositionStamped(_stream, stamp);
}

void MusicStream::setMsPosition(int milliseconds) {
	_driver->setStreamMsPosition(_stream, milliseconds);
}

void MusicStream::getMsPosition(int *total, int *current) {
	_driver->getStreamMsPosition(_stream, total, current);
}

} // End of namespace Grim
