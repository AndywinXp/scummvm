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
	tracks_waveCall = -1; // This is just a function which returns -1
	tracks_pauseTimer = 0;
	tracks_trackList = NULL;
	tracks_prefSampleRate = 22050;

	if (tracks_waveCall) {
		/*
		if (waveapi_moduleInit(initDataPtr->unused, 22050, &waveapi_waveOutParams))
			return -1;*/
	} else {
		/*
		warning("TR: No WAVE driver loaded...");
		waveapi_waveOutParams.mixBuf = 0;
		waveapi_waveOutParams.offsetBeginMixBuf = 0;
		waveapi_waveOutParams.sizeSampleKB = 0;
		waveapi_waveOutParams.bytesPerSample = 8;
		waveapi_waveOutParams.numChannels = 1;*/
	}

	/*
	if (mixer_moduleInit(&waveapi_waveOutParams) || dispatch_moduleInit() || streamer_moduleInit())
		return -1;
	*/
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
	//waveapi_increaseSlice();
	int result = dispatch_save(buffer, bufferSize);
	if (result < 0) {
		//waveapi_decreaseSlice();
		return result;
	}
	bufferSize -= result;
	if (bufferSize < 480) {
		//waveapi_decreaseSlice();
		return -5;
	}
	memcpy(buffer + result, &tracks, 480);
	//waveapi_decreaseSlice();
	return result + 480;
}

int DiMUSE_v2::tracks_restore(uint8 *buffer) {
	//waveapi_increaseSlice();
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
				//iMUSE_addItemToList(&tracks_trackList, &tracks[l]);
			}
		}
	}

	dispatch_allocStreamZones();
	//waveapi_decreaseSlice();

	return result + 480;
}

void DiMUSE_v2::tracks_setGroupVol() {
	iMUSETrack* curTrack = tracks_trackList;
	while (curTrack) {
		curTrack->effVol = ((curTrack->vol + 1) * groups_getGroupVol(curTrack->group)) / 128;
		curTrack = (iMUSETrack *)curTrack->next;
	};
}

void DiMUSE_v2::tracks_callback() {
	if (tracks_pauseTimer) {
		if (++tracks_pauseTimer < 3)
			return;
		tracks_pauseTimer = 3;
	}

	//waveapi_increaseSlice();
	dispatch_predictFirstStream();
	if (tracks_waveCall) {
		//waveapi_write(&iMUSE_audioBuffer, &iMUSE_feedSize, iMUSE_sampleRate);
	} else {
		// 40 Hz frequency for filling the audio buffer, for some reason
		// Anyway it appears we never reach this block since tracks_waveCall is assigned to a (dummy) function
		tracks_running40HzCount += timer_getUsecPerInt(); // Rename to timer_running40HzCount or similar
		if (tracks_running40HzCount >= 25000) {
			tracks_running40HzCount -= 25000;
			iMUSE_feedSize = tracks_prefSampleRate / 40;
			iMUSE_sampleRate = tracks_prefSampleRate;
		}
		else {
			iMUSE_feedSize = 0;
		}
	}

	if (iMUSE_feedSize != 0) {
		//mixer_clearMixBuff();
		if (!tracks_pauseTimer) {
			iMUSETrack *track = tracks_trackList;

			while (track) {
				iMUSETrack *next = (iMUSETrack *)track->next;
				dispatch_processDispatches(track, iMUSE_feedSize, iMUSE_sampleRate);
				track = next;
			};
		}

		if (tracks_waveCall) {
			//mixer_loop(iMUSE_audioBuffer, iMUSE_feedSize);
			if (tracks_waveCall) {
				//waveapi_write(&iMUSE_audioBuffer, &iMUSE_feedSize, 0);
			}
		}
	}

	//waveapi_decreaseSlice();
}

