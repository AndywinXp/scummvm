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

// Almost validated, check if there has to be a separate malloc for dispatch_smallFadeBufs; edit dispatch_free accordingly
int DiMUSE_v2::dispatch_moduleInit() {
	dispatch_buf = (int *)malloc(SMALL_FADES * SMALL_FADE_DIM + LARGE_FADE_DIM * LARGE_FADES);

	if (dispatch_buf) {
		dispatch_largeFadeBufs = dispatch_buf;
		dispatch_smallFadeBufs = dispatch_buf + (LARGE_FADE_DIM * LARGE_FADES);

		for (int i = 0; i < LARGE_FADES; i++) {
			dispatch_largeFadeFlags[i] = 0;
		}

		for (int i = 0; i < SMALL_FADES; i++) {
			dispatch_smallFadeFlags[i] = 0;
		}

		for (int i = 0; i < MAX_STREAMZONES; i++) {
			streamZones[i].useFlag = 0;
		}
		/*
		for (int i = 0; i < MAX_DISPATCHES; i++) {
			dispatches[i].wordSize = 0;
			dispatches[i].sampleRate = 0;
			dispatches[i].channelCount = 0;
			dispatches[i].currentOffset = 0;
			dispatches[i].audioRemaining = 0;

			memset(dispatches[i].map, 0, sizeof(dispatches[i].map));

			dispatches[i].streamPtr = 0;
			dispatches[i].streamBufID = 0;
			dispatches[i].streamZoneList = 0;
			dispatches[i].streamErrFlag = 0;
			dispatches[i].fadeBuf = 0;
			dispatches[i].fadeOffset = 0;
			dispatches[i].fadeRemaining = 0;
			dispatches[i].fadeWordSize = 0;
			dispatches[i].fadeSampleRate = 0;
			dispatches[i].fadeChannelCount = 0;
			dispatches[i].fadeSyncFlag = 0;
			dispatches[i].fadeSyncDelta = 0;
			dispatches[i].fadeVol = 0;
			dispatches[i].fadeSlope = 0;
		}*/	
	} else {
		debug(5, "ERR:in dispatch allocating buffers...\n");
		return -1;
	}

	return 0;
}

DiMUSE_v2::iMUSEDispatch *DiMUSE_v2::dispatch_getDispatchByTrackId(int trackId) {
	return &dispatches[trackId];
}

// Validated
int DiMUSE_v2::dispatch_save(uint8 *dst, int size) {
	if (size < 8832)
		return -5;
	memcpy((void *)dst, dispatches, 8832);
	return 8832;
}

// Validated
int DiMUSE_v2::dispatch_restore(uint8 *src) {
	memcpy(dispatches, src, 8832);

	for (int i = 0; i < LARGE_FADES; i++) {
		dispatch_largeFadeFlags[i] = 0;
	}

	for (int i = 0; i < SMALL_FADES; i++) {
		dispatch_smallFadeFlags[i] = 0;
	}

	for (int i = 0; i < MAX_STREAMZONES; i++) {
		streamZones[i].useFlag = 0;
	}

	return 8832;
}

// Almost validated, check if the list related code works
int DiMUSE_v2::dispatch_allocStreamZones() {
	iMUSEDispatch *curDispatchPtr;
	iMUSEStream *curAllocatedStream;
	iMUSEStreamZone *curStreamZone;
	iMUSEStreamZone *curStreamZoneList;

	curDispatchPtr = dispatches;
	for (int i = 0; i < MAX_TRACKS; i++) {
		curDispatchPtr = &dispatches[i];
		curDispatchPtr->fadeBuf = 0;

		if (curDispatchPtr->trackPtr->soundId && curDispatchPtr->streamPtr) {
			// Try allocating the stream
			curAllocatedStream = streamer_alloc(curDispatchPtr->trackPtr->soundId, curDispatchPtr->streamBufID, 0x4000); // 0x2000
			curDispatchPtr->streamPtr = curAllocatedStream;

			if (curAllocatedStream) {
				streamer_setSoundToStreamWithCurrentOffset(curAllocatedStream, curDispatchPtr->trackPtr->soundId, curDispatchPtr->currentOffset);
				if (curDispatchPtr->audioRemaining) {

					// Try finding a stream zone which is not in use
					curStreamZone = NULL;
					for (int j = 0; i < MAX_STREAMZONES; i++) {
						if (streamZones[j].useFlag == 0) {
							curStreamZone = &streamZones[j];
						}
					}

					if (!curStreamZone) {
						debug(5, "DiMUSE_v2::dispatch_allocStreamZones(): out of streamZones");
						curStreamZoneList = (iMUSEStreamZone *)&curDispatchPtr->streamZoneList;
						curDispatchPtr->streamZoneList = 0;
					} else {
						curStreamZoneList = (iMUSEStreamZone *)&curDispatchPtr->streamZoneList;
						curStreamZone->prev = 0;
						curStreamZone->next = 0;
						curStreamZone->useFlag = 1;
						curStreamZone->offset = 0;
						curStreamZone->size = 0;
						curStreamZone->fadeFlag = 0;
						curDispatchPtr->streamZoneList = curStreamZone;
					}

					if (curStreamZoneList->prev) {
						curStreamZoneList->prev->offset = curDispatchPtr->currentOffset;
						curStreamZoneList->prev->size = 0;
						curStreamZoneList->prev->fadeFlag = 0;
					} else {
						debug(5, "DiMUSE_v2::dispatch_allocStreamZones(): unable to alloc zone during restore");
					}
				}
			} else {
				debug(5, "DiMUSE_v2::dispatch_allocStreamZones(): unable to start stream during restore");
			}
		}
	}
	return 0;
}

// Almost validated, check if the list removal code works
int DiMUSE_v2::dispatch_alloc(iMUSETrack *trackPtr, int groupId) {
	iMUSEDispatch *trackDispatch;
	iMUSEDispatch *dispatchToDeallocate;
	iMUSEStreamZone *streamZoneList;
	int getMapResult;
	int *fadeBuf;

	trackDispatch = trackPtr->dispatchPtr;
	trackDispatch->currentOffset = 0;
	trackDispatch->audioRemaining = 0;
	trackDispatch->map[0] = NULL;
	trackDispatch->fadeBuf = 0;
	if (groupId) {
		trackDispatch->streamPtr = streamer_alloc(trackPtr->soundId, groupId, 0x4000u); // 0x2000
		if (!trackDispatch->streamPtr) {
			debug(5, "DiMUSE_v2::dispatch_alloc(): unable to alloc stream");
			return -1;
		}
		trackDispatch->streamBufID = groupId;
		trackDispatch->streamZoneList = 0;
		trackDispatch->streamErrFlag = 0;
	} else {
		trackDispatch->streamPtr = 0;
	}

	getMapResult = dispatch_getNextMapEvent(trackDispatch);
	if (!getMapResult || getMapResult == -3)
		return 0;

	debug(5, "DiMUSE_v2::dispatch_alloc(): problem starting sound in dispatch");

	// Remove streamZones from list
	dispatchToDeallocate = trackDispatch->trackPtr->dispatchPtr;
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = dispatchToDeallocate->streamZoneList;
		streamer_clearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				streamZoneList->useFlag = 0;
				iMUSE_removeStreamZoneFromList(&dispatchToDeallocate->streamZoneList, streamZoneList);
			} while (streamZoneList);
		}
	}

	fadeBuf = dispatchToDeallocate->fadeBuf;
	if (!fadeBuf)
		return -1;

	// Mark the fade corresponding to our fadeBuf as unused
	// Check between the large fades 
	int ptrCtr = 0;
	int i, j;
	for (i = 0, ptrCtr = 0;
		i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
		i++, ptrCtr += LARGE_FADE_DIM) {

		if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == fadeBuf) { // Found it!
			if (dispatch_largeFadeFlags[i] == 0) {
				debug(5, "DiMUSE_v2::dispatch_alloc(): redundant large fade buf de-allocation");
			}
			dispatch_largeFadeFlags[i] = 0;
			return -1;
		}
	}

	// Check between the small fades 
	ptrCtr = 0;
	for (j = 0, ptrCtr = 0;
		j < SMALL_FADES, ptrCtr < SMALL_FADE_DIM * SMALL_FADES;
		j++, ptrCtr += SMALL_FADE_DIM) {

		if (dispatch_largeFadeBufs + (SMALL_FADE_DIM * j) == fadeBuf) { // Found it!
			if (dispatch_smallFadeFlags[j] == 0) {
				debug(5, "DiMUSE_v2::dispatch_alloc(): redundant small fade buf de-allocation");
			}
			dispatch_smallFadeFlags[j] = 0;
			return -1;
		}
	}

	debug(5, "DiMUSE_v2::dispatch_alloc(): couldn't find fade buf to de-allocate");
	return -1;
}

