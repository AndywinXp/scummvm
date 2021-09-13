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

int DiMUSE_v2::tracks_moduleInit() {
	tracks_trackCount = 6;
	tracks_pauseTimer = 0;
	tracks_trackList = NULL;
	tracks_prefSampleRate = 22050;

	if (waveapi_moduleInit(22050, &waveapi_waveOutParams))
		return -1;
	
	if (_internalMixer->init(waveapi_waveOutParams.bytesPerSample,
			waveapi_waveOutParams.numChannels,
			waveapi_waveOutParams.mixBuf,
			waveapi_waveOutParams.mixBufSize,
			waveapi_waveOutParams.sizeSampleKB,
			tracks_trackCount) ||
			dispatch_moduleInit() ||
			streamer_moduleInit())
		return -1;
	
	for (int l = 0; l < tracks_trackCount; l++) {
		tracks[l].prev = NULL;
		tracks[l].next = NULL;
		tracks[l].dispatchPtr = dispatch_getDispatchByTrackId(l);
		tracks[l].dispatchPtr->trackPtr = &tracks[l];
		tracks[l].soundId = 0;
	}

	return 0;
}

void DiMUSE_v2::tracks_pause() {
	tracks_pauseTimer = 1;
}

void DiMUSE_v2::tracks_resume() {
	tracks_pauseTimer = 0;
}

int DiMUSE_v2::tracks_save(uint8 *buffer, int bufferSize) {
	waveapi_increaseSlice();
	int result = dispatch_save(buffer, bufferSize);
	if (result < 0) {
		waveapi_decreaseSlice();
		return result;
	}
	bufferSize -= result;
	if (bufferSize < 480) {
		waveapi_decreaseSlice();
		return -5;
	}
	memcpy(buffer + result, &tracks, 480);
	waveapi_decreaseSlice();
	return result + 480;
}

int DiMUSE_v2::tracks_restore(uint8 *buffer) {
	waveapi_increaseSlice();
	tracks_trackList = NULL;
	int result = dispatch_restore(buffer);
	memcpy(&tracks, buffer + result, 480);
	if (tracks_trackCount > 0) {
		for (int l = 0; l < tracks_trackCount; l++) {
			tracks[l].prev = NULL;
			tracks[l].next = NULL;
			tracks[l].dispatchPtr = dispatch_getDispatchByTrackId(l);
			tracks[l].dispatchPtr->trackPtr = &tracks[l];
			if (tracks[l].soundId) {
				diMUSE_addTrackToList(&tracks_trackList, &tracks[l]);
			}
		}
	}

	dispatch_allocStreamZones();
	waveapi_decreaseSlice();

	return result + 480;
}

void DiMUSE_v2::tracks_setGroupVol() {
	DiMUSETrack* curTrack = (DiMUSETrack *)tracks_trackList;
	while (curTrack) {
		curTrack->effVol = ((curTrack->vol + 1) * _groupsHandler->getGroupVol(curTrack->group)) / 128;
		curTrack = (DiMUSETrack *)curTrack->next;
	};
}

void DiMUSE_v2::tracks_callback() {
	if (tracks_pauseTimer) {
		if (++tracks_pauseTimer < 3)
			return;
		tracks_pauseTimer = 3;
	}
	
	waveapi_increaseSlice();
	//debug(5, "tracks_callback() called increaseSlice()");
	if (_internalMixer->_stream->numQueuedStreams() < 2) {
		dispatch_predictFirstStream();

		waveapi_write(&diMUSE_audioBuffer, &diMUSE_feedSize, &diMUSE_sampleRate);

		if (diMUSE_feedSize != 0) {
			_internalMixer->clearMixerBuffer();
			if (!tracks_pauseTimer) {
				DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;

				while (track) {
					DiMUSETrack *next = (DiMUSETrack *)track->next;
					dispatch_processDispatches(track, diMUSE_feedSize, diMUSE_sampleRate);
					track = next;
				};
			}

			_internalMixer->loop(&diMUSE_audioBuffer, diMUSE_feedSize);

			// The Dig tries to write a second time
			if (_vm->_game.id == GID_DIG) {
				waveapi_write(&diMUSE_audioBuffer, &diMUSE_feedSize, &diMUSE_sampleRate);
			}
		}
	}
	
	waveapi_decreaseSlice();
	//debug(5, "tracks_callback() called decreaseSlice()");
}

