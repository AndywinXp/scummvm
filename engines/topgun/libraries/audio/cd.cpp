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

#include "audio/audiostream.h"
#include "backends/audiocd/audiocd.h"
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/topgun.h"

namespace Topgun {

uint8 AudioLibrary::getCDTrackCount() {
	if (_cdDeviceId != 0)
		return (uint8)(_cdLimit >> 24);

	if (!openCDAudioDevice())
		return 0;

	uint8 trackCount = (uint8)(_cdLimit >> 24);

	closeCDAudioDevice();
	return trackCount;
}

uint32 AudioLibrary::getCDCurrentPosition() {
	DeviceParms parms;

	parms.item = kStatusPosition;

	if (!sendCdDeviceCommand(kDevStatus, kDevStatusItem, parms))
		return 0;

	return ((parms.value & 0xFF) << 24) | (((parms.value >> 8) & 0xFF) << 16) |
	       (((parms.value >> 16) & 0xFF) << 8) | ((parms.value >> 24) & 0xFF);
}

uint32 AudioLibrary::readCDVolumeLevels() {
	if (!_cdAuxPresent)
		return 0;

	return _cdAuxVolume;
}

bool AudioLibrary::sendCdDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms) {
	AudioCDManager *manager = g_system->getAudioCDManager();

	if (message == kDevOpen) {
		if (!_engine->existExtractedCDAudioFiles(kFirstAudioTrack))
			return false;

		parms.deviceId = 1;
		return true;
	}

	if (_cdDeviceId == 0)
		return false;

	switch (message) {
	case kDevClose:
		return true;

	case kDevStop:
		manager->stop();
		_cdPlayStarted = false;
		return true;

	case kDevSet:
		return (flags & kDevSetTimeFormat) && parms.timeFormat == kFormatTmsf;

	case kDevStatus:
		if (!(flags & kDevStatusItem))
			return false;

		if (parms.item == kStatusNumberOfTracks) {
			uint32 trackCount = 1;

			while (_engine->existExtractedCDAudioFiles(trackCount + 1))
				trackCount++;

			parms.value = trackCount;
			return true;
		}

		if (parms.item == kStatusLength) {
			if (!(flags & kDevTrack))
				return false;

			uint32 frames = getExtractedTrackFrames(parms.track);
			if (frames == 0)
				return false;

			parms.value = (frames / (kFramesPerSecond * 60)) | (((frames / kFramesPerSecond) % 60) << 8) |
			              ((frames % kFramesPerSecond) << 16);
			return true;
		}

		if (parms.item == kStatusPosition) {
			uint32 track = 1;
			uint32 frames = 0;

			if (_cdPlayStarted) {
				uint32 elapsed = (uint32)(((uint64)(_engine->getEngineMillis() - _cdPlayStartMillis) * kFramesPerSecond) / 1000);

				track = _cdPlayTrack;
				frames = MIN<uint32>(_cdPlayStartFrame + elapsed, _cdPlayEndFrame);
			}

			parms.value = track | ((frames / (kFramesPerSecond * 60)) << 8) |
			              (((frames / kFramesPerSecond) % 60) << 16) | ((frames % kFramesPerSecond) << 24);
			return true;
		}

		return false;

	case kDevPlay: {
		if (!(flags & kDevFrom) || !(flags & kDevTo)) {
			warning("AudioLibrary::sendCdDeviceCommand(): play needs a start and an end position");
			return false;
		}

		uint32 track = parms.from & 0xFF;
		uint32 startFrame = ((parms.from >> 8) & 0xFF) * kFramesPerSecond * 60 + ((parms.from >> 16) & 0xFF) * kFramesPerSecond + (parms.from >> 24);
		uint32 endTrack = parms.to & 0xFF;
		uint32 endFrame = ((parms.to >> 8) & 0xFF) * kFramesPerSecond * 60 + ((parms.to >> 16) & 0xFF) * kFramesPerSecond + (parms.to >> 24);

		if (track < kFirstAudioTrack || endTrack < track)
			return false;

		uint32 duration = 0;

		if (endTrack == track) {
			if (endFrame <= startFrame)
				return false;

			duration = endFrame - startFrame;
		} else {
			warning("AudioLibrary::sendCdDeviceCommand(): playing from track %d to track %d, the playback stops at the end of track %d", track, endTrack, track);
		}

		uint32 trackFrames = getExtractedTrackFrames(track);

		if (!manager->play(track, 1, startFrame, duration, true))
			return false;

		_cdPlayStarted = true;
		_cdPlayTrack = track;
		_cdPlayStartFrame = startFrame;
		_cdPlayEndFrame = (duration != 0) ? startFrame + duration : trackFrames;
		_cdPlayStartMillis = _engine->getEngineMillis();
		return true;
	}

	default:
		return false;
	}
}

