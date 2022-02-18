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
#include "engines/grim/imuse/imuse_defs.h"

namespace Grim {

int Imuse::dispatchInit() {
	_dispatchBuffer = (uint8 *)malloc(DIMUSE_SMALL_FADES * DIMUSE_SMALL_FADE_DIM + DIMUSE_LARGE_FADE_DIM * DIMUSE_LARGE_FADES);

	if (_dispatchBuffer) {
		_dispatchLargeFadeBufs = _dispatchBuffer;
		_dispatchSmallFadeBufs = _dispatchBuffer + (DIMUSE_LARGE_FADE_DIM * DIMUSE_LARGE_FADES);

		for (int i = 0; i < DIMUSE_LARGE_FADES; i++) {
			_dispatchLargeFadeFlags[i] = 0;
		}

		for (int i = 0; i < DIMUSE_SMALL_FADES; i++) {
			_dispatchSmallFadeFlags[i] = 0;
		}

		for (int i = 0; i < DIMUSE_MAX_STREAMZONES; i++) {
			_streamZones[i].useFlag = 0;
			_streamZones[i].fadeFlag = 0;
			_streamZones[i].prev = nullptr;
			_streamZones[i].next = nullptr;
			_streamZones[i].size = 0;
			_streamZones[i].offset = 0;
		}

		for (int i = 0; i < DIMUSE_MAX_DISPATCHES; i++) {
			_dispatches[i].trackPtr = nullptr;
			_dispatches[i].wordSize = 0;
			_dispatches[i].sampleRate = 0;
			_dispatches[i].channelCount = 0;
			_dispatches[i].swapEndiannessFlag = 0;
			_dispatches[i].currentOffset = 0;
			_dispatches[i].audioRemaining = 0;
			memset(_dispatches[i].map, 0, sizeof(_dispatches[i].map));
			_dispatches[i].streamPtr = nullptr;
			_dispatches[i].streamBufID = 0;
			_dispatches[i].streamZoneList = nullptr;
			_dispatches[i].streamErrFlag = 0;
			_dispatches[i].fadeBuf = nullptr;
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
		Debug::debug(Debug::Sound, "Imuse::dispatchInit(): ERROR: couldn't allocate buffers\n");
		return -1;
	}

	return 0;
}

IMuseDigiDispatch *Imuse::dispatchGetDispatchByTrackId(int trackId) {
	return &_dispatches[trackId];
}

void Imuse::dispatchSaveLoad(Common::Serializer &ser) {

	for (int l = 0; l < DIMUSE_MAX_DISPATCHES; l++) {
		ser.syncAsSint32LE(_dispatches[l].wordSize, VER(103));
		ser.syncAsSint32LE(_dispatches[l].sampleRate, VER(103));
		ser.syncAsSint32LE(_dispatches[l].channelCount, VER(103));
		ser.syncAsSint32LE(_dispatches[l].swapEndiannessFlag, VER(103));
		ser.syncAsSint32LE(_dispatches[l].currentOffset, VER(103));
		ser.syncAsSint32LE(_dispatches[l].audioRemaining, VER(103));
		ser.syncArray(_dispatches[l].map, 2048, Common::Serializer::Sint32LE, VER(103));

		// This is only needed to signal if the sound originally had a stream associated with it
		int hasStream = 0;
		if (ser.isSaving()) {
			if (_dispatches[l].streamPtr)
				hasStream = 1;
			ser.syncAsSint32LE(hasStream, VER(103));
		} else {
			ser.syncAsSint32LE(hasStream, VER(103));
			_dispatches[l].streamPtr = hasStream ? (IMuseDigiStream *)1 : nullptr;
		}

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
		for (int i = 0; i < DIMUSE_LARGE_FADES; i++) {
			_dispatchLargeFadeFlags[i] = 0;
		}

		for (int i = 0; i < DIMUSE_SMALL_FADES; i++) {
			_dispatchSmallFadeFlags[i] = 0;
		}

		for (int i = 0; i < DIMUSE_MAX_STREAMZONES; i++) {
			_streamZones[i].useFlag = 0;
		}
	}
}

int Imuse::dispatchRestoreStreamZones() {
	IMuseDigiDispatch *curDispatchPtr;
	IMuseDigiStreamZone *curStreamZone;

	curDispatchPtr = _dispatches;
	for (int i = 0; i < _trackCount; i++) {
		curDispatchPtr = &_dispatches[i];
		curDispatchPtr->fadeBuf = nullptr;

		if (curDispatchPtr->trackPtr->soundId && curDispatchPtr->streamPtr) {
			// Try allocating the stream
			curDispatchPtr->streamPtr = streamerAllocateSound(curDispatchPtr->trackPtr->soundId, curDispatchPtr->streamBufID, 0x4000);

			if (curDispatchPtr->streamPtr) {
				streamerSetSoundToStreamFromOffset(curDispatchPtr->streamPtr, curDispatchPtr->trackPtr->soundId, curDispatchPtr->currentOffset);

				if (curDispatchPtr->audioRemaining) {
					// Try allocating the first streamZone of the dispatch
					curStreamZone = dispatchAllocateStreamZone();
					curDispatchPtr->streamZoneList = curStreamZone;

					if (curStreamZone) {
						curStreamZone->offset = curDispatchPtr->currentOffset;
						curStreamZone->size = 0;
						curStreamZone->fadeFlag = 0;
					} else {
						Debug::debug(Debug::Sound, "Imuse::dispatchRestoreStreamZones(): unable to allocate streamZone during restore");
					}
				}
			} else {
				Debug::debug(Debug::Sound, "Imuse::dispatchRestoreStreamZones(): unable to start stream during restore");
			}
		}
	}
	return 0;
}

int Imuse::dispatchAllocateSound(IMuseDigiTrack *trackPtr, int groupId) {
	IMuseDigiDispatch *trackDispatch;
	int navigateMapResult;

	trackDispatch = trackPtr->dispatchPtr;
	trackDispatch->currentOffset = 0;
	trackDispatch->audioRemaining = 0;
	trackDispatch->fadeBuf = nullptr;

	memset(trackDispatch->map, 0, sizeof(trackDispatch->map));

	if (groupId) {
		trackDispatch->streamPtr = streamerAllocateSound(trackPtr->soundId, groupId, 0x4000);
		trackDispatch->streamBufID = groupId;

		if (!trackDispatch->streamPtr) {
			Debug::debug(Debug::Sound, "Imuse::dispatchAllocateSound(): unable to allocate stream for sound %d", trackPtr->soundId);
			return -1;
		}

		trackDispatch->streamZoneList = 0;
		trackDispatch->streamErrFlag = 0;
	} else {
		trackDispatch->streamPtr = nullptr;
	}

	navigateMapResult = dispatchNavigateMap(trackDispatch);
	if (navigateMapResult && navigateMapResult != -3) {
		// At this point, something went wrong, so let's release the dispatch
		Debug::debug(Debug::Sound, "Imuse::dispatchAllocateSound(): problem starting sound (%d) in dispatch", trackPtr->soundId);
		dispatchRelease(trackPtr);
		return -1;
	}

	return 0;
}

int Imuse::dispatchRelease(IMuseDigiTrack *trackPtr) {
	IMuseDigiDispatch *dispatchToDeallocate;
	IMuseDigiStreamZone *streamZoneList;

	dispatchToDeallocate = trackPtr->dispatchPtr;

	// Remove streamZones from list
	if (dispatchToDeallocate->streamPtr) {
		streamerClearSoundInStream(dispatchToDeallocate->streamPtr);
		streamZoneList = dispatchToDeallocate->streamZoneList;
		if (dispatchToDeallocate->streamZoneList) {
			do {
				streamZoneList->useFlag = 0;
				removeStreamZoneFromList(&dispatchToDeallocate->streamZoneList, streamZoneList);
				streamZoneList = dispatchToDeallocate->streamZoneList;
			} while (dispatchToDeallocate->streamZoneList);
		}
	}

	// Mark the fade corresponding to our fadeBuf as unused
	if (dispatchToDeallocate->fadeBuf)
		dispatchDeallocateFade(dispatchToDeallocate, "dispatchRelease");

	return 0;
}

int Imuse::dispatchSwitchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag) {
	int32 effFadeLen, effFadeSize, strZnSize;
	int getMapResult, i;
	IMuseDigiDispatch *curDispatch = _dispatches;

	effFadeLen = fadeLength;

	if (fadeLength > 2000)
		effFadeLen = 2000;

	for (i = 0; i < _trackCount; i++) {
		curDispatch = &_dispatches[i];
		if (oldSoundId && curDispatch->trackPtr->soundId == oldSoundId && curDispatch->streamPtr) {
			break;
		}
	}

	if (i >= _trackCount) {
		Debug::debug(Debug::Sound, "Imuse::dispatchSwitchStream(): couldn't find sound, index went past _trackCount (%d)", _trackCount);
		return -1;
	}

	if (curDispatch->streamZoneList) {
		if (!curDispatch->wordSize) {
			Debug::debug(Debug::Sound, "Imuse::dispatchSwitchStream(): found streamZoneList but NULL wordSize");
			return -1;
		}

		if (curDispatch->fadeBuf) {
			// Mark the fade corresponding to our fadeBuf as unused
			dispatchDeallocateFade(curDispatch, "dispatchSwitchStream");
		}

		_dispatchFadeSize = dispatchGetFadeSize(curDispatch, effFadeLen);

		strZnSize = curDispatch->streamZoneList->size;
		if (_dispatchFadeSize >= strZnSize)
			_dispatchFadeSize = strZnSize;

		// Validate and adjust the fade dispatch size further;
		// this should correctly align the dispatch size to avoid starting a fade without
		// inverting the stereo image by mistake
		_dispatchFadeSize -= (_dispatchFadeSize & (curDispatch->channelCount * (curDispatch->wordSize >> 3) - 1));

		curDispatch->fadeBuf = dispatchAllocateFade(_dispatchFadeSize, "dispatchSwitchStream");

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
			curDispatch->fadeVol = DIMUSE_MAX_FADE_VOLUME;
			curDispatch->fadeSlope = 0;

			if (_dispatchFadeSize) {
				while (curDispatch->fadeRemaining < _dispatchFadeSize) {
					effFadeSize = _dispatchFadeSize - curDispatch->fadeRemaining;
					if (effFadeSize >= 0x4000)
						effFadeSize = 0x4000;
					uint8 *strBuf = streamerGetStreamBuffer(curDispatch->streamPtr, effFadeSize);
					if (strBuf) {
						memcpy(&curDispatch->fadeBuf[curDispatch->fadeRemaining], strBuf, effFadeSize);
						curDispatch->fadeRemaining += effFadeSize;
					} else {
						memset(&curDispatch->fadeBuf[curDispatch->fadeRemaining], 0, _dispatchFadeSize - curDispatch->fadeRemaining);
						curDispatch->fadeRemaining = _dispatchFadeSize;
					}
				}
			}
		} else {
			Debug::debug(Debug::Sound, "Imuse::dispatchSwitchStream(): WARNING: couldn't allocate fade buffer (from sound %d to sound %d)", oldSoundId, newSoundId);
		}
	}