int DiMUSE_v2::tracks_startSound(int soundId, int tryPriority, int group) {
	debug(5, "DiMUSE_v2::tracks_startSound(): sound %d with priority %d and group %d", soundId, tryPriority, group);
	int priority = diMUSE_clampNumber(tryPriority, 0, 127);
	if (tracks_trackCount > 0) {
		int l = 0;
		while (tracks[l].soundId) {
			l++;
			if (l >= tracks_trackCount) {
				l = -1;
				break;
			}
		}

		if (l != -1) {
			DiMUSETrack *foundTrack = &tracks[l];
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

			if (dispatch_alloc(foundTrack, group)) {
				debug(5, "DiMUSE_v2::tracks_startSound(): ERROR: dispatch couldn't start sound %d", soundId);
				foundTrack->soundId = 0;
				return -1;
			}
			waveapi_increaseSlice();
			//debug(5, "tracks_startSound() called increaseSlice()");
			diMUSE_addTrackToList(&tracks_trackList, foundTrack);
			waveapi_decreaseSlice();
			//debug(5, "tracks_startSound() called decreaseSlice()");
			return 0;
		}
	}

	debug(5, "DiMUSE_v2::tracks_startSound(): WARNING: no spare tracks for sound %d, attempting to steal a lower priority track", soundId);

	// Let's steal the track with the lowest priority
	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	int bestPriority = 127;
	DiMUSETrack *stolenTrack = NULL;

	while (track) {
		int curTrackPriority = track->priority;
		if (curTrackPriority <= bestPriority) {
			bestPriority = curTrackPriority;
			stolenTrack = track;
		}
		track = (DiMUSETrack *)track->next;
	}

	if (!stolenTrack || priority < bestPriority) {
		debug(5, "DiMUSE_v2::tracks_startSound(): ERROR: couldn't steal a lower priority track", soundId);
		return -6;
	} else {
		diMUSE_removeTrackFromList(&tracks_trackList, stolenTrack);
		dispatch_release(stolenTrack);
		_fadesHandler->clearFadeStatus(stolenTrack->soundId, -1);
		_triggersHandler->clearTrigger(stolenTrack->soundId, (char *)"", -1);
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

	if (dispatch_alloc(stolenTrack, group)) {
		debug(5, "DiMUSE_v2::tracks_startSound(): ERROR: dispatch couldn't start sound %d", soundId);
		stolenTrack->soundId = 0;
		return -1;
	}

	waveapi_increaseSlice();
	//debug(5, "tracks_startSound() called increaseSlice() 2");
	diMUSE_addTrackToList(&tracks_trackList, stolenTrack);
	waveapi_decreaseSlice();
	//debug(5, "tracks_startSound() called decreaseSlice() 2");

	return 0;
}

int DiMUSE_v2::tracks_stopSound(int soundId) {
	if (!tracks_trackList)
		return -1;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	do {
		if (track->soundId == soundId) {
			diMUSE_removeTrackFromList(&tracks_trackList, track);
			dispatch_release(track);
			_fadesHandler->clearFadeStatus(track->soundId, -1);
			_triggersHandler->clearTrigger(track->soundId, (char *)-1, -1);
			track->soundId = 0;
		}
		track = (DiMUSETrack *)track->next;
	} while (track);

	return 0;
}

int DiMUSE_v2::tracks_stopAllSounds() {
	waveapi_increaseSlice();
	//debug(5, "tracks_stopAllSounds() called increaseSlice()");
	if (tracks_trackList) {
		DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
		do {
			diMUSE_removeTrackFromList(&tracks_trackList, track);
			dispatch_release(track);
			_fadesHandler->clearFadeStatus(track->soundId, -1);
			_triggersHandler->clearTrigger(track->soundId, (char *)-1, -1);
			track->soundId = 0;
			track = (DiMUSETrack *)track->next;
		} while (track);
	}

	waveapi_decreaseSlice();
	//debug(5, "tracks_stopAllSounds() called decreaseSlice()");
	return 0;
}

int DiMUSE_v2::tracks_getNextSound(int soundId) {
	int found_soundId = 0;
	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	while (track) {
		if (track->soundId > soundId) {
			if (!found_soundId || track->soundId < found_soundId) {
				found_soundId = track->soundId;
			}
		}
		track = (DiMUSETrack *)track->next;
	};

	return found_soundId;
}

int DiMUSE_v2::tracks_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	if (!tracks_trackList)
		return -1;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_queryStream(track->dispatchPtr->streamPtr, bufSize, criticalSize, freeSpace, paused);
				return 0;
			}
		}
		track = (DiMUSETrack *)track->next;
	} while (track);

	return -1;
}

int DiMUSE_v2::tracks_feedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused) {
	if (!tracks_trackList)
		return -1;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	do {
		if (track->soundId != 0) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_feedStream(track->dispatchPtr->streamPtr, srcBuf, sizeToFeed, paused);
				return 0;
			}
		}
		track = (DiMUSETrack *)track->next;
	} while (track);

	return -1;
}

