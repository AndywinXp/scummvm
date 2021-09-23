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
#include "scumm/dimuse_v2/dimuse_v2_defs.h"

namespace Scumm {

int DiMUSE_v2::dispatchInit() {
	_dispatchBuffer = (uint8 *)malloc(SMALL_FADES * SMALL_FADE_DIM + LARGE_FADE_DIM * LARGE_FADES);

	if (_dispatchBuffer) {
		_dispatchLargeFadeBufs = _dispatchBuffer;
		_dispatchSmallFadeBufs = _dispatchBuffer + (LARGE_FADE_DIM * LARGE_FADES);

		for (int i = 0; i < LARGE_FADES; i++) {
			_dispatchLargeFadeFlags[i] = 0;
		}

		for (int i = 0; i < SMALL_FADES; i++) {
			_dispatchSmallFadeFlags[i] = 0;
		}

		for (int i = 0; i < MAX_STREAMZONES; i++) {
			_streamZones[i].useFlag = 0;
			_streamZones[i].fadeFlag = 0;
			_streamZones[i].prev = NULL;
			_streamZones[i].next = NULL;
			_streamZones[i].size = 0;
			_streamZones[i].offset = 0;
		}

		for (int i = 0; i < MAX_DISPATCHES; i++) {
			_dispatches[i].trackPtr = NULL;
			_dispatches[i].wordSize = 0;
			_dispatches[i].sampleRate = 0;
			_dispatches[i].channelCount = 0;
			_dispatches[i].currentOffset = 0;
			_dispatches[i].audioRemaining = 0;
			memset(_dispatches[i].map, 0, sizeof(_dispatches[i].map));
			_dispatches[i].streamPtr = NULL;
			_dispatches[i].streamBufID = 0;
			_dispatches[i].streamZoneList = NULL;
			_dispatches[i].streamErrFlag = 0;
			_dispatches[i].fadeBuf = NULL;
			_dispatches[i].fadeOffset = 0;
			_dispatches[i].fadeRemaining = 0;
			_dispatches[i].fadeWordSize = 0;
			_dispatches[i].fadeSampleRate = 0;
			_dispatches[i].fadeChannelCount	= 0;
			_dispatches[i].fadeSyncFlag = 0;
			_dispatches[i].fadeSyncDelta = 0;
			_dispatches[i].fadeVol = 0;
			_dispatches[i].fadeSlope = 0;
		}

	} else {
		debug(5, "DiMUSE_v2::dispatchInit(): ERROR: couldn't allocate buffers\n");
		return -1;
	}

	return 0;
}

DiMUSEDispatch *DiMUSE_v2::dispatchGetDispatchByTrackId(int trackId) {
	return &_dispatches[trackId];
}

void DiMUSE_v2::dispatchSaveLoad(Common::Serializer &ser) {

	for (int l = 0; l < MAX_DISPATCHES; l++) {
		ser.syncAsSint32LE(_dispatches[l].wordSize, VER(103));
		ser.syncAsSint32LE(_dispatches[l].sampleRate, VER(103));
		ser.syncAsSint32LE(_dispatches[l].channelCount, VER(103));
		ser.syncAsSint32LE(_dispatches[l].currentOffset, VER(103));
		ser.syncAsSint32LE(_dispatches[l].audioRemaining, VER(103));
		ser.syncArray(_dispatches[l].map, 2048, Common::Serializer::Sint32LE, VER(103));
		ser.syncAsSint32LE(_dispatches[l].streamBufID, VER(103));
		ser.syncAsSint32LE(_dispatches[l].streamErrFlag, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeOffset, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeRemaining, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeWordSize, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeSampleRate, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeChannelCount, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeSyncFlag, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeSyncDelta, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeVol, VER(103));
		ser.syncAsSint32LE(_dispatches[l].fadeSlope, VER(103));
	}

	if (ser.isLoading()) {
		for (int i = 0; i < LARGE_FADES; i++) {
			_dispatchLargeFadeFlags[i] = 0;
		}

		for (int i = 0; i < SMALL_FADES; i++) {
			_dispatchSmallFadeFlags[i] = 0;
		}

		for (int i = 0; i < MAX_STREAMZONES; i++) {
			_streamZones[i].useFlag = 0;
		}
	}
}

int DiMUSE_v2::dispatchAllocStreamZones() {
	DiMUSEDispatch *curDispatchPtr;
	DiMUSEStream *curAllocatedStream;
	DiMUSEStreamZone *curStreamZone;
	DiMUSEStreamZone *curStreamZoneList;
	int sizeToFeed = 0x4000; // (_vm->_game.id == GID_DIG) ? 0x2000 : 0x4000;

	curDispatchPtr = _dispatches;
	for (int i = 0; i < _trackCount; i++) {
		curDispatchPtr = &_dispatches[i];
		curDispatchPtr->fadeBuf = 0;

		// This operation below is needed because the EXE saves the streamPtr when saving the dispatches;
		// we don't do that, but we still have to signal this function that the sound is not a SFX (which has a streamPtr == NULL)
		if (curDispatchPtr->trackPtr->soundId < 1000) {
			curDispatchPtr->streamPtr = NULL;
		} else {
			curDispatchPtr->streamPtr = (DiMUSEStream *)1; // Bogus value, just to signal that this sound is either music or speech
		}

		if (curDispatchPtr->trackPtr->soundId && curDispatchPtr->streamPtr) {
			// Try allocating the stream
			curAllocatedStream = streamerAllocateSound(curDispatchPtr->trackPtr->soundId, curDispatchPtr->streamBufID, sizeToFeed);
			curDispatchPtr->streamPtr = curAllocatedStream;

			if (curAllocatedStream) {
				streamerSetSoundToStreamWithCurrentOffset(curAllocatedStream, curDispatchPtr->trackPtr->soundId, curDispatchPtr->currentOffset);
				if (curDispatchPtr->audioRemaining) {

					// Try finding a stream zone which is not in use
					curStreamZone = NULL;
					for (int j = 0; j < MAX_STREAMZONES; j++) {
						if (_streamZones[j].useFlag == 0) {
							curStreamZone = &_streamZones[j];
						}
					}

					if (!curStreamZone) {
						debug(5, "DiMUSE_v2::dispatchAllocStreamZones(): out of streamZones");
						curStreamZoneList = (DiMUSEStreamZone *)&curDispatchPtr->streamZoneList;
						curDispatchPtr->streamZoneList = 0;
					} else {
						curStreamZoneList = (DiMUSEStreamZone *)&curDispatchPtr->streamZoneList;
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
						debug(5, "DiMUSE_v2::dispatchAllocStreamZones(): unable to alloc zone during restore");
					}
				}
			} else {
				debug(5, "DiMUSE_v2::dispatchAllocStreamZones(): unable to start stream during restore");
			}
		}
	}
	return 0;
}

int DiMUSE_v2::dispatchAlloc(DiMUSETrack *trackPtr, int groupId) {
	DiMUSEDispatch *trackDispatch;
	DiMUSEDispatch *dispatchToDeallocate;
	DiMUSEStreamZone *streamZoneList;
	int getMapResult;
	int sizeToFeed = 0x4000; // (_vm->_game.id == GID_DIG) ? 0x2000 : 0x4000;

	trackDispatch = trackPtr->dispatchPtr;
	trackDispatch->currentOffset = 0;
	trackDispatch->audioRemaining = 0;
	memset(trackDispatch->map, 0, sizeof(trackDispatch->map));
	trackDispatch->fadeBuf = 0;
	if (groupId) {
		trackDispatch->streamPtr = streamerAllocateSound(trackPtr->soundId, groupId, sizeToFeed);
		if (!trackDispatch->streamPtr) {
			debug(5, "DiMUSE_v2::dispatchAlloc(): unable to allocate stream for sound %d", trackPtr->soundId);
			return -1;
		}
		trackDispatch->streamBufID = groupId;
		trackDispatch->streamZoneList = 0;
		trackDispatch->streamErrFlag = 0;
	} else {
		trackDispatch->streamPtr = 0;
	}

	getMapResult = dispatchGetNextMapEvent(trackDispatch);
	if (!getMapResult || getMapResult == -3)
		return 0;

	// At this point, something went wrong, so deallocate what we have to...
	debug(5, "DiMUSE_v2::dispatchAlloc(): problem starting sound (%d) in dispatch", trackPtr->soundId);
	
	// Remove streamZones from list
	dispatchToDeallocate = trackDispatch->trackPtr->dispatchPtr;
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = dispatchToDeallocate->streamZoneList;
		streamerClearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				streamZoneList->useFlag = 0;
				removeStreamZoneFromList(&dispatchToDeallocate->streamZoneList, streamZoneList);
			} while (streamZoneList);
		}
	}

	if (!dispatchToDeallocate->fadeBuf)
		return -1;

	// Mark the fade corresponding to our fadeBuf as unused
	dispatchDeallocateFade(dispatchToDeallocate, "dispatchAlloc");
	return -1;
}