// Almost validated, check if the list removal code works
int DiMUSE_v2::dispatch_release(iMUSETrack *trackPtr) {
	iMUSEDispatch *dispatchToDeallocate;
	iMUSEStreamZone *streamZoneList;
	int *fadeBuf;

	dispatchToDeallocate = trackPtr->dispatchPtr;

	// Remove streamZones from list
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = dispatchToDeallocate->streamZoneList;
		streamer_clearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				streamZoneList->useFlag = 0;
				iMUSE_removeStreamZoneFromList(&dispatchToDeallocate->streamZoneList, streamZoneList); // TODO: Is it right?
			} while (dispatchToDeallocate->streamZoneList);
		}
	}

	fadeBuf = dispatchToDeallocate->fadeBuf;
	if (!fadeBuf)
		return 0;

	// Mark the fade corresponding to our fadeBuf as unused
	// Check between the large fades 
	int ptrCtr = 0;
	int i, j;
	for (i = 0, ptrCtr = 0;
		i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
		i++, ptrCtr += LARGE_FADE_DIM) {

		if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == fadeBuf) { // Found it!
			if (dispatch_largeFadeFlags[i] == 0) {
				debug(5, "DiMUSE_v2::dispatch_release(): redundant large fade buf de-allocation");
			}
			dispatch_largeFadeFlags[i] = 0;
			return 0;
		}
	}

	// Check between the small fades 
	ptrCtr = 0;
	for (j = 0, ptrCtr = 0;
		j < SMALL_FADES, ptrCtr < SMALL_FADE_DIM*SMALL_FADES;
		j++, ptrCtr += SMALL_FADE_DIM) {

		if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == fadeBuf) { // Found it!
			if (dispatch_smallFadeFlags[j] == 0) {
				debug(5, "DiMUSE_v2::dispatch_release(): redundant small fade buf de-allocation");
			}
			dispatch_smallFadeFlags[j] = 0;
			return 0;
		}
	}

	debug(5, "DiMUSE_v2::dispatch_release(): couldn't find fade buf to de-allocate");
	return 0;
}

int DiMUSE_v2::dispatch_switchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag) {
	int effFadeLen;
	/*unsigned*/int strZnSize;
	int alignmentModDividend;
	iMUSEDispatch *curDispatch = dispatches;
	int ptrCtr, i, j;
	int *fadeBuffer;
	/*unsigned*/int effFadeSize;
	int getMapResult;

	effFadeLen = fadeLength;

	if (fadeLength > 2000)
		effFadeLen = 2000;

	if (tracks_trackCount <= 0) {
		debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't find sound");
		return -1;
	}

	for (i = 0; i <= tracks_trackCount; i++) {
		if (i >= tracks_trackCount) {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't find sound");
			return -1;
		}

		if (oldSoundId && curDispatch->trackPtr->soundId == oldSoundId && curDispatch->streamPtr) {
			curDispatch = &dispatches[i];
			break;
		}
	}

	if (curDispatch->streamZoneList) {
		if (!curDispatch->wordSize) {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): found streamZoneList but null wordSize");
			return -1;
		}

		fadeBuffer = curDispatch->fadeBuf;

		if (fadeBuffer) {
			ptrCtr = 0;

			// Mark the fade corresponding to our fadeBuf as unused
			ptrCtr = 0;
			for (i = 0, ptrCtr = 0;
				i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
				i++, ptrCtr += LARGE_FADE_DIM) {

				if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == fadeBuffer) {
					if (dispatch_largeFadeFlags[i] == 0) {
						debug(5, "DiMUSE_v2::dispatch_switchStream(): redundant large fade buf de-allocation");
					}
					dispatch_largeFadeFlags[i] = 0;
					break;
				}
			}

			if (ptrCtr + dispatch_largeFadeBufs != fadeBuffer) {
				for (j = 0, ptrCtr = 0;
					j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
					j++, ptrCtr += SMALL_FADE_DIM) {

					if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
						debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't find fade buf to de-allocate");
						break;
					}

					if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == fadeBuffer) {
						if (dispatch_smallFadeFlags[j] == 0) {
							debug(5, "DiMUSE_v2::dispatch_switchStream(): redundant small fade buf de-allocation");
						}
						dispatch_smallFadeFlags[j] = 0;
						break;
					}
				}
			}
		}

		dispatch_fadeSize = (curDispatch->channelCount * curDispatch->wordSize * ((effFadeLen * curDispatch->sampleRate / 1000) & 0xFFFFFFFE)) / 8;

		strZnSize = curDispatch->streamZoneList->size;
		if (strZnSize >= dispatch_fadeSize)
			strZnSize = dispatch_fadeSize;
		dispatch_fadeSize = strZnSize;

		// Validate and adjust the fade dispatch size further;
		// this should correctly align the dispatch size to avoid starting a fade without
		// inverting the stereo image by mistake

		if (_vm->_game.id == GID_DIG) {
			alignmentModDividend = curDispatch->channelCount * (curDispatch->wordSize == 8 ? 1 : 3);
		} else {
			if (curDispatch->wordSize == 8) {
				alignmentModDividend = curDispatch->channelCount * 1;
			} else {
				alignmentModDividend = curDispatch->channelCount * ((curDispatch->wordSize == 12) + 2);
			}
		}

		if (alignmentModDividend) {
			dispatch_fadeSize -= dispatch_fadeSize % alignmentModDividend;
		} else {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): ValidateFadeSize() tried mod by 0");
		}

		if (dispatch_fadeSize > LARGE_FADE_DIM) {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): WARNING: requested fade too large (%lu)", dispatch_fadeSize);
			dispatch_fadeSize = LARGE_FADE_DIM;
		}

		if (dispatch_fadeSize <= SMALL_FADE_DIM) { // Small fade
			for (i = 0; i <= SMALL_FADES; i++) {
				if (i == SMALL_FADES) {
					debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't allocate small fade buf");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!dispatch_smallFadeFlags[i]) {
					dispatch_smallFadeFlags[i] = 1;
					curDispatch->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
					break;
				}
			}
		} else { // Large fade
			for (i = 0; i <= LARGE_FADES; i++) {
				if (i == LARGE_FADES) {
					debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't allocate large fade buf");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!dispatch_largeFadeFlags[i]) {
					dispatch_largeFadeFlags[i] = 1;
					curDispatch->fadeBuf = dispatch_largeFadeFlags + LARGE_FADE_DIM * i;
					break;
				}
			}

			// Fallback to a small fade if large fades are unavailable
			if (!curDispatch->fadeBuf) {
				for (i = 0; i <= SMALL_FADES; i++) {
					if (i == SMALL_FADES) {
						debug(5, "DiMUSE_v2::dispatch_switchStream(): couldn't allocate small fade buf");
						curDispatch->fadeBuf = NULL;
						break;
					}

					if (!dispatch_smallFadeFlags[i]) {
						dispatch_smallFadeFlags[i] = 1;
						curDispatch->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
						break;
					}
				}
			}
		}

		// If we were able to allocate a fade, set up a fade out for the old sound.
		// We'll copy data from the stream buffer to the fade buffer
		if (curDispatch->fadeBuf) {
			curDispatch->fadeOffset = 0;
			curDispatch->fadeRemaining = 0;
			curDispatch->fadeWordSize = curDispatch->wordSize;
			curDispatch->fadeSampleRate = curDispatch->sampleRate;
			curDispatch->fadeChannelCount = curDispatch->channelCount;
			curDispatch->fadeSyncFlag = unusedFadeSyncFlag | offsetFadeSyncFlag;
			curDispatch->fadeSyncDelta = 0;
			curDispatch->fadeVol = MAX_FADE_VOLUME;
			curDispatch->fadeSlope = 0;

			while (curDispatch->fadeRemaining < dispatch_fadeSize) {
				effFadeSize = dispatch_fadeSize - curDispatch->fadeRemaining;
				if ((dispatch_fadeSize - curDispatch->fadeRemaining) >= 0x4000) // Originally 0x2000 for DIG, but it shouldn't be a problem
					effFadeSize = 0x4000;

				//uint8 *ptr = (uint8 *)streamer_reAllocReadBuffer(curDispatch->streamPtr, effFadeSize);
				//memcpy(((uint8 *)curDispatch->fadeBuf + curDispatch->fadeRemaining), ptr, effFadeSize);

				curDispatch->fadeRemaining += effFadeSize;
			}
		} else {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): WARNING: couldn't alloc fade buf");
		}
	}

	// Clear fades and triggers for the old newSoundId
	fades_clearFadeStatus(curDispatch->trackPtr->soundId, -1);
	triggers_clearTrigger(curDispatch->trackPtr->soundId, (char *)"", -1);

	// Setup the new newSoundId
	curDispatch->trackPtr->soundId = newSoundId;

	streamer_setIndex1(curDispatch->streamPtr, streamer_getFreeBuffer(curDispatch->streamPtr));

	if (offsetFadeSyncFlag && curDispatch->streamZoneList) {
		// Start the newSoundId from an offset
		streamer_setSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, curDispatch->currentOffset);
		while (curDispatch->streamZoneList->next) {
			curDispatch->streamZoneList->next->useFlag = 0;
			iMUSE_removeStreamZoneFromList(&curDispatch->streamZoneList->next, curDispatch->streamZoneList->next);
		}
		curDispatch->streamZoneList->size = 0;

		return 0;
	} else {
		// Start the newSoundId from the beginning
		streamer_setSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, 0);

		while (curDispatch->streamZoneList) {
			curDispatch->streamZoneList->useFlag = 0;
			iMUSE_removeStreamZoneFromList(&curDispatch->streamZoneList, curDispatch->streamZoneList);
		}

		curDispatch->currentOffset = 0;
		curDispatch->audioRemaining = 0;
		curDispatch->map[0] = NULL;

		// Sanity check: make sure we correctly switched 
		// the stream and getNextMapEvent gives an error
		getMapResult = dispatch_getNextMapEvent(curDispatch);
		if (!getMapResult || getMapResult == -3) {
			return 0;
		} else {
			debug(5, "DiMUSE_v2::dispatch_switchStream(): problem switching stream in dispatch");
			tracks_clear(curDispatch->trackPtr);
			return -1;
		}
	}
}