bool AudioLibrary::openCDAudioDevice() {
	if (_cdDeviceId != 0)
		return true;

	resetWaveQueueAndEnableAllWaves(1);

	DeviceParms openParms;

	if (!sendCdDeviceCommand(kDevOpen, kDevOpenType, openParms)) {
		resetWaveQueueAndEnableAllWaves(0);
		return false;
	}

	_cdDeviceId = openParms.deviceId;
	_cdPlayStarted = false;

	DeviceParms setParms;

	setParms.timeFormat = kFormatTmsf;
	sendCdDeviceCommand(kDevSet, kDevSetTimeFormat, setParms);

	DeviceParms status;

	status.item = kStatusNumberOfTracks;
	sendCdDeviceCommand(kDevStatus, kDevStatusItem, status);

	byte trackCount = (byte)status.value;

	status.track = trackCount;
	status.item = kStatusLength;
	sendCdDeviceCommand(kDevStatus, kDevStatusItem | kDevTrack, status);

	_cdLimit = ((uint32)trackCount << 24) | ((status.value & 0xFF) << 16) | (((status.value >> 8) & 0xFF) << 8);
	return true;
}

uint16 AudioLibrary::startCDPlayback(uint32 from, uint32 to) {
	if (_cdDeviceId == 0 && !openCDAudioDevice())
		return 0;

	if (from >= _cdLimit) {
		closeCDAudioDevice();
		return 0;
	}

	_cdFrom = from;
	_cdPos = from;

	if (to == 0 || to > _cdLimit)
		to = _cdLimit;

	_cdTo = to;

	DeviceParms play;

	play.from = ((_cdFrom >> 24) & 0xFF) | (((_cdFrom >> 16) & 0xFF) << 8) | (((_cdFrom >> 8) & 0xFF) << 16) | ((_cdFrom & 0xFF) << 24);
	play.to = ((_cdTo >> 24) & 0xFF) | (((_cdTo >> 16) & 0xFF) << 8) | (((_cdTo >> 8) & 0xFF) << 16) | ((_cdTo & 0xFF) << 24);
	sendCdDeviceCommand(kDevPlay, kDevFrom | kDevTo, play);

	_cdNextPoll = _engine->getEngineMillis() + kPollInterval;
	return 1;
}

uint16 AudioLibrary::playCDTrackRangeByNumber(uint16 first, uint16 last) {
	uint32 from = (uint32)(first & 0xFF) << 24;
	uint32 to;

	if (_cdDeviceId == 0 && !openCDAudioDevice())
		return 0;

	if ((uint32)(first & 0xFF) >= _cdLimit) {
		closeCDAudioDevice();
		return 0;
	}

	if ((uint32)(last & 0xFF) > (_cdLimit >> 24)) {
		to = _cdLimit;
	} else {
		DeviceParms status;

		status.track = last & 0xFF;
		status.item = kStatusLength;
		sendCdDeviceCommand(kDevStatus, kDevStatusItem | kDevTrack, status);

		uint32 frames = (status.value >> 16) & 0xFF;
		uint32 lastFrame = (frames == 0) ? 0 : frames - 1;

		to = ((uint32)(last & 0xFF) << 24) | ((status.value & 0xFF) << 16) | (((status.value >> 8) & 0xFF) << 8) | lastFrame;
	}

	return startCDPlayback(from, to);
}