void DiMUSE_v2::tracks_clear(DiMUSETrack *trackPtr) {
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

	diMUSE_removeTrackFromList(&tracks_trackList, trackPtr);
	dispatch_release(trackPtr);
	_fadesHandler->clearFadeStatus(trackPtr->soundId, -1);
	_triggersHandler->clearTrigger(trackPtr->soundId, (char *)-1, -1);
	trackPtr->soundId = 0;
}

int DiMUSE_v2::tracks_setParam(int soundId, int opcode, int value) {
	if (!tracks_trackList)
		return -4;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
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
						track->transpose = diMUSE_clampTuning(track->detune + value, -12, 12);
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
				debug(5, "DiMUSE_v2::tracks_setParam(): unknown opcode %lu", opcode);
				return -5;
			}
		}
		track = (DiMUSETrack *)track->next;
	}

	return -4;
}

int DiMUSE_v2::tracks_getParam(int soundId, int opcode) {
	if (!tracks_trackList) {
		if (opcode != 0x100)
			return -4;
		else
			return 0;
	}
	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
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

		track = (DiMUSETrack *)track->next;
	} while (track);

	if (opcode != 0x100)
		return -4;
	else
		return 0;
}
int DiMUSE_v2::tracks_lipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height) {
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
	curTrack = tracks_trackList;

	if (msPos >= 0) {
		msPosDiv = msPos >> 4;
		if (((msPos >> 4) & 0xFFFF0000) != 0) {
			return -5;
		} else {
			if (tracks_trackList) {
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
						w = (val >> 8) & 0x7F;           // (val >> 8) & 0x7F;
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

int DiMUSE_v2::tracks_setHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!tracks_trackList)
		return -4;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	while (track->soundId != soundId) {
		track = (DiMUSETrack *)track->next;
		if (!track)
			return -4;
	}

	track->jumpHook = hookId;

	return 0;
}

int DiMUSE_v2::tracks_getHook(int soundId) {
	if (!tracks_trackList)
		return -4;

	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	while (track->soundId != soundId) {
		track = (DiMUSETrack *)track->next;
		if (!track)
			return -4;
	}

	return track->jumpHook;
}

void DiMUSE_v2::tracks_free() {
	if (!tracks_trackList)
		return;

	waveapi_increaseSlice();
	//debug(5, "tracks_free() called increaseSlice()");
	DiMUSETrack *track = (DiMUSETrack *)tracks_trackList;
	do {
		diMUSE_removeTrackFromList(&tracks_trackList, track);

		dispatch_release(track);
		_fadesHandler->clearFadeStatus(track->soundId, -1);
		_triggersHandler->clearTrigger(track->soundId, (char *)-1, -1);

		track->soundId = 0;
		track = (DiMUSETrack *)track->next;
	} while (track);

	waveapi_decreaseSlice();
	//debug(5, "tracks_free() called decreaseSlice()");
}

int DiMUSE_v2::tracks_debug() {
	debug(5, "trackCount: %d\n", tracks_trackCount);
	debug(5, "pauseTimer: %d\n", tracks_pauseTimer);
	debug(5, "trackList: %p\n", *(int *)tracks_trackList);
	debug(5, "prefSampleRate: %d\n", tracks_prefSampleRate);

	for (int i = 0; i < MAX_TRACKS; i++) {
		debug(5, "trackId: %d\n", i);
		debug(5, "\tprev: %p\n", tracks[i].prev);
		debug(5, "\tnext: %p\n", tracks[i].next);
		debug(5, "\tdispatchPtr: %p\n", tracks[i].dispatchPtr);
		debug(5, "\tsound: %d\n", tracks[i].soundId);
		debug(5, "\tmarker: %d\n", tracks[i].marker);
		debug(5, "\tgroup: %d\n", tracks[i].group);
		debug(5, "\tpriority: %d\n", tracks[i].priority);
		debug(5, "\tvol: %d\n", tracks[i].vol);
		debug(5, "\teffVol: %d\n", tracks[i].effVol);
		debug(5, "\tpan: %d\n", tracks[i].pan);
		debug(5, "\tdetune: %d\n", tracks[i].detune);
		debug(5, "\ttranspose: %d\n", tracks[i].transpose);
		debug(5, "\tpitchShift: %d\n", tracks[i].pitchShift);
		debug(5, "\tmailbox: %d\n", tracks[i].mailbox);
		debug(5, "\tjumpHook: %d\n", tracks[i].jumpHook);
	}

	return 0;
}

} // End of namespace Scumm
