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

int DiMUSE_v2::streamer_moduleInit() {
	for (int l = 0; l < MAX_STREAMS; l++) {
		streamer_streams[l].soundId = 0;
	}
	streamer_lastStreamLoaded = NULL;
	return 0;
}

DiMUSE_v2::iMUSEStream *DiMUSE_v2::streamer_alloc(int soundId, int bufId, int maxRead) {
	iMUSESoundBuffer *bufInfoPtr = files_getBufInfo(bufId);
	if (!bufInfoPtr) {
		debug(5, "DiMUSE_v2::streamer_alloc(): ERROR: couldn't get buffer info");
		return NULL;
	}

	if ((bufInfoPtr->bufSize / 4) <= maxRead) {
		debug(5, "DiMUSE_v2::streamer_alloc(): ERROR: maxRead too big for buffer");
		return NULL;
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (streamer_streams[l].soundId && streamer_streams[l].bufId == bufId) {
			debug(5, "DiMUSE_v2::streamer_alloc(): ERROR: stream bufId %lu already in use", bufId);
			return NULL;
		}
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (!streamer_streams[l].soundId) {
			streamer_streams[l].endOffset = files_seek(soundId, 0, SEEK_END, bufId);
			streamer_streams[l].curOffset = 0;
			streamer_streams[l].soundId = soundId;
			streamer_streams[l].bufId = bufId;
			streamer_streams[l].buf = bufInfoPtr->buffer;
			streamer_streams[l].bufFreeSize = bufInfoPtr->bufSize - maxRead - 4;
			streamer_streams[l].loadSize = bufInfoPtr->loadSize;
			streamer_streams[l].criticalSize = bufInfoPtr->criticalSize;
			streamer_streams[l].maxRead = maxRead;
			streamer_streams[l].loadIndex = 0;
			streamer_streams[l].readIndex = 0;
			streamer_streams[l].paused = 0;
			return &streamer_streams[l];
		}
	}
	debug(5, "DiMUSE_v2::streamer_alloc(): ERROR: no spare streams");
	return NULL;
}

int DiMUSE_v2::streamer_clearSoundInStream(iMUSEStream *streamPtr) {
	streamPtr->soundId = 0;
	if (streamer_lastStreamLoaded == streamPtr) {
		streamer_lastStreamLoaded = 0;
	}
	return 0;
}

int DiMUSE_v2::streamer_processStreams() {
	dispatch_predictFirstStream();
	iMUSEStream *stream1 = NULL;
	iMUSEStream *stream2 = NULL;
	iMUSEStream *stream1_tmp = NULL;
	iMUSEStream *stream2_tmp = NULL;

	// Rewrite this mess in a more readable way
	for (int l = 0; l < MAX_STREAMS; l++) {
		if ((streamer_streams[l].soundId != 0) &&
			(!streamer_streams[l].paused) &&
			(stream1 = &streamer_streams[l], stream1_tmp != NULL) &&
			(stream1 = stream1_tmp, stream2 = &streamer_streams[l], stream2_tmp != NULL)) {
			debug(5, "DiMUSE_v2::streamer_processStreams(): WARNING: three streams in use");
			stream2 = stream2_tmp;
		}
		stream1_tmp = stream1;
		stream2_tmp = stream2;
	}

	if (!stream1) {
		if (stream2)
			streamer_fetchData(stream2);
		return 0;
	}

	if (!stream2) {
		if (stream1)
			streamer_fetchData(stream1);
		return 0;
	}

	int size1 = stream1->loadIndex - stream1->readIndex;
	if (size1 < 0) {
		size1 += stream1->bufFreeSize;
	}

	int size2 = stream2->loadIndex - stream2->readIndex;
	if (size1 < 0) {
		size2 += stream2->bufFreeSize;
	}

	// Check if we've reached or surpassed criticalSize
	int critical1 = (size1 >= stream1->criticalSize);
	int critical2 = (size2 >= stream2->criticalSize);

	if (!critical1) {
		if (!critical2) {
			if (stream1 == streamer_lastStreamLoaded) {
				streamer_fetchData(stream1);
				streamer_fetchData(stream2);
				return 0;
			} else {
				streamer_fetchData(stream2);
				streamer_fetchData(stream1);
				return 0;
			}
		} else {
			streamer_fetchData(stream1);
			return 0;
		}
	}

	if (!critical2) {
		streamer_fetchData(stream2);
		return 0;
	} else {
		if (stream1 == streamer_lastStreamLoaded) {
			streamer_fetchData(stream1);
		} else {
			streamer_fetchData(stream2);
		}
		return 0;
	}
}