int DiMUSE_v2::dispatchRelease(DiMUSETrack *trackPtr) {
	DiMUSEDispatch *dispatchToDeallocate;
	DiMUSEStreamZone *streamZoneList;

	dispatchToDeallocate = trackPtr->dispatchPtr;

	// Remove streamZones from list
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = dispatchToDeallocate->streamZoneList;
		streamerClearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				streamZoneList->useFlag = 0;
				removeStreamZoneFromList(&dispatchToDeallocate->streamZoneList, streamZoneList);
			} while (dispatchToDeallocate->streamZoneList);
		}
	}

	if (!dispatchToDeallocate->fadeBuf)
		return 0;

	// Mark the fade corresponding to our fadeBuf as unused
	dispatchDeallocateFade(dispatchToDeallocate, "dispatchRelease");
	return 0;
}

int DiMUSE_v2::dispatchSwitchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag) {
	int effFadeLen;
	int strZnSize;
	int alignmentModDividend;
	DiMUSEDispatch *curDispatch = _dispatches;
	int effFadeSize;
	int getMapResult;
	int i;
	int sizeToFeed = 0x4000; // (_vm->_game.id == GID_DIG) ? 0x2000 : 0x4000;

	effFadeLen = fadeLength;

	if (fadeLength > 2000)
		effFadeLen = 2000;

	if (_trackCount <= 0) {
		debug(5, "DiMUSE_v2::dispatchSwitchStream(): couldn't find sound, _trackCount is %d", _trackCount);
		return -1;
	}

	for (i = 0; i < _trackCount; i++) {
		curDispatch = &_dispatches[i];
		if (oldSoundId && curDispatch->trackPtr->soundId == oldSoundId && curDispatch->streamPtr) {
			break;
		}
	}

	if (i >= _trackCount) {
		debug(5, "DiMUSE_v2::dispatchSwitchStream(): couldn't find sound, index went past _trackCount (%d)", _trackCount);
		return -1;
	}

	if (curDispatch->streamZoneList) {
		if (!curDispatch->wordSize) {
			debug(5, "DiMUSE_v2::dispatchSwitchStream(): found streamZoneList but NULL wordSize");
			return -1;
		}

		if (curDispatch->fadeBuf) {
			// Mark the fade corresponding to our fadeBuf as unused
			dispatchDeallocateFade(curDispatch, "dispatchSwitchStream");
		}

		_dispatchFadeSize = (curDispatch->channelCount * curDispatch->wordSize * ((effFadeLen * curDispatch->sampleRate / 1000) & 0xFFFFFFFE)) / 8;

		strZnSize = curDispatch->streamZoneList->size;
		if (strZnSize >= _dispatchFadeSize)
			strZnSize = _dispatchFadeSize;
		_dispatchFadeSize = strZnSize;

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
			_dispatchFadeSize -= _dispatchFadeSize % alignmentModDividend;
		} else {
			debug(5, "DiMUSE_v2::dispatchSwitchStream(): ERROR: tried mod by 0 while validating fade size");
		}

		if (_dispatchFadeSize > LARGE_FADE_DIM) {
			debug(5, "DiMUSE_v2::dispatchSwitchStream(): WARNING: requested fade too large (%d)", _dispatchFadeSize);
			_dispatchFadeSize = LARGE_FADE_DIM;
		}

		if (_dispatchFadeSize <= SMALL_FADE_DIM) { // Small fade
			for (i = 0; i <= SMALL_FADES; i++) {
				if (i == SMALL_FADES) {
					debug(5, "DiMUSE_v2::dispatchSwitchStream(): couldn't allocate small fade buf");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!_dispatchSmallFadeFlags[i]) {
					_dispatchSmallFadeFlags[i] = 1;
					curDispatch->fadeBuf = _dispatchSmallFadeBufs + SMALL_FADE_DIM * i;
					break;
				}
			}
		} else { // Large fade
			for (i = 0; i <= LARGE_FADES; i++) {
				if (i == LARGE_FADES) {
					debug(5, "DiMUSE_v2::dispatchSwitchStream(): couldn't allocate large fade buffer");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!_dispatchLargeFadeFlags[i]) {
					_dispatchLargeFadeFlags[i] = 1;
					curDispatch->fadeBuf = _dispatchLargeFadeBufs + LARGE_FADE_DIM * i;
					break;
				}
			}

			// Fallback to a small fade if large fades are unavailable
			if (!curDispatch->fadeBuf) {
				for (i = 0; i <= SMALL_FADES; i++) {
					if (i == SMALL_FADES) {
						debug(5, "DiMUSE_v2::dispatchSwitchStream(): couldn't allocate small fade buffer");
						curDispatch->fadeBuf = NULL;
						break;
					}

					if (!_dispatchSmallFadeFlags[i]) {
						_dispatchSmallFadeFlags[i] = 1;
						curDispatch->fadeBuf = _dispatchSmallFadeBufs + SMALL_FADE_DIM * i;
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
			
			while (curDispatch->fadeRemaining < _dispatchFadeSize) {
				effFadeSize = _dispatchFadeSize - curDispatch->fadeRemaining;
				if ((_dispatchFadeSize - curDispatch->fadeRemaining) >= sizeToFeed)
					effFadeSize = sizeToFeed;

				memcpy((curDispatch->fadeBuf + curDispatch->fadeRemaining), streamerReAllocReadBuffer(curDispatch->streamPtr, effFadeSize), effFadeSize);

				curDispatch->fadeRemaining += effFadeSize;
			}
		} else {
			debug(5, "DiMUSE_v2::dispatchSwitchStream(): WARNING: couldn't allocate fade buffer (from sound %d to sound %d)", oldSoundId, newSoundId);
		}
	}

	// Clear fades and triggers for the old soundId
	char emptyMarker[1] = "";
	_fadesHandler->clearFadeStatus(curDispatch->trackPtr->soundId, -1);
	_triggersHandler->clearTrigger(curDispatch->trackPtr->soundId, (char *)emptyMarker, -1);

	// Setup the new soundId
	curDispatch->trackPtr->soundId = newSoundId;

	streamerSetIndex1(curDispatch->streamPtr, streamerGetFreeBuffer(curDispatch->streamPtr));

	if (offsetFadeSyncFlag && curDispatch->streamZoneList) {
		// Start the soundId from an offset
		streamerSetSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, curDispatch->currentOffset);
		while (curDispatch->streamZoneList->next) {
			curDispatch->streamZoneList->next->useFlag = 0;
			removeStreamZoneFromList(&curDispatch->streamZoneList->next, curDispatch->streamZoneList->next);
		}
		curDispatch->streamZoneList->size = 0;

		return 0;
	} else {
		// Start the soundId from the beginning
		streamerSetSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, 0);

		while (curDispatch->streamZoneList) {
			curDispatch->streamZoneList->useFlag = 0;
			removeStreamZoneFromList(&curDispatch->streamZoneList, curDispatch->streamZoneList);
		}

		curDispatch->currentOffset = 0;
		curDispatch->audioRemaining = 0;
		memset(curDispatch->map, 0, sizeof(curDispatch->map));

		// Sanity check: make sure we correctly switched 
		// the stream and getNextMapEvent gives an error
		getMapResult = dispatchGetNextMapEvent(curDispatch);
		if (!getMapResult || getMapResult == -3) {
			return 0;
		} else {
			debug(5, "DiMUSE_v2::dispatchSwitchStream(): problem switching stream in dispatch (from sound %d to sound %d)", oldSoundId, newSoundId);
			tracksClear(curDispatch->trackPtr);
			return -1;
		}
	}
}