// Validated
void DiMUSE_v2::dispatch_processDispatches(iMUSETrack *trackPtr, int feedSize, int sampleRate) {
	iMUSEDispatch *dispatchPtr;
	int inFrameCount;
	int effFeedSize;
	int effWordSize;
	int effRemainingAudio;
	int effRemainingFade;
	int getNextMapEventResult;
	int mixVolume;
	int ptrCtr, i, j;
	int elapsedFadeDelta;
	uint8 *srcBuf;
	uint8 *soundAddrData;
	int mixStartingPoint;

	dispatchPtr = trackPtr->dispatchPtr;
	if (dispatchPtr->streamPtr && dispatchPtr->streamZoneList)
		dispatch_predictStream(dispatchPtr);

	// If there's a fade
	if (dispatchPtr->fadeBuf) {
		inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);

		if (inFrameCount >= dispatchPtr->fadeSampleRate * feedSize / sampleRate) {
			inFrameCount = dispatchPtr->fadeSampleRate * feedSize / sampleRate;
			effFeedSize = feedSize;
		} else {
			effFeedSize = sampleRate * inFrameCount / dispatchPtr->fadeSampleRate;
		}

		if (dispatchPtr->fadeWordSize == 12 && dispatchPtr->fadeChannelCount == 1)
			inFrameCount &= 0xFFFFFFFE;

		// If the fade is still going on
		if (inFrameCount) {
			mixVolume = (((dispatchPtr->fadeVol / 65536) + 1) * dispatchPtr->trackPtr->effVol) / 128;

			// Update fadeVol
			effRemainingFade = ((dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) * inFrameCount) / 8;
			dispatchPtr->fadeVol += effRemainingFade * dispatchPtr->fadeSlope;

			// Clamp fadeVol between 0 and MAX_FADE_VOLUME
			if (dispatchPtr->fadeVol + effRemainingFade * dispatchPtr->fadeSlope < 0)
				dispatchPtr->fadeVol = 0;
			if (dispatchPtr->fadeVol > MAX_FADE_VOLUME)
				dispatchPtr->fadeVol = MAX_FADE_VOLUME;

			// Send it all to the mixer
			srcBuf = (uint8 *)(dispatchPtr->fadeBuf + dispatchPtr->fadeOffset);
			
			_diMUSEMixer->mixer_mix(
				srcBuf,
				inFrameCount,
				dispatchPtr->fadeWordSize,
				dispatchPtr->fadeChannelCount,
				effFeedSize,
				0,
				mixVolume,
				trackPtr->pan);

			dispatchPtr->fadeOffset += effRemainingFade;
			dispatchPtr->fadeRemaining -= effRemainingFade;

			// Deallocate fade if it ended
			if (!dispatchPtr->fadeRemaining) {

				ptrCtr = 0;
				for (i = 0, ptrCtr = 0;
					i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
					i++, ptrCtr += LARGE_FADE_DIM) {

					if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
						if (dispatch_largeFadeFlags[i] == 0) {
							debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant large fade buf de-allocation");
						}
						dispatch_largeFadeFlags[i] = 0;
						break;
					}
				}

				if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
					for (j = 0, ptrCtr = 0;
						j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
						j++, ptrCtr += SMALL_FADE_DIM) {

						if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
							debug(5, "DiMUSE_v2::dispatch_processDispatches(): couldn't find fade buf to de-allocate");
							break;
						}

						if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
							if (dispatch_smallFadeFlags[j] == 0) {
								debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant small fade buf de-allocation");
							}
							dispatch_smallFadeFlags[j] = 0;
							break;
						}
					}
				}
			}
		} else {
			debug(5, "DiMUSE_v2::dispatch_processDispatches(): WARNING: fade ends with incomplete frame (or odd 12-bit mono frame)");

			// Fade ended, deallocate it
			ptrCtr = 0;
			for (i = 0, ptrCtr = 0;
				i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
				i++, ptrCtr += LARGE_FADE_DIM) {

				if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
					if (dispatch_largeFadeFlags[i] == 0) {
						debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant large fade buf de-allocation");
					}
					dispatch_largeFadeFlags[i] = 0;
					break;
				}
			}

			if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
				for (j = 0, ptrCtr = 0;
					j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
					j++, ptrCtr += SMALL_FADE_DIM) {

					if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
						debug(5, "DiMUSE_v2::dispatch_processDispatches(): couldn't find fade buf to de-allocate");
						break;
					}

					if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
						if (dispatch_smallFadeFlags[j] == 0) {
							debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant small fade buf de-allocation");
						}
						dispatch_smallFadeFlags[j] = 0;
						break;
					}
				}
			}
		}

		if (!dispatchPtr->fadeRemaining)
			dispatchPtr->fadeBuf = NULL;
	}

	// This index keeps track of the offset position until which we have
	// filled the buffer; with each update it is incremented by the effective
	// feed size.
	mixStartingPoint = 0;

	while (1) {
		// If the current region is finished playing
		// go check for any event on the map for the current offset
		if (!dispatchPtr->audioRemaining) {
			dispatch_fadeStartedFlag = 0;
			getNextMapEventResult = dispatch_getNextMapEvent(dispatchPtr);

			if (getNextMapEventResult)
				break;

			// We reached a JUMP, therefore we have to crossfade to
			// the destination region: start a fade-out
			if (dispatch_fadeStartedFlag) {
				inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);

				if (inFrameCount >= dispatchPtr->fadeSampleRate * feedSize / sampleRate) {
					inFrameCount = dispatchPtr->fadeSampleRate * feedSize / sampleRate;
					effFeedSize = feedSize;
				} else {
					effFeedSize = sampleRate * inFrameCount / dispatchPtr->fadeSampleRate;
				}

				if (dispatchPtr->fadeWordSize == 12 && dispatchPtr->fadeChannelCount == 1)
					inFrameCount &= 0xFFFFFFFE;

				if (!inFrameCount)
					debug(5, "DiMUSE_v2::dispatch_processDispatches(): WARNING: fade ends with incomplete frame (or odd 12-bit mono frame)");

				effRemainingFade = (inFrameCount * dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) / 8;
				mixVolume = (((dispatchPtr->fadeVol / 65536) + 1) * dispatchPtr->trackPtr->effVol) / 128;

				// Update fadeVol
				dispatchPtr->fadeVol += effRemainingFade * dispatchPtr->fadeSlope;

				// Clamp fadeVol between 0 and MAX_FADE_VOLUME
				if (dispatchPtr->fadeVol < 0)
					dispatchPtr->fadeVol = 0;
				if (dispatchPtr->fadeVol > MAX_FADE_VOLUME)
					dispatchPtr->fadeVol = MAX_FADE_VOLUME;

				// Send it all to the mixer
				srcBuf = (uint8 *)(dispatchPtr->fadeBuf + dispatchPtr->fadeOffset);
				
				_diMUSEMixer->mixer_mix(
					srcBuf,
					inFrameCount,
					dispatchPtr->fadeWordSize,
					dispatchPtr->fadeChannelCount,
					effFeedSize,
					mixStartingPoint,
					mixVolume,
					trackPtr->pan);

				dispatchPtr->fadeOffset += effRemainingFade;
				dispatchPtr->fadeRemaining -= effRemainingFade;

				if (!dispatchPtr->fadeRemaining) {
					// Fade ended, deallocate it
					ptrCtr = 0;
					for (i = 0, ptrCtr = 0;
						i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
						i++, ptrCtr += LARGE_FADE_DIM) {

						if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
							if (dispatch_largeFadeFlags[i] == 0) {
								debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant large fade buf de-allocation");
							}
							dispatch_largeFadeFlags[i] = 0;
							break;
						}
					}

					if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
						for (j = 0, ptrCtr = 0;
							j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
							j++, ptrCtr += SMALL_FADE_DIM) {

							if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
								debug(5, "DiMUSE_v2::dispatch_processDispatches(): couldn't find fade buf to de-allocate");
								break;
							}

							if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
								if (dispatch_smallFadeFlags[j] == 0) {
									debug(5, "DiMUSE_v2::dispatch_processDispatches(): redundant small fade buf de-allocation");
								}
								dispatch_smallFadeFlags[j] = 0;
								break;
							}
						}
					}
				}
			}
		}

		if (!feedSize)
			return;

		effWordSize = dispatchPtr->channelCount * dispatchPtr->wordSize;
		inFrameCount = dispatchPtr->sampleRate * feedSize / sampleRate;

		if (inFrameCount <= (8 * dispatchPtr->audioRemaining / effWordSize)) {
			effFeedSize = feedSize;
		} else {
			inFrameCount = 8 * dispatchPtr->audioRemaining / effWordSize;
			effFeedSize = sampleRate * (8 * dispatchPtr->audioRemaining / effWordSize) / dispatchPtr->sampleRate;
		}

		if (dispatchPtr->wordSize == 12 && dispatchPtr->channelCount == 1)
			inFrameCount &= 0xFFFFFFFE;

		if (!inFrameCount) {
			debug(5, "DiMUSE_v2::dispatch_processDispatches(): ERROR: region ends with incomplete frame (or odd 12-bit mono frame)");
			tracks_clear(trackPtr);
			return;
		}

		// Play the audio of the current region
		effRemainingAudio = (effWordSize * inFrameCount) / 8;

		if (dispatchPtr->streamPtr) {
			srcBuf = (uint8 *)streamer_reAllocReadBuffer(dispatchPtr->streamPtr, (effWordSize * inFrameCount) / 8);
			if (!srcBuf) {

				dispatchPtr->streamErrFlag = 1;
				if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
					dispatchPtr->fadeSyncDelta += feedSize;

				streamer_queryStream(
					dispatchPtr->streamPtr,
					&dispatch_curStreamBufSize,
					&dispatch_curStreamCriticalSize,
					&dispatch_curStreamFreeSpace,
					&dispatch_curStreamPaused);

				if (dispatch_curStreamPaused) {
					debug(5, "DiMUSE_v2::dispatch_processDispatches(): WARNING: stopping starving paused stream");
					tracks_clear(trackPtr);
				}

				return;
			}
			dispatchPtr->streamZoneList->offset += effRemainingAudio;
			dispatchPtr->streamZoneList->size -= effRemainingAudio;
			dispatchPtr->streamErrFlag = 0;
		} else {
			soundAddrData = files_getSoundAddrData(trackPtr->soundId);
			if (!soundAddrData) {
				debug(5, "ERR: dispatch got NULL file addr...\n");
				return;
			}

			srcBuf = (uint8 *)(soundAddrData + dispatchPtr->currentOffset);
		}

		if (dispatchPtr->fadeBuf) {
			// If the fadeSyncFlag is active (e.g. we are crossfading
			// to another version of the same music piece), do this... thing,
			// and update the fadeSyncDelta
			if (dispatchPtr->fadeSyncFlag) {
				if (dispatchPtr->fadeSyncDelta) {
					elapsedFadeDelta = effFeedSize;
					if (effFeedSize >= dispatchPtr->fadeSyncDelta)
						elapsedFadeDelta = dispatchPtr->fadeSyncDelta;

					dispatchPtr->fadeSyncDelta -= elapsedFadeDelta;
					effFeedSize -= elapsedFadeDelta;
					inFrameCount = effFeedSize * dispatchPtr->sampleRate / sampleRate;

					if (dispatchPtr->wordSize == 12 && dispatchPtr->channelCount == 1)
						inFrameCount &= 0xFFFFFFFE;

					srcBuf = &srcBuf[effRemainingAudio - ((dispatchPtr->wordSize * inFrameCount * dispatchPtr->channelCount) / 8)];
				}
			}

			// If there's still a fadeBuffer active in our dispatch
			// we balance the volume of the considered track with 
			// the fade volume, effectively creating a crossfade
			if (dispatchPtr->fadeBuf) {
				// Fade-in
				mixVolume = (dispatchPtr->trackPtr->effVol * (128 - (dispatchPtr->fadeVol / 65536))) / 128;

				// Update the fadeSlope
				if (!dispatchPtr->fadeSlope) {
					effRemainingFade = dispatchPtr->fadeRemaining;
					if (effRemainingFade <= 1)
						effRemainingFade = 2;
					dispatchPtr->fadeSlope = -(MAX_FADE_VOLUME / effRemainingFade);
				}
			} else {
				mixVolume = trackPtr->effVol;
			}
		} else {
			mixVolume = trackPtr->effVol;
		}

		
		if (trackPtr->mailbox) // Whatever this thing does
			_diMUSEMixer->mixer_setRadioChatter();
			
		_diMUSEMixer->mixer_mix(
			srcBuf,
			inFrameCount,
			dispatchPtr->wordSize,
			dispatchPtr->channelCount,
			effFeedSize,
			mixStartingPoint,
			mixVolume,
			trackPtr->pan);

		_diMUSEMixer->mixer_clearRadioChatter();
		mixStartingPoint += effFeedSize;
		feedSize -= effFeedSize;

		dispatchPtr->currentOffset += effRemainingAudio;
		dispatchPtr->audioRemaining -= effRemainingAudio;
	}

	// Behavior of errors and STOP marker
	if (getNextMapEventResult == -1)
		tracks_clear(trackPtr);

	if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
		dispatchPtr->fadeSyncDelta += feedSize;
}