// Validated
int DiMUSE_v2::tracks_startSound(int soundId, int tryPriority, int group) {
	int priority = tryPriority;// iMUSE_clampNumber(tryPriority, 0, 127);
	if (tracks_trackCount > 0) {
		int l = 0;
		while (tracks[l].soundId == 0) {
			l++;
			if (tracks_trackCount <= l)
				break;
		}

		iMUSETrack *foundTrack = &tracks[l];
		if (!foundTrack)
			return -6;

		foundTrack->soundId = soundId;
		foundTrack->marker = 0;
		foundTrack->group = 0;
		foundTrack->priority = priority;
		foundTrack->vol = 127;
		foundTrack->effVol = groups_getGroupVol(0);
		foundTrack->pan = 64;
		foundTrack->detune = 0;
		foundTrack->transpose = 0;
		foundTrack->pitchShift = 0;
		foundTrack->mailbox = 0;
		foundTrack->jumpHook = 0;

		if (dispatch_alloc(foundTrack, group)) {
			debug(5, "ERR: dispatch couldn't start sound...\n");
			foundTrack->soundId = 0;
			return -1;
		}
		//waveapi_increaseSlice();
		//iMUSE_addItemToList(&tracks_trackList, foundTrack);
		//waveapi_decreaseSlice();
		return 0;
	}

	debug(5, "ERR: no spare tracks...\n");

	// Let's steal the track with the lowest priority
	iMUSETrack *track = tracks_trackList;
	int bestPriority = 127;
	iMUSETrack *stolenTrack = NULL;

	while (track) {
		int curTrackPriority = track->priority;
		if (curTrackPriority <= bestPriority) {
			bestPriority = curTrackPriority;
			stolenTrack = track;
		}
		track = (iMUSETrack *)track->next;
	}

	if (!stolenTrack || priority < bestPriority) {
		return -6;
	}
	else {
		//iMUSE_removeItemFromList(&tracks_trackList, stolenTrack);
		dispatch_release(stolenTrack);
		fades_clearFadeStatus(stolenTrack->soundId, -1);
		triggers_clearTrigger(stolenTrack->soundId, (char *)-1, -1);
		stolenTrack->soundId = 0;
	}

	stolenTrack->soundId = soundId;
	stolenTrack->marker = 0;
	stolenTrack->group = 0;
	stolenTrack->priority = priority;
	stolenTrack->vol = 127;
	stolenTrack->effVol = groups_getGroupVol(0);
	stolenTrack->pan = 64;
	stolenTrack->detune = 0;
	stolenTrack->transpose = 0;
	stolenTrack->pitchShift = 0;
	stolenTrack->mailbox = 0;
	stolenTrack->jumpHook = 0;

	if (dispatch_alloc(stolenTrack, group)) {
		debug(5, "ERR: dispatch couldn't start sound...\n");
		stolenTrack->soundId = 0;
		return -1;
	}

	//waveapi_increaseSlice();
	//iMUSE_addItemToList(&tracks_trackList, stolenTrack);
	//waveapi_decreaseSlice();

	return 0;
}

int DiMUSE_v2::tracks_stopSound(int soundId) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId == soundId) {
			//iMUSE_removeItemFromList(&tracks_trackList, track);
			dispatch_release(track);
			fades_clearFadeStatus(track->soundId, -1);
			triggers_clearTrigger(track->soundId, (char *)-1, -1);
			track->soundId = 0;
		}
		track = (iMUSETrack *)track->next;
	} while (track);

	return 0;
}

int DiMUSE_v2::tracks_stopAllSounds() {
	//waveapi_increaseSlice();

	if (tracks_trackList) {
		iMUSETrack *track = tracks_trackList;
		do {
			//iMUSE_removeItemFromList(&tracks_trackList, track);
			dispatch_release(track);
			fades_clearFadeStatus(track->soundId, -1);
			triggers_clearTrigger(track->soundId, (char *)-1, -1);
			track->soundId = 0;
			track = (iMUSETrack *)track->next;
		} while (track);
	}

	//waveapi_decreaseSlice();

	return 0;
}

int DiMUSE_v2::tracks_getNextSound(int soundId) {
	int found_soundId = 0;
	iMUSETrack *track = tracks_trackList;
	while (track) {
		if (track->soundId > soundId) {
			if (!found_soundId || track->soundId < found_soundId) {
				found_soundId = track->soundId;
			}
		}
		track = (iMUSETrack *)track->next;
	};

	return found_soundId;
}

int DiMUSE_v2::tracks_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_queryStream(track->dispatchPtr->streamPtr, bufSize, criticalSize, freeSpace, paused);
				return 0;
			}
		}
		track = (iMUSETrack *)track->next;
	} while (track);

	return -1;
}

int DiMUSE_v2::tracks_feedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId != 0) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_feedStream(track->dispatchPtr->streamPtr, srcBuf, sizeToFeed, paused);
				return 0;
			}
		}
		track = (iMUSETrack *)track->next;
	} while (track);

	return -1;
}