void DiMUSE_v2::dispatchProcessDispatches(DiMUSETrack *trackPtr, int feedSize, int sampleRate) {
	DiMUSEDispatch *dispatchPtr;
	int inFrameCount;
	int effFeedSize;
	int effWordSize;
	int effRemainingAudio;
	int effRemainingFade;
	int effSampleRate;
	int getNextMapEventResult;
	int mixVolume;
	int elapsedFadeDelta;
	uint8 *srcBuf;
	uint8 *soundAddrData;
	int mixStartingPoint;

	dispatchPtr = trackPtr->dispatchPtr;
	if (dispatchPtr->streamPtr && dispatchPtr->streamZoneList)
		dispatchPredictStream(dispatchPtr);

	// If there's a fade
	if (dispatchPtr->fadeBuf) {
		inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);

		if (_vm->_game.id == GID_DIG) {
			effSampleRate = dispatchPtr->fadeSampleRate;
		} else {
			effSampleRate = (trackPtr->pitchShift * dispatchPtr->fadeSampleRate) >> 8;
		}

		if (inFrameCount >= effSampleRate * feedSize / sampleRate) {
			inFrameCount = effSampleRate * feedSize / sampleRate;
			effFeedSize = feedSize;
		} else {
			effFeedSize = sampleRate * inFrameCount / effSampleRate;
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
			
			_internalMixer->mix(
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
				dispatchDeallocateFade(dispatchPtr, "dispatchProcessDispatches");
			}
		} else {
			debug(5, "DiMUSE_v2::dispatchProcessDispatches(): WARNING: fade for sound %d ends with incomplete frame (or odd 12-bit mono frame)", trackPtr->soundId);

			// Fade ended, deallocate it
			dispatchDeallocateFade(dispatchPtr, "dispatchProcessDispatches");
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
			_dispatchFadeStartedFlag = 0;
			getNextMapEventResult = dispatchGetNextMapEvent(dispatchPtr);

			if (getNextMapEventResult)
				break;

			// We reached a JUMP, therefore we have to crossfade to
			// the destination region: start a fade-out
			if (_dispatchFadeStartedFlag) {
				if (_vm->_game.id == GID_DIG) {
					effSampleRate = dispatchPtr->fadeSampleRate;
				} else {
					effSampleRate = (trackPtr->pitchShift * dispatchPtr->fadeSampleRate) >> 8;
				}

				inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);
				if (inFrameCount >= effSampleRate * feedSize / sampleRate) {
					inFrameCount = effSampleRate * feedSize / sampleRate;
					effFeedSize = feedSize;
				} else {
					effFeedSize = sampleRate * inFrameCount / effSampleRate;
				}

				if (dispatchPtr->fadeWordSize == 12 && dispatchPtr->fadeChannelCount == 1)
					inFrameCount &= 0xFFFFFFFE;

				if (!inFrameCount)
					debug(5, "DiMUSE_v2::dispatchProcessDispatches(): WARNING: fade for sound %d ends with incomplete frame (or odd 12-bit mono frame)", trackPtr->soundId);

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
				
				_internalMixer->mix(
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
					dispatchDeallocateFade(dispatchPtr, "dispatchProcessDispatches");
				}
			}
		}

		if (!feedSize)
			return;

		if (_vm->_game.id == GID_DIG) {
			effSampleRate = dispatchPtr->sampleRate;
		} else {
			effSampleRate = (trackPtr->pitchShift * dispatchPtr->sampleRate) >> 8;
		}

		effWordSize = dispatchPtr->channelCount * dispatchPtr->wordSize;
		inFrameCount = effSampleRate * feedSize / sampleRate;

		if (inFrameCount <= (8 * dispatchPtr->audioRemaining / effWordSize)) {
			effFeedSize = feedSize;
		} else {
			inFrameCount = 8 * dispatchPtr->audioRemaining / effWordSize;
			effFeedSize = sampleRate * (8 * dispatchPtr->audioRemaining / effWordSize) / effSampleRate;
		}

		if (dispatchPtr->wordSize == 12 && dispatchPtr->channelCount == 1)
			inFrameCount &= 0xFFFFFFFE;

		if (!inFrameCount) {
			debug(5, "DiMUSE_v2::dispatchProcessDispatches(): WARNING: region in sound %d ends with incomplete frame (or odd 12-bit mono frame)", trackPtr->soundId);
			tracksClear(trackPtr);
			return;
		}

		// Play the audio of the current region
		effRemainingAudio = (effWordSize * inFrameCount) / 8;

		if (dispatchPtr->streamPtr) {
			srcBuf = (uint8 *)streamerReAllocReadBuffer(dispatchPtr->streamPtr, effRemainingAudio);
			if (!srcBuf) {

				dispatchPtr->streamErrFlag = 1;
				if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
					dispatchPtr->fadeSyncDelta += feedSize;

				streamerQueryStream(
					dispatchPtr->streamPtr,
					&_dispatchCurStreamBufSize,
					&_dispatchCurStreamCriticalSize,
					&_dispatchCurStreamFreeSpace,
					&_dispatchCurStreamPaused);

				if (_dispatchCurStreamPaused) {
					debug(5, "DiMUSE_v2::dispatchProcessDispatches(): WARNING: stopping starving paused stream for sound %d", dispatchPtr->trackPtr->soundId);
					tracksClear(trackPtr);
				}

				return;
			}
			dispatchPtr->streamZoneList->offset += effRemainingAudio;
			dispatchPtr->streamZoneList->size -= effRemainingAudio;
			dispatchPtr->streamErrFlag = 0;
		} else {
			soundAddrData = _filesHandler->getSoundAddrData(trackPtr->soundId);
			if (!soundAddrData) {
				debug(5, "DiMUSE_v2::dispatchProcessDispatches(): ERROR: soundAddrData for sound %d is NULL", trackPtr->soundId);
				// Try to gracefully play nothing instead of getting stuck on an infinite loop
				dispatchPtr->currentOffset += effRemainingAudio;
				dispatchPtr->audioRemaining -= effRemainingAudio;
				return;
			}

			srcBuf = &soundAddrData[dispatchPtr->currentOffset];
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

					if (_vm->_game.id == GID_DIG) {
						effSampleRate = dispatchPtr->sampleRate;
					} else {
						effSampleRate = (trackPtr->pitchShift * dispatchPtr->sampleRate) >> 8;
					}

					inFrameCount = effFeedSize * effSampleRate / sampleRate;

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

		// Real-time lo-fi Radio voice effect
		if (trackPtr->mailbox)
			_internalMixer->setRadioChatter();
			
		_internalMixer->mix(
			srcBuf,
			inFrameCount,
			dispatchPtr->wordSize,
			dispatchPtr->channelCount,
			effFeedSize,
			mixStartingPoint,
			mixVolume,
			trackPtr->pan);

		_internalMixer->clearRadioChatter();
		mixStartingPoint += effFeedSize;
		feedSize -= effFeedSize;

		dispatchPtr->currentOffset += effRemainingAudio;
		dispatchPtr->audioRemaining -= effRemainingAudio;
	}

	// Behavior of errors and STOP marker
	if (getNextMapEventResult == -1)
		tracksClear(trackPtr);

	if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
		dispatchPtr->fadeSyncDelta += feedSize;
}