int DiMUSE_v2::dispatch_predictFirstStream() {
	waveapi_increaseSlice();

	if (tracks_trackCount > 0) {
		for (int i = 0; i < tracks_trackCount; i++) {
			if (dispatches[i].trackPtr->soundId && dispatches[i].streamPtr && dispatches[i].streamZoneList)
				dispatch_predictStream(&dispatches[i]);
		}
	}

	return waveapi_decreaseSlice();
}

int DiMUSE_v2::dispatch_getNextMapEvent(iMUSEDispatch *dispatchPtr) {
	int *dstMap;
	uint8 *rawMap;
	iMUSEStreamZone *allStrZn;
	uint8 *copiedBuf;
	uint8 *soundAddrData;
	int size;
	int *mapCurPos;
	int curOffset;
	int blockName;
	/*unsigned */int effFadeSize;
	/*unsigned */int elapsedFadeSize;
	int regionOffset;
	int ptrCtr, i, j;


	dstMap = dispatchPtr->map;

	// Sanity checks for the correct file format
	// and the correct state of the stream in the current dispatch
	if (dispatchPtr->map[0] != 'MAP ') {
		if (dispatchPtr->currentOffset) {
			debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): found offset but no map");
			return -1;
		}

		// This appears to always be NULL since func_fetchMap is never initialized
		rawMap = (uint8 *)files_fetchMap(dispatchPtr->trackPtr->soundId);
		if (rawMap) {
			if (dispatch_convertMap((uint8 *)rawMap, (uint8 *)dstMap)) {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): dispatch_convertMap() failed");
				return -1;
			}

			if (dispatchPtr->map[2] != 'FRMT') {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: expected 'FRMT' at start of map");
				return -1;
			}

			dispatchPtr->currentOffset = dispatchPtr->map[4];
			if (dispatchPtr->streamPtr) {
				if (streamer_getFreeBuffer(dispatchPtr->streamPtr)) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: expected empty stream buf");
					return -1;
				}

				if (dispatchPtr->streamZoneList) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: expected null streamZoneList");
					return -1;
				}

				allStrZn = dispatch_allocStreamZone();
				dispatchPtr->streamZoneList = allStrZn;
				if (!allStrZn) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: couldn't alloc zone");
					return -1;
				}

				allStrZn->offset = dispatchPtr->currentOffset;
				dispatchPtr->streamZoneList->size = 0;
				dispatchPtr->streamZoneList->fadeFlag = 0;
				streamer_setSoundToStreamWithCurrentOffset(
					dispatchPtr->streamPtr,
					dispatchPtr->trackPtr->soundId,
					dispatchPtr->currentOffset);
			}

		}
		// If there's a streamPtr it means that this is a sound loaded
		// from a bundle (either music or speech)
		if (dispatchPtr->streamPtr) {
			
			copiedBuf = (uint8 *)streamer_copyBufferAbsolute(dispatchPtr->streamPtr, 0, 0x10u);

			if (!copiedBuf) {
				return -3;
			}

			if (iMUSE_SWAP32(copiedBuf) == 'iMUS' && iMUSE_SWAP32(copiedBuf + 8) == 'MAP ') {
				size = iMUSE_SWAP32(copiedBuf + 12) + 24;
				if (!streamer_copyBufferAbsolute(dispatchPtr->streamPtr, 0, size)) {
					return -3;
				}
				rawMap = (uint8 *)streamer_reAllocReadBuffer(dispatchPtr->streamPtr, size);
				if (!rawMap) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: stream read failed after view succeeded in GetMap()");
					return -1;
				}

				dispatchPtr->currentOffset = size;
				if (dispatch_convertMap(rawMap + 8, (uint8 *)dstMap)) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: ConvertMap() failed");
					return -1;
				}

				if (dispatchPtr->map[2] != 'FRMT') {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: expected 'FRMT' at start of map");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetMap() expected data to follow map");
					return -1;
				} else {
					if (dispatchPtr->streamZoneList) {
						debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetMap() expected null streamZoneList");
						return -1;
					}

					dispatchPtr->streamZoneList = dispatch_allocStreamZone();
					if (!dispatchPtr->streamZoneList) {
						debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetMap() couldn't alloc zone");
						return -1;
					}

					dispatchPtr->streamZoneList->offset = dispatchPtr->currentOffset;
					dispatchPtr->streamZoneList->size = streamer_getFreeBuffer(dispatchPtr->streamPtr);
					dispatchPtr->streamZoneList->fadeFlag = 0;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: unrecognized file format in stream buf");
				return -1;
			}

		} else {
			// Otherwise, this is a SFX and we must load it using its resource pointer
			soundAddrData = files_getSoundAddrData(dispatchPtr->trackPtr->soundId);

			if (!soundAddrData) {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetMap() couldn't get sound address");
				return -1;
			}

			if (iMUSE_SWAP32(soundAddrData) == 'iMUS' && iMUSE_SWAP32(soundAddrData + 8) == 'MAP ') {
				dispatchPtr->currentOffset = iMUSE_SWAP32(soundAddrData + 12) + 24;
				if (dispatch_convertMap((soundAddrData + 8), (uint8 *)dstMap)) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERR: ConvertMap() failed");
					return -1;
				}

				if (dispatchPtr->map[2] != 'FRMT') {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: expected 'FRMT' at start of map");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetMap() expected data to follow map");
					return -1;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: unrecognized file format in stream buf");
				return -1;
			}
		}
	}

	if (dispatchPtr->audioRemaining
		|| dispatchPtr->streamPtr && dispatchPtr->streamZoneList->offset != dispatchPtr->currentOffset) {
		debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: navigation error in dispatch");
		return -1;
	}

	mapCurPos = NULL;
	while (1) {
		curOffset = dispatchPtr->currentOffset;

		if (mapCurPos) {
			// Advance the map to the next block (mapCurPos[1] + 8 is the size of the block)
			mapCurPos = (int *)((int8 *)mapCurPos + mapCurPos[1] + 8);
			if ((int8 *)&dispatchPtr->map[2] + dispatchPtr->map[1] > (int8 *)mapCurPos) {
				if (mapCurPos[2] != curOffset) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetNextMapEvent() no more events at offset %lu", dispatchPtr->currentOffset);
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: no more map events at offset %lx", dispatchPtr->currentOffset);
					return -1;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetNextMapEvent() map overrun");
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: no more map events at offset %lx", dispatchPtr->currentOffset);
				return -1;
			}

		} else {
			// Init the current map position starting from the first block
			// (cells 0 and 1 are the string 'MAP ' and the map size respectively)
			mapCurPos = &dispatchPtr->map[2];

			// Search for the block with the same offset as ours
			while (mapCurPos[2] != curOffset) {
				// Check if we've overrun the offset, to make sure 
				// that there actually is an event at our offset
				mapCurPos = (int *)((int8 *)mapCurPos + mapCurPos[1] + 8);

				if ((int8 *)&dispatchPtr->map[2] + dispatchPtr->map[1] <= (int8 *)mapCurPos) {
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: GetNextMapEvent() couldn't find event at offset %lu", dispatchPtr->currentOffset);
					debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: no more map events at offset %lx", dispatchPtr->currentOffset);
					return -1;
				}
			}
		}

		if (!mapCurPos) {
			debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: no more map events at offset %lx", dispatchPtr->currentOffset);
			return -1;
		}

		blockName = mapCurPos[0];

		// Handle any event found at this offset
		// Jump block (fixed size: 28 bytes)
		// - The string 'JUMP' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (hook position) (4 bytes)
		// - Jump destination offset (4 bytes)
		// - Hook ID (4 bytes)
		// - Fade time in ms (4 bytes)
		if (blockName == 'JUMP') {
			if (!iMUSE_checkHookId(&dispatchPtr->trackPtr->jumpHook, mapCurPos[4])) {
				// This is the right hookId, let's jump
				dispatchPtr->currentOffset = mapCurPos[3];
				if (dispatchPtr->streamPtr) {
					if (dispatchPtr->streamZoneList->size || !dispatchPtr->streamZoneList->next) {
						debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: failed to prepare for jump"); // Not a real error, apparently
						dispatch_parseJump(dispatchPtr, dispatchPtr->streamZoneList, mapCurPos, 1);
					}

					dispatchPtr->streamZoneList->useFlag = 0;
					iMUSE_removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);

					if (dispatchPtr->streamZoneList->fadeFlag) {
						if (dispatchPtr->fadeBuf) {
							// Mark the fade corresponding to our fadeBuf as unused
							ptrCtr = 0;
							for (i = 0, ptrCtr = 0;
								i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
								i++, ptrCtr += LARGE_FADE_DIM) {

								if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
									if (dispatch_largeFadeFlags[i] == 0) {
										debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: redundant large fade buf de-allocation");
									}
									dispatch_largeFadeFlags[i] = 0;
									break;
								}
							}

							if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
								for (j = 0, ptrCtr = 0;
									j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
									j++, ptrCtr += SMALL_FADE_DIM) {

									if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
										debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: couldn't find fade buf to de-allocate");
										break;
									}

									if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
										if (dispatch_smallFadeFlags[j] == 0) {
											debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: redundant small fade buf de-allocation");
										}
										dispatch_smallFadeFlags[j] = 0;
										break;
									}
								}
							}
						}

						dispatch_requestedFadeSize = dispatchPtr->streamZoneList->size;
						if (dispatch_requestedFadeSize > LARGE_FADE_DIM) {
							debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): WARNING: requested fade too large (%lu)", dispatch_requestedFadeSize);
							dispatch_requestedFadeSize = LARGE_FADE_DIM;
						}

						if (dispatch_fadeSize <= SMALL_FADE_DIM) { // Small fade
							for (i = 0; i <= SMALL_FADES; i++) {
								if (i == SMALL_FADES) {
									debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: couldn't allocate small fade buf");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!dispatch_smallFadeFlags[i]) {
									dispatch_smallFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
									break;
								}
							}
						} else { // Large fade
							for (i = 0; i <= LARGE_FADES; i++) {
								if (i == LARGE_FADES) {
									debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: couldn't allocate large fade buf");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!dispatch_largeFadeFlags[i]) {
									dispatch_largeFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = dispatch_largeFadeFlags + LARGE_FADE_DIM * i;
									break;
								}
							}

							// Fallback to a small fade if large fades are unavailable
							if (!dispatchPtr->fadeBuf) {
								for (i = 0; i <= SMALL_FADES; i++) {
									if (i == SMALL_FADES) {
										debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: couldn't allocate small fade buf");
										dispatchPtr->fadeBuf = NULL;
										break;
									}

									if (!dispatch_smallFadeFlags[i]) {
										dispatch_smallFadeFlags[i] = 1;
										dispatchPtr->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
										break;
									}
								}
							}
						}

						// If the fade buffer is allocated
						// set up the fade
						if (dispatchPtr->fadeBuf) {
							dispatchPtr->fadeWordSize = dispatchPtr->wordSize;
							dispatchPtr->fadeSampleRate = dispatchPtr->sampleRate;
							dispatchPtr->fadeChannelCount = dispatchPtr->channelCount;
							dispatchPtr->fadeOffset = 0;
							dispatchPtr->fadeRemaining = 0;
							dispatchPtr->fadeSyncFlag = 0;
							dispatchPtr->fadeSyncDelta = 0;
							dispatchPtr->fadeVol = MAX_FADE_VOLUME;
							dispatchPtr->fadeSlope = 0;

							// Basically clone the old sound in the fade buffer for just the duration of the fade 
							if (dispatch_requestedFadeSize) {
								do {
									effFadeSize = dispatch_requestedFadeSize - dispatchPtr->fadeRemaining;
									if ((unsigned int)(dispatch_requestedFadeSize - dispatchPtr->fadeRemaining) >= 0x2000)
										effFadeSize = 0x2000;

									memcpy(
										(void *)(dispatchPtr->fadeBuf + dispatchPtr->fadeRemaining),
										(const void *)streamer_reAllocReadBuffer(dispatchPtr->streamPtr, effFadeSize),
										effFadeSize);

									elapsedFadeSize = effFadeSize + dispatchPtr->fadeRemaining;
									dispatchPtr->fadeRemaining = elapsedFadeSize;
								} while (dispatch_requestedFadeSize > elapsedFadeSize);
							}

							dispatch_fadeStartedFlag = 1;
						}

						dispatchPtr->streamZoneList->useFlag = 0;
						iMUSE_removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);
					}
				}

				mapCurPos = NULL;
				continue;
			}

			if (dispatchPtr->audioRemaining)
				return 0;
		}

		// SYNC block (fixed size: x bytes)
		// - The string 'SYNC' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - Don't know yet
		if (blockName == 'SYNC') {
			debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): WARNING: SYNC block not implemented yet");

			continue;
		}

		// Format block (fixed size: 28 bytes)
		// - The string 'FRMT' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - Empty field (4 bytes) (which is set to 1 in Grim Fandango)
		// - Word size between 8, 12 and 16 (4 bytes)
		// - Sample rate (4 bytes)
		// - Number of channels (4 bytes)
		if (blockName == 'FRMT') {
			dispatchPtr->wordSize = mapCurPos[4];
			dispatchPtr->sampleRate = mapCurPos[5];
			dispatchPtr->channelCount = mapCurPos[6];

			continue;
		}

		// Region block (fixed size: 16 bytes)
		// - The string 'REGN' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - Region length (4 bytes)
		if (blockName == 'REGN') {
			regionOffset = mapCurPos[2];
			if (regionOffset == dispatchPtr->currentOffset) {
				dispatchPtr->audioRemaining = mapCurPos[3];
				return 0;
			} else {
				debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: region offset %lu != currentOffset %lu", regionOffset, dispatchPtr->currentOffset);
				return -1;
			}
		}

		// Stop block (fixed size: 12 bytes)
		// Contains:
		// - The string 'STOP' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		if (blockName == 'STOP')
			return -1;

		// Marker block (variable size)
		// Contains:
		// - The string 'TEXT' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - A string of characters ending with '\0' (variable length)
		if (blockName == 'TEXT') {
			char *marker = (char *)mapCurPos + 12;
			triggers_processTriggers(dispatchPtr->trackPtr->soundId, marker);
			if (dispatchPtr->audioRemaining)
				return 0;

			continue;
		}

		debug(5, "DiMUSE_v2::dispatch_getNextMapEvent(): ERROR: Unrecognized map event at offset %lx", dispatchPtr->currentOffset);
		return -1;
	}

	// You should never be able to reach this return
	return 0;
}