	// Clear fades and triggers for the old soundId
	char emptyMarker[1] = "";
	_fadesHandler->clearFadeStatus(curDispatch->trackPtr->soundId, -1);
	_triggersHandler->clearTrigger(curDispatch->trackPtr->soundId, (char *)emptyMarker, -1);

	// Setup the new soundId
	curDispatch->trackPtr->soundId = newSoundId;

	streamerSetReadIndex(curDispatch->streamPtr, streamerGetFreeBufferAmount(curDispatch->streamPtr));

	if (offsetFadeSyncFlag && curDispatch->streamZoneList) {
		// Start the soundId from an offset
		streamerSetSoundToStreamFromOffset(curDispatch->streamPtr, newSoundId, curDispatch->currentOffset);
		while (curDispatch->streamZoneList->next) {
			curDispatch->streamZoneList->next->useFlag = 0;
			removeStreamZoneFromList(&curDispatch->streamZoneList->next, curDispatch->streamZoneList->next);
		}
		curDispatch->streamZoneList->size = 0;

		return 0;
	} else {
		// Start the soundId from the beginning
		streamerSetSoundToStreamFromOffset(curDispatch->streamPtr, newSoundId, 0);

		while (curDispatch->streamZoneList) {
			curDispatch->streamZoneList->useFlag = 0;
			removeStreamZoneFromList(&curDispatch->streamZoneList, curDispatch->streamZoneList);
		}

		curDispatch->currentOffset = 0;
		curDispatch->audioRemaining = 0;
		memset(curDispatch->map, 0, sizeof(curDispatch->map));

		getMapResult = dispatchNavigateMap(curDispatch);
		if (!getMapResult || getMapResult == -3) {
			return 0;
		} else {
			Debug::debug(Debug::Sound, "Imuse::dispatchSwitchStream(): problem switching stream in dispatch (from sound %d to sound %d)", oldSoundId, newSoundId);
			tracksClear(curDispatch->trackPtr);
			return -1;
		}
	}
}