uint16 AudioLibrary::setCDAudioVolume(uint32 volume) {
	if (!_cdAuxPresent)
		return 0;

	_cdAuxVolume = volume;
	applyCdVolume(volume);
	return 1;
}

void AudioLibrary::releaseCDDeviceIfOwned() {
	if (_cdDeviceId != 0)
		closeCDAudioDevice();

	if (_cdAuxPresent) {
		_cdAuxVolume = _cdAuxSavedVolume;
		applyCdVolume(_cdAuxSavedVolume);
	}
}

uint16 AudioLibrary::findCdAuxDevice() {
	for (uint16 i = 0; i < kNumAuxDevices; i++) {
		if (auxDeviceTechnology(i) == kAuxTechnologyCd) {
			uint32 level = (uint32)g_system->getAudioCDManager()->getStatus().volume * 0x101;

			_cdAuxPresent = true;
			_cdAuxDeviceId = i;
			_cdAuxSavedVolume = (level << 16) | level;
			_cdAuxVolume = _cdAuxSavedVolume;
			break;
		}
	}

	return 1;
}

void AudioLibrary::pollCDEndAndClose() {
	if (_cdDeviceId == 0)
		return;

	uint32 now = _engine->getEngineMillis();
	if (now < _cdNextPoll)
		return;

	_cdNextPoll = now + kPollInterval;
	_cdPos = getCDCurrentPosition();

	if (_cdTo <= _cdPos)
		closeCDAudioDevice();
}

uint16 AudioLibrary::closeCDAudioDevice() {
	if (_cdDeviceId != 0) {
		DeviceParms parms;

		sendCdDeviceCommand(kDevStop, kDevWait, parms);
		sendCdDeviceCommand(kDevClose, kDevWait, parms);
		resetWaveQueueAndEnableAllWaves(0);
		_cdDeviceId = 0;
	}

	return 0;
}

void AudioLibrary::resetWaveQueueAndEnableAllWaves(uint16 on) {
	_cdSuspend = (on != 0);

	if (on != 0) {
		resetWaveOutQueue();

		for (uint32 i = 0; i < _waveObjects.size(); i++) {
			if (_waveObjects[i]->backgroundGroup || _waveObjects[i]->streaming)
				_waveObjects[i]->suspended = true;
		}
	} else {
		for (uint32 i = 0; i < _waveObjects.size(); i++)
			_waveObjects[i]->suspended = false;
	}
}

uint32 AudioLibrary::getExtractedTrackFrames(uint32 track) {
	Common::String names[4] = {
		Common::String::format("track%d", track),
		Common::String::format("track%02d", track),
		Common::String::format("track_%d", track),
		Common::String::format("track_%02d", track)
	};

	for (uint32 i = 0; i < 4; i++) {
		Audio::SeekableAudioStream *stream = Audio::SeekableAudioStream::openStreamFile(Common::Path(names[i], '/'));

		if (stream) {
			uint32 frames = (uint32)stream->getLength().convertToFramerate(kFramesPerSecond).totalNumberOfFrames();

			delete stream;
			return frames;
		}
	}

	return 0;
}

void AudioLibrary::applyCdVolume(uint32 volume) {
	AudioCDManager *manager = g_system->getAudioCDManager();
	int32 left = (int32)(volume & 0xFFFF);
	int32 right = (int32)(volume >> 16);
	int32 loudest = MAX(left, right);

	manager->setVolume((byte)(loudest >> 8));
	manager->setBalance((int8)((loudest == 0) ? 0 : ((right - left) * 127 / loudest)));
}

} // End of namespace Topgun