// Validated
int DiMUSE_v2::dispatch_convertMap(uint8 *rawMap, uint8 *destMap) {
	int effMapSize;
	uint8 *mapCurPos;
	int blockName;
	uint8 *blockSizePtr;
	unsigned int blockSizeMin8;
	uint8 *firstChar;
	uint8 *otherChars;
	unsigned int remainingFieldsNum;
	int bytesUntilEndOfMap;
	uint8 *endOfMapPtr;

	if (iMUSE_SWAP32(rawMap) == 'MAP ') {
		bytesUntilEndOfMap = iMUSE_SWAP32(rawMap + 4);
		effMapSize = bytesUntilEndOfMap + 8;
		if ((_vm->_game.id == GID_DIG && effMapSize <= 0x400) || (_vm->_game.id == GID_CMI && effMapSize <= 0x2000)) {
			memcpy(destMap, rawMap, effMapSize);

			// Fill (or rather, swap32) the fields:
			// - The 4 bytes string 'MAP '
			// - Size of the map
			//int dest = READ_BE_UINT32(destMap);
			*(int *)destMap = iMUSE_SWAP32(destMap);
			*((int *)destMap + 1) = iMUSE_SWAP32(destMap + 4);

			mapCurPos = destMap + 8;
			endOfMapPtr = &destMap[effMapSize];

			// Swap32 the rest of the map
			while (mapCurPos < endOfMapPtr) {
				// Swap32 the 4 characters block name
				int swapped = iMUSE_SWAP32(mapCurPos);
				*(int *)mapCurPos = swapped;
				blockName = swapped;

				// Advance and Swap32 the block size (minus 8) field
				blockSizePtr = mapCurPos + 4;
				blockSizeMin8 = iMUSE_SWAP32(blockSizePtr);
				*(int *)blockSizePtr = blockSizeMin8;
				mapCurPos = blockSizePtr + 4;

				// Swapping32 a TEXT block is different:
				// it also contains single characters, so we skip them
				// since they're already good like this
				if (blockName == 'TEXT') {
					// Swap32 the block offset position
					*(int *)mapCurPos = iMUSE_SWAP32(mapCurPos);

					// Skip the single characters
					firstChar = mapCurPos + 4;
					mapCurPos += 5;
					if (*firstChar) {
						do {
							otherChars = mapCurPos++;
						} while (*otherChars);
					}
				} else if ((blockSizeMin8 & 0xFFFFFFFC) != 0) {
					// Basically divide by 4 to retrieve the number
					// of fields to swap
					remainingFieldsNum = blockSizeMin8 >> 2;

					// ...and swap them of course
					do {
						*(int *)mapCurPos = iMUSE_SWAP32(mapCurPos);
						mapCurPos += 4;
						--remainingFieldsNum;
					} while (remainingFieldsNum);
				}
			}

			// Just a sanity check to see if we've parsed the whole map,
			// nothing more and nothing less than that
			// TODO: Rewrite this mess: for now I trust this won't go out of bounds
			/*
			if (&destMap[bytesUntilEndOfMap - (uint16)mapCurPos] == (uint8 *)'\xF8') {
				return 0;
			} else {
				debug(5, "ERR: ConvertMap() converted wrong number of bytes...\n");
				return -1;
			}*/
		} else {
			debug(5, "DiMUSE_v2::dispatch_convertMap(): ERROR: map is too big (%lu)", effMapSize);
			return -1;
		}
	} else {
		debug(5, "DiMUSE_v2::dispatch_convertMap(): ERROR: got bogus map");
		return -1;
	}

	return 0;
}

