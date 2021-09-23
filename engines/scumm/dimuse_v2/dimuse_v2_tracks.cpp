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

#include "scumm/dimuse_v2/dimuse_v2.h"

namespace Scumm {

int DiMUSE_v2::tracksInit() {
	_trackCount = 6;
	_tracksPauseTimer = 0;
	_trackList = NULL;
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
			streamerInit())
		return -1;
	
	for (int l = 0; l < _trackCount; l++) {
		_tracks[l].prev = NULL;
		_tracks[l].next = NULL;
		_tracks[l].dispatchPtr = dispatchGetDispatchByTrackId(l);
		_tracks[l].dispatchPtr->trackPtr = &_tracks[l];
		_tracks[l].soundId = 0;
		_tracks[l].syncSize_0 = 0;
		_tracks[l].syncSize_1 = 0;
		_tracks[l].syncSize_2 = 0;
		_tracks[l].syncSize_3 = 0;
		_tracks[l].syncPtr_0 = NULL;
		_tracks[l].syncPtr_1 = NULL;
		_tracks[l].syncPtr_2 = NULL;
		_tracks[l].syncPtr_3 = NULL;
	}

	return 0;
}

void DiMUSE_v2::tracksPause() {
	_tracksPauseTimer = 1;
}

void DiMUSE_v2::tracksResume() {
	_tracksPauseTimer = 0;
}

void DiMUSE_v2::tracksSaveLoad(Common::Serializer &ser) {
	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	dispatchSaveLoad(ser);

	for (int l = 0; l < _trackCount; l++) {
		ser.syncAsSint32LE(_tracks[l].soundId, VER(103));
		ser.syncAsSint32LE(_tracks[l].marker, VER(103));
		ser.syncAsSint32LE(_tracks[l].group, VER(103));
		ser.syncAsSint32LE(_tracks[l].priority, VER(103));
		ser.syncAsSint32LE(_tracks[l].vol, VER(103));
		ser.syncAsSint32LE(_tracks[l].effVol, VER(103));
		ser.syncAsSint32LE(_tracks[l].pan, VER(103));
		ser.syncAsSint32LE(_tracks[l].detune, VER(103));
		ser.syncAsSint32LE(_tracks[l].transpose, VER(103));
		ser.syncAsSint32LE(_tracks[l].pitchShift, VER(103));
		ser.syncAsSint32LE(_tracks[l].mailbox, VER(103));
		ser.syncAsSint32LE(_tracks[l].jumpHook, VER(103));

		if (_vm->_game.id == GID_CMI) {
			ser.syncAsSint32LE(_tracks[l].syncSize_0, VER(103));
			ser.syncAsSint32LE(_tracks[l].syncSize_1, VER(103));
			ser.syncAsSint32LE(_tracks[l].syncSize_2, VER(103));
			ser.syncAsSint32LE(_tracks[l].syncSize_3, VER(103));

			if (_tracks[l].syncSize_0) {
				if (ser.isLoading())
					_tracks[l].syncPtr_0 = (byte *)malloc(_tracks[l].syncSize_0);
				ser.syncArray(_tracks[l].syncPtr_0, _tracks[l].syncSize_0, Common::Serializer::Byte, VER(103));
			}

			if (_tracks[l].syncSize_1) {
				if (ser.isLoading())
					_tracks[l].syncPtr_1 = (byte *)malloc(_tracks[l].syncSize_1);
				ser.syncArray(_tracks[l].syncPtr_1, _tracks[l].syncSize_1, Common::Serializer::Byte, VER(103));
			}

			if (_tracks[l].syncSize_2) {
				if (ser.isLoading())
					_tracks[l].syncPtr_2 = (byte *)malloc(_tracks[l].syncSize_2);
				ser.syncArray(_tracks[l].syncPtr_2, _tracks[l].syncSize_2, Common::Serializer::Byte, VER(103));
			}

			if (_tracks[l].syncSize_3) {
				if (ser.isLoading())
					_tracks[l].syncPtr_3 = (byte *)malloc(_tracks[l].syncSize_3);
				ser.syncArray(_tracks[l].syncPtr_3, _tracks[l].syncSize_3, Common::Serializer::Byte, VER(103));
			}
		}
	}

	if (ser.isLoading()) {
		if (_trackCount > 0) {
			for (int l = 0; l < _trackCount; l++) {
				_tracks[l].prev = NULL;
				_tracks[l].next = NULL;
				_tracks[l].dispatchPtr = dispatchGetDispatchByTrackId(l);
				_tracks[l].dispatchPtr->trackPtr = &_tracks[l];
				if (_tracks[l].soundId) {
					addTrackToList(&_trackList, &_tracks[l]);
				}
			}
		}

		dispatchAllocStreamZones();
	}

	//waveOutDecreaseSlice();
}