uint8 *DiMUSE_v2::streamer_reAllocReadBuffer(iMUSEStream *streamPtr, int reallocSize) {
	int size = streamPtr->loadIndex - streamPtr->readIndex;

	if (size < 0)
		size += streamPtr->bufFreeSize;

	if (size < reallocSize || streamPtr->maxRead < reallocSize)
		return 0;

	if (streamPtr->bufFreeSize - streamPtr->readIndex < reallocSize) {
		memcpy(&streamPtr->buf[streamPtr->bufFreeSize], streamPtr->buf, reallocSize - streamPtr->bufFreeSize + streamPtr->readIndex + 4);
	}

	int readIndex_tmp = streamPtr->readIndex;
	uint8 *ptr = &streamPtr->buf[readIndex_tmp];
	streamPtr->readIndex = readIndex_tmp + reallocSize;
	if (streamPtr->bufFreeSize <= readIndex_tmp + reallocSize)
		streamPtr->readIndex = readIndex_tmp + reallocSize - streamPtr->bufFreeSize;

	return ptr;
}

uint8 *DiMUSE_v2::streamer_copyBufferAbsolute(iMUSEStream *streamPtr, int offset, int size) {
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset + size > value || streamPtr->maxRead < size)
		return 0;
	int offsetReadIndex = offset + streamPtr->readIndex;
	if (offsetReadIndex >= streamPtr->bufFreeSize) {
		offsetReadIndex -= streamPtr->bufFreeSize;
	}

	if (streamPtr->bufFreeSize - offsetReadIndex < size) {
		memcpy(&streamPtr->buf[streamPtr->bufFreeSize], streamPtr->buf, size + offsetReadIndex - streamPtr->bufFreeSize);
	}

	return &streamPtr->buf[offsetReadIndex];
}

