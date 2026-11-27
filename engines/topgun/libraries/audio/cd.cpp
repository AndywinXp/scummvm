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
#include "common/config-manager.h"
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/topgun_v1_2.h"

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

	parms.item = TG_AUDIO_STATUS_POSITION;

	if (!sendCdDeviceCommand(TG_AUDIO_DEV_STATUS, TG_AUDIO_DEV_STATUS_ITEM, parms))
		return 0;

	return ((parms.value & 0xFF) << 24) | (((parms.value >> 8) & 0xFF) << 16) |
	       (((parms.value >> 16) & 0xFF) << 8) | ((parms.value >> 24) & 0xFF);
}

uint32 AudioLibrary::readCDVolumeLevels() {
	if (!_cdAuxPresent)
		return 0;

	return _cdAuxVolume;
}

uint32 AudioLibrary::getCDTrackLength(byte track) {
	DeviceParms status;

	status.track = track;
	status.item = TG_AUDIO_STATUS_LENGTH;

	if (!sendCdDeviceCommand(TG_AUDIO_DEV_STATUS, TG_AUDIO_DEV_STATUS_ITEM | TG_AUDIO_DEV_TRACK, status))
		return 0;

	// Milliseconds from the minutes and seconds only, the frames are dropped
	return ((status.value & 0xFF) * 60 + ((status.value >> 8) & 0xFF)) * 1000;
}

uint16 AudioLibrary::audioGetCdTrackLengths(byte first, byte last, uint32 *lengths) {
	bool wasOpen = (_cdDeviceId != 0);

	if (!wasOpen && !openCDAudioDevice())
		return 0;

	uint16 count = 0;

	while (last >= first) {
		lengths[count] = getCDTrackLength(first);
		first++;

		if (lengths[count] == 0)
			break;

		count++;
	}

	// A device opened here stays open, only one which was already open gets closed
	if (wasOpen)
		closeCDAudioDevice();

	return count;
}