// Validated
void DiMUSE_v2::dispatch_predictStream(iMUSEDispatch *dispatch) {
	iMUSEStreamZone *szTmp;
	iMUSEStreamZone *lastStreamInList;
	iMUSEStreamZone *szList;
	int cumulativeStreamOffset;
	int *curMapPlace;
	int mapPlaceName;
	int *endOfMap;
	int bytesUntilNextPlace, mapPlaceHookId;
	/*unsigned*/int mapPlaceHookPosition;

	if (!dispatch->streamPtr || !dispatch->streamZoneList) {
		debug(5, "DiMUSE_v2::dispatch_predictStream(): ERROR: null streamId or zoneList");
		return;
	}

	szTmp = dispatch->streamZoneList;

	// Get the offset which our stream is currently at
	cumulativeStreamOffset = 0;
	do {
		cumulativeStreamOffset += szTmp->size;
		lastStreamInList = szTmp;
		szTmp = (iMUSEStreamZone *)szTmp->next;
	} while (szTmp);

	lastStreamInList->size += streamer_getFreeBuffer(dispatch->streamPtr) - cumulativeStreamOffset;
	szList = dispatch->streamZoneList;

	buff_hookid = dispatch->trackPtr->jumpHook;
	while (szList) {
		if (!szList->fadeFlag) {
			curMapPlace = &dispatch->map[2];
			endOfMap = (int *)((int8 *)&dispatch->map[2] + dispatch->map[1]);

			while (1) {
				// End of the map, stream
				if (curMapPlace >= endOfMap) {
					curMapPlace = NULL;
					break;
				}

				mapPlaceName = curMapPlace[0]; // TODO: Is this right?
				bytesUntilNextPlace = curMapPlace[1] + 8;

				if (mapPlaceName == 'JUMP') {
					// We assign these here, to avoid going out of bounds
					// on a place which doesn't have as many fields, like TEXT
					mapPlaceHookPosition = curMapPlace[4];
					mapPlaceHookId = curMapPlace[6];

					if (mapPlaceHookPosition > szList->offset && mapPlaceHookPosition <= szList->size + szList->offset) {
						// Break out of the loop if we have to JUMP
						if (!iMUSE_checkHookId(&buff_hookid, mapPlaceHookId))
							break;
					}
				}
				// Advance the getNextMapEventResult by bytesUntilNextPlace bytes
				curMapPlace = (int *)((int8 *)curMapPlace + bytesUntilNextPlace);
			}

			if (curMapPlace) {
				// This is where we should end up if iMUSE_checkHookId has been successful
				dispatch_parseJump(dispatch, szList, curMapPlace, 0);
			} else {
				// Otherwise this is where we end up when we reached the end of the getNextMapEventResult,
				// which means: if we don't have to jump, just play the next streamZone available
				if (szList->next) {
					cumulativeStreamOffset = szList->size;
					szTmp = dispatch->streamZoneList;
					while (szTmp != szList) {
						cumulativeStreamOffset += szTmp->size;
						szTmp = (iMUSEStreamZone *)szTmp->next;
					}

					// Continue streaming the newSoundId from where we left off
					streamer_setIndex2(dispatch->streamPtr, cumulativeStreamOffset);

					// Remove che previous streamZone from the list, we don't need it anymore
					while (szList->next->prev) {
						szList->next->prev->useFlag = 0;
						iMUSE_removeStreamZoneFromList(&szList->next, szList->next->prev);
					}

					streamer_setSoundToStreamWithCurrentOffset(
						dispatch->streamPtr,
						dispatch->trackPtr->soundId,
						szList->size + szList->offset);
				}
			}
		}
		szList = szList->next;
	}
}