void DiMUSE_v2::dispatchPredictFirstStream() {
	Common::StackLock lock(_mutex);
	//waveOutIncreaseSlice();
	//debug(5, "dispatch_predictFirstStream() called waveOutIncreaseSlice(): %d", _waveSlicingHalted);

	if (_trackCount > 0) {
		for (int i = 0; i < _trackCount; i++) {
			if (_dispatches[i].trackPtr->soundId && _dispatches[i].streamPtr && _dispatches[i].streamZoneList)
				dispatchPredictStream(&_dispatches[i]);
		}
	}

	//waveOutDecreaseSlice();
	//debug(5, "dispatch_predictFirstStream() called waveOutDecreaseSlice(): %d", _waveSlicingHalted);
}

int DiMUSE_v2::dispatchGetNextMapEvent(DiMUSEDispatch *dispatchPtr) {
	int *dstMap;
	uint8 *rawMap;
	DiMUSEStreamZone *allStrZn;
	uint8 *copiedBuf;
	uint8 *soundAddrData;
	int size;
	int *mapCurPos;
	int curOffset;
	int blockName;
	int effFadeSize;
	int elapsedFadeSize;
	int regionOffset;
	int i;

	dstMap = dispatchPtr->map;

	// Sanity checks for the correct file format
	// and the correct state of the stream in the current dispatch
	if (dispatchPtr->map[0] != MKTAG('M', 'A', 'P', ' ')) {
		if (dispatchPtr->currentOffset) {
			debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): found offset but no map");
			return -1;
		}

		// This appears to always be NULL since func_fetchMap is never initialized
		rawMap = _filesHandler->fetchMap(dispatchPtr->trackPtr->soundId);
		if (rawMap) {
			if (dispatchConvertMap((uint8 *)rawMap, (uint8 *)dstMap)) {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): dispatchConvertMap() failed");
				return -1;
			}

			if (dispatchPtr->map[2] != MKTAG('F', 'R', 'M', 'T')) {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected 'FRMT' at start of map");
				return -1;
			}

			dispatchPtr->currentOffset = dispatchPtr->map[4];
			if (dispatchPtr->streamPtr) {
				if (streamerGetFreeBuffer(dispatchPtr->streamPtr)) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected empty stream buffer");
					return -1;
				}

				if (dispatchPtr->streamZoneList) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected NULL streamZoneList");
					return -1;
				}

				allStrZn = dispatchAllocStreamZone();
				dispatchPtr->streamZoneList = allStrZn;
				if (!allStrZn) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't allocate zone");
					return -1;
				}

				allStrZn->offset = dispatchPtr->currentOffset;
				dispatchPtr->streamZoneList->size = 0;
				dispatchPtr->streamZoneList->fadeFlag = 0;
				streamerSetSoundToStreamWithCurrentOffset(
					dispatchPtr->streamPtr,
					dispatchPtr->trackPtr->soundId,
					dispatchPtr->currentOffset);
			}

		}
		// If there's a streamPtr it means that this is a sound loaded
		// from a bundle (either music or speech)
		if (dispatchPtr->streamPtr) {
			
			copiedBuf = (uint8 *)streamerCopyBufferAbsolute(dispatchPtr->streamPtr, 0, 0x10u);

			if (!copiedBuf) {
				return -3;
			}

			if (READ_BE_UINT32(copiedBuf) == MKTAG('i', 'M', 'U', 'S') && READ_BE_UINT32(copiedBuf + 8) == MKTAG('M', 'A', 'P', ' ')) {
				size = READ_BE_UINT32(copiedBuf + 12) + 24;
				if (!streamerCopyBufferAbsolute(dispatchPtr->streamPtr, 0, size)) {
					return -3;
				}
				rawMap = (uint8 *)streamerReAllocReadBuffer(dispatchPtr->streamPtr, size);
				if (!rawMap) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: stream read failed after view succeeded in GetMap()");
					return -1;
				}

				dispatchPtr->currentOffset = size;
				if (dispatchConvertMap(rawMap + 8, (uint8 *)dstMap)) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: ConvertMap() failed");
					return -1;
				}

				if (dispatchPtr->map[2] != MKTAG('F', 'R', 'M', 'T')) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected 'FRMT' at start of map");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: GetMap() expected data to follow map");
					return -1;
				} else {
					if (dispatchPtr->streamZoneList) {
						debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: GetMap() expected NULL streamZoneList");
						return -1;
					}

					dispatchPtr->streamZoneList = dispatchAllocStreamZone();
					if (!dispatchPtr->streamZoneList) {
						debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: GetMap() couldn't allocate zone");
						return -1;
					}

					dispatchPtr->streamZoneList->offset = dispatchPtr->currentOffset;
					dispatchPtr->streamZoneList->size = streamerGetFreeBuffer(dispatchPtr->streamPtr);
					dispatchPtr->streamZoneList->fadeFlag = 0;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: unrecognized file format in stream buffer");
				return -1;
			}

		} else {
			// Otherwise, this is a SFX and we must load it using its resource pointer
			soundAddrData = _filesHandler->getSoundAddrData(dispatchPtr->trackPtr->soundId);

			if (!soundAddrData) {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't get sound address");
				return -1;
			}

			if (READ_BE_UINT32(soundAddrData) == MKTAG('i', 'M', 'U', 'S') && READ_BE_UINT32(soundAddrData + 8) == MKTAG('M', 'A', 'P', ' ')) {
				dispatchPtr->currentOffset = READ_BE_UINT32(soundAddrData + 12) + 24;
				if (dispatchConvertMap((soundAddrData + 8), (uint8 *)dstMap)) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: dispatchConvertMap() failure");
					return -1;
				}

				if (dispatchPtr->map[2] != MKTAG('F', 'R', 'M', 'T')) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected 'FRMT' at start of map");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: expected data to follow map");
					return -1;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: unrecognized file format in stream buffer");
				return -1;
			}
		}
	}

	if (dispatchPtr->audioRemaining
		|| (dispatchPtr->streamPtr && dispatchPtr->streamZoneList->offset != dispatchPtr->currentOffset)) {
		debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: navigation error in dispatch");
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
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: no more events at offset %d", dispatchPtr->currentOffset);
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: no more map events at offset %dx", dispatchPtr->currentOffset);
					return -1;
				}
			} else {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: map overrun");
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: no more map events at offset %dx", dispatchPtr->currentOffset);
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
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't find event at offset %d", dispatchPtr->currentOffset);
					debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: no more map events at offset %dx", dispatchPtr->currentOffset);
					return -1;
				}
			}
		}

		if (!mapCurPos) {
			debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: no more map events at offset %dx", dispatchPtr->currentOffset);
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
		if (blockName == MKTAG('J', 'U', 'M', 'P')) {
			if (!checkHookId(&dispatchPtr->trackPtr->jumpHook, mapCurPos[4])) {
				// This is the right hookId, let's jump
				dispatchPtr->currentOffset = mapCurPos[3];
				if (dispatchPtr->streamPtr) {
					if (dispatchPtr->streamZoneList->size || !dispatchPtr->streamZoneList->next) {
						debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): \n"
							"\tJUMP found for sound %d with valid hookId (%d), \n"
							"\tgoing to offset %d with a crossfade of %d ms",
							dispatchPtr->trackPtr->soundId, mapCurPos[4], mapCurPos[3], mapCurPos[5]);

						dispatchParseJump(dispatchPtr, dispatchPtr->streamZoneList, mapCurPos, 1);
					}

					dispatchPtr->streamZoneList->useFlag = 0;
					removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);

					if (dispatchPtr->streamZoneList->fadeFlag) {
						if (dispatchPtr->fadeBuf) {
							// Mark the fade corresponding to our fadeBuf as unused
							dispatchDeallocateFade(dispatchPtr, "dispatchGetNextMapEvent");
						}

						_dispatchRequestedFadeSize = dispatchPtr->streamZoneList->size;
						if (_dispatchRequestedFadeSize > LARGE_FADE_DIM) {
							debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): WARNING: requested fade too large (%d)", _dispatchRequestedFadeSize);
							_dispatchRequestedFadeSize = LARGE_FADE_DIM;
						}

						if (_dispatchFadeSize <= SMALL_FADE_DIM) { // Small fade
							for (i = 0; i <= SMALL_FADES; i++) {
								if (i == SMALL_FADES) {
									debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't allocate small fade buffer");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!_dispatchSmallFadeFlags[i]) {
									_dispatchSmallFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = _dispatchSmallFadeBufs + SMALL_FADE_DIM * i;
									break;
								}
							}
						} else { // Large fade
							for (i = 0; i <= LARGE_FADES; i++) {
								if (i == LARGE_FADES) {
									debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't allocate large fade buffer");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!_dispatchLargeFadeFlags[i]) {
									_dispatchLargeFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = _dispatchLargeFadeBufs + LARGE_FADE_DIM * i;
									break;
								}
							}

							// Fallback to a small fade if large fades are unavailable
							if (!dispatchPtr->fadeBuf) {
								for (i = 0; i <= SMALL_FADES; i++) {
									if (i == SMALL_FADES) {
										debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: couldn't allocate small fade buffer");
										dispatchPtr->fadeBuf = NULL;
										break;
									}

									if (!_dispatchSmallFadeFlags[i]) {
										_dispatchSmallFadeFlags[i] = 1;
										dispatchPtr->fadeBuf = _dispatchSmallFadeBufs + SMALL_FADE_DIM * i;
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

							int sizeToFeed = 0x4000; // (_vm->_game.id == GID_DIG) ? 0x2000 : 0x4000;
							// Clone the old sound in the fade buffer for just the duration of the fade 
							if (_dispatchRequestedFadeSize) {
								do {
									effFadeSize = _dispatchRequestedFadeSize - dispatchPtr->fadeRemaining;
									if ((_dispatchRequestedFadeSize - dispatchPtr->fadeRemaining) >= sizeToFeed)
										effFadeSize = sizeToFeed;

									memcpy((dispatchPtr->fadeBuf + dispatchPtr->fadeRemaining),
										streamerReAllocReadBuffer(dispatchPtr->streamPtr, effFadeSize),
										effFadeSize);

									elapsedFadeSize = effFadeSize + dispatchPtr->fadeRemaining;
									dispatchPtr->fadeRemaining = elapsedFadeSize;
								} while (_dispatchRequestedFadeSize > elapsedFadeSize);
							}

							_dispatchFadeStartedFlag = 1;
						}

						dispatchPtr->streamZoneList->useFlag = 0;
						removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);
					}
				}

				mapCurPos = NULL;
			}
			continue;
		}

		// SYNC block (fixed size: x bytes)
		// - The string 'SYNC' (4 bytes)
		// - SYNC size in bytes (4 bytes)
		// - SYNC data (variable length)
		if (blockName == MKTAG('S', 'Y', 'N', 'C')) {
			// It is possible to gather a total maximum of 4 SYNCs for a single track;
			// this is not a problem however, as speech files only have one SYNC block,
			// and the most we get is four (one for each character) in 
			// A Pirate I Was Meant To Be, in Part 3 of COMI

			// Curiously we skip the first four bytes of data, ending up having the first
			// four bytes of the next block in our syncPtr; but this is exactly what happens
			// within the interpreter, so I'm not going to argue with it

			if (!dispatchPtr->trackPtr->syncPtr_0) {
				dispatchPtr->trackPtr->syncPtr_0 = (byte *)malloc(mapCurPos[1]);
				memcpy(dispatchPtr->trackPtr->syncPtr_0, mapCurPos + 3, mapCurPos[1]);
				dispatchPtr->trackPtr->syncSize_0 = mapCurPos[1];

			} else if (!dispatchPtr->trackPtr->syncPtr_1) {
				dispatchPtr->trackPtr->syncPtr_1 = (byte *)malloc(mapCurPos[1]);
				memcpy(dispatchPtr->trackPtr->syncPtr_1, mapCurPos + 3, mapCurPos[1]);
				dispatchPtr->trackPtr->syncSize_1 = mapCurPos[1];

			} else if (!dispatchPtr->trackPtr->syncPtr_2) {
				dispatchPtr->trackPtr->syncPtr_2 = (byte *)malloc(mapCurPos[1]);
				memcpy(dispatchPtr->trackPtr->syncPtr_2, mapCurPos + 3, mapCurPos[1]);
				dispatchPtr->trackPtr->syncSize_2 = mapCurPos[1];

			} else if (!dispatchPtr->trackPtr->syncPtr_3) {
				dispatchPtr->trackPtr->syncPtr_3 = (byte *)malloc(mapCurPos[1]);
				memcpy(dispatchPtr->trackPtr->syncPtr_3, mapCurPos + 3, mapCurPos[1]);
				dispatchPtr->trackPtr->syncSize_3 = mapCurPos[1];
			}

			continue;
		}

		// Format block (fixed size: 28 bytes)
		// - The string 'FRMT' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - Empty field (4 bytes) (which is set to 1 in Grim Fandango, I suspect this is the endianness)
		// - Word size between 8, 12 and 16 (4 bytes)
		// - Sample rate (4 bytes)
		// - Number of channels (4 bytes)
		if (blockName == MKTAG('F', 'R', 'M', 'T')) {
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
		if (blockName == MKTAG('R', 'E', 'G', 'N')) {
			regionOffset = mapCurPos[2];
			if (regionOffset == dispatchPtr->currentOffset) {
				dispatchPtr->audioRemaining = mapCurPos[3];
				return 0;
			} else {
				debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: region offset %d != currentOffset %d", regionOffset, dispatchPtr->currentOffset);
				return -1;
			}
		}

		// Stop block (fixed size: 12 bytes)
		// Contains:
		// - The string 'STOP' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		if (blockName == MKTAG('S', 'T', 'O', 'P'))
			return -1;

		// Marker block (variable size)
		// Contains:
		// - The string 'TEXT' (4 bytes)
		// - Block size in bytes minus 8 (4 bytes)
		// - Block offset (4 bytes)
		// - A string of characters ending with '\0' (variable length)
		if (blockName == MKTAG('T', 'E', 'X', 'T')) {
			char *marker = (char *)mapCurPos + 12;
			_triggersHandler->processTriggers(dispatchPtr->trackPtr->soundId, marker);
			if (dispatchPtr->audioRemaining)
				return 0;

			continue;
		}

		debug(5, "DiMUSE_v2::dispatchGetNextMapEvent(): ERROR: Unrecognized map event at offset %dx", dispatchPtr->currentOffset);
		return -1;
	}

	// You should never be able to reach this return
	return 0;
}

