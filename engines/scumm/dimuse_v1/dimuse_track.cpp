/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/config-manager.h"
#include "common/timer.h"

#include "scumm/actor.h"
#include "scumm/scumm_v7.h"
#include "scumm/sound.h"
#include "scumm/dimuse_v1/dimuse_v1.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_track.h"
#include "scumm/dimuse_v1/dimuse_tables.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"

namespace Scumm {

int DiMUSE_v1::allocSlot(int priority) {
	int l, lowest_priority = 127;
	int trackId = -1;

	for (l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		if (!_track[l]->used) {
			trackId = l;
			break;
		}
	}

	if (trackId == -1) {
		debug(5, "DiMUSE_v1::allocSlot(): All slots are full");
		for (l = 0; l < MAX_DIGITAL_TRACKS; l++) {
			Track *track = _track[l];
			if (track->used && !track->toBeRemoved &&
					(lowest_priority > track->soundPriority) && !track->souStreamUsed) {
				lowest_priority = track->soundPriority;
				trackId = l;
			}
		}
		if (lowest_priority <= priority) {
			assert(trackId != -1);
			Track *track = _track[trackId];

			// Stop the track immediately
			_mixer->stopHandle(track->mixChanHandle);
			if (track->soundDesc) {
				_sound->closeSound(track->soundDesc);
			}

			// Mark it as unused
			track->reset();

			debug(5, "DiMUSE_v1::allocSlot(): Removed sound %d from track %d", _track[trackId]->soundId, trackId);
		} else {
			debug(5, "DiMUSE_v1::allocSlot(): Priority sound too low");
			return -1;
		}
	}

	return trackId;
}

int DiMUSE_v1::startSound(int soundId, const char *soundName, int soundType, int volGroupId, Audio::AudioStream *input, int hookId, int volume, int priority, Track *otherTrack) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::startSound()");
	debug(5, "DiMUSE_v1::startSound(%d) - begin func", soundId);