void Imuse::dispatchProcessDispatches(IMuseDigiTrack *trackPtr, int feedSize, int sampleRate) {
	IMuseDigiDispatch *dispatchPtr;
	int32 effFeedSize, effWordSize, effRemainingAudio, effRemainingFade, effSampleRate;
	int32 inFrameCount, mixVolume, mixStartingPoint, elapsedFadeDelta;
	int navigateMapResult;
	uint8 *srcBuf, *soundAddrData;

	dispatchPtr = trackPtr->dispatchPtr;
	if (dispatchPtr->streamPtr && dispatchPtr->streamZoneList)
		dispatchPredictStream(dispatchPtr);

	// If a fade has previously been allocated
	if (dispatchPtr->fadeBuf) {
		inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);
		effSampleRate = (trackPtr->pitchShift * dispatchPtr->fadeSampleRate) >> 8;

		if (inFrameCount >= effSampleRate * feedSize / sampleRate) {
			inFrameCount = effSampleRate * feedSize / sampleRate;
			effFeedSize = feedSize;
		} else {
			effFeedSize = sampleRate * inFrameCount / effSampleRate;
		}

		// If the fade is still going on
		if (inFrameCount) {
			// Update the fade volume
			effRemainingFade = ((dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) * inFrameCount) / 8;
			mixVolume = dispatchUpdateFadeMixVolume(dispatchPtr, effRemainingFade);

			// Send it all to the mixer
			srcBuf = &dispatchPtr->fadeBuf[dispatchPtr->fadeOffset];

			_internalMixer->mix(
				srcBuf,
				inFrameCount,
				dispatchPtr->fadeWordSize,
				dispatchPtr->fadeChannelCount,
				dispatchPtr->swapEndiannessFlag,
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
			Debug::debug(Debug::Sound, "Imuse::dispatchProcessDispatches(): WARNING: fade for sound %d ends with incomplete frame (or odd 12-bit mono frame)", trackPtr->soundId);

			// Fade ended, deallocate it
			dispatchDeallocateFade(dispatchPtr, "dispatchProcessDispatches");
		}

		if (!dispatchPtr->fadeRemaining)
			dispatchPtr->fadeBuf = nullptr;
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
			navigateMapResult = dispatchNavigateMap(dispatchPtr);

			if (navigateMapResult)
				break;

			if (_dispatchFadeStartedFlag) {
				// We reached a JUMP, therefore we have to crossfade to
				// the destination region: start a fade-out
				effSampleRate = (trackPtr->pitchShift * dispatchPtr->fadeSampleRate) >> 8;

				inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);
				if (inFrameCount >= effSampleRate * feedSize / sampleRate) {
					inFrameCount = effSampleRate * feedSize / sampleRate;
					effFeedSize = feedSize;
				} else {
					effFeedSize = sampleRate * inFrameCount / effSampleRate;
				}

				if (!inFrameCount)
					Debug::debug(Debug::Sound, "Imuse::dispatchProcessDispatches(): WARNING: fade for sound %d ends with incomplete frame (or odd 12-bit mono frame)", trackPtr->soundId);

				// Update the fade volume
				effRemainingFade = (inFrameCount * dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) / 8;
				mixVolume = dispatchUpdateFadeMixVolume(dispatchPtr, effRemainingFade);

				// Send it all to the mixer
				srcBuf = &dispatchPtr->fadeBuf[dispatchPtr->fadeOffset];

				_internalMixer->mix(
					srcBuf,
					inFrameCount,
					dispatchPtr->fadeWordSize,
					dispatchPtr->fadeChannelCount,
					dispatchPtr->swapEndiannessFlag,
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

		effSampleRate = (trackPtr->pitchShift * dispatchPtr->sampleRate) >> 8;

		effWordSize = dispatchPtr->channelCount * dispatchPtr->wordSize;
		inFrameCount = effSampleRate * feedSize / sampleRate;

		if (inFrameCount <= (8 * dispatchPtr->audioRemaining / effWordSize)) {
			effFeedSize = feedSize;
		} else {
			inFrameCount = 8 * dispatchPtr->audioRemaining / effWordSize;
			effFeedSize = sampleRate * (8 * dispatchPtr->audioRemaining / effWordSize) / effSampleRate;
		}

		if (!inFrameCount) {
			tracksClear(trackPtr);
			return;
		}

		// Play the audio of the current region
		effRemainingAudio = (effWordSize * inFrameCount) / 8;

		if (dispatchPtr->streamPtr) {
			srcBuf = streamerGetStreamBuffer(dispatchPtr->streamPtr, effRemainingAudio);
			if (!srcBuf) {
				dispatchPtr->streamErrFlag = 1;
				if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
					dispatchPtr->fadeSyncDelta += feedSize;

				streamerQueryStream(
					dispatchPtr->streamPtr,
					_dispatchCurStreamBufSize,
					_dispatchCurStreamCriticalSize,
					_dispatchCurStreamFreeSpace,
					_dispatchCurStreamPaused);

				if (_dispatchCurStreamPaused) {
					Debug::debug(Debug::Sound, "Imuse::dispatchProcessDispatches(): WARNING: stopping starving paused stream for sound %d", dispatchPtr->trackPtr->soundId);
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
				Debug::debug(Debug::Sound, "Imuse::dispatchProcessDispatches(): ERROR: soundAddrData for sound %d is NULL", trackPtr->soundId);
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
					effSampleRate = (trackPtr->pitchShift * dispatchPtr->sampleRate) >> 8;
					inFrameCount = effFeedSize * effSampleRate / sampleRate;
					srcBuf = &srcBuf[effRemainingAudio - ((dispatchPtr->wordSize * inFrameCount * dispatchPtr->channelCount) / 8)];
				}
			}

			// If there's still a fadeBuffer active in our dispatch
			// we balance the volume of the considered track with
			// the fade volume, effectively creating a crossfade
			if (dispatchPtr->fadeBuf) {
				// Fade-in
				mixVolume = dispatchUpdateFadeSlope(dispatchPtr);
			} else {
				mixVolume = trackPtr->effVol;
			}
		} else {
			mixVolume = trackPtr->effVol;
		}

		_internalMixer->mix(
			srcBuf,
			inFrameCount,
			dispatchPtr->wordSize,
			dispatchPtr->channelCount,
			dispatchPtr->swapEndiannessFlag,
			effFeedSize,
			mixStartingPoint,
			mixVolume,
			trackPtr->pan);

		mixStartingPoint += effFeedSize;
		feedSize -= effFeedSize;

		dispatchPtr->currentOffset += effRemainingAudio;
		dispatchPtr->audioRemaining -= effRemainingAudio;
	}

	// Behavior of errors and STOP marker
	if (navigateMapResult == -1)
		tracksClear(trackPtr);

	if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
		dispatchPtr->fadeSyncDelta += feedSize;
}

void Imuse::dispatchPredictFirstStream() {
	Common::StackLock lock(_mutex);

	for (int i = 0; i < _trackCount; i++) {
		if (_dispatches[i].trackPtr->soundId && _dispatches[i].streamPtr && _dispatches[i].streamZoneList)
			dispatchPredictStream(&_dispatches[i]);
	}
}

int Imuse::dispatchNavigateMap(IMuseDigiDispatch *dispatchPtr) {
	int32 *mapCurEvent;
	int32 blockTag, effFadeSize, elapsedFadeSize, regionOffset;
	char *marker = NULL;

	Common::StackLock lock(_mutex);

	int getMapResult = dispatchGetMap(dispatchPtr);
	if (getMapResult)
		return getMapResult;

	if (dispatchPtr->audioRemaining
		|| (dispatchPtr->streamPtr && dispatchPtr->streamZoneList->offset != dispatchPtr->currentOffset)) {
		Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): ERROR: navigation error in dispatch");
		return -1;
	}

	mapCurEvent = NULL;
	while (1) {
		mapCurEvent = dispatchGetNextMapEvent(dispatchPtr->map, dispatchPtr->currentOffset, mapCurEvent);

		if (!mapCurEvent) {
			Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): ERROR: no more map events at offset %dx", dispatchPtr->currentOffset);
			return -1;
		}

		blockTag = mapCurEvent[0];
		switch (blockTag) {
		case MKTAG('J', 'U', 'M', 'P'):
			// Handle any event found at this offset
			// Jump block (fixed size: 28 bytes)
			// - The tag 'JUMP' (4 bytes)
			// - Block size in bytes minus 8 (4 bytes)
			// - Block offset (hook position) (4 bytes)
			// - Jump destination offset (4 bytes)
			// - Hook ID (4 bytes)
			// - Fade time in ms (4 bytes)
			if (!checkHookId(dispatchPtr->trackPtr->jumpHook, mapCurEvent[4])) {
				// This is the right hookId, let's jump
				dispatchPtr->currentOffset = mapCurEvent[3];
				if (dispatchPtr->streamPtr) {
					if (dispatchPtr->streamZoneList->size || !dispatchPtr->streamZoneList->next) {
						Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): next streamZone is unallocated, calling dispatchPrepareToJump()");
						dispatchPrepareToJump(dispatchPtr, dispatchPtr->streamZoneList, mapCurEvent, 1);
					}

					Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): \n"
							 "\tJUMP found for sound %d with valid candidateHookId (%d), \n"
							 "\tgoing to offset %d with a crossfade of %d ms",
						  dispatchPtr->trackPtr->soundId, mapCurEvent[4], mapCurEvent[3], mapCurEvent[5]);

					dispatchPtr->streamZoneList->useFlag = 0;
					removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);

					if (dispatchPtr->streamZoneList->fadeFlag) {
						if (dispatchPtr->fadeBuf) {
							// Mark the fade corresponding to our fadeBuf as unused
							dispatchDeallocateFade(dispatchPtr, "dispatchNavigateMap");
						}

						_dispatchJumpFadeSize = dispatchPtr->streamZoneList->size;
						dispatchPtr->fadeBuf = dispatchAllocateFade(_dispatchJumpFadeSize, "dispatchNavigateMap");

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
							dispatchPtr->fadeVol = DIMUSE_MAX_FADE_VOLUME;
							dispatchPtr->fadeSlope = 0;

							// Clone the old sound in the fade buffer for just the duration of the fade
							if (_dispatchJumpFadeSize) {
								do {
									effFadeSize = _dispatchJumpFadeSize - dispatchPtr->fadeRemaining;
									if ((_dispatchJumpFadeSize - dispatchPtr->fadeRemaining) >= 0x4000)
										effFadeSize = 0x4000;

									memcpy(&dispatchPtr->fadeBuf[dispatchPtr->fadeRemaining],
										   streamerGetStreamBuffer(dispatchPtr->streamPtr, effFadeSize),
										   effFadeSize);

									elapsedFadeSize = effFadeSize + dispatchPtr->fadeRemaining;
									dispatchPtr->fadeRemaining = elapsedFadeSize;
								} while (_dispatchJumpFadeSize > elapsedFadeSize);
							}
							_dispatchFadeStartedFlag = 1;
						}
						dispatchPtr->streamZoneList->useFlag = 0;
						removeStreamZoneFromList(&dispatchPtr->streamZoneList, dispatchPtr->streamZoneList);
					}
				}
				mapCurEvent = nullptr;
			}

			continue;
		case MKTAG('S', 'Y', 'N', 'C'):
			// SYNC block:
			// Albeit originally used in The Curse of Monkey Island (SCUMM engine) for speech sync,
			// Grim Fandango uses a different lipsync mechanism, so we ignore this block if ever found
			continue;
		case MKTAG('F', 'R', 'M', 'T'):
			// Format block (fixed size: 28 bytes)
			// - The tag 'FRMT' (4 bytes)
			// - Block size in bytes minus 8 (4 bytes)
			// - Block offset (4 bytes)
			// - Endianness flag (4 bytes)
			// - Word size between 8, 12 and 16 (4 bytes)
			// - Sample rate (4 bytes)
			// - Number of channels (4 bytes)
			dispatchPtr->swapEndiannessFlag = mapCurEvent[3] == 0;
			dispatchPtr->wordSize = mapCurEvent[4];
			dispatchPtr->sampleRate = mapCurEvent[5];
			dispatchPtr->channelCount = mapCurEvent[6];

			continue;
		case MKTAG('R', 'E', 'G', 'N'):
			// Region block (fixed size: 16 bytes)
			// - The tag 'REGN' (4 bytes)
			// - Block size in bytes minus 8 (4 bytes)
			// - Block offset (4 bytes)
			// - Region length (4 bytes)
			dispatchPtr->audioRemaining = mapCurEvent[3];
			return 0;
		case MKTAG('S', 'T', 'O', 'P'):
			// Stop block (fixed size: 12 bytes)
			// Contains:
			// - The tag 'STOP' (4 bytes)
			// - Block size in bytes minus 8 (4 bytes)
			// - Block offset (4 bytes)
			return -1;
		case MKTAG('T', 'E', 'X', 'T'):
			// Marker block (variable size)
			// Contains:
			// - The tag 'TEXT' (4 bytes)
			// - Block size in bytes minus 8 (4 bytes)
			// - Block offset (4 bytes)
			// - A string of characters ending with '\0' (variable length)
			marker = (char *)mapCurEvent + 12;
			_triggersHandler->processTriggers(dispatchPtr->trackPtr->soundId, marker);
			if (dispatchPtr->audioRemaining)
				return 0;

			continue;
		default:
			Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): ERROR: Unrecognized map event at offset %dx", dispatchPtr->currentOffset);
			break;
		};
	}
	return -1;
}