int DiMUSE_v2::dispatchConvertMap(uint8 *rawMap, uint8 *destMap) {
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

	if (READ_BE_UINT32(rawMap) == MKTAG('M', 'A', 'P', ' ')) {
		bytesUntilEndOfMap = READ_BE_UINT32(rawMap + 4);
		effMapSize = bytesUntilEndOfMap + 8;
		if ((_vm->_game.id == GID_DIG && effMapSize <= 0x400) || (_vm->_game.id == GID_CMI && effMapSize <= 0x2000)) {
			memcpy(destMap, rawMap, effMapSize);

			// Fill (or rather, swap32) the fields:
			// - The 4 bytes string 'MAP '
			// - Size of the map
			//int dest = READ_BE_UINT32(destMap);
			*(int *)destMap = READ_BE_UINT32(destMap);
			*((int *)destMap + 1) = READ_BE_UINT32(destMap + 4);

			mapCurPos = destMap + 8;
			endOfMapPtr = &destMap[effMapSize];

			// Swap32 the rest of the map
			while (mapCurPos < endOfMapPtr) {
				// Swap32 the 4 characters block name
				int swapped = READ_BE_UINT32(mapCurPos);
				*(int *)mapCurPos = swapped;
				blockName = swapped;

				// Advance and Swap32 the block size (minus 8) field
				blockSizePtr = mapCurPos + 4;
				blockSizeMin8 = READ_BE_UINT32(blockSizePtr);
				*(int *)blockSizePtr = blockSizeMin8;
				mapCurPos = blockSizePtr + 4;

				// Swapping32 a TEXT block is different:
				// it also contains single characters, so we skip them
				// since they're already good like this
				if (blockName == MKTAG('T', 'E', 'X', 'T')) {
					// Swap32 the block offset position
					*(int *)mapCurPos = READ_BE_UINT32(mapCurPos);

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
						*(int *)mapCurPos = READ_BE_UINT32(mapCurPos);
						mapCurPos += 4;
						--remainingFieldsNum;
					} while (remainingFieldsNum);
				}
			}

			// Just a sanity check to see if we've parsed the whole map
			if (&destMap[bytesUntilEndOfMap] - mapCurPos == -8) {
				return 0;
			} else {
				debug(5, "DiMUSE_v2::dispatchConvertMap(): ERROR: converted wrong number of bytes");
				return -1;
			}
		} else {
			debug(5, "DiMUSE_v2::dispatchConvertMap(): ERROR: map is too big (%d)", effMapSize);
			return -1;
		}
	} else {
		debug(5, "DiMUSE_v2::dispatchConvertMap(): ERROR: got bogus map");
		return -1;
	}

	return 0;
}