	bool forceFadeIn = false;
	if (_vm->_game.id == GID_FT && volGroupId == IMUSE_VOLGRP_MUSIC) {
		// First of all we check if there's another track playing the target soundId
		int alreadyPlayingTrackId = -1;
		for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
			if (_track[l]->volGroupId == IMUSE_VOLGRP_MUSIC && _track[l]->used) {
				forceFadeIn = true;
				if (_track[l]->soundId == soundId) {
					alreadyPlayingTrackId = l;
					break;
				}
			}
		}
		// If the current music state corresponds to the same soundId
		// we're trying to play, we just adjust its volume to the new one.
		// Otherwise... just flush the old track and start the new one
		if (alreadyPlayingTrackId != -1) {
			if (getSoundIdByName(_ftStateMusicTable[_curMusicState].audioName) == soundId) {
				Track *alreadyPlayingTrack = _track[alreadyPlayingTrackId];
				alreadyPlayingTrack->volFadeDelay = 60;
				alreadyPlayingTrack->volFadeDest = volume * 1000;
				alreadyPlayingTrack->volFadeStep = (alreadyPlayingTrack->volFadeDest - alreadyPlayingTrack->vol) * 60 * (1000 / _callbackFps) / (1000 * alreadyPlayingTrack->volFadeDelay);
				alreadyPlayingTrack->volFadeUsed = true;
				alreadyPlayingTrack->toBeRemoved = false;
				return alreadyPlayingTrackId;
			} else {
				flushTrack(_track[alreadyPlayingTrackId]);
			}
		}
	} else if (_vm->_game.id == GID_CMI) {
		forceFadeIn = true;
	}

	int l = allocSlot(priority);
	if (l == -1) {
		warning("DiMUSE_v1::startSound() Can't start sound - no free slots");
		return -1;
	}
	debug(5, "DiMUSE_v1::startSound(%d, trackId:%d)", soundId, l);

	Track *track = _track[l];

	// Reset the track
	track->reset();

	track->pan = 64;
	track->vol = volume * 1000;
	track->soundId = soundId;
	track->volGroupId = volGroupId;
	track->curHookId = hookId;
	track->soundPriority = priority;
	track->curRegion = -1;
	track->soundType = soundType;
	track->trackId = l;

	if (_vm->_game.id == GID_FT) {
		// Tweak the default gain reduction to about 2 dB
		track->gainRedFadeDest = 127 * 180;
	} else if (_vm->_game.id == GID_CMI) {
		// Tweak the default gain reduction to about 4 dB
		track->gainRedFadeDest = 127 * 290;
	}

	int bits = 0, freq = 0, channels = 0;

	track->souStreamUsed = (input != 0);

	if (track->souStreamUsed) {
		int effVol = track->getVol();
		if (_vm->_game.id == GID_FT) {
			effVol = int(round(effVol * 1.3));
		}
		_mixer->playStream(track->getType(), &track->mixChanHandle, input, -1, effVol, track->getPan());
	} else {
		strcpy(track->soundName, soundName);
		track->soundDesc = _sound->openSound(soundId, soundName, soundType, volGroupId, -1);
		if (!track->soundDesc)
			track->soundDesc = _sound->openSound(soundId, soundName, soundType, volGroupId, 1);
		if (!track->soundDesc)
			track->soundDesc = _sound->openSound(soundId, soundName, soundType, volGroupId, 2);

		if (!track->soundDesc)
			return -1;

		track->sndDataExtComp = _sound->isSndDataExtComp(track->soundDesc);

		bits = _sound->getBits(track->soundDesc);
		channels = _sound->getChannels(track->soundDesc);
		freq = _sound->getFreq(track->soundDesc);

		if ((soundId == kTalkSoundID) && (soundType == IMUSE_BUNDLE)) {
			if (_vm->_actorToPrintStrFor != 0xFF && _vm->_actorToPrintStrFor != 0) {
				Actor *a = _vm->derefActor(_vm->_actorToPrintStrFor, "DiMUSE_v1::startSound");
				freq = (freq * a->_talkFrequency) / 256;

				if (a->_talkPan <= 127)
					track->pan = a->_talkPan;
				else
					track->pan = 64;

				if (a->_talkVolume <= 127)
					track->vol = a->_talkVolume * 1000;
				else
					track->vol = 127 * 1000;

				// Keep track of the current actor in COMI:
				// This is necessary since pan and volume settings for actors
				// are often changed AFTER the speech sound is started,
				// so this is a way to keep track of the changes in real time.
				if (_vm->_game.id == GID_CMI)
					track->speakingActor = a;
			}

			// The volume is set to zero, when using subtitles only setting in COMI
			if (ConfMan.getBool("speech_mute") || _vm->VAR(_vm->VAR_VOICE_MODE) == 2) {
				track->vol = 0;
			}
		}

		assert(bits == 8 || bits == 12 || bits == 16);
		assert(channels == 1 || channels == 2);
		assert(0 < freq && freq <= 65535);

		track->feedSize = freq * channels;
		if (channels == 2)
			track->mixerFlags = kFlagStereo;

		if ((bits == 12) || (bits == 16)) {
			track->mixerFlags |= kFlag16Bits;
			track->feedSize *= 2;
		} else if (bits == 8) {
			track->mixerFlags |= kFlagUnsigned;
		} else
			error("DiMUSE_v1::startSound(): Can't handle %d bit samples", bits);

		track->littleEndian = track->soundDesc->littleEndian;

		int fadeDelay = 30; // Default fade value if not found anywhere else

		if (otherTrack && otherTrack->used && !otherTrack->toBeRemoved) {
			track->curRegion = otherTrack->curRegion;
			track->dataOffset = otherTrack->dataOffset;
			track->regionOffset = otherTrack->regionOffset;
			track->dataMod12Bit = otherTrack->dataMod12Bit;

			if (_vm->_game.id == GID_CMI) {
				fadeDelay = otherTrack->volFadeDelay != 0 ? otherTrack->volFadeDelay : fadeDelay;
				track->regionOffset -= track->regionOffset >= (track->feedSize / _callbackFps) ? (track->feedSize / _callbackFps) : 0;
			}
		}
		if (_vm->_game.id != GID_DIG && (track->volGroupId == IMUSE_VOLGRP_MUSIC) && forceFadeIn) {
			// Fade in the new track
			track->vol = 0;
			track->volFadeDelay = fadeDelay;
			track->volFadeDest = volume * 1000;
			track->volFadeStep = (track->volFadeDest - track->vol) * 60 * (1000 / _callbackFps) / (1000 * fadeDelay);
			track->volFadeUsed = true;
		}

		track->stream = Audio::makeQueuingAudioStream(freq, track->mixerFlags & kFlagStereo);
		_mixer->playStream(track->getType(), &track->mixChanHandle, track->stream, -1, track->getVol(), track->getPan());
	}

	track->used = true;

	return track->trackId;
}