int Imuse::dispatchGetMap(IMuseDigiDispatch *dispatchPtr) {
	int32 *dstMap;
	uint8 *rawMap, *copiedBuf, *soundAddrData;
	int32 size;
	int sampleRate, wordSize, nChans, swapFlag, audioLength, formatId = 0;
	uint8 fakeWavHeader[DIMUSE_WAV_HEADER_SIZE];

	dstMap = dispatchPtr->map;

	// If there's no map, try to fetch it
	if (dispatchPtr->map[0] != MKTAG('M', 'A', 'P', ' ') || dispatchPtr->streamPtr && !dispatchPtr->streamZoneList) {
		if (dispatchPtr->currentOffset) {
			Debug::debug(Debug::Sound, "Imuse::dispatchNavigateMap(): found offset but no map");
			return -1;
		}

		// If there's a streamPtr it means that this is a sound loaded
		// from a bundle (either music or speech)
		if (dispatchPtr->streamPtr) {
			copiedBuf = (uint8 *)streamerGetStreamBufferAtOffset(dispatchPtr->streamPtr, 0, 0x40);

			if (!copiedBuf) {
				return -3;
			}

			if (READ_BE_UINT32(copiedBuf)      == MKTAG('R', 'I', 'F', 'F') &&
				READ_BE_UINT32(copiedBuf + 8)  == MKTAG('W', 'A', 'V', 'E') &&
				READ_BE_UINT32(copiedBuf + 12) == MKTAG('f', 'm', 't', ' ')) {

				uint8 *extFrmtBufPtr = _filesHandler->getSoundMgr()->getExtFormatBuffer(copiedBuf, formatId, sampleRate, wordSize, nChans, swapFlag, audioLength);

				dispatchPtr->currentOffset = extFrmtBufPtr - copiedBuf;
				if (!streamerGetStreamBuffer(dispatchPtr->streamPtr, extFrmtBufPtr - copiedBuf))
					return -1;
				_filesHandler->getSoundMgr()->createSoundHeader((int32 *)fakeWavHeader, sampleRate, 16, nChans, audioLength);
				rawMap = fakeWavHeader;
			} else if (READ_BE_UINT32(copiedBuf) == MKTAG('i', 'M', 'U', 'S') && READ_BE_UINT32(copiedBuf + 8) == MKTAG('M', 'A', 'P', ' ')) {
				size = READ_BE_UINT32(copiedBuf + 12) + 24;
				if (!streamerGetStreamBufferAtOffset(dispatchPtr->streamPtr, 0, size)) {
					return -3;
				}
				rawMap = (uint8 *)streamerGetStreamBuffer(dispatchPtr->streamPtr, size);
				if (!rawMap) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: stream read failed after view succeeded");
					return -1;
				}

				dispatchPtr->currentOffset = size;
			} else {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: unrecognized file format in stream buffer");
				return -1;
			}

			if (formatId) {
				dispatchPtr->map[4] = dispatchPtr->currentOffset;
				dispatchPtr->map[11] = dispatchPtr->currentOffset;
				dispatchPtr->map[15] = dispatchPtr->currentOffset + audioLength;
			}

			if (dispatchConvertMap(rawMap + 8, (uint8 *)dstMap)) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: dispatchConvertMap() failed");
				return -1;
			}

			if (dispatchPtr->map[2] != MKTAG('F', 'R', 'M', 'T')) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: expected 'FRMT' at start of map");
				return -1;
			}

			if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: expected data to follow map");
				return -1;
			} else {
				if (dispatchPtr->streamZoneList) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: expected NULL streamZoneList");
					return -1;
				}

				dispatchPtr->streamZoneList = dispatchAllocateStreamZone();
				if (!dispatchPtr->streamZoneList) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: couldn't allocate zone");
					return -1;
				}

				dispatchPtr->streamZoneList->offset = dispatchPtr->currentOffset;
				dispatchPtr->streamZoneList->size = streamerGetFreeBufferAmount(dispatchPtr->streamPtr);
				dispatchPtr->streamZoneList->fadeFlag = 0;
			}
		} else {
			// Otherwise, this is a SFX and we must load it using its resource pointer
			rawMap = _filesHandler->fetchMap(dispatchPtr->trackPtr->soundId);

			if (!rawMap) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: couldn't fetch map for sound %d", dispatchPtr->trackPtr->soundId);
				return -1;
			}

			if (READ_BE_UINT32(rawMap) == MKTAG('i', 'M', 'U', 'S') && READ_BE_UINT32(rawMap + 8) == MKTAG('M', 'A', 'P', ' ')) {
				dispatchPtr->currentOffset = READ_BE_UINT32(rawMap + 12) + 24;
				if (dispatchConvertMap((rawMap + 8), (uint8 *)dstMap)) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: dispatchConvertMap() failure");
					return -1;
				}

				if (dispatchPtr->map[2] != MKTAG('F', 'R', 'M', 'T')) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: expected 'FRMT' at start of map");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: expected data to follow map");
					return -1;
				}
			} else {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetMap(): ERROR: unrecognized file format in stream buffer");
				return -1;
			}
		}
	}

	return 0;
}