// Almost validated, list stuff to check
void DiMUSE_v2::dispatch_parseJump(iMUSEDispatch *dispatchPtr, iMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent) {
	int hookPosition, jumpDestination, fadeTime;
	iMUSEStreamZone *nextStreamZone;
	unsigned int alignmentModDividend;
	iMUSEStreamZone *zoneForJump;
	iMUSEStreamZone *zoneAfterJump;
	unsigned int streamOffset;
	iMUSEStreamZone *zoneCycle;

	/* jumpParamsFromMap format:
		jumpParamsFromMap[0]: four bytes which form the string 'JUMP'
		jumpParamsFromMap[1]: block size in bytes minus 8 (16 for a JUMP block like this; total == 24 bytes)
		jumpParamsFromMap[2]: hook position
		jumpParamsFromMap[3]: jump destination
		jumpParamsFromMap[4]: hook ID
		jumpParamsFromMap[5]: fade time in milliseconds
	*/

	hookPosition = jumpParamsFromMap[2];
	jumpDestination = jumpParamsFromMap[3];
	fadeTime = jumpParamsFromMap[5];

	// Edge cases handling
	if (streamZonePtr->size + streamZonePtr->offset == hookPosition) {
		nextStreamZone = streamZonePtr->next;
		if (nextStreamZone) {
			if (nextStreamZone->fadeFlag) {
				// Avoid jumping if the next stream zone is already fading
				// and its ending position is our jump destination.
				// Basically: cancel the jump if there's already a fade in progress
				if (nextStreamZone->offset == hookPosition) {
					if (nextStreamZone->next) {
						if (nextStreamZone->next->offset == jumpDestination)
							return;
					}
				}
			}
			else {
				// Avoid jumping if we're trying to jump to the next stream zone
				if (nextStreamZone->offset == jumpDestination)
					return;
			}
		}
	}

	// Maximum size of the dispatch for the fade (in bytes)
	dispatch_size = (dispatchPtr->wordSize
		* dispatchPtr->channelCount
		* ((dispatchPtr->sampleRate * fadeTime / 1000u) & 0xFFFFFFFE)) / 8;

	// If this function is being called from predictStream, 
	// avoid accepting an oversized dispatch
	if (!calledFromGetNextMapEvent) {
		if (dispatch_size > streamZonePtr->size + streamZonePtr->offset - hookPosition)
			return;
	}

	// Cap the dispatch size, if oversized
	if (dispatch_size > streamZonePtr->size + streamZonePtr->offset - hookPosition)
		dispatch_size = streamZonePtr->size + streamZonePtr->offset - hookPosition;

	// Validate and adjust the fade dispatch size further;
	// this should correctly align the dispatch size to avoid starting a fade without
	// inverting the stereo image by mistake
	if (_vm->_game.id == GID_DIG) {
		alignmentModDividend = dispatchPtr->channelCount * (dispatchPtr->wordSize == 8 ? 1 : 3);
	} else {
		if (dispatchPtr->wordSize == 8) {
			alignmentModDividend = dispatchPtr->channelCount * 1;
		} else {
			alignmentModDividend = dispatchPtr->channelCount * ((dispatchPtr->wordSize == 12) + 2);
		}
	}
		
	if (alignmentModDividend) {
		dispatch_size -= dispatch_size % alignmentModDividend;
	} else {
		debug(5, "DiMUSE_v2::dispatch_parseJump(): ERROR: ValidateFadeSize() tried mod by 0");
	}

	if (hookPosition < jumpDestination)
		dispatch_size = NULL;

	// Try allocating the two zones needed for the jump
	zoneForJump = NULL;
	if (dispatch_size) {
		for (int i = 0; i < MAX_STREAMZONES; i++) {
			if (!streamZones[i].useFlag) {
				zoneForJump = &streamZones[i];
			}
		}

		if (!zoneForJump) {
			debug(5, "DiMUSE_v2::dispatch_parseJump(): ERROR: out of streamZones");
			debug(5, "DiMUSE_v2::dispatch_parseJump(): ERROR: couldn't alloc zone");
			return;
		}

		zoneForJump->prev = NULL;
		zoneForJump->next = NULL;
		zoneForJump->useFlag = 1;
		zoneForJump->offset = 0;
		zoneForJump->size = 0;
		zoneForJump->fadeFlag = 0;
	}

	zoneAfterJump = NULL;
	for (int i = 0; i < MAX_STREAMZONES; i++) {
		if (!streamZones[i].useFlag) {
			zoneAfterJump = &streamZones[i];
		}
	}

	if (!zoneAfterJump) {
		debug(5, "DiMUSE_v2::dispatch_parseJump(): ERROR: out of streamZones");
		debug(5, "DiMUSE_v2::dispatch_parseJump(): ERROR: couldn't alloc zone");
		return;
	}

	zoneAfterJump->prev = NULL;
	zoneAfterJump->next = NULL;
	zoneAfterJump->useFlag = 1;
	zoneAfterJump->offset = 0;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;

	streamZonePtr->size = hookPosition - streamZonePtr->offset;
	streamOffset = hookPosition - streamZonePtr->offset + dispatch_size;

	// Go to the interested stream zone to calculate the stream offset, 
	// and schedule the sound to stream with that offset
	zoneCycle = dispatchPtr->streamZoneList;
	while (zoneCycle != streamZonePtr) {
		streamOffset += zoneCycle->size;
		zoneCycle = zoneCycle->next;
	}

	streamer_setIndex2(dispatchPtr->streamPtr, streamOffset);

	while (streamZonePtr->next) {
		streamZonePtr->next->useFlag = 0;
		iMUSE_removeStreamZoneFromList(&streamZonePtr->next, streamZonePtr->next);
	}

	streamer_setSoundToStreamWithCurrentOffset(dispatchPtr->streamPtr,
		dispatchPtr->trackPtr->soundId, jumpDestination);

	// Prepare the fading zone for the jump
	// and also a subsequent empty dummy zone
	if (dispatch_size) {
		streamZonePtr->next = zoneForJump;
		zoneForJump->prev = streamZonePtr;
		streamZonePtr = zoneForJump;
		zoneForJump->next = 0;
		zoneForJump->offset = hookPosition;
		zoneForJump->size = dispatch_size;
		zoneForJump->fadeFlag = 1;
	}

	streamZonePtr->next = zoneAfterJump;
	zoneAfterJump->prev = streamZonePtr;
	zoneAfterJump->next = NULL;
	zoneAfterJump->offset = jumpDestination;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;

	return;
}

// Validated
DiMUSE_v2::iMUSEStreamZone *DiMUSE_v2::dispatch_allocStreamZone() {
	for (int i = 0; i < MAX_STREAMZONES; i++) {
		if (streamZones[i].useFlag == 0) {
			streamZones[i].prev = 0;
			streamZones[i].next = 0;
			streamZones[i].useFlag = 1;
			streamZones[i].offset = 0;
			streamZones[i].size = 0;
			streamZones[i].fadeFlag = 0;

			return &streamZones[i];
		}
	}
	debug(5, "DiMUSE_v2::dispatch_allocStreamZone(): ERROR: out of streamZones");
	return NULL;
}

// Validated
void DiMUSE_v2::dispatch_free() {
	free(dispatch_buf);
	dispatch_buf = NULL;
}

} // End of namespace Scumm