void DiMUSE_v2::tracksSetGroupVol() {
	DiMUSETrack* curTrack = _trackList;
	while (curTrack) {
		curTrack->effVol = ((curTrack->vol + 1) * _groupsHandler->getGroupVol(curTrack->group)) / 128;
		curTrack = curTrack->next;
	}
}

void DiMUSE_v2::tracksCallback() {
	if (_tracksPauseTimer) {
		if (++_tracksPauseTimer < 3)
			return;
		_tracksPauseTimer = 3;
	}

	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	//debug(5, "tracks_callback() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);

	// If we leave the number of queued streams unbounded, we fill the queue with streams faster than
	// we can play them: this leads to a very noticeable audio latency and desync with the graphics.
	// While 8 should be the limit sort of used by the EXE, this still creates a very small but
	// noticeable latency; the best scenario would be to put the limit to 2, but this appears to cause
	// some small desyncs on the crossfades, so until I understand why that desync happens since we're throwing
	// everything into a single audio buffer, keeping the limit to 4 appears to partially suffice for both issues
	if (_internalMixer->_stream->numQueuedStreams() < 4) {
		dispatchPredictFirstStream();

		waveOutWrite(&_outputAudioBuffer, &_outputFeedSize, &_outputSampleRate);

		if (_outputFeedSize != 0) {
			_internalMixer->clearMixerBuffer();
			if (!_tracksPauseTimer) {
				DiMUSETrack *track = _trackList;

				while (track) {
					DiMUSETrack *next = track->next;
					dispatchProcessDispatches(track, _outputFeedSize, _outputSampleRate);
					track = next;
				};
			}

			_internalMixer->loop(&_outputAudioBuffer, _outputFeedSize);

			// The Dig tries to write a second time
			if (_vm->_game.id == GID_DIG) {
				waveOutWrite(&_outputAudioBuffer, &_outputFeedSize, &_outputSampleRate);
			}
		}
	}
	
	//waveOutDecreaseSlice();
	//debug(5, "tracks_callback() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
}