void DiMUSE_v2::dispatchPredictStream(DiMUSEDispatch *dispatch) {
	DiMUSEStreamZone *szTmp;
	DiMUSEStreamZone *lastStreamInList;
	DiMUSEStreamZone *szList;
	int cumulativeStreamOffset;
	int *curMapPlace;
	int mapPlaceName;
	int *endOfMap;
	int bytesUntilNextPlace, mapPlaceHookId;
	int mapPlaceHookPosition;

	if (!dispatch->streamPtr || !dispatch->streamZoneList) {
		debug(5, "DiMUSE_v2::dispatchPredictStream(): ERROR: NULL streamId or zoneList");
		return;
	}

	szTmp = dispatch->streamZoneList;

	// Get the offset which our stream is currently at
	cumulativeStreamOffset = 0;
	do {
		cumulativeStreamOffset += szTmp->size;
		lastStreamInList = szTmp;
		szTmp = (DiMUSEStreamZone *)szTmp->next;
	} while (szTmp);

	lastStreamInList->size += streamerGetFreeBuffer(dispatch->streamPtr) - cumulativeStreamOffset;
	szList = dispatch->streamZoneList;

	_dispatchBufferedHookId = dispatch->trackPtr->jumpHook;
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

				if (mapPlaceName == MKTAG('J', 'U', 'M', 'P')) {
					// We assign these here, to avoid going out of bounds
					// on a place which doesn't have as many fields, like TEXT
					mapPlaceHookPosition = curMapPlace[4];
					mapPlaceHookId = curMapPlace[6];

					if (mapPlaceHookPosition > szList->offset && mapPlaceHookPosition <= szList->size + szList->offset) {
						// Break out of the loop if we have to JUMP
						if (!checkHookId(&_dispatchBufferedHookId, mapPlaceHookId))
							break;
					}
				}
				// Advance the getNextMapEventResult by bytesUntilNextPlace bytes
				curMapPlace = (int *)((int8 *)curMapPlace + bytesUntilNextPlace);
			}

			if (curMapPlace) {
				// This is where we should end up if iMUSE_checkHookId has been successful
				dispatchParseJump(dispatch, szList, curMapPlace, 0);
			} else {
				// Otherwise this is where we end up when we reached the end of the getNextMapEventResult,
				// which means: if we don't have to jump, just play the next streamZone available
				if (szList->next) {
					cumulativeStreamOffset = szList->size;
					szTmp = dispatch->streamZoneList;
					while (szTmp != szList) {
						cumulativeStreamOffset += szTmp->size;
						szTmp = (DiMUSEStreamZone *)szTmp->next;
					}

					// Continue streaming the newSoundId from where we left off
					streamerSetIndex2(dispatch->streamPtr, cumulativeStreamOffset);

					// Remove che previous streamZone from the list, we don't need it anymore
					while (szList->next->prev) {
						szList->next->prev->useFlag = 0;
						removeStreamZoneFromList(&szList->next, szList->next->prev);
					}

					streamerSetSoundToStreamWithCurrentOffset(
						dispatch->streamPtr,
						dispatch->trackPtr->soundId,
						szList->size + szList->offset);
				}
			}
		}
		szList = szList->next;
	}
}