void DiMUSE_v2::tracks_clear(iMUSETrack *trackPtr) {
	//iMUSE_removeItemFromList(&tracks_trackList, trackPtr);
	dispatch_release(trackPtr);
	fades_clearFadeStatus(trackPtr->soundId, -1);
	triggers_clearTrigger(trackPtr->soundId, (char *)-1, -1);
	trackPtr->soundId = 0;
}

int DiMUSE_v2::tracks_setParam(int soundId, int opcode, int value) {
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track) {
		if (track->soundId == soundId) {
			switch (opcode) {
			case 0x400:
				if (value >= 16)
					return -5;
				track->group = value;
				track->effVol = ((track->vol + 1) * groups_getGroupVol(value)) / 128;
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
				track->effVol = ((value + 1) * groups_getGroupVol(track->group)) / 128;
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
				// The DIG
				if (_vm->_game.id == GID_DIG) {
					if (value < -12 || value > 12)
						return -5;

					if (value == 0) {
						track->transpose = 0;
					} else {
						track->transpose = value;//iMUSE_clampTuning(track->detune + value, -12, 12);
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
				debug(5, "ERR: setParam: unknown opcode %lu\n", opcode);
				return -5;
			}

			track = (iMUSETrack *)track->next;
		}
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
	iMUSETrack *track = tracks_trackList;
	int l = 0;
	do {
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

		track = (iMUSETrack *)track->next;
		l++;
	} while (track);

	if (opcode != 0x100)
		return -4;
	else
		return 0;
}
/*
int tracks_lipSync(int soundId, int syncId, int msPos, int *width, char *height) {
	int w = 0;
	int h = 0;
	iMUSETrack *track = *(iMUSETrack**)tracks_trackList;
	int result = 0;
	if (msPos < 0) {
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		return result;
	}
	msPos /= 16;
	if (msPos >= 65536) {
		result = -5;
	}
	else {
		if (tracks_trackList) {
			do {
				if (track->soundId == soundId)
					break;
				track = track->next;
			} while (track);
		}
		if (track) {
			switch (syncId) {
			case 0:
				sync_ptr = track->sync_ptr_0;
				sync_size = track->sync_size_0;
				goto break;
			case 1:
				sync_ptr = track->sync_ptr_1
					sync_size = track->sync_size_1;
				goto break;
			case 2:
				sync_ptr = track->sync_ptr_2;
				sync_size = track->sync_size_2;
				break;
			case 3:
				sync_ptr = track->sync_ptr_3;
				sync_size = track->sync_size_3;
				break;
			default:
				break;
			}
			if (sync_size && sync_ptr) {
				sync_size /= 4;
				v12 = sync_ptr + 2;
				v15 = sync_size;
				v13 = sync_size - 1;
				while (sync_size--) {
					if (sync_ptr[2] >= msPos)
						break;
					sync_ptr += 4;
				}
				if (sync_size < 0 || sync_ptr[2] > msPos)
					sync_size -= 4;
				val = *(sync_ptr - 2);
				w = (val >> 8) & 0x7F;
				h = val & 0x7F;
			}
		}
		else {
			result = -4;
		}
	}
	if (width)
		*width = w;
	if (height)
		*height = h;
	return result;
}*/

int DiMUSE_v2::tracks_setHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track->soundId != soundId) {
		track = (iMUSETrack *)track->next;
		if (!track)
			return -4;
	}

	track->jumpHook = hookId;

	return 0;
}

int DiMUSE_v2::tracks_getHook(int soundId) {
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track->soundId != soundId) {
		track = (iMUSETrack *)track->next;
		if (!track)
			return -4;
	}

	return track->jumpHook;
}

void DiMUSE_v2::tracks_free() {
	if (!tracks_trackList)
		return;

	//waveapi_increaseSlice();

	iMUSETrack *track = tracks_trackList;
	do {
		//iMUSE_removeItemFromList(&tracks_trackList, &track);

		dispatch_release(track);
		fades_clearFadeStatus(track->soundId, -1);
		triggers_clearTrigger(track->soundId, (char *)-1, -1);

		track->soundId = 0;
		track = (iMUSETrack *)track->next;
	} while (track);

	//waveapi_decreaseSlice();
}

int DiMUSE_v2::tracks_debug() {
	debug(5, "trackCount: %d\n", tracks_trackCount);
	debug(5, "waveCall: %p\n", tracks_waveCall);
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