void DiMUSE_v1::setPriority(int soundId, int priority) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setPriority()");
	debug(5, "DiMUSE_v1::setPriority(%d, %d)", soundId, priority);
	assert ((priority >= 0) && (priority <= 127));

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			debug(5, "DiMUSE_v1::setPriority(%d) - setting", soundId);
			track->soundPriority = priority;
		}
	}
}

void DiMUSE_v1::setVolume(int soundId, int volume) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setVolume()");
	debug(5, "DiMUSE_v1::setVolume(%d, %d)", soundId, volume);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			debug(5, "DiMUSE_v1::setVolume(%d) - setting", soundId);
			track->vol = volume * 1000;
		}
	}
}

void DiMUSE_v1::setHookId(int soundId, int hookId) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setHookId()");

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			track->curHookId = hookId;
		}
	}
}

int DiMUSE_v1::getCurMusicSoundId() {
	Common::StackLock lock(_mutex, "DiMUSE_v1::getCurMusicSoundId()");
	int soundId = -1;

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->volGroupId == IMUSE_VOLGRP_MUSIC)) {
			soundId = track->soundId;
			break;
		}
	}

	return soundId;
}

void DiMUSE_v1::setPan(int soundId, int pan) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setPan()");
	debug(5, "DiMUSE_v1::setPan(%d, %d)", soundId, pan);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			debug(5, "DiMUSE_v1::setPan(%d) - setting", soundId);
			track->pan = pan;
		}
	}
}

void DiMUSE_v1::selectVolumeGroup(int soundId, int volGroupId) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::selectVolumeGroup()");
	debug(5, "DiMUSE_v1::setGroupVolume(%d, %d)", soundId, volGroupId);
	assert((volGroupId >= 1) && (volGroupId <= 4));

	if (volGroupId == 4)
		volGroupId = 3;

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			debug(5, "DiMUSE_v1::setVolumeGroup(%d) - setting", soundId);
			track->volGroupId = volGroupId;
		}
	}
}

void DiMUSE_v1::setFade(int soundId, int destVolume, int delay60HzTicks) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setFade()");
	debug(5, "DiMUSE_v1::setFade(%d, %d, %d)", soundId, destVolume, delay60HzTicks);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->soundId == soundId)) {
			debug(5, "DiMUSE_v1::setFade(%d) - setting", soundId);
			track->volFadeDelay = delay60HzTicks;
			track->volFadeDest = destVolume * 1000;
			track->volFadeStep = (track->volFadeDest - track->vol) * 60 * (1000 / _callbackFps) / (1000 * delay60HzTicks);
			track->volFadeUsed = true;
		}
	}
}

void DiMUSE_v1::fadeOutMusicAndStartNew(int fadeDelay, const char *filename, int soundId) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::fadeOutMusicAndStartNew()");
	debug(5, "DiMUSE_v1::fadeOutMusicAndStartNew(fade:%d, file:%s, sound:%d)", fadeDelay, filename, soundId);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->volGroupId == IMUSE_VOLGRP_MUSIC)) {
			debug(5, "DiMUSE_v1::fadeOutMusicAndStartNew(sound:%d) - starting", soundId);

			// Store the fadeDelay in the track: startMusicWithOtherPos will use it to
			// fade in the new track; this will match fade in and fade out speeds.
			if (_vm->_game.id == GID_CMI) {
				track->volFadeDelay = fadeDelay;
				startMusicWithOtherPos(filename, soundId, 0, 127, track);
				handleFadeOut(track, fadeDelay);
			} else {
				startMusicWithOtherPos(filename, soundId, 0, 127, track);
				cloneToFadeOutTrack(track, fadeDelay);
				flushTrack(track);
			}
			break;
		}
	}
}