bool AudioLibrary::sendCdDeviceCommand(uint16 message, uint32 flags, DeviceParms &parms) {
	AudioCDManager *manager = g_system->getAudioCDManager();

	if (message == TG_AUDIO_DEV_OPEN) {
		if (!_engine->existExtractedCDAudioFiles(TG_AUDIO_FIRST_AUDIO_TRACK))
			return false;

		parms.deviceId = 1;
		return true;
	}

	if (_cdDeviceId == 0)
		return false;

	switch (message) {
	case TG_AUDIO_DEV_CLOSE:
		return true;

	case TG_AUDIO_DEV_STOP:
		manager->stop();
		_cdPlayStarted = false;
		return true;

	case TG_AUDIO_DEV_SET:
		return (flags & TG_AUDIO_DEV_SET_TIME_FORMAT) && parms.timeFormat == TG_AUDIO_FORMAT_TMSF;

	case TG_AUDIO_DEV_STATUS:
		if (!(flags & TG_AUDIO_DEV_STATUS_ITEM))
			return false;

		if (parms.item == TG_AUDIO_STATUS_NUMBER_OF_TRACKS) {
			uint32 trackCount = 1;

			while (_engine->existExtractedCDAudioFiles(trackCount + 1))
				trackCount++;

			parms.value = trackCount;
			return true;
		}

		if (parms.item == TG_AUDIO_STATUS_LENGTH) {
			if (!(flags & TG_AUDIO_DEV_TRACK))
				return false;

			uint32 frames = getExtractedTrackFrames(parms.track);
			if (frames == 0)
				return false;

			parms.value = (frames / (TG_AUDIO_FRAMES_PER_SECOND * 60)) | (((frames / TG_AUDIO_FRAMES_PER_SECOND) % 60) << 8) |
			              ((frames % TG_AUDIO_FRAMES_PER_SECOND) << 16);
			return true;
		}

		if (parms.item == TG_AUDIO_STATUS_POSITION) {
			uint32 track = 1;
			uint32 frames = 0;

			if (_cdPlayStarted) {
				uint32 elapsed = (uint32)(((uint64)(_engine->getEngineMillis() - _cdPlayStartMillis) * TG_AUDIO_FRAMES_PER_SECOND) / 1000);

				track = _cdPlayTrack;
				frames = MIN<uint32>(_cdPlayStartFrame + elapsed, _cdPlayEndFrame);
			}

			parms.value = track | ((frames / (TG_AUDIO_FRAMES_PER_SECOND * 60)) << 8) |
			              (((frames / TG_AUDIO_FRAMES_PER_SECOND) % 60) << 16) | ((frames % TG_AUDIO_FRAMES_PER_SECOND) << 24);
			return true;
		}

		return false;

	case TG_AUDIO_DEV_PLAY: {
		if (!(flags & TG_AUDIO_DEV_FROM) || !(flags & TG_AUDIO_DEV_TO)) {
			warning("AudioLibrary::sendCdDeviceCommand(): play needs a start and an end position");
			return false;
		}

		uint32 track = parms.from & 0xFF;
		uint32 startFrame = ((parms.from >> 8) & 0xFF) * TG_AUDIO_FRAMES_PER_SECOND * 60 + ((parms.from >> 16) & 0xFF) * TG_AUDIO_FRAMES_PER_SECOND + (parms.from >> 24);
		uint32 endTrack = parms.to & 0xFF;
		uint32 endFrame = ((parms.to >> 8) & 0xFF) * TG_AUDIO_FRAMES_PER_SECOND * 60 + ((parms.to >> 16) & 0xFF) * TG_AUDIO_FRAMES_PER_SECOND + (parms.to >> 24);

		if (track < TG_AUDIO_FIRST_AUDIO_TRACK || endTrack < track)
			return false;

		uint32 duration = 0;

		if (endTrack == track) {
			if (endFrame <= startFrame)
				return false;

			duration = endFrame - startFrame;
		} else if (endTrack != track + 1 || endFrame != 0) {
			warning("AudioLibrary::sendCdDeviceCommand(): playing from track %d to track %d, the playback stops at the end of track %d", track, endTrack, track);
		}

		uint32 trackFrames = getExtractedTrackFrames(track);

		if (!manager->play(track, 1, startFrame, duration, true, Audio::Mixer::kPlainSoundType))
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

	if (!sendCdDeviceCommand(TG_AUDIO_DEV_OPEN, TG_AUDIO_DEV_OPEN_TYPE, openParms)) {
		resetWaveQueueAndEnableAllWaves(0);
		return false;
	}

	_cdDeviceId = openParms.deviceId;
	_cdPlayStarted = false;

	DeviceParms setParms;

	setParms.timeFormat = TG_AUDIO_FORMAT_TMSF;
	sendCdDeviceCommand(TG_AUDIO_DEV_SET, TG_AUDIO_DEV_SET_TIME_FORMAT, setParms);

	DeviceParms status;

	status.item = TG_AUDIO_STATUS_NUMBER_OF_TRACKS;
	sendCdDeviceCommand(TG_AUDIO_DEV_STATUS, TG_AUDIO_DEV_STATUS_ITEM, status);

	byte trackCount = (byte)status.value;

	status.track = trackCount;
	status.item = TG_AUDIO_STATUS_LENGTH;
	sendCdDeviceCommand(TG_AUDIO_DEV_STATUS, TG_AUDIO_DEV_STATUS_ITEM | TG_AUDIO_DEV_TRACK, status);

	_cdLimit = ((uint32)trackCount << 24) | ((status.value & 0xFF) << 16) | (((status.value >> 8) & 0xFF) << 8);
	return true;
}

uint16 AudioLibrary::startCDPlayback(uint32 from, uint32 to) {
	if (_engine->_version >= kVersionV1_1 && _muted)
		return 0;

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
	sendCdDeviceCommand(TG_AUDIO_DEV_PLAY, TG_AUDIO_DEV_FROM | TG_AUDIO_DEV_TO, play);

	if (_engine->_version >= kVersionV1_1) {
		if (_cdProfileFirst || _cdSlowStart)
			profileCDStart();
	} else {
		_cdNextPoll = _engine->getEngineMillis() + TG_AUDIO_POLL_INTERVAL;
	}

	return 1;
}

void AudioLibrary::profileCDStart() {
	uint32 deadline = _engine->getEngineMillis() + 1000;
	uint32 polls = 0;
	uint32 position;
	bool inTime;

	while (true) {
		position = getCDCurrentPosition();
		inTime = (_engine->getEngineMillis() < deadline);

		if (position != _cdFrom || !inTime)
			break;

		polls++;
		g_system->delayMillis(1);
	}

	if (_cdProfileFirst) {
		_cdProfileFirst = false;

		if (inTime && polls >= 3 && (position >> 24) == (_cdFrom >> 24) &&
			((position >> 16) & 0xFF) == ((_cdFrom >> 16) & 0xFF) && ((position >> 8) & 0xFF) == ((_cdFrom >> 8) & 0xFF) &&
			(int32)(_cdFrom & 0xFF) + 2 > (int32)(position & 0xFF))
			_cdSlowStart = true;
	}
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
		status.item = TG_AUDIO_STATUS_LENGTH;
		sendCdDeviceCommand(TG_AUDIO_DEV_STATUS, TG_AUDIO_DEV_STATUS_ITEM | TG_AUDIO_DEV_TRACK, status);

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
	for (uint16 i = 0; i < TG_AUDIO_NUM_AUX_DEVICES; i++) {
		if (auxDeviceTechnology(i) == TG_AUDIO_AUX_TECHNOLOGY_CD) {
			uint32 level = (uint32)g_system->getAudioCDManager()->getStatus().volume * 0x101;

			_cdAuxPresent = true;
			_cdAuxDeviceId = i;
			_cdAuxSavedVolume = (level << 16) | level;
			_cdAuxVolume = _cdAuxSavedVolume;
			break;
		}
	}

	if (_engine->_version >= kVersionV1_2)
		_cdProfileFirst = static_cast<TopgunEngine_v1_2 *>(_engine)->getPrivateProfileInt("audio", "ProfileCD", 1) != 0;
	else if (_engine->_version >= kVersionV1_1)
		_cdProfileFirst = !ConfMan.hasKey("profilecd") || ConfMan.getInt("profilecd") != 0;

	return 1;
}

void AudioLibrary::pollCDEndAndClose() {
	if (_cdDeviceId == 0)
		return;

	uint32 now = _engine->getEngineMillis();
	if (now < _cdNextPoll)
		return;

	_cdNextPoll = now + TG_AUDIO_POLL_INTERVAL;
	_cdPos = getCDCurrentPosition();

	if (_cdTo <= _cdPos)
		closeCDAudioDevice();
}

uint16 AudioLibrary::closeCDAudioDevice() {
	if (_cdDeviceId != 0) {
		DeviceParms parms;

		sendCdDeviceCommand(TG_AUDIO_DEV_STOP, TG_AUDIO_DEV_WAIT, parms);
		sendCdDeviceCommand(TG_AUDIO_DEV_CLOSE, TG_AUDIO_DEV_WAIT, parms);
		resetWaveQueueAndEnableAllWaves(0);
		_cdDeviceId = 0;
	}

	if (_engine->_version >= kVersionV1_1 && _cdFadeMode != 0) {
		setCDAudioVolume(((uint32)_cdFadeTargetRight << 16) | _cdFadeTargetLeft);
		_cdFadeMode = 0;
	}

	return 0;
}

uint16 AudioLibrary::startCDFadeIn(uint16 milliseconds, uint32 from) {
	if (_cdDeviceId != 0)
		return 0;

	uint16 steps = (uint16)((uint16)(milliseconds * 5) / 1000);
	if (steps == 0)
		return 0;

	_cdFadeTargetLeft = (uint16)(_cdAuxVolume & 0xFFFF);
	_cdFadeTargetRight = (uint16)(_cdAuxVolume >> 16);
	_cdFadeCurrentLeft = 0;
	_cdFadeCurrentRight = 0;
	_cdFadeSteps = steps;
	_cdFadeStepLeft = (uint16)(((uint32)_cdFadeTargetLeft + (steps >> 1)) / steps);
	_cdFadeStepRight = (uint16)(((uint32)_cdFadeTargetRight + (steps >> 1)) / steps);

	setCDAudioVolume(0);

	uint16 result = startCDPlayback(from, 0);

	_cdFadeMode = 1;
	_cdFadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	return result;
}

uint16 AudioLibrary::startCDFadeOut(uint16 milliseconds) {
	if (_cdDeviceId == 0)
		return 0;

	uint16 steps = (uint16)((uint16)(milliseconds * 5) / 1000);
	if (steps == 0)
		return 0;

	_cdFadeTargetLeft = (uint16)(_cdAuxVolume & 0xFFFF);
	_cdFadeTargetRight = (uint16)(_cdAuxVolume >> 16);
	_cdFadeCurrentLeft = _cdFadeTargetLeft;
	_cdFadeCurrentRight = _cdFadeTargetRight;
	_cdFadeSteps = steps;
	_cdFadeStepLeft = (uint16)(((uint32)_cdFadeTargetLeft + (steps >> 1)) / steps);
	_cdFadeStepRight = (uint16)(((uint32)_cdFadeTargetRight + (steps >> 1)) / steps);
	_cdFadeMode = 0xFF;
	_cdFadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	return 1;
}

void AudioLibrary::cdFadeTick() {
	if (_cdDeviceId == 0 || _cdFadeMode == 0)
		return;

	uint32 now = _engine->getEngineMillis();
	if (now < _cdFadeNextTick)
		return;

	if (_cdFadeMode == 1) {
		_cdFadeCurrentLeft = (uint16)MIN<uint32>((uint32)_cdFadeCurrentLeft + _cdFadeStepLeft, _cdFadeTargetLeft);
		_cdFadeCurrentRight = (uint16)MIN<uint32>((uint32)_cdFadeCurrentRight + _cdFadeStepRight, _cdFadeTargetRight);
		_cdFadeSteps--;
		setCDAudioVolume(((uint32)_cdFadeCurrentRight << 16) | _cdFadeCurrentLeft);

		if (_cdFadeSteps == 0 || (_cdFadeCurrentLeft == _cdFadeTargetLeft && _cdFadeCurrentRight == _cdFadeTargetRight)) {
			_cdFadeMode = 0;
			setCDAudioVolume(((uint32)_cdFadeTargetRight << 16) | _cdFadeTargetLeft);
		}
	} else {
		_cdFadeCurrentLeft = (_cdFadeCurrentLeft > _cdFadeStepLeft) ? (_cdFadeCurrentLeft - _cdFadeStepLeft) : 0;
		_cdFadeCurrentRight = (_cdFadeCurrentRight > _cdFadeStepRight) ? (_cdFadeCurrentRight - _cdFadeStepRight) : 0;
		_cdFadeSteps--;
		setCDAudioVolume(((uint32)_cdFadeCurrentRight << 16) | _cdFadeCurrentLeft);

		if (_cdFadeSteps == 0 || (_cdFadeCurrentLeft == 0 && _cdFadeCurrentRight == 0))
			closeCDAudioDevice();
	}

	_cdFadeNextTick = now + TG_AUDIO_FADE_TICK_INTERVAL;
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
			uint32 frames = (uint32)stream->getLength().convertToFramerate(TG_AUDIO_FRAMES_PER_SECOND).totalNumberOfFrames();

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