int Imuse::dispatchConvertMap(uint8 *rawMap, uint8 *destMap) {
	int32 effMapSize;
	uint8 *mapCurPos;
	int32 blockName;
	uint8 *blockSizePtr;
	uint32 blockSizeMin8;
	uint8 *firstChar;
	uint8 *otherChars;
	uint32 remainingFieldsNum;
	int32 bytesUntilEndOfMap;
	uint8 *endOfMapPtr;

	if (READ_BE_UINT32(rawMap) == MKTAG('M', 'A', 'P', ' ')) {
		bytesUntilEndOfMap = READ_BE_UINT32(rawMap + 4);
		effMapSize = bytesUntilEndOfMap + 8;
		if (effMapSize <= 0x800) {
			memcpy(destMap, rawMap, effMapSize);

			// Fill (or rather, swap32) the fields:
			// - The 4 bytes string 'MAP '
			// - Size of the map
			*(int32 *)destMap = READ_BE_UINT32(destMap);
			*((int32 *)destMap + 1) = READ_BE_UINT32(destMap + 4);

			mapCurPos = destMap + 8;
			endOfMapPtr = &destMap[effMapSize];

			// Swap32 the rest of the map
			while (mapCurPos < endOfMapPtr) {
				// Swap32 the 4 characters block name
				int32 swapped = READ_BE_UINT32(mapCurPos);
				*(int32 *)mapCurPos = swapped;
				blockName = swapped;

				// Advance and Swap32 the block size (minus 8) field
				blockSizePtr = mapCurPos + 4;
				blockSizeMin8 = READ_BE_UINT32(blockSizePtr);
				*(int32 *)blockSizePtr = blockSizeMin8;
				mapCurPos = blockSizePtr + 4;

				// Swapping32 a TEXT block is different:
				// it also contains single characters, so we skip them
				// since they're already good like this
				if (blockName == MKTAG('T', 'E', 'X', 'T')) {
					// Swap32 the block offset position
					*(int32 *)mapCurPos = READ_BE_UINT32(mapCurPos);

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
						*(int32 *)mapCurPos = READ_BE_UINT32(mapCurPos);
						mapCurPos += 4;
						--remainingFieldsNum;
					} while (remainingFieldsNum);
				}
			}

			// Just a sanity check to see if we've parsed the whole map
			if (&destMap[bytesUntilEndOfMap] - mapCurPos == -8) {
				return 0;
			} else {
				Debug::debug(Debug::Sound, "Imuse::dispatchConvertMap(): ERROR: converted wrong number of bytes");
				return -1;
			}
		} else {
			Debug::debug(Debug::Sound, "Imuse::dispatchConvertMap(): ERROR: map is too big (%d)", effMapSize);
			return -1;
		}
	} else {
		Debug::debug(Debug::Sound, "Imuse::dispatchConvertMap(): ERROR: got bogus map");
		return -1;
	}

	return 0;
}