void DiMUSE_v2::dispatchParseJump(DiMUSEDispatch *dispatchPtr, DiMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent) {
	int hookPosition, jumpDestination, fadeTime;
	DiMUSEStreamZone *nextStreamZone;
	unsigned int alignmentModDividend;
	DiMUSEStreamZone *zoneForJump;
	DiMUSEStreamZone *zoneAfterJump;
	unsigned int streamOffset;
	DiMUSEStreamZone *zoneCycle;

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
			} else {
				// Avoid jumping if we're trying to jump to the next stream zone
				if (nextStreamZone->offset == jumpDestination)
					return;
			}
		}
	}

	// Maximum size of the dispatch for the fade (in bytes)
	_dispatchSize = (dispatchPtr->wordSize
		* dispatchPtr->channelCount
		* ((dispatchPtr->sampleRate * fadeTime / 1000u) & 0xFFFFFFFE)) / 8;

	// If this function is being called from predictStream, 
	// avoid accepting an oversized dispatch
	if (!calledFromGetNextMapEvent) {
		if (_dispatchSize > streamZonePtr->size + streamZonePtr->offset - hookPosition)
			return;
	}

	// Cap the dispatch size, if oversized
	if (_dispatchSize > streamZonePtr->size + streamZonePtr->offset - hookPosition)
		_dispatchSize = streamZonePtr->size + streamZonePtr->offset - hookPosition;

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
		_dispatchSize -= _dispatchSize % alignmentModDividend;
	} else {
		debug(5, "DiMUSE_v2::dispatchParseJump(): ERROR: tried mod by 0 while validating fade size");
	}

	if (_vm->_game.id == GID_DIG) {
		if (hookPosition < jumpDestination)
			_dispatchSize = 0;
	} else {
		if (dispatchPtr->fadeRemaining)
			_dispatchSize = 0;
	}
	// Try allocating the two zones needed for the jump
	zoneForJump = NULL;
	if (_dispatchSize) {
		for (int i = 0; i < MAX_STREAMZONES; i++) {
			if (!_streamZones[i].useFlag) {
				zoneForJump = &_streamZones[i];
			}
		}

		if (!zoneForJump) {
			debug(5, "DiMUSE_v2::dispatchParseJump(): ERROR: out of streamZones");
			debug(5, "DiMUSE_v2::dispatchParseJump(): ERROR: couldn't allocate zone");
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
		if (!_streamZones[i].useFlag) {
			zoneAfterJump = &_streamZones[i];
		}
	}

	if (!zoneAfterJump) {
		debug(5, "DiMUSE_v2::dispatchParseJump(): ERROR: out of streamZones");
		debug(5, "DiMUSE_v2::dispatchParseJump(): ERROR: couldn't allocate zone");
		return;
	}

	zoneAfterJump->prev = NULL;
	zoneAfterJump->next = NULL;
	zoneAfterJump->useFlag = 1;
	zoneAfterJump->offset = 0;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;

	streamZonePtr->size = hookPosition - streamZonePtr->offset;
	streamOffset = hookPosition - streamZonePtr->offset + _dispatchSize;

	// Go to the interested stream zone to calculate the stream offset, 
	// and schedule the sound to stream with that offset
	zoneCycle = dispatchPtr->streamZoneList;
	while (zoneCycle != streamZonePtr) {
		streamOffset += zoneCycle->size;
		zoneCycle = zoneCycle->next;
	}

	streamerSetIndex2(dispatchPtr->streamPtr, streamOffset);

	while (streamZonePtr->next) {
		streamZonePtr->next->useFlag = 0;
		removeStreamZoneFromList(&streamZonePtr->next, streamZonePtr->next);
	}

	streamerSetSoundToStreamWithCurrentOffset(dispatchPtr->streamPtr,
		dispatchPtr->trackPtr->soundId, jumpDestination);

	// Prepare the fading zone for the jump
	// and also a subsequent empty dummy zone
	if (_dispatchSize) {
		streamZonePtr->next = zoneForJump;
		zoneForJump->prev = streamZonePtr;
		streamZonePtr = zoneForJump;
		zoneForJump->next = 0;
		zoneForJump->offset = hookPosition;
		zoneForJump->size = _dispatchSize;
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

DiMUSEStreamZone *DiMUSE_v2::dispatchAllocStreamZone() {
	for (int i = 0; i < MAX_STREAMZONES; i++) {
		if (_streamZones[i].useFlag == 0) {
			_streamZones[i].prev = 0;
			_streamZones[i].next = 0;
			_streamZones[i].useFlag = 1;
			_streamZones[i].offset = 0;
			_streamZones[i].size = 0;
			_streamZones[i].fadeFlag = 0;

			return &_streamZones[i];
		}
	}
	debug(5, "DiMUSE_v2::dispatchAllocStreamZone(): ERROR: out of streamZones");
	return NULL;
}

void DiMUSE_v2::dispatchDeallocateFade(DiMUSEDispatch *dispatchPtr, const char *function) {
	// This function flags the fade corresponding to our fadeBuf as unused

	// First, check if our fade buffer is one of the large fade buffers
	for (int i = 0; i < LARGE_FADES; i++) {
		if (_dispatchLargeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) { // Found it!
			if (_dispatchLargeFadeFlags[i] == 0) {
				debug(5, "DiMUSE_v2::%s(): redundant large fade buf de-allocation", function);
			}
			_dispatchLargeFadeFlags[i] = 0;
			return;
		}
	}

	// If not, check between the small fade buffers
	for (int j = 0; j < SMALL_FADES; j++) {
		if (_dispatchSmallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) { // Found it!
			if (_dispatchSmallFadeFlags[j] == 0) {
				debug(5, "DiMUSE_v2::%s(): redundant small fade buf de-allocation", function);
			}
			_dispatchSmallFadeFlags[j] = 0;
			return;
		}
	}

	debug(5, "DiMUSE_v2::%s(): couldn't find fade buf to de-allocate", function);
}


} // End of namespace Scumm