int DiMUSE_v2::tracksStartSound(int soundId, int tryPriority, int group) {
	debug(5, "DiMUSE_v2::tracksStartSound(): sound %d with priority %d and group %d", soundId, tryPriority, group);
	int priority = clampNumber(tryPriority, 0, 127);
	if (_trackCount > 0) {
		int l = 0;
		while (_tracks[l].soundId) {
			l++;
			if (l >= _trackCount) {
				l = -1;
				break;
			}
		}

		if (l != -1) {
			DiMUSETrack *foundTrack = &_tracks[l];
			if (!foundTrack)
				return -6;

			foundTrack->soundId = soundId;
			foundTrack->marker = 0;
			foundTrack->group = 0;
			foundTrack->priority = priority;
			foundTrack->vol = 127;
			foundTrack->effVol = _groupsHandler->getGroupVol(0);
			foundTrack->pan = 64;
			foundTrack->detune = 0;
			foundTrack->transpose = 0;
			foundTrack->pitchShift = 256;
			foundTrack->mailbox = 0;
			foundTrack->jumpHook = 0;
			foundTrack->syncSize_0 = 0;
			foundTrack->syncPtr_0 = NULL;
			foundTrack->syncSize_1 = 0;
			foundTrack->syncPtr_1 = NULL;
			foundTrack->syncSize_2 = 0;
			foundTrack->syncPtr_2 = NULL;
			foundTrack->syncSize_3 = 0;
			foundTrack->syncPtr_3 = NULL;

			if (soundId == kTalkSoundID) {
				_currentSpeechVolume = 127;
				_currentSpeechPan = 64;
				_currentSpeechFrequency = 0;
			}

			if (dispatchAlloc(foundTrack, group)) {
				debug(5, "DiMUSE_v2::tracksStartSound(): ERROR: dispatch couldn't start sound %d", soundId);
				foundTrack->soundId = 0;
				return -1;
			}
			Common::StackLock lock(_mutex);
			//waveOutIncreaseSlice();
			//debug(5, "tracksStartSound() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);
			addTrackToList(&_trackList, foundTrack);
			//waveOutDecreaseSlice();
			//debug(5, "tracksStartSound() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
			Common::StackLock unlock(_mutex);
			return 0;
		}
	}

	debug(5, "DiMUSE_v2::tracksStartSound(): WARNING: no spare tracks for sound %d, attempting to steal a lower priority track", soundId);

	// Let's steal the track with the lowest priority
	DiMUSETrack *track = _trackList;
	int bestPriority = 127;
	DiMUSETrack *stolenTrack = NULL;

	while (track) {
		int curTrackPriority = track->priority;
		if (curTrackPriority <= bestPriority) {
			bestPriority = curTrackPriority;
			stolenTrack = track;
		}
		track = track->next;
	}

	if (!stolenTrack || priority < bestPriority) {
		debug(5, "DiMUSE_v2::tracksStartSound(): ERROR: couldn't steal a lower priority track for sound %d", soundId);
		return -6;
	} else {
		removeTrackFromList(&_trackList, stolenTrack);
		dispatchRelease(stolenTrack);
		_fadesHandler->clearFadeStatus(stolenTrack->soundId, -1);
		_triggersHandler->clearTrigger(stolenTrack->soundId, _emptyMarker, -1);
		stolenTrack->soundId = 0;
	}

	stolenTrack->soundId = soundId;
	stolenTrack->marker = 0;
	stolenTrack->group = 0;
	stolenTrack->priority = priority;
	stolenTrack->vol = 127;
	stolenTrack->effVol = _groupsHandler->getGroupVol(0);
	stolenTrack->pan = 64;
	stolenTrack->detune = 0;
	stolenTrack->transpose = 0;
	stolenTrack->pitchShift = 256;
	stolenTrack->mailbox = 0;
	stolenTrack->jumpHook = 0;
	stolenTrack->syncSize_0 = 0;
	stolenTrack->syncPtr_0 = NULL;
	stolenTrack->syncSize_1 = 0;
	stolenTrack->syncPtr_1 = NULL;
	stolenTrack->syncSize_2 = 0;
	stolenTrack->syncPtr_2 = NULL;
	stolenTrack->syncSize_3 = 0;
	stolenTrack->syncPtr_3 = NULL;

	if (dispatchAlloc(stolenTrack, group)) {
		debug(5, "DiMUSE_v2::tracksStartSound(): ERROR: dispatch couldn't start sound %d", soundId);
		stolenTrack->soundId = 0;
		return -1;
	}

	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	//debug(5, "tracksStartSound() called waveOutIncreaseSlice() 2: %d", _waveSlicingHalted);
	addTrackToList(&_trackList, stolenTrack);
	//waveOutDecreaseSlice();
	//debug(5, "tracksStartSound() called waveOutDecreaseSlice() 2: %d", _waveSlicingHalted);
	Common::StackLock unlock(_mutex);

	return 0;
}

int DiMUSE_v2::tracksStopSound(int soundId) {
	if (!_trackList)
		return -1;

	DiMUSETrack *track = _trackList;
	do {
		if (track->soundId == soundId) {
			removeTrackFromList(&_trackList, track);
			dispatchRelease(track);
			_fadesHandler->clearFadeStatus(track->soundId, -1);
			_triggersHandler->clearTrigger(track->soundId, (char *)-1, -1);
			track->soundId = 0;
		}
		track = track->next;
	} while (track);

	return 0;
}

int DiMUSE_v2::tracksStopAllSounds() {
	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	//debug(5, "tracks_stopAllSounds() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);
	DiMUSETrack *nextTrack = _trackList;
	DiMUSETrack *curTrack;

	if (_vm->_game.id == GID_DIG) {
		if (_trackList) {
			do {
				curTrack = nextTrack->next;
				removeTrackFromList(&_trackList, nextTrack);
				dispatchRelease(nextTrack);
				_fadesHandler->clearFadeStatus(nextTrack->soundId, -1);
				_triggersHandler->clearTrigger(nextTrack->soundId, _emptyMarker, -1);
				nextTrack->soundId = 0;
				nextTrack = curTrack;
			} while (nextTrack);
		}
	} else {
		while (nextTrack) {
			curTrack = nextTrack;
			nextTrack = curTrack->next;
			tracksClear(curTrack);
		}
	}
	

	//waveOutDecreaseSlice();
	//debug(5, "tracks_stopAllSounds() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
	return 0;
}

