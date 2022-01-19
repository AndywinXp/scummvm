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

#include "engines/grim/imuse/imuse_engine.h"

namespace Grim {

int Imuse::tracksInit() {
	_trackCount = 9;
	_tracksPauseTimer = 0;
	_trackList = nullptr;
	_tracksPrefSampleRate = DIMUSE_SAMPLERATE;

	if (waveOutInit(DIMUSE_SAMPLERATE, &waveOutSettings))
		return -1;

	if (_internalMixer->init(waveOutSettings.bytesPerSample,
			waveOutSettings.numChannels,
			waveOutSettings.mixBuf,
			waveOutSettings.mixBufSize,
			waveOutSettings.sizeSampleKB,
			_trackCount) ||
			dispatchInit() ||
		streamerInit()) {
		return -1;
	}

	for (int l = 0; l < _trackCount; l++) {
		_tracks[l].prev = nullptr;
		_tracks[l].next = nullptr;
		_tracks[l].dispatchPtr = dispatchGetDispatchByTrackId(l);
		_tracks[l].dispatchPtr->trackPtr = &_tracks[l];
		_tracks[l].soundId = 0;
	}

	return 0;
}

void Imuse::tracksPause() {
	_tracksPauseTimer = 1;
}

void Imuse::tracksResume() {
	_tracksPauseTimer = 0;
}

void Imuse::tracksSaveLoad(Common::Serializer &ser) {
	//Common::StackLock lock(_mutex);
	//dispatchSaveLoad(ser);
	//
	//for (int l = 0; l < _trackCount; l++) {
	//	ser.syncAsSint32LE(_tracks[l].soundId, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].marker, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].group, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].priority, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].vol, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].effVol, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].pan, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].detune, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].transpose, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].pitchShift, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].mailbox, VER(103));
	//	ser.syncAsSint32LE(_tracks[l].jumpHook, VER(103));
	//
	//	if (_vm->_game.id == GID_CMI) {
	//		ser.syncAsSint32LE(_tracks[l].syncSize_0, VER(103));
	//		ser.syncAsSint32LE(_tracks[l].syncSize_1, VER(103));
	//		ser.syncAsSint32LE(_tracks[l].syncSize_2, VER(103));
	//		ser.syncAsSint32LE(_tracks[l].syncSize_3, VER(103));
	//
	//		if (_tracks[l].syncSize_0) {
	//			if (ser.isLoading())
	//				_tracks[l].syncPtr_0 = (byte *)malloc(_tracks[l].syncSize_0);
	//			ser.syncArray(_tracks[l].syncPtr_0, _tracks[l].syncSize_0, Common::Serializer::Byte, VER(103));
	//		}
	//
	//		if (_tracks[l].syncSize_1) {
	//			if (ser.isLoading())
	//				_tracks[l].syncPtr_1 = (byte *)malloc(_tracks[l].syncSize_1);
	//			ser.syncArray(_tracks[l].syncPtr_1, _tracks[l].syncSize_1, Common::Serializer::Byte, VER(103));
	//		}
	//
	//		if (_tracks[l].syncSize_2) {
	//			if (ser.isLoading())
	//				_tracks[l].syncPtr_2 = (byte *)malloc(_tracks[l].syncSize_2);
	//			ser.syncArray(_tracks[l].syncPtr_2, _tracks[l].syncSize_2, Common::Serializer::Byte, VER(103));
	//		}
	//
	//		if (_tracks[l].syncSize_3) {
	//			if (ser.isLoading())
	//				_tracks[l].syncPtr_3 = (byte *)malloc(_tracks[l].syncSize_3);
	//			ser.syncArray(_tracks[l].syncPtr_3, _tracks[l].syncSize_3, Common::Serializer::Byte, VER(103));
	//		}
	//	}
	//}
	//
	//if (ser.isLoading()) {
	//	for (int l = 0; l < _trackCount; l++) {
	//		_tracks[l].prev = nullptr;
	//		_tracks[l].next = nullptr;
	//		_tracks[l].dispatchPtr = dispatchGetDispatchByTrackId(l);
	//		_tracks[l].dispatchPtr->trackPtr = &_tracks[l];
	//		if (_tracks[l].soundId) {
	//			addTrackToList(&_trackList, &_tracks[l]);
	//		}
	//	}
	//
	//	dispatchRestoreStreamZones();
	//}
}

void Imuse::tracksSetGroupVol() {
	IMuseDigiTrack* curTrack = _trackList;
	while (curTrack) {
		curTrack->effVol = ((curTrack->vol + 1) * _groupsHandler->getGroupVol(curTrack->group)) / 128;
		curTrack = curTrack->next;
	}
}

void Imuse::tracksCallback() {
	if (_tracksPauseTimer) {
		if (++_tracksPauseTimer < 3)
			return;
		_tracksPauseTimer = 3;
	}

	Common::StackLock lock(_mutex);

	// If we leave the number of queued streams unbounded, we fill the queue with streams faster than
	// we can play them: this leads to a very noticeable audio latency and desync with the graphics.
	if ((int)_internalMixer->_stream->numQueuedStreams() < _maxQueuedStreams) {
		dispatchPredictFirstStream();

		waveOutWrite(&_outputAudioBuffer, _outputFeedSize, _outputSampleRate);

		if (_outputFeedSize) {
			_internalMixer->clearMixerBuffer();
			if (!_tracksPauseTimer) {
				IMuseDigiTrack *track = _trackList;

				while (track) {
					IMuseDigiTrack *next = track->next;
					dispatchProcessDispatches(track, _outputFeedSize, _outputSampleRate);
					track = next;
				};
			}

			_internalMixer->loop(&_outputAudioBuffer, _outputFeedSize);
		}
	}
}

int Imuse::tracksStartSound(int soundId, int tryPriority, int group) {
	int priority = clampNumber(tryPriority, 0, 127);

	Debug::debug(Debug::Sound, "Imuse::tracksStartSound(): sound %d with priority %d and group %d", soundId, priority, group);
	IMuseDigiTrack *allocatedTrack = tracksReserveTrack(priority);

	if (!allocatedTrack) {
		Debug::debug(Debug::Sound, "Imuse::tracksStartSound(): ERROR: couldn't find a spare track to allocate sound %d", soundId);
		return -6;
	}

	allocatedTrack->soundId = soundId;
	allocatedTrack->marker = 0;
	allocatedTrack->group = 0;
	allocatedTrack->priority = priority;
	allocatedTrack->vol = 127;
	allocatedTrack->effVol = _groupsHandler->getGroupVol(0);
	allocatedTrack->pan = 64;
	allocatedTrack->detune = 0;
	allocatedTrack->transpose = 0;
	allocatedTrack->pitchShift = 256;
	allocatedTrack->mailbox = 0;
	allocatedTrack->jumpHook = 0;

	if (dispatchAllocateSound(allocatedTrack, group)) {
		Debug::debug(Debug::Sound, "Imuse::tracksStartSound(): ERROR: dispatch couldn't start sound %d", soundId);
		allocatedTrack->soundId = 0;
		return -1;
	}

	Common::StackLock lock(_mutex);
	addTrackToList(&_trackList, allocatedTrack);
	Common::StackLock unlock(_mutex);

	return 0;
}

int Imuse::tracksStopSound(int soundId) {
	if (!_trackList)
		return -1;

	IMuseDigiTrack *track = _trackList;
	do {
		if (track->soundId == soundId) {
			tracksClear(track);
		}
		track = track->next;
	} while (track);

	return 0;
}

int Imuse::tracksStopAllSounds() {
	Common::StackLock lock(_mutex);
	IMuseDigiTrack *nextTrack = _trackList;
	IMuseDigiTrack *curTrack;

	while (nextTrack) {
		curTrack = nextTrack;
		nextTrack = curTrack->next;
		tracksClear(curTrack);
	}

	_filesHandler->closeAllSounds();

	return 0;
}

int Imuse::tracksGetNextSound(int soundId) {
	int foundSoundId = 0;
	IMuseDigiTrack *track = _trackList;
	while (track) {
		if (track->soundId > soundId) {
			if (!foundSoundId || track->soundId < foundSoundId) {
				foundSoundId = track->soundId;
			}
		}
		track = track->next;
	};

	return foundSoundId;
}

void Imuse::tracksQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused) {
	if (!_trackList)
		Debug::debug(Debug::Sound, "Imuse::tracksQueryStream(): WARNING: empty trackList, ignoring call...");

	IMuseDigiTrack *track = _trackList;
	do {
		if (track->soundId) {
			if (soundId == track->soundId && track->dispatchPtr->streamPtr) {
				streamerQueryStream(track->dispatchPtr->streamPtr, bufSize, criticalSize, freeSpace, paused);
				return;
			}
		}
		track = track->next;
	} while (track);

	Debug::debug(Debug::Sound, "Imuse::tracksQueryStream(): WARNING: couldn't find sound %d in trackList, ignoring call...", soundId);
}

