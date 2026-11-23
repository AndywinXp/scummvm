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

#include "topgun/libraries/audio/audio_v1_2_3.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

uint16 AudioLibrary_v1_2::getCurrentTask() const {
	return TG_AUDIO_V1_2_TASK_ID;
}

uint16 AudioLibrary_v1_2::getCdOwnerTask() const {
	// The task that opened the device is the only task there is
	return (_cdDeviceId != 0) ? TG_AUDIO_V1_2_TASK_ID : 0;
}

void AudioLibrary_v1_2::muteCDAudio() {
	if (_mute) {
		_cdMuteSavedVolume = readCDVolumeLevels();
		setCDAudioVolume(0);
	} else {
		setCDAudioVolume(_cdMuteSavedVolume);
	}
}

uint16 AudioLibrary_v1_2::stopCDIfOwned() {
	if (_cdDeviceId != 0 && getCurrentTask() == getCdOwnerTask())
		closeCDAudioDevice();

	return (_cdDeviceId != 0) ? 1 : 0;
}

uint16 AudioLibrary_v1_2::audioPlayCdFadeIn(uint32 milliseconds, uint32 from, uint32 to) {
	if (_cdDeviceId != 0)
		return 0;

	_cdFadeSteps = (uint16)((milliseconds * 5) / 1000);

	if (_cdFadeSteps == 0)
		return 0;

	uint32 volume = readCDVolumeLevels();

	_cdFadeTargetLeft = (uint16)(volume & 0xFFFF);
	_cdFadeTargetRight = (uint16)(volume >> 16);

	uint32 steps = _cdFadeSteps;

	_cdFadeStepLeft = (uint16)(((steps >> 1) + _cdFadeTargetLeft) / steps);
	_cdFadeStepRight = (uint16)(((steps >> 1) + _cdFadeTargetRight) / steps);
	_cdFadeCurrentRight = 0;
	_cdFadeCurrentLeft = 0;
	setCDAudioVolume(0);

	if (!startCDPlayback(from, to))
		return 0;

	_cdFadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	_cdFadeMode = 1;
	return 1;
}

uint16 AudioLibrary_v1_2::audioCdFadeOut(uint32 milliseconds) {
	if (_cdDeviceId == 0 || getCurrentTask() != getCdOwnerTask())
		return 0;

	_cdFadeSteps = (uint16)((milliseconds * 5) / 1000);

	if (_cdFadeSteps == 0)
		return 0;

	uint32 volume = readCDVolumeLevels();

	_cdFadeCurrentLeft = (uint16)(volume & 0xFFFF);
	_cdFadeCurrentRight = (uint16)(volume >> 16);
	_cdFadeTargetLeft = _cdFadeCurrentLeft;
	_cdFadeTargetRight = _cdFadeCurrentRight;

	uint32 steps = _cdFadeSteps;

	_cdFadeStepLeft = (uint16)(((steps >> 1) + _cdFadeCurrentLeft) / steps);
	_cdFadeStepRight = (uint16)(((steps >> 1) + _cdFadeCurrentRight) / steps);
	_cdFadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	_cdFadeMode = 0xFF;
	return 1;
}

uint32 AudioLibrary_v1_2::audioGetCdPosition() {
	if (_cdDeviceId != 0 && getCurrentTask() == getCdOwnerTask())
		return getCDCurrentPosition();

	return 0;
}

uint8 AudioLibrary_v1_2::audioGetCdTrackCount() {
	if (_cdDeviceId != 0)
		return (uint8)(_cdLimit >> 24);

	if (!openCDAudioDevice())
		return 0;

	uint8 trackCount = (uint8)(_cdLimit >> 24);

	closeCDAudioDevice();
	return trackCount;
}

uint32 AudioLibrary_v1_2::audioGetCdVolume() {
	return readCDVolumeLevels();
}

uint16 AudioLibrary_v1_2::audioIsCdPlaying() {
	return (_cdDeviceId != 0) ? 1 : 0;
}

uint16 AudioLibrary_v1_2::audioPlayCd(uint32 from, uint32 to) {
	if (getCdOwnerTask() != 0 && getCurrentTask() != getCdOwnerTask())
		return 0;

	return startCDPlayback(from, to);
}

uint16 AudioLibrary_v1_2::audioPlayCdTrack(uint16 track) {
	if (getCdOwnerTask() == 0 || getCurrentTask() == getCdOwnerTask())
		playCDTrackRangeByNumber(track & 0xFF, track & 0xFF);

	return (_cdDeviceId != 0) ? 1 : 0;
}

uint16 AudioLibrary_v1_2::audioSetCdVolume(uint32 volume) {
	return setCDAudioVolume(volume);
}

uint16 AudioLibrary_v1_2_3::audioSetCdVolume(uint32 volume) {
	uint16 result = AudioLibrary_v1_2::audioSetCdVolume(volume);

	if (_engine->_version >= kVersionV1_2_6 && result != 0) {
		_iniCdVolume = (uint16)volume;
		saveCdVolumeToIni();
	}

	return result;
}

bool AudioLibrary_v1_2_3::openCDAudioDevice() {
	if (_cdDeviceId != 0)
		return true;

	if (!AudioLibrary::openCDAudioDevice())
		return false;

	if (_engine->_version >= kVersionV1_2_6) {
		_cdAuxSavedVolume = _cdAuxVolume;

		if (_iniCdVolume != 0)
			setCDAudioVolume(((uint32)_iniCdVolume << 16) | _iniCdVolume);

		if (_mute)
			muteCDAudio();
	}

	return true;
}

uint16 AudioLibrary_v1_2::audioStopCd() {
	return stopCDIfOwned();
}

uint16 AudioLibrary_v1_2_3::closeCDAudioDevice() {
	DeviceParms parms;

	sendCdDeviceCommand(TG_AUDIO_DEV_STOP, TG_AUDIO_DEV_WAIT, parms);

	if (_engine->_version >= kVersionV1_2_6 && _cdAuxPresent) {
		_cdAuxVolume = _cdAuxSavedVolume;
		applyCdVolume(_cdAuxSavedVolume);
	}

	sendCdDeviceCommand(TG_AUDIO_DEV_CLOSE, TG_AUDIO_DEV_WAIT, parms);
	_cdDeviceId = 0;
	resetWaveQueueAndEnableAllWaves(0);
	return 0;
}

} // End of namespace Topgun