int32 *Imuse::dispatchGetNextMapEvent(int32 *mapPtr, int32 soundOffset, int32 *mapEvent) {
	if (mapEvent) {
		// Advance the map to the next block (mapEvent[1] + 8 is the size of the block)
		mapEvent = (int32 *)((int8 *)mapEvent + mapEvent[1] + 8);

		if ((int8 *)&mapPtr[2] + mapPtr[1] > (int8 *)mapEvent) {
			if (mapEvent[2] != soundOffset) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetNextMapEvent(): ERROR: no more events at offset %d", soundOffset);
				return nullptr;
			}
		} else {
			Debug::debug(Debug::Sound, "Imuse::dispatchGetNextMapEvent(): ERROR: map overrun");
			return nullptr;
		}

	} else {
		// Init the current map position starting from the first block
		// (cells 0 and 1 are the tag 'MAP ' and the map size respectively)
		mapEvent = &mapPtr[2];

		// Search for the block with the same offset as ours
		while (mapEvent[2] != soundOffset) {
			// Check if we've overrun the offset, to make sure
			// that there actually is an event at our offset
			mapEvent = (int32 *)((int8 *)mapEvent + mapEvent[1] + 8);

			if ((int8 *)&mapPtr[2] + mapPtr[1] <= (int8 *)mapEvent) {
				Debug::debug(Debug::Sound, "Imuse::dispatchGetNextMapEvent(): ERROR: couldn't find event at offset %d", soundOffset);
				return nullptr;
			}
		}
	}

	return mapEvent;
}

void Imuse::dispatchPredictStream(IMuseDigiDispatch *dispatchPtr) {
	IMuseDigiStreamZone *szTmp, *lastStreamInList, *curStrZn;
	int32 cumulativeStreamOffset;
	int32 *jumpParameters;

	if (!dispatchPtr->streamPtr || !dispatchPtr->streamZoneList) {
		Debug::debug(Debug::Sound, "Imuse::dispatchPredictStream(): ERROR: NULL streamId or streamZoneList");
		return;
	}

	szTmp = dispatchPtr->streamZoneList;

	// Get the offset which our stream is currently at
	cumulativeStreamOffset = 0;
	do {
		cumulativeStreamOffset += szTmp->size;
		lastStreamInList = szTmp;
		szTmp = szTmp->next;
	} while (szTmp);

	lastStreamInList->size += streamerGetFreeBufferAmount(dispatchPtr->streamPtr) - cumulativeStreamOffset;
	curStrZn = dispatchPtr->streamZoneList;

	for (_dispatchBufferedHookId = dispatchPtr->trackPtr->jumpHook; curStrZn; curStrZn = curStrZn->next) {
		if (!curStrZn->fadeFlag) {
			jumpParameters = dispatchCheckForJump(dispatchPtr->map, curStrZn, _dispatchBufferedHookId);
			if (jumpParameters) {
				// If we've reached a JUMP and it's successful, allocate the streamZone of the destination
				dispatchPrepareToJump(dispatchPtr, curStrZn, jumpParameters, 0);
			} else {
				// If we don't have to jump, just play the next streamZone available
				dispatchStreamNextZone(dispatchPtr, curStrZn);
			}
		}
	}
}