int Imuse::tracksFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused) {
	if (!_trackList)
		return -1;

	IMuseDigiTrack *track = _trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamerFeedStream(track->dispatchPtr->streamPtr, srcBuf, sizeToFeed, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);

	return -1;
}

void Imuse::tracksClear(IMuseDigiTrack *trackPtr) {
	removeTrackFromList(&_trackList, trackPtr);
	dispatchRelease(trackPtr);
	_fadesHandler->clearFadeStatus(trackPtr->soundId, -1);
	_triggersHandler->clearTrigger(trackPtr->soundId, _emptyMarker, -1);

	// Unlock the sound, if it's a SFX
	//if (trackPtr->soundId < 1000 && trackPtr->soundId) {
	//	_vm->_res->unlock(rtSound, trackPtr->soundId);
	//}

	trackPtr->soundId = 0;
}

int Imuse::tracksSetParam(int soundId, int transitionType, int value) {
	if (!_trackList)
		return -4;

	IMuseDigiTrack *track = _trackList;
	while (track) {
		if (track->soundId == soundId) {
			switch (transitionType) {
			case DIMUSE_P_GROUP:
				if (value >= 16)
					return -5;
				track->group = value;
				track->effVol = ((track->vol + 1) * _groupsHandler->getGroupVol(value)) / 128;
				return 0;
			case DIMUSE_P_PRIORITY:
				if (value > 127)
					return -5;
				track->priority = value;
				return 0;
			case DIMUSE_P_VOLUME:
				if (value > 127)
					return -5;
				track->vol = value;
				track->effVol = ((value + 1) * _groupsHandler->getGroupVol(track->group)) / 128;
				return 0;
			case DIMUSE_P_PAN:
				if (value > 127)
					return -5;
				track->pan = value;
				return 0;
			case DIMUSE_P_DETUNE:
				if (value < -9216 || value > 9216)
					return -5;
				track->detune = value;
				track->pitchShift = value + track->transpose * 256;
				return 0;
			case DIMUSE_P_TRANSPOSE:
				if (value < 0 || value > 4095)
					return -5;
				track->pitchShift = value;
				return 0;
			case DIMUSE_P_MAILBOX:
				track->mailbox = value;
				return 0;
			default:
				Debug::debug(Debug::Sound, "Imuse::tracksSetParam(): unknown transitionType %d", transitionType);
				return -5;
			}
		}
		track = track->next;
	}

	return -4;
}

