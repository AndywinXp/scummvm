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

#include "common/endian.h"
#include "common/stream.h"
#include "common/substream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "audio/decoders/mp3.h"

#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavechannel.h"
#include "engines/grim/emi/musicstream.h"
#include "engines/grim/emi/streamscript.h"

namespace Grim {

static const int kMaxVolume = 127;
static const int kCenterPan = 64;

MusicStream *MusicStream::open(const LString &fileName) {
#ifdef USE_MAD
	Audio::Mixer *mixer = g_system->getMixer();

	if (!mixer || !mixer->isReady())
		return nullptr;

	Common::SeekableReadStream *file = StreamScript::openStream(fileName.c_str());

	if (!file)
		return nullptr;

	uint dataOffset = 0;
	uint dataSize = (uint)file->size();

	if (file->readUint32BE() == MKTAG('R', 'I', 'F', 'F')) {
		file->readUint32LE();

		if (file->readUint32BE() == MKTAG('W', 'A', 'V', 'E')) {
			bool foundData = false;

			while (file->pos() + 8 <= file->size()) {
				uint32 chunkId = file->readUint32BE();
				uint32 chunkSize = file->readUint32LE();

				if (chunkId == MKTAG('d', 'a', 't', 'a')) {
					dataOffset = (uint)file->pos();
					dataSize = chunkSize;
					foundData = true;

					break;
				}

				file->seek((int64)chunkSize + (chunkSize & 1), SEEK_CUR);
			}

			if (!foundData) {
				warning("MusicStream::open(): no data chunk in <%s>", fileName.c_str());
				delete file;

				return nullptr;
			}
		}
	}

	if ((uint64)dataOffset + dataSize > (uint64)file->size()) {
		warning("MusicStream::open(): the data chunk of <%s> runs past the end of the file", fileName.c_str());
		dataSize = (uint)file->size() - dataOffset;
	}

	Common::SeekableSubReadStream *data = new Common::SeekableSubReadStream(file, dataOffset, dataOffset + dataSize, DisposeAfterUse::YES);
	Audio::SeekableAudioStream *decoder = Audio::makeMP3Stream(data, DisposeAfterUse::YES);

	if (!decoder) {
		warning("MusicStream::open(): <%s> is not a valid MP3 stream", fileName.c_str());

		return nullptr;
	}

	return new MusicStream(decoder, dataSize);
#else
	warning("MusicStream::open(): ScummVM was built without MP3 support, <%s> will not play", fileName.c_str());

	return nullptr;
#endif
}

MusicStream::MusicStream(Audio::SeekableAudioStream *decoder, uint dataSize) {
	_decoder = decoder;
	_dataSize = dataSize;
	_totalFrames = decoder->getLength().totalNumberOfFrames();
	_rate = decoder->getRate();
	_stereo = decoder->isStereo();
	_started = false;
	_paused = false;
	_done = false;
	_playFrame = 0;
	_pendingFrame = 0;
	_loopStartFrame = 0;
	_loopEndFrame = -1;
	_loopCount = 1;
	_volume = kMaxVolume;
	_pan = kCenterPan;
}

MusicStream::~MusicStream() {
	Audio::Mixer *mixer = g_system->getMixer();

	if (mixer)
		mixer->stopHandle(_handle);

	delete _decoder;
	_decoder = nullptr;
}

void MusicStream::start() {
	Audio::Mixer *mixer = g_system->getMixer();

	{
		Common::StackLock lock(_mutex);

		if (_started)
			_pendingFrame = 0;

		_started = true;
		_paused = false;
		_done = false;
		_playFrame = _pendingFrame;

		seekDecoder(_playFrame);

		if (_playFrame >= _totalFrames)
			_done = true;
	}

	if (mixer->isSoundHandleActive(_handle))
		mixer->stopHandle(_handle);

	if (!_done)
		mixer->playStream(Audio::Mixer::kMusicSoundType, &_handle, this, -1, (byte)getMixerVolume(), (int8)getMixerBalance(), DisposeAfterUse::NO);
}

void MusicStream::pause(bool paused) {
	bool changed = false;

	{
		Common::StackLock lock(_mutex);

		if (paused) {
			if (_started && !_done && !_paused) {
				_paused = true;
				changed = true;
			}
		} else if (_paused) {
			_paused = false;
			changed = true;
		}
	}

	if (changed)
		g_system->getMixer()->pauseHandle(_handle, paused);
}

int MusicStream::getStatus() {
	Common::StackLock lock(_mutex);

	if (!_started || _done)
		return kStatusDone;

	if (_paused)
		return kStatusStopped;

	return kStatusPlaying;
}

void MusicStream::setVolume(int volume) {
	_volume = volume;

	refreshMixerVolume();
}

void MusicStream::setPan(int pan) {
	_pan = pan;

	refreshMixerVolume();
}

void MusicStream::setLoopCount(int loopCount) {
	Common::StackLock lock(_mutex);

	_loopCount = loopCount;
}

void MusicStream::setLoopBlock(int startByte, int endByte) {
	Common::StackLock lock(_mutex);

	_loopStartFrame = bytesToFrames(startByte);
	_loopEndFrame = endByte < 0 ? -1 : bytesToFrames(endByte);
}

void MusicStream::setPosition(int bytePosition) {
	setFramePosition(bytesToFrames(bytePosition));
}

int MusicStream::getPosition() {
	Common::StackLock lock(_mutex);

	return framesToBytes(_started ? _playFrame : _pendingFrame);
}

void MusicStream::setMsPosition(int milliseconds) {
	if (_rate <= 0)
		return;

	if (milliseconds < 0)
		milliseconds = 0;

	int64 frame = (int64)milliseconds * _rate / 1000;

	if (frame > _totalFrames)
		frame = _totalFrames;

	setFramePosition((int)frame);
}

void MusicStream::getMsPosition(int *total, int *current) {
	Common::StackLock lock(_mutex);

	// The original could divide by zero here, let's avoid that...
	if (_rate <= 0) {
		if (total)
			*total = 0;

		if (current)
			*current = 0;

		return;
	}

	if (total)
		*total = (int)((int64)_totalFrames * 1000 / _rate);

	if (current)
		*current = (int)((int64)(_started ? _playFrame : _pendingFrame) * 1000 / _rate);
}

void MusicStream::refreshMixerVolume() {
	Audio::Mixer *mixer = g_system->getMixer();

	if (mixer && mixer->isSoundHandleActive(_handle)) {
		mixer->setChannelVolume(_handle, (byte)getMixerVolume());
		mixer->setChannelBalance(_handle, (int8)getMixerBalance());
	}
}

int MusicStream::readBuffer(int16 *buffer, const int numSamples) {
	Common::StackLock lock(_mutex);

	if (!_started || _done)
		return 0;

	int channels = _stereo ? 2 : 1;
	int written = 0;
	bool jumpedWithoutData = false;

	while (written < numSamples && !_done) {
		int remaining = _totalFrames - _playFrame;

		if (_loopCount != 1 && _loopEndFrame >= 0 && _loopEndFrame < _totalFrames)
			remaining = _loopEndFrame - _playFrame;

		if (remaining <= 0) {
			if (jumpedWithoutData) {
				warning("MusicStream::readBuffer(): the loop block is empty");
				_done = true;

				continue;
			}

			jumpedWithoutData = true;

			if (_loopCount != 1) {
				_playFrame = _loopStartFrame;

				seekDecoder(_playFrame);

				if (_loopCount > 0)
					_loopCount--;
			} else {
				_done = true;
			}

			continue;
		}

		int framesWanted = (numSamples - written) / channels;

		if (framesWanted <= 0)
			break;

		if (framesWanted > remaining)
			framesWanted = remaining;

		int samples = _decoder->readBuffer(buffer + written, framesWanted * channels);

		if (samples <= 0) {
			_playFrame = _totalFrames;

			continue;
		}

		jumpedWithoutData = false;
		written += samples;
		_playFrame += samples / channels;

		if (samples < framesWanted * channels)
			_playFrame = _totalFrames;
	}

	return written;
}

bool MusicStream::isStereo() const {
	return _stereo;
}

int MusicStream::getRate() const {
	return _rate;
}

bool MusicStream::endOfData() const {
	return _done;
}

int MusicStream::bytesToFrames(int bytes) {
	// The original could divide by zero here, let's avoid that...
	if (bytes <= 0 || _dataSize == 0)
		return 0;

	int64 frames = (int64)bytes * _totalFrames / _dataSize;

	if (frames > _totalFrames)
		frames = _totalFrames;

	return (int)frames;
}

int MusicStream::framesToBytes(int frames) {
	// The original could divide by zero here, let's avoid that...
	if (_totalFrames <= 0)
		return 0;

	return (int)((int64)frames * _dataSize / _totalFrames);
}

void MusicStream::seekDecoder(int frame) {
	if (frame <= 0 || _rate <= 0) {
		_decoder->rewind();

		return;
	}

	uint milliseconds = (uint)((int64)frame * 1000 / _rate);

	if (!_decoder->seek(Audio::Timestamp(milliseconds, _rate)))
		_done = true;
}

void MusicStream::setFramePosition(int frame) {
	Common::StackLock lock(_mutex);

	if (!_started) {
		_pendingFrame = frame;

		return;
	}

	_playFrame = frame;

	seekDecoder(frame);
}

int MusicStream::getMixerVolume() {
	return CWaveChannel::volumeToMixer(_volume);
}

int MusicStream::getMixerBalance() {
	return CWaveChannel::panToBalance(_pan);
}

} // End of namespace Grim