int DiMUSE_v2::streamer_setIndex1(iMUSEStream *streamPtr, int offset) {
	streamer_bailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset > value)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->readIndex = offsetReadIndex;
	if (streamPtr->bufFreeSize <= offsetReadIndex) {
		streamPtr->readIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

int DiMUSE_v2::streamer_setIndex2(iMUSEStream *streamPtr, int offset) {
	streamer_bailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (value < offset)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->loadIndex = offsetReadIndex;
	if (offsetReadIndex >= streamPtr->bufFreeSize) {
		streamPtr->loadIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

int DiMUSE_v2::streamer_getFreeBuffer(iMUSEStream *streamPtr) {
	int freeBufferSize = streamPtr->loadIndex - streamPtr->readIndex;
	if (freeBufferSize < 0)
		freeBufferSize += streamPtr->bufFreeSize;
	return freeBufferSize;
}

int DiMUSE_v2::streamer_setSoundToStreamWithCurrentOffset(iMUSEStream *streamPtr, int soundId, int currentOffset) {
	streamer_bailFlag = 1;
	streamPtr->soundId = soundId;
	streamPtr->curOffset = currentOffset;
	streamPtr->endOffset = 0;
	streamPtr->paused = 0;
	if (streamer_lastStreamLoaded == streamPtr) {
		streamer_lastStreamLoaded = NULL;
	}
	return 0;
}

int DiMUSE_v2::streamer_queryStream(iMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	dispatch_predictFirstStream();
	*bufSize = streamPtr->bufFreeSize;
	*criticalSize = streamPtr->criticalSize;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	*freeSpace = value;
	*paused = streamPtr->paused;
	return 0;
}

// (appears to be used for IACT blocks)
int DiMUSE_v2::streamer_feedStream(iMUSEStream *streamPtr, uint8 *srcBuf, int sizeToFeed, int paused) {
	int size = streamPtr->loadIndex - streamPtr->readIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;

	if (sizeToFeed > size - 4) {
		debug(5, "DiMUSE_v2::streamer_feedStream(): ERROR: buffer overflow");
		streamer_bailFlag = 1;

		int newTentativeSize = sizeToFeed - size - 4;
		newTentativeSize -= newTentativeSize % 12 + 12;
		size = streamPtr->loadIndex - streamPtr->readIndex;
		if (size < 0)
			size += streamPtr->bufFreeSize;
		if (size >= newTentativeSize) {
			streamPtr->readIndex = newTentativeSize + streamPtr->bufFreeSize;
			if (streamPtr->bufFreeSize <= streamPtr->readIndex)
				streamPtr->readIndex -= streamPtr->bufFreeSize;
		}
	}

	if (sizeToFeed > 0) {
		do {
			size = streamPtr->bufFreeSize - streamPtr->loadIndex;
			if (size >= sizeToFeed) {
				size = sizeToFeed;
			}
			sizeToFeed -= size;
			memcpy(&streamPtr->buf[streamPtr->loadIndex], srcBuf, size);
			srcBuf += size;

			int val = size + streamPtr->loadIndex;
			streamPtr->curOffset += size;
			streamPtr->loadIndex += size;
			if (val >= streamPtr->bufFreeSize) {
				streamPtr->loadIndex = val - streamPtr->bufFreeSize;
			}
		} while (sizeToFeed > 0);
	}

	streamPtr->paused = paused;
	return 0;
}

int DiMUSE_v2::streamer_fetchData(iMUSEStream *streamPtr) {
	if (streamPtr->endOffset == 0) {
		streamPtr->endOffset = files_seek(streamPtr->soundId, 0, SEEK_END, streamPtr->bufId);
	}

	int size = streamPtr->readIndex - streamPtr->loadIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;

	int loadSize = streamPtr->loadSize;
	int remainingSize = streamPtr->endOffset - streamPtr->curOffset;

	if (loadSize >= size - 4) {
		loadSize = size - 4;
	}

	if (loadSize >= remainingSize) {
		loadSize = remainingSize;
	}

	if (remainingSize <= 0) {
		streamPtr->paused = 1;

		// Pad the buffer
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
	}

	int actualAmount;
	int requestedAmount;

	while (1) {
		if (loadSize <= 0)
			return 0;

		requestedAmount = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (requestedAmount >= loadSize) {
			requestedAmount = loadSize;
		}

		if (files_seek(streamPtr->soundId, streamPtr->curOffset, SEEK_SET, streamPtr->bufId) != streamPtr->curOffset) {
			debug(5, "DiMUSE_v2::streamer_fetchData(): ERROR: invalid seek in streamer (%lu)", streamPtr->curOffset);
			streamPtr->paused = 1;
			return 0;
		}

		streamer_bailFlag = 0;
		waveapi_decreaseSlice();
		actualAmount = files_read(streamPtr->soundId, &streamPtr->buf[streamPtr->loadIndex], requestedAmount, streamPtr->bufId);
		waveapi_increaseSlice();
		if (streamer_bailFlag != 0)
			return 0;

		loadSize -= actualAmount;
		streamPtr->curOffset += actualAmount;
		streamer_lastStreamLoaded = streamPtr;

		int newLoadIndex = actualAmount + streamPtr->loadIndex;
		streamPtr->loadIndex = newLoadIndex;
		if (newLoadIndex >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = newLoadIndex - streamPtr->bufFreeSize;
		}
	}

	debug(5, "DiMUSE_v2::streamer_fetchData(): ERROR: unable to load the correct amount of data (req=%lu, act=%lu)", requestedAmount, actualAmount);
	streamer_lastStreamLoaded = NULL;
	return 0;
}

} // End of namespace Scumm