int32 *Imuse::dispatchCheckForJump(int32 *mapPtr, IMuseDigiStreamZone *strZnPtr, int &candidateHookId) {
	int32 *curMapPlace = &mapPtr[2];
	int32 *endOfMap = (int32 *)((int8 *)&mapPtr[2] + mapPtr[1]);
	int32 mapPlaceTag, jumpHookPos, jumpHookId, bytesUntilNextPlace;

	while (curMapPlace < endOfMap) {
		mapPlaceTag = curMapPlace[0];
		bytesUntilNextPlace = curMapPlace[1] + 8;

		if (mapPlaceTag == MKTAG('J', 'U', 'M', 'P')) {
			jumpHookPos = curMapPlace[2];
			jumpHookId = curMapPlace[4];

			if (jumpHookPos > strZnPtr->offset && jumpHookPos <= strZnPtr->size + strZnPtr->offset) {
				if (!checkHookId(candidateHookId, jumpHookId))
					return curMapPlace;
			}
		}
		// Advance the map to the next place
		curMapPlace = (int32 *)((int8 *)curMapPlace + bytesUntilNextPlace);
	}

	return nullptr;
}

void Imuse::dispatchPrepareToJump(IMuseDigiDispatch *dispatchPtr, IMuseDigiStreamZone *strZnPtr, int32 *jumpParams, int calledFromNavigateMap) {
	int32 hookPosition, jumpDestination, fadeTime;
	IMuseDigiStreamZone *nextStreamZone;
	IMuseDigiStreamZone *zoneForJump = nullptr;
	IMuseDigiStreamZone *zoneAfterJump;
	uint32 streamOffset;
	IMuseDigiStreamZone *zoneCycle;

	// jumpParams format:
	// jumpParams[0]: four bytes which form the string 'JUMP'
	// jumpParams[1]: block size in bytes minus 8 (16 for a JUMP block like this one; total == 24 bytes)
	// jumpParams[2]: hook position
	// jumpParams[3]: jump destination
	// jumpParams[4]: hook ID
	// jumpParams[5]: fade time in milliseconds

	hookPosition = jumpParams[2];
	jumpDestination = jumpParams[3];
	fadeTime = jumpParams[5];

	// Edge cases handling
	if (strZnPtr->size + strZnPtr->offset == hookPosition) {
		nextStreamZone = strZnPtr->next;
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
	_dispatchSize = dispatchGetFadeSize(dispatchPtr, fadeTime);

	// If this function is being called from dispatchPredictStream,
	// avoid accepting an oversized dispatch
	if (!calledFromNavigateMap) {
		if (_dispatchSize > strZnPtr->size + strZnPtr->offset - hookPosition)
			return;
	}

	// Cap the dispatch size, if oversized
	if (_dispatchSize > strZnPtr->size + strZnPtr->offset - hookPosition)
		_dispatchSize = strZnPtr->size + strZnPtr->offset - hookPosition;

	// This prevents starting a fade with an inverted stereo image
	_dispatchSize -= (_dispatchSize & (dispatchPtr->channelCount * (dispatchPtr->wordSize >> 3) - 1));

	if ((hookPosition < 2000 && jumpDestination > hookPosition) || dispatchPtr->fadeRemaining) {
		_dispatchSize = 0;
	}

	// Try allocating the two streamZones needed for the jump
	if (_dispatchSize) {
		zoneForJump = dispatchAllocateStreamZone();
		if (!zoneForJump) {
			Debug::debug(Debug::Sound, "Imuse::dispatchPrepareToJump(): ERROR: couldn't allocate streamZone");
			return;
		}
	}

	zoneAfterJump = dispatchAllocateStreamZone();
	if (!zoneAfterJump) {
		Debug::debug(Debug::Sound, "Imuse::dispatchPrepareToJump(): ERROR: couldn't allocate streamZone");
		return;
	}

	strZnPtr->size = hookPosition - strZnPtr->offset;
	streamOffset = hookPosition - strZnPtr->offset + _dispatchSize;

	// Go to the interested stream zone to calculate the stream offset,
	// and schedule the sound to stream with that offset
	zoneCycle = dispatchPtr->streamZoneList;
	while (zoneCycle != strZnPtr) {
		streamOffset += zoneCycle->size;
		zoneCycle = zoneCycle->next;
	}

	streamerSetLoadIndex(dispatchPtr->streamPtr, streamOffset);

	while (strZnPtr->next) {
		strZnPtr->next->useFlag = 0;
		removeStreamZoneFromList(&strZnPtr->next, strZnPtr->next);
	}

	streamerSetSoundToStreamFromOffset(dispatchPtr->streamPtr,
		dispatchPtr->trackPtr->soundId, jumpDestination);

	// Prepare the fading zone for the jump
	// and also a subsequent empty dummy zone
	if (_dispatchSize) {
		strZnPtr->next = zoneForJump;
		zoneForJump->prev = strZnPtr;
		strZnPtr = zoneForJump;
		zoneForJump->next = 0;
		zoneForJump->offset = hookPosition;
		zoneForJump->size = _dispatchSize;
		zoneForJump->fadeFlag = 1;
	}

	strZnPtr->next = zoneAfterJump;
	zoneAfterJump->prev = strZnPtr;
	zoneAfterJump->next = nullptr;
	zoneAfterJump->offset = jumpDestination;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;
}

void Imuse::dispatchStreamNextZone(IMuseDigiDispatch *dispatchPtr, IMuseDigiStreamZone *strZnPtr) {
	int32 cumulativeStreamOffset;
	IMuseDigiStreamZone *szTmp;

	if (strZnPtr->next) {
		cumulativeStreamOffset = strZnPtr->size;
		szTmp = dispatchPtr->streamZoneList;
		while (szTmp != strZnPtr) {
			cumulativeStreamOffset += szTmp->size;
			szTmp = szTmp->next;
		}

		// Set the stream load index of the sound to the new streamZone
		streamerSetLoadIndex(dispatchPtr->streamPtr, cumulativeStreamOffset);

		// Remove che previous streamZone from the list, we don't need it anymore
		while (strZnPtr->next->prev) {
			strZnPtr->next->prev->useFlag = 0;
			removeStreamZoneFromList(&strZnPtr->next, strZnPtr->next->prev);
		}

		streamerSetSoundToStreamFromOffset(
			dispatchPtr->streamPtr,
			dispatchPtr->trackPtr->soundId,
			strZnPtr->size + strZnPtr->offset);
	}
}

IMuseDigiStreamZone *Imuse::dispatchAllocateStreamZone() {
	for (int i = 0; i < DIMUSE_MAX_STREAMZONES; i++) {
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
	Debug::debug(Debug::Sound, "Imuse::dispatchAllocateStreamZone(): ERROR: out of streamZones");
	return nullptr;
}

uint8 *Imuse::dispatchAllocateFade(int32 &fadeSize, const char *function) {
	uint8 *allocatedFadeBuf = nullptr;
	if (fadeSize > DIMUSE_LARGE_FADE_DIM) {
		Debug::debug(Debug::Sound, "Imuse::dispatchAllocateFade(): WARNING: requested fade too large (%d) in %s()", fadeSize, function);
		fadeSize = DIMUSE_LARGE_FADE_DIM;
	}

	if (fadeSize <= DIMUSE_SMALL_FADE_DIM) { // Small fade
		for (int i = 0; i <= DIMUSE_SMALL_FADES; i++) {
			if (i == DIMUSE_SMALL_FADES) {
				Debug::debug(Debug::Sound, "Imuse::dispatchAllocateFade(): couldn't allocate small fade buffer in %s()", function);
				allocatedFadeBuf = nullptr;
				break;
			}

			if (!_dispatchSmallFadeFlags[i]) {
				_dispatchSmallFadeFlags[i] = 1;
				allocatedFadeBuf = &_dispatchSmallFadeBufs[DIMUSE_SMALL_FADE_DIM * i];
				break;
			}
		}
	} else { // Large fade
		for (int i = 0; i <= DIMUSE_LARGE_FADES; i++) {
			if (i == DIMUSE_LARGE_FADES) {
				Debug::debug(Debug::Sound, "Imuse::dispatchAllocateFade(): couldn't allocate large fade buffer in %s()", function);
				allocatedFadeBuf = nullptr;
				break;
			}

			if (!_dispatchLargeFadeFlags[i]) {
				_dispatchLargeFadeFlags[i] = 1;
				allocatedFadeBuf = &_dispatchLargeFadeBufs[DIMUSE_LARGE_FADE_DIM * i];
				break;
			}
		}

		// Fallback to a small fade if large fades are unavailable
		if (!allocatedFadeBuf) {
			for (int i = 0; i <= DIMUSE_SMALL_FADES; i++) {
				if (i == DIMUSE_SMALL_FADES) {
					Debug::debug(Debug::Sound, "Imuse::dispatchAllocateFade(): couldn't allocate small fade buffer in %s()", function);
					allocatedFadeBuf = nullptr;
					break;
				}

				if (!_dispatchSmallFadeFlags[i]) {
					_dispatchSmallFadeFlags[i] = 1;
					allocatedFadeBuf = &_dispatchSmallFadeBufs[DIMUSE_SMALL_FADE_DIM * i];
					break;
				}
			}
		}
	}

	return allocatedFadeBuf;
}

void Imuse::dispatchDeallocateFade(IMuseDigiDispatch *dispatchPtr, const char *function) {
	// This function flags the fade corresponding to our fadeBuf as unused

	// First, check if our fade buffer is one of the large fade buffers
	for (int i = 0; i < DIMUSE_LARGE_FADES; i++) {
		if (_dispatchLargeFadeBufs + (DIMUSE_LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) { // Found it!
			if (_dispatchLargeFadeFlags[i] == 0) {
				Debug::debug(Debug::Sound, "Imuse::dispatchDeallocateFade(): redundant large fade buf de-allocation in %s()", function);
			}
			_dispatchLargeFadeFlags[i] = 0;
			return;
		}
	}

	// If not, check between the small fade buffers
	for (int j = 0; j < DIMUSE_SMALL_FADES; j++) {
		if (_dispatchSmallFadeBufs + (DIMUSE_SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) { // Found it!
			if (_dispatchSmallFadeFlags[j] == 0) {
				Debug::debug(Debug::Sound, "Imuse::dispatchDeallocateFade(): redundant small fade buf de-allocation in %s()", function);
			}
			_dispatchSmallFadeFlags[j] = 0;
			return;
		}
	}

	Debug::debug(Debug::Sound, "Imuse::dispatchDeallocateFade(): couldn't find fade buf to de-allocate in %s()", function);
}

int Imuse::dispatchGetFadeSize(IMuseDigiDispatch *dispatchPtr, int fadeLength) {
	return (dispatchPtr->wordSize * dispatchPtr->channelCount * (dispatchPtr->sampleRate * fadeLength / 1000)) / 8;
}

int Imuse::dispatchUpdateFadeMixVolume(IMuseDigiDispatch *dispatchPtr, int32 remainingFade) {
	int mixVolume = (((dispatchPtr->fadeVol / 65536) + 1) * dispatchPtr->trackPtr->effVol) / 128;
	dispatchPtr->fadeVol += remainingFade * dispatchPtr->fadeSlope;

	if (dispatchPtr->fadeVol < 0)
		dispatchPtr->fadeVol = 0;
	if (dispatchPtr->fadeVol > DIMUSE_MAX_FADE_VOLUME)
		dispatchPtr->fadeVol = DIMUSE_MAX_FADE_VOLUME;

	return mixVolume;
}

int Imuse::dispatchUpdateFadeSlope(IMuseDigiDispatch *dispatchPtr) {
	int32 updatedVolume, effRemainingFade;

	updatedVolume = (dispatchPtr->trackPtr->effVol * (128 - (dispatchPtr->fadeVol / 65536))) / 128;
	if (!dispatchPtr->fadeSlope) {
		effRemainingFade = dispatchPtr->fadeRemaining;
		if (effRemainingFade <= 1)
			effRemainingFade = 2;
		dispatchPtr->fadeSlope = -(DIMUSE_MAX_FADE_VOLUME / effRemainingFade);
	}

	return updatedVolume;
}

} // End of namespace Grim