int Imuse::tracksGetParam(int soundId, int transitionType) {
	if (!_trackList) {
		if (transitionType != DIMUSE_P_SND_TRACK_NUM)
			return -4;
		else
			return 0;
	}

	IMuseDigiTrack *track = _trackList;
	int l = 0;
	do {
		if (track)
			l++;
		if (track->soundId == soundId) {
			switch (transitionType) {
			case DIMUSE_P_BOGUS_ID:
				return -1;
			case DIMUSE_P_SND_TRACK_NUM:
				return l;
			case DIMUSE_P_TRIGS_SNDS:
				return -1;
			case DIMUSE_P_MARKER:
				return track->marker;
			case DIMUSE_P_GROUP:
				return track->group;
			case DIMUSE_P_PRIORITY:
				return track->priority;
			case DIMUSE_P_VOLUME:
				return track->vol;
			case DIMUSE_P_PAN:
				return track->pan;
			case DIMUSE_P_DETUNE:
				return track->detune;
			case DIMUSE_P_TRANSPOSE:
				return track->transpose;
			case DIMUSE_P_MAILBOX:
				return track->mailbox;
			case DIMUSE_P_SND_HAS_STREAM:
				return (track->dispatchPtr->streamPtr != 0);
			case DIMUSE_P_STREAM_BUFID:
				return track->dispatchPtr->streamBufID;
			case DIMUSE_P_SND_POS_IN_MS: // getCurSoundPositionInMs
				if (track->dispatchPtr->wordSize == 0)
					return 0;
				if (track->dispatchPtr->sampleRate == 0)
					return 0;
				if (track->dispatchPtr->channelCount == 0)
					return 0;
				return (track->dispatchPtr->currentOffset * 5) / (((track->dispatchPtr->wordSize / 8) * track->dispatchPtr->sampleRate * track->dispatchPtr->channelCount) / 200);
			default:
				return -5;
			}
		}

		track = track->next;
	} while (track);

	return 0;
}

int Imuse::tracksSetHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!_trackList)
		return -4;

	IMuseDigiTrack *track = _trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	track->jumpHook = hookId;

	return 0;
}

int Imuse::tracksGetHook(int soundId) {
	if (!_trackList)
		return -4;

	IMuseDigiTrack *track = _trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	return track->jumpHook;
}

IMuseDigiTrack *Imuse::tracksReserveTrack(int priority) {
	IMuseDigiTrack *curTrack;
	IMuseDigiTrack *reservedTrack = nullptr;
	int minPriorityFound;

	// Pick the track from the pool of free tracks
	for (int i = 0; i < _trackCount; i++) {
		reservedTrack = &_tracks[i];
		if (!reservedTrack->soundId) {
			return reservedTrack;
		}
	}

	// If no free track is found, steal the lower priority one
	curTrack = _trackList;
	for (minPriorityFound = 127; curTrack; curTrack = curTrack->next) {
		if (curTrack->priority <= minPriorityFound) {
			minPriorityFound = curTrack->priority;
			reservedTrack = curTrack;
		}
	}

	if (reservedTrack && priority >= minPriorityFound) {
		tracksClear(reservedTrack);
	}

	return reservedTrack;
}

void Imuse::tracksDeinit() {
	tracksStopAllSounds();
}

} // End of namespace Scumm