void DiMUSE_v1::fadeOutMusic(int fadeDelay) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::fadeOutMusic()");
	debug(5, "DiMUSE_v1::fadeOutMusic(fade:%d)", fadeDelay);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->volGroupId == IMUSE_VOLGRP_MUSIC)) {
			debug(5, "DiMUSE_v1::fadeOutMusic(fade:%d, sound:%d)", fadeDelay, track->soundId);
			if (_vm->_game.id == GID_CMI || _vm->_game.id == GID_FT) {
				handleFadeOut(track, fadeDelay);
			} else {
				cloneToFadeOutTrack(track, fadeDelay);
				flushTrack(track);
			}
			break;
		}
	}
}

void DiMUSE_v1::setHookIdForMusic(int hookId) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setHookIdForMusic()");
	debug(5, "DiMUSE_v1::setHookIdForMusic(hookId:%d)", hookId);

	for (int l = 0; l < MAX_DIGITAL_TRACKS; l++) {
		Track *track = _track[l];
		if (track->used && !track->toBeRemoved && (track->volGroupId == IMUSE_VOLGRP_MUSIC)) {
			debug(5, "DiMUSE_v1::setHookIdForMusic - setting for sound:%d", track->soundId);
			track->curHookId = hookId;
			break;
		}
	}
}

void DiMUSE_v1::setTrigger(TriggerParams *trigger) {
	Common::StackLock lock(_mutex, "DiMUSE_v1::setTrigger()");
	debug(5, "DiMUSE_v1::setTrigger(%s)", trigger->filename);

	memcpy(&_triggerParams, trigger, sizeof(TriggerParams));
	_triggerUsed = true;
}

Track *DiMUSE_v1::handleFadeOut(Track *track, int fadeDelay) {
	track->volFadeDelay = fadeDelay != 0 ? fadeDelay : 60;
	track->volFadeDest = 0;
	track->volFadeStep = (track->volFadeDest - track->vol) * 60 * (1000 / _callbackFps) / (1000 * track->volFadeDelay);
	track->volFadeUsed = true;
	track->toBeRemoved = true;
	return track;
}


Track *DiMUSE_v1::cloneToFadeOutTrack(Track *track, int fadeDelay) {
	assert(track);
	Track *fadeTrack;

	debug(5, "cloneToFadeOutTrack(soundId:%d, fade:%d) - begin of func", track->trackId, fadeDelay);

	if (track->toBeRemoved) {
		error("cloneToFadeOutTrack: Tried to clone a track to be removed, please bug report");
		return NULL;
	}

	assert(track->trackId < MAX_DIGITAL_TRACKS);
	fadeTrack = _track[track->trackId + MAX_DIGITAL_TRACKS];
	if (fadeTrack->used) {
		debug(5, "cloneToFadeOutTrack: No free fade track, force flush fade soundId:%d", fadeTrack->soundId);
		flushTrack(fadeTrack);
		_mixer->stopHandle(fadeTrack->mixChanHandle);
	}

	// Clone the settings of the given track
	memcpy(fadeTrack, track, sizeof(Track));
	fadeTrack->trackId = track->trackId + MAX_DIGITAL_TRACKS;

	// Clone the sound.
	// leaving bug number for now #3005
	ImuseDigiSndMgr::SoundDesc *soundDesc = _sound->cloneSound(track->soundDesc);
	if (!soundDesc) {
		// it fail load open old song after switch to different CDs
		// so gave up
		error("Game not supported while playing on 2 different CDs");
	}
	track->soundDesc = soundDesc;

	// Set the volume fading parameters to indicate a fade out
	fadeTrack->volFadeDelay = fadeDelay;
	fadeTrack->volFadeDest = 0;
	fadeTrack->volFadeStep = (fadeTrack->volFadeDest - fadeTrack->vol) * 60 * (1000 / _callbackFps) / (1000 * fadeDelay);
	fadeTrack->volFadeUsed = true;

	// Create an appendable output buffer
	fadeTrack->stream = Audio::makeQueuingAudioStream(_sound->getFreq(fadeTrack->soundDesc), track->mixerFlags & kFlagStereo);
	_mixer->playStream(track->getType(), &fadeTrack->mixChanHandle, fadeTrack->stream, -1, fadeTrack->getVol(), fadeTrack->getPan());
	fadeTrack->used = true;
	debug(5, "cloneToFadeOutTrack() - end of func, soundId %d, fade soundId %d", track->soundId, fadeTrack->soundId);

	return fadeTrack;
}