int DiMUSE_v2::tracksGetNextSound(int soundId) {
	int found_soundId = 0;
	DiMUSETrack *track = _trackList;
	while (track) {
		if (track->soundId > soundId) {
			if (!found_soundId || track->soundId < found_soundId) {
				found_soundId = track->soundId;
			}
		}
		track = track->next;
	};

	return found_soundId;
}

int DiMUSE_v2::tracksQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	if (!_trackList)
		return -1;

	DiMUSETrack *track = _trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamerQueryStream(track->dispatchPtr->streamPtr, bufSize, criticalSize, freeSpace, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);

	return -1;
}

int DiMUSE_v2::tracksFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused) {
	if (!_trackList)
		return -1;

	DiMUSETrack *track = _trackList;
	do {
		if (track->soundId != 0) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamerFeedStream(track->dispatchPtr->streamPtr, srcBuf, sizeToFeed, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);

	return -1;
}

void DiMUSE_v2::tracksClear(DiMUSETrack *trackPtr) {
	if (_vm->_game.id == GID_CMI) {
		if (trackPtr->syncPtr_0) {
			trackPtr->syncSize_0 = 0;
			free(trackPtr->syncPtr_0);
			trackPtr->syncPtr_0 = NULL;
		}

		if (trackPtr->syncPtr_1) {
			trackPtr->syncSize_1 = 0;
			free(trackPtr->syncPtr_1);
			trackPtr->syncPtr_1 = NULL;
		}

		if (trackPtr->syncPtr_2) {
			trackPtr->syncSize_2 = 0;
			free(trackPtr->syncPtr_2);
			trackPtr->syncPtr_2 = NULL;
		}

		if (trackPtr->syncPtr_3) {
			trackPtr->syncSize_3 = 0;
			free(trackPtr->syncPtr_3);
			trackPtr->syncPtr_3 = NULL;
		}
	}

	removeTrackFromList(&_trackList, trackPtr);
	dispatchRelease(trackPtr);
	_fadesHandler->clearFadeStatus(trackPtr->soundId, -1);
	_triggersHandler->clearTrigger(trackPtr->soundId, _emptyMarker, -1);

	if (trackPtr->soundId < 1000 && trackPtr->soundId) {
		_vm->_res->unlock(rtSound, trackPtr->soundId);
	}

	trackPtr->soundId = 0;
}

int DiMUSE_v2::tracksSetParam(int soundId, int opcode, int value) {
	if (!_trackList)
		return -4;

	DiMUSETrack *track = _trackList;
	while (track) {
		if (track->soundId == soundId) {
			switch (opcode) {
			case 0x400:
				if (value >= 16)
					return -5;
				track->group = value;
				track->effVol = ((track->vol + 1) * _groupsHandler->getGroupVol(value)) / 128;
				return 0;
			case 0x500:
				if (value > 127)
					return -5;
				track->priority = value;
				return 0;
			case 0x600:
				if (value > 127)
					return -5;
				track->vol = value;
				track->effVol = ((value + 1) * _groupsHandler->getGroupVol(track->group)) / 128;
				return 0;
			case 0x700:
				if (value > 127)
					return -5;
				track->pan = value;
				return 0;
			case 0x800:
				if (value < -9216 || value > 9216)
					return -5;
				track->detune = value;
				track->pitchShift = value + track->transpose * 256;
				return 0;
			case 0x900:
				if (_vm->_game.id == GID_DIG) {
					if (value < -12 || value > 12)
						return -5;

					if (value == 0) {
						track->transpose = 0;
					} else {
						track->transpose = clampTuning(track->detune + value, -12, 12);
					}

					track->pitchShift = track->detune + (track->transpose * 256);
				} else if (_vm->_game.id == GID_CMI) {
					if (value < 0 || value > 4095)
						return -5;

					track->pitchShift = value;
				}
				
				return 0;
			case 0xA00:
				track->mailbox = value;
				return 0;
			default:
				debug(5, "DiMUSE_v2::tracksSetParam(): unknown opcode %d", opcode);
				return -5;
			}
		}
		track = track->next;
	}

	return -4;
}

int DiMUSE_v2::tracksGetParam(int soundId, int opcode) {
	if (!_trackList) {
		if (opcode != 0x100)
			return -4;
		else
			return 0;
	}
	DiMUSETrack *track = _trackList;
	int l = 0;
	do {
		if (track)
			l++;
		if (track->soundId == soundId) {
			switch (opcode) {
			case 0:
				return -1;
			case 0x100:
				return l;
			case 0x200:
				return -1;
			case 0x300:
				return track->marker;
			case 0x400:
				return track->group;
			case 0x500:
				return track->priority;
			case 0x600:
				return track->vol;
			case 0x700:
				return track->pan;
			case 0x800:
				return track->detune;
			case 0x900:
				return track->transpose;
			case 0xA00:
				return track->mailbox;
			case 0x1800:
				return (track->dispatchPtr->streamPtr != 0);
			case 0x1900:
				return track->dispatchPtr->streamBufID;
			case 0x1A00: // getCurSoundPositionInMs
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

	if (opcode != 0x100)
		return -4;
	else
		return 0;
}

int DiMUSE_v2::tracksLipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height) {
	int h, w;

	byte *syncPtr = NULL;
	int syncSize = 0;

	DiMUSETrack *curTrack;
	uint16 msPosDiv;
	uint16 *tmpPtr;
	int loopIndex;
	int16 val;

	h = 0;
	w = 0;
	curTrack = _trackList;

	if (msPos >= 0) {
		msPosDiv = msPos >> 4;
		if (((msPos >> 4) & 0xFFFF0000) != 0) {
			return -5;
		} else {
			if (_trackList) {
				do {
					if (curTrack->soundId == soundId)
						break;
					curTrack = curTrack->next;
				} while (curTrack);
			}

			if (curTrack) {
				if (syncId >= 0 && syncId < 4) {
					if (syncId == 0) {
						syncPtr = curTrack->syncPtr_0;
						syncSize = curTrack->syncSize_0;
					} else if (syncId == 1) {
						syncPtr = curTrack->syncPtr_1;
						syncSize = curTrack->syncSize_1;
					} else if (syncId == 2) {
						syncPtr = curTrack->syncPtr_2;
						syncSize = curTrack->syncSize_2;
					} else if (syncId == 3) {
						syncPtr = curTrack->syncPtr_3;
						syncSize = curTrack->syncSize_3;
					}

					if (syncSize && syncPtr) {
						tmpPtr = (uint16 *)(syncPtr + 2);
						loopIndex = (syncSize >> 2) - 1;
						if (syncSize >> 2) {
							do {
								if (*tmpPtr >= msPosDiv)
									break;
								tmpPtr += 2;
							} while (loopIndex--);
						}

						if (loopIndex < 0 || *tmpPtr > msPosDiv)
							tmpPtr -= 2;

						val = *(tmpPtr - 1);
						w = (val >> 8) & 0x7F;
						h = val & 0x7F;
					}
				}
			} else {
				return -4;
			}
		}
	}

	if (width)
		*width = w;
	if (height)
		*height = h;

	return 0;
}

int DiMUSE_v2::tracksSetHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!_trackList)
		return -4;

	DiMUSETrack *track = _trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	track->jumpHook = hookId;

	return 0;
}

int DiMUSE_v2::tracksGetHook(int soundId) {
	if (!_trackList)
		return -4;

	DiMUSETrack *track = _trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	return track->jumpHook;
}

void DiMUSE_v2::tracksDeinit() {
	if (!_trackList)
		return;

	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	//debug(5, "tracksDeinit() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);
	DiMUSETrack *track = _trackList;
	do {
		removeTrackFromList(&_trackList, track);

		dispatchRelease(track);
		_fadesHandler->clearFadeStatus(track->soundId, -1);
		_triggersHandler->clearTrigger(track->soundId, (char *)-1, -1);

		track->soundId = 0;
		track = track->next;
	} while (track);

	//waveOutDecreaseSlice();
	//debug(5, "tracksDeinit() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
}

} // End of namespace Scumm