int DiMUSE_v1::transformVolumeLinearToEqualPow(int volume, int mode) {
	if (volume == 0 || volume == 127000)
		return volume;

	int result = volume;
	if (!(volume < 0 || volume > 127000)) {
		// Change range of values from 0-127*1000 to 0.0-1.0
		double mappedValue = (((volume - 0) * (1.0 - 0.0)) / (127000 - 0)) + 0;
		double eqPowValue;

		switch (mode) {
		case 0:  // Sqrt curve
			eqPowValue = sqrt(mappedValue);
			break;
		case 1:  // Quarter sine curve
			eqPowValue = sin(mappedValue * M_PI / 2.0);
			break;
		case 2:  // Parabola
			eqPowValue = (1 - (1 - mappedValue) * (1 - mappedValue));
			break;
		case 3:  // Logarithmic 1
			eqPowValue = 1 + 0.2 * log10(mappedValue);
			break;
		case 4:  // Logarithmic 2
			eqPowValue = 1 + 0.5 * log10(mappedValue);
			break;
		case 5:  // Logarithmic 3
			eqPowValue = 1 + 0.7 * log10(mappedValue);
			break;
		case 6:  // Quadratic
			eqPowValue = mappedValue * mappedValue;
			break;
		default: // Fallback to linear
			eqPowValue = mappedValue;
			break;
		}

		// Change range again, this time round the result
		result = (((eqPowValue - 0.0) * (127000 - 0)) / (1.0 - 0.0)) + 0.0;
		result = (int)round(result);
	}

	return result;
}

int DiMUSE_v1::transformVolumeEqualPowToLinear(int volume, int mode) {
	if (volume == 0 || volume == 127000)
		return volume;

	int result = volume;
	if (!(volume < 0 || volume > 127000)) {
		// Change range of values from 0-127*1000 to 0.0-1.0
		double mappedValue = (((volume - 0) * (1.0 - 0.0)) / (127000 - 0)) + 0;
		double linearValue;

		switch (mode) {
		case 0:  // Sqrt curve
			linearValue = mappedValue * mappedValue;
			break;
		case 1:  // Quarter sine curve
			linearValue = 0.63662 * asin(mappedValue);
			break;
		case 2:  // Parabola
			linearValue = 1 - sqrt(1 - mappedValue);
			break;
		case 3:  // Logarithmic 1
			linearValue = 0.00001 * pow(M_E, 11.5129 * mappedValue);
			break;
		case 4:  // Logarithmic 2
			linearValue = 0.01 * pow(M_E, 4.60517 * mappedValue);
			break;
		case 5:  // Logarithmic 3
			linearValue = 0.0372759 * pow(M_E, 3.28941 * mappedValue);
			break;
		case 6:  // Quadratic
			linearValue = sqrt(mappedValue);
			break;
		default: // Fallback to linear
			linearValue = mappedValue;
			break;
		}

		// Change range again, this time round the result
		result = (((linearValue - 0.0) * (127000 - 0)) / (1.0 - 0.0)) + 0.0;
		result = (int)round(result);
	}

	return result;
}

} // End of namespace Scumm
