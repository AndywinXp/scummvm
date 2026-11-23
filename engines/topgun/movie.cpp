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

#include "common/endian.h"
#include "common/rect.h"
#include "common/textconsole.h"

#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool TopgunEngine_v1_2::ensureMovieResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureMovieResourceLoaded"))
		return false;

	if (_records[index].data)
		return true;

	uint32 size = 0;
	byte *data = loadResourceWithProcessingPause(id, &size);
	if (!data)
		return false;

	uint16 handle = createMovieObject(id, (uint16)size, data);
	_records[index].object = handle;

	return handle != 0;
}

bool TopgunEngine_v1_2::destroyMovieResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyMovieResourceObject"))
		return true;

	ResourceRecord &record = _records[index];

	if (record.data && record.refCount == 0) {
		utilsLibrary()->tgFree(record.data);
		record.data = nullptr;

		record.object = destroyMovieObject(record.object);
	}

	return true;
}

void TopgunEngine_v1_2::stopMovieByHandle(uint16 object) {
	Movie_v1_2 *movie = movieFromHandle(object);

	if (object != 0 && movie)
		movieStop(movie);
}

Movie_v1_2 *TopgunEngine_v1_2::movieFromHandle(uint16 handle) {
	if (handle == 0 || handle > _movieObjects.size())
		return nullptr;

	return _movieObjects[handle - 1];
}

// The argument is the size of the timeline and it is only used by the later version
uint16 TopgunEngine_v1_2::createMovieObject(uint16 id, uint16 argument, byte *data) {
	if (_numMovies >= TG_HOST_V1_2_MAX_MOVIES)
		return 0;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "createMovieObject"))
		return 0;

	Movie_v1_2 *movie = new Movie_v1_2();

	uint16 handle = 0;
	for (uint32 i = 0; i < _movieObjects.size(); i++) {
		if (!_movieObjects[i]) {
			_movieObjects[i] = movie;
			handle = (uint16)(i + 1);
			break;
		}
	}

	if (handle == 0) {
		_movieObjects.push_back(movie);
		handle = (uint16)_movieObjects.size();
	}

	movie->handle = handle;
	movie->id = id;
	movie->index = index;

	_movieList[_numMovies] = movie->index;
	_numMovies++;

	_records[movie->index].object = handle;

	movie->data = data;

	if (_version >= kVersionV1_2_2) {
		movie->timelineSize = argument;
	} else {
		movie->timelineSize = READ_LE_UINT16(data);
		movie->frameCount = READ_LE_UINT16(data + TG_MOVIE_V1_2_HEADER_FRAME_COUNT);
		movie->frameTableOffset = READ_LE_UINT32(data + TG_MOVIE_V1_2_HEADER_TABLE_OFFSET);
	}

	movie->bitmap = spriteLib()->spriteDibCreate(1, 1, 1);
	if (!movie->bitmap)
		return destroyMovieObject(handle);

	WRITE_LE_UINT16(movie->bitmap + ((_version >= kVersionV1_2_3) ? TG_HOST_V1_2_3_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID), id);
	_records[movie->index].data = movie->bitmap;

	if (!movieLoadFrameTable(movie))
		return destroyMovieObject(handle);

	if (!movieAllocPrefetchBuffers(movie))
		return destroyMovieObject(handle);

	movieResetPlaybackState(movie);
	return handle;
}

uint16 TopgunEngine_v1_2::destroyMovieObject(uint16 handle) {
	Movie_v1_2 *movie = movieFromHandle(handle);
	if (!movie)
		return 0;

	movieStop(movie);
	movieFreePrefetchBuffers(movie);

	if (movie->frameTableBlock) {
		utilsLibrary()->tgFree(movie->frameTableBlock);
		movie->frameTableBlock = nullptr;
		movie->frameTable = nullptr;
	} else if (movie->frameTable) {
		movie->frameTable = utilsLibrary()->tgFree(movie->frameTable);
	}

	if (movie->audioBuffer)
		movie->audioBuffer = utilsLibrary()->tgFree(movie->audioBuffer);

	uint16 position = 0;
	while (_numMovies > position) {
		if (movie->index == _movieList[position]) {
			for (uint16 i = position; i + 1 < _numMovies; i++)
				_movieList[i] = _movieList[i + 1];

			_numMovies--;
		} else {
			position++;
		}
	}

	_movieObjects[handle - 1] = nullptr;
	delete movie;

	return 0;
}

void TopgunEngine_v1_2::destroyAllMovieObjects() {
	for (uint32 i = 0; i < _movieObjects.size(); i++) {
		if (!_movieObjects[i])
			continue;

		if (_movieObjects[i]->bitmap)
			_movieObjects[i]->bitmap = utilsLibrary()->tgFree(_movieObjects[i]->bitmap);

		destroyMovieObject((uint16)(i + 1));
	}
}

bool TopgunEngine_v1_2::movieLoadFrameTable(Movie_v1_2 *movie) {
	if (_version >= kVersionV1_2_2) {
		uint32 size = 0;

		movie->frameTable = resourceLib()->resLoadResource(_file, (uint16)(movie->index + ((_version >= kVersionV1_2_3) ? TG_MOVIE_V1_2_3_FRAME_TABLE_RESOURCE_OFFSET : TG_MOVIE_V1_2_2_FRAME_TABLE_RESOURCE_OFFSET)), &size);
		if (!movie->frameTable)
			return false;

		if (_version >= kVersionV1_2_6) {
			movie->frameTableBlock = movie->frameTable;
			movie->frameTable = movie->frameTableBlock + 2;
			movie->frameCount = (uint16)((size - 2) / TG_MOVIE_V1_2_FRAME_ENTRY_SIZE);
		} else {
			movie->frameCount = (uint16)(size / TG_MOVIE_V1_2_FRAME_ENTRY_SIZE);
		}

		if (_version >= kVersionV1_2_3) {
			if (static_cast<ResourceLibrary_v1_2_3 *>(resourceLib())->resSeekResource(_file, (uint16)(movie->index + TG_MOVIE_V1_2_3_STREAM_RESOURCE_OFFSET)) == 0)
				return false;

			movie->streamBase = static_cast<ResourceLibrary_v1_2_3 *>(resourceLib())->resTell(_file);
		}

		return true;
	}

	if (movie->frameCount > TG_MOVIE_V1_2_MAX_FRAME_TABLE_ENTRIES) {
		warning("TopgunEngine_v1_2::movieLoadFrameTable(): the table of %d frames does not fit in 64 KB", movie->frameCount);
		return false;
	}

	uint32 size = (uint32)movie->frameCount * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

	movie->frameTable = utilsLibrary()->tgMalloc(size, 0);
	if (!movie->frameTable)
		return false;

	return resourceLib()->resReadData(_file, movie->frameTable, size, movie->frameTableOffset) == size;
}

bool TopgunEngine_v1_2::movieAllocPrefetchBuffers(Movie_v1_2 *movie) {
	movie->fillBuffer = 0;
	movie->numBuffers = TG_MOVIE_V1_2_NUM_PREFETCH_BUFFERS;

	uint16 current = 0;

	while (current < TG_MOVIE_V1_2_NUM_PREFETCH_BUFFERS) {
		if (movie->prefetchDone)
			break;

		movie->buffers[current].data = utilsLibrary()->tgMalloc(TG_MOVIE_V1_2_PREFETCH_BUFFER_SIZE, 0);
		if (!movie->buffers[current].data)
			return false;

		if (!moviePrefetchGroup(movie))
			return false;

		current++;
	}

	movie->numBuffers = current;
	return true;
}

void TopgunEngine_v1_2::movieFreePrefetchBuffers(Movie_v1_2 *movie) {
	for (uint16 i = 0; i < TG_MOVIE_V1_2_NUM_PREFETCH_BUFFERS; i++) {
		if (movie->buffers[i].data)
			utilsLibrary()->tgFree(movie->buffers[i].data);

		movie->buffers[i] = PrefetchBuffer_v1_2();
	}
}

void TopgunEngine_v1_2::movieResetPlaybackState(Movie_v1_2 *movie) {
	movie->consumeBuffer = 0;
	movie->frame = 0;
	movie->timeline = (_version >= kVersionV1_2_2) ? movie->data : movie->data + TG_MOVIE_V1_2_HEADER_SIZE;
	movie->timelineRemaining = movie->timelineSize;
	movie->audioBuffered = 0;
	movie->started = false;
	movie->late = false;
	movie->skipping = false;
	movie->prefetchExhausted = false;
}

bool TopgunEngine_v1_2::movieRestart(Movie_v1_2 *movie) {
	movieResetPlaybackState(movie);

	movie->prefetchFrame = 0;

	for (uint16 i = 0; i < movie->frameCount; i++)
		movie->frameTable[i * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE + TG_MOVIE_V1_2_FRAME_ENTRY_STATE] = 0;

	movie->fillBuffer = 0;
	movie->prefetchDone = false;

	for (uint16 i = 0; i < movie->numBuffers; i++) {
		if (movie->prefetchDone)
			break;

		movie->buffers[i].ready = false;

		if (!moviePrefetchGroup(movie))
			return false;
	}

	return true;
}

uint16 TopgunEngine_v1_2::movieStop(Movie_v1_2 *movie) {
	if (movie->audioStream != 0)
		audioLib()->audioDestroyWave(movie->audioStream);

	movie->prefetchDone = true;
	movie->audioStream = 0;
	movie->audioBuffered = 0;

	return 0;
}

uint16 TopgunEngine_v1_2::movieCountContiguousChunks(Movie_v1_2 *movie) {
	byte *entry = movie->frameTable + movie->prefetchFrame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;
	uint16 room = (uint16)(TG_MOVIE_V1_2_PREFETCH_BUFFER_SIZE - movie->buffers[movie->fillBuffer].filled);

	if (READ_LE_UINT16(entry + TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE) > room)
		return 0;

	uint16 first = movie->prefetchFrame;

	movie->runOffset = READ_LE_UINT32(entry + TG_MOVIE_V1_2_FRAME_ENTRY_FILE_OFFSET);
	movie->runSize = READ_LE_UINT16(entry + TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE);

	for (;;) {
		movie->prefetchFrame++;
		if (movie->frameCount <= movie->prefetchFrame)
			break;

		entry += TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;
		if (entry[TG_MOVIE_V1_2_FRAME_ENTRY_STATE] != 0)
			break;

		uint32 total = (uint32)movie->runSize + READ_LE_UINT16(entry + TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE);
		if (total > room)
			break;

		if ((uint32)(movie->runSize + movie->runOffset) != READ_LE_UINT32(entry + TG_MOVIE_V1_2_FRAME_ENTRY_FILE_OFFSET))
			break;

		movie->runSize = (uint16)total;
	}

	return (uint16)(movie->prefetchFrame - first);
}

uint16 TopgunEngine_v1_2::movieReadNextBlock(Movie_v1_2 *movie) {
	spriteLib()->spriteSetSendMsg(0);
	bool previousCallback = resourceLib()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);

	PrefetchBuffer_v1_2 &buffer = movie->buffers[movie->fillBuffer];
	uint32 streamBase = (_version >= kVersionV1_2_3) ? movie->streamBase : 0;
	uint32 read = resourceLib()->resReadData(_file, buffer.data + buffer.filled, movie->runSize, streamBase + movie->runOffset);
	buffer.filled = (uint16)(buffer.filled + movie->runSize);

	resourceLib()->resSetCallback(_file, previousCallback, TG_HOST_PROGRESS_INTERVAL);
	spriteLib()->spriteSetSendMsg(1);

	return (uint16)read;
}

bool TopgunEngine_v1_2::moviePrefetchGroup(Movie_v1_2 *movie) {
	PrefetchBuffer_v1_2 &buffer = movie->buffers[movie->fillBuffer];

	if (buffer.ready)
		return false;

	buffer.cursor = 0;
	buffer.count = buffer.cursor;
	buffer.filled = buffer.count;

	while (movie->frameCount > movie->prefetchFrame) {
		byte *entry = movie->frameTable + movie->prefetchFrame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

		if (entry[TG_MOVIE_V1_2_FRAME_ENTRY_STATE] != 0) {
			movie->prefetchFrame++;
		} else {
			uint16 chunks = movieCountContiguousChunks(movie);
			if (chunks == 0)
				break;

			if (movieReadNextBlock(movie) == 0)
				return false;

			buffer.count = (uint16)(buffer.count + chunks);
		}
	}

	buffer.ready = (buffer.count != 0);

	movie->fillBuffer++;
	if (movie->numBuffers <= movie->fillBuffer)
		movie->fillBuffer = 0;

	if (movie->frameCount <= movie->prefetchFrame) {
		if (_version >= kVersionV1_2_6 && movie->frameTableBlock[0] != 0)
			movie->prefetchFrame = 0;
		else
			movie->prefetchDone = true;
	}

	return true;
}

void TopgunEngine_v1_2::serviceMovies() {
	for (uint16 i = 0; i < _numMovies; i++) {
		uint16 index = _movieList[i];
		if (!isRecordIndexValid(index, "serviceMovies"))
			continue;

		Movie_v1_2 *movie = movieFromHandle(_records[index].object);

		if (movie && !movie->prefetchDone && !movie->buffers[movie->fillBuffer].ready)
			moviePrefetchGroup(movie);
	}
}

void TopgunEngine_v1_2::movieMarkFramesSkippable(Movie_v1_2 *movie, uint16 count, uint16 step) {
	uint16 counter = step;
	uint16 frame = movie->prefetchFrame;

	if (movie->frameCount > frame) {
		byte *entry = movie->frameTable + frame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

		while (count != 0) {
			if (entry[TG_MOVIE_V1_2_FRAME_ENTRY_STATE] == 0 && entry[0] == kMovieChunkDeltaFrame) {
				counter--;

				if (counter == 0) {
					counter = step;
					entry[TG_MOVIE_V1_2_FRAME_ENTRY_STATE] = 1;
					count--;
				}
			}

			entry += TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;
			frame++;

			if (movie->frameCount <= frame)
				break;
		}
	}

	if (movie->frameCount <= frame)
		movie->prefetchExhausted = true;
}

bool TopgunEngine_v1_2::movieCheckStartLeadIn(Movie_v1_2 *movie, uint32 now) {
	if (movie->deadline + TG_MOVIE_V1_2_LATE_THRESHOLD > now)
		return false;

	if (!movie->prefetchExhausted)
		movieMarkFramesSkippable(movie, TG_MOVIE_V1_2_SKIPPED_FRAMES, TG_MOVIE_V1_2_SKIP_STEP);

	movie->late = true;
	return true;
}

void TopgunEngine_v1_2::movieComputeFrameRect(const byte *bitmap, Common::Rect &rect) {
	int16 width = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 hotX = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET);
	int16 hotY = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET);

	rect.left = (int16)((1 - width) / 2 + hotX);
	rect.right = (int16)(width + rect.left);
	rect.top = (int16)(hotY - (height - 1) / 2);
	rect.bottom = (int16)(height + rect.top);
}

void TopgunEngine_v1_2::movieBlitDeltaRect(byte *movieBitmap, byte *frame) {
	Common::Rect movieRect;
	Common::Rect frameRect;

	movieComputeFrameRect(movieBitmap, movieRect);
	movieComputeFrameRect(frame, frameRect);

	int16 left = MAX(movieRect.left, frameRect.left);
	int16 top = MAX(movieRect.top, frameRect.top);
	int16 right = MIN(movieRect.right, frameRect.right);
	int16 bottom = MIN(movieRect.bottom, frameRect.bottom);

	if (left >= right || top >= bottom)
		return;

	spriteLib()->spriteDibBlit(frame, movieBitmap, (uint16)(right - left), (uint16)(bottom - top),
		(int16)(left - frameRect.left), (int16)(top - frameRect.top),
		(int16)(left - movieRect.left), (int16)(top - movieRect.top), TG_MOVIE_V1_2_BITMAP_BLIT_DIFF);
}

bool TopgunEngine_v1_2::movieDecodeBitmapFrame(Movie_v1_2 *movie, uint16 *advance) {
	if (movie->frame >= movie->frameCount) {
		warning("TopgunEngine_v1_2::movieDecodeBitmapFrame(): the frame %d is past the end of the frame table", movie->frame);
		return false;
	}

	byte *entry = movie->frameTable + movie->frame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

	if (entry[0] == kMovieChunkAudio)
		return movieCopyAudioChunk(movie);

	if (entry[TG_MOVIE_V1_2_FRAME_ENTRY_STATE] == 0) {
		PrefetchBuffer_v1_2 &buffer = movie->buffers[movie->consumeBuffer];

		if (!buffer.ready) {
			movie->stalled = true;
			return true;
		}

		bool decode;

		if (movie->skipping)
			decode = false;
		else
			decode = !(movie->late && entry[0] != kMovieChunkKeyFrame);

		if (decode) {
			// The pixel pointer of the compressed record is not stored: the pixels are at the start of the record plus its header size
			byte *bitmap = resourceLib()->resExpandBitmap(_file, buffer.data + buffer.cursor);
			if (!bitmap)
				return false;

			WRITE_LE_UINT16(bitmap + ((_version >= kVersionV1_2_3) ? TG_HOST_V1_2_3_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID), movie->id);

			bool merge;
			bool delta;

			// The later version keeps the kind in another byte and tests it as flags
			if (_version >= kVersionV1_2_5) {
				merge = ((bitmap[TG_MOVIE_V1_2_5_BITMAP_KIND] & TG_MOVIE_V1_2_BITMAP_KIND_MERGE) != 0);
				delta = ((bitmap[TG_MOVIE_V1_2_5_BITMAP_KIND] & TG_MOVIE_V1_2_BITMAP_KIND_DELTA) != 0);
			} else {
				merge = (bitmap[TG_MOVIE_V1_2_BITMAP_KIND] == TG_MOVIE_V1_2_BITMAP_KIND_MERGE);
				delta = (bitmap[TG_MOVIE_V1_2_BITMAP_KIND] == TG_MOVIE_V1_2_BITMAP_KIND_DELTA);
			}

			if (merge) {
				spriteLib()->spriteDibBlit(movie->bitmap, bitmap, READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET),
					READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET), 0, 0, 0, 0, TG_MOVIE_V1_2_BITMAP_BLIT_MERGE);
				utilsLibrary()->tgFree(bitmap);
			} else {
				if (delta)
					movieBlitDeltaRect(movie->bitmap, bitmap);

				utilsLibrary()->tgFree(movie->bitmap);
				movie->bitmap = bitmap;
				_records[movie->index].data = bitmap;
			}

			movie->frameUpdated = true;
			movie->late = false;
		}

		buffer.cursor = (uint16)(buffer.cursor + READ_LE_UINT16(entry + TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE));
		buffer.count--;

		if (buffer.count == 0) {
			buffer.ready = false;

			movie->consumeBuffer++;
			if (movie->numBuffers <= movie->consumeBuffer)
				movie->consumeBuffer = 0;
		}
	}

	movie->delay = READ_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_DELAY);

	uint16 counter = (uint16)(READ_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_COUNTER) - 1);
	WRITE_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_COUNTER, counter);

	if (counter == 0)
		*advance = TG_MOVIE_V1_2_TIMELINE_BITMAP_SIZE;

	movie->frame++;
	return true;
}

bool TopgunEngine_v1_2::movieCopyAudioChunk(Movie_v1_2 *movie) {
	PrefetchBuffer_v1_2 &buffer = movie->buffers[movie->consumeBuffer];

	if (!buffer.ready) {
		movie->stalled = true;
		return true;
	}

	if (movie->frame >= movie->frameCount) {
		warning("TopgunEngine_v1_2::movieCopyAudioChunk(): the frame %d is past the end of the frame table", movie->frame);
		return false;
	}

	byte *entry = movie->frameTable + movie->frame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;
	uint16 size = READ_LE_UINT16(entry + TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE);

	if (_audioPresent != 0) {
		if (!movie->audioBuffer) {
			movie->audioBuffer = utilsLibrary()->tgMalloc(TG_MOVIE_V1_2_AUDIO_BUFFER_SIZE, 0);
			if (!movie->audioBuffer)
				return false;
		}

		if (movie->audioBuffered + size > TG_MOVIE_V1_2_AUDIO_BUFFER_SIZE)
			return false;

		utilsLibrary()->copyHugeBytes(movie->audioBuffer + movie->audioBuffered, buffer.data + buffer.cursor, size);
		movie->audioBuffered += size;
	}

	buffer.cursor = (uint16)(buffer.cursor + size);
	movie->frame++;
	buffer.count--;

	if (buffer.count == 0) {
		buffer.ready = false;

		movie->consumeBuffer++;
		if (movie->numBuffers <= movie->consumeBuffer)
			movie->consumeBuffer = 0;
	}

	return true;
}

bool TopgunEngine_v1_2::movieStartAudioStream(Movie_v1_2 *movie) {
	if (movie->frame >= movie->frameCount) {
		warning("TopgunEngine_v1_2::movieStartAudioStream(): the frame %d is past the end of the frame table", movie->frame);
		return false;
	}

	const byte *entry = movie->frameTable + movie->frame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

	if (entry[0] == kMovieChunkAudio)
		movieCopyAudioChunk(movie);

	if (_audioPresent == 0)
		return true;

	SoundCreateInfo_v1_2 info;
	memcpy(info.header, movie->timeline + TG_MOVIE_V1_2_TIMELINE_SCRIPT_PREFIX, (_version >= kVersionV1_2_3) ? TG_AUDIO_V1_2_3_CREATE_HEADER_SIZE : TG_AUDIO_V1_2_CREATE_HEADER_SIZE);
	info.readProc = (_version >= kVersionV1_2_3) ? TG_AUDIO_V1_2_3_PROC_MOVIE_AUDIO : TG_AUDIO_V1_2_PROC_MOVIE_AUDIO;
	info.handle = movie->handle;

	movie->audioStream = audioLib()->audioCreateWave(&info);

	if (_version >= kVersionV1_2_3) {
		if (movie->audioStream == 0)
			return false;

		return audioLib()->audioPlayWave(movie->audioStream, nullptr) != 0;
	}

	if (movie->audioStream != 0)
		audioLib()->audioPlayWave(movie->audioStream, nullptr);

	return true;
}

bool TopgunEngine_v1_2::movieRunScriptEvent(Movie_v1_2 *movie, uint16 *advance) {
	executeScriptOp(movie->timeline + TG_MOVIE_V1_2_TIMELINE_SCRIPT_PREFIX, advance);

	return _records[movie->index].object == movie->handle;
}

void TopgunEngine_v1_2::movieAdvanceTimelinePtr(Movie_v1_2 *movie, uint16 advance) {
	movie->timeline += advance;

	if (READ_LE_UINT16(movie->timeline) == kMovieTimelineBitmap)
		WRITE_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_COUNTER, READ_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_REPEAT));
}

bool TopgunEngine_v1_2::movieStepFrame(Movie_v1_2 *movie) {
	uint16 advance = 0;
	bool ok = true;

	movie->delay = 0;

	switch (READ_LE_UINT16(movie->timeline)) {
	case kMovieTimelineBitmap:
		ok = movieDecodeBitmapFrame(movie, &advance);
		break;

	case kMovieTimelineWait:
		movie->delay = READ_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_TIMELINE_REPEAT);
		advance = TG_MOVIE_V1_2_TIMELINE_WAIT_SIZE;
		break;

	case kMovieTimelineAudio:
		ok = movieStartAudioStream(movie);
		movie->audioStarted = true;
		movie->deadline = getEngineMillis();
		advance = (_version >= kVersionV1_2_3) ? TG_MOVIE_V1_2_3_TIMELINE_AUDIO_SIZE : TG_MOVIE_V1_2_TIMELINE_AUDIO_SIZE;
		break;

	case kMovieTimelineSkip:
		if (_version >= kVersionV1_2_3) {
			// The same code of the timeline holds the audio state in the later version
			movie->audioStarted = (READ_LE_UINT16(movie->timeline + TG_MOVIE_V1_2_3_TIMELINE_AUDIO_STATE) != 0);
			movie->deadline = getEngineMillis();
			advance = TG_MOVIE_V1_2_3_TIMELINE_AUDIO_STATE_SIZE;
		} else {
			advance = TG_MOVIE_V1_2_TIMELINE_SKIP_SIZE;
		}

		break;

	case kMovieTimelineScript:
		if (!movieRunScriptEvent(movie, &advance))
			return false;

		advance += TG_MOVIE_V1_2_TIMELINE_SCRIPT_PREFIX;
		break;

	default:
		ok = false;
		break;
	}

	if (!ok)
		return movieStop(movie) != 0;

	if (advance != 0) {
		movie->timelineRemaining -= advance;

		if (movie->timelineRemaining != 0)
			movieAdvanceTimelinePtr(movie, advance);
	}

	return true;
}

bool TopgunEngine_v1_2::movieBegin(byte *command, uint16 sprite) {
	uint16 index = operandValue(READ_LE_UINT16(command + TG_MOVIE_V1_2_COMMAND_RESOURCE));
	if (!isRecordIndexValid(index, "movieBegin"))
		return false;

	Movie_v1_2 *movie = movieFromHandle(_records[index].object);
	if (!movie)
		return false;

	WRITE_LE_UINT16(command + TG_MOVIE_V1_2_COMMAND_INDEX, movie->index);

	uint16 spriteId = spriteLib()->spriteGetId(sprite);

	if (movie->sprite != 0 && movie->sprite != spriteId)
		return false;

	movie->sprite = spriteId;

	if (movie->started) {
		if (!movieRestart(movie)) {
			movieStop(movie);
			return false;
		}
	}

	movieAdvanceTimelinePtr(movie, 0);

	movie->deadline = getEngineMillis();
	movie->started = true;
	return true;
}

uint16 TopgunEngine_v1_2::spriteMovieService(uint16 sprite, byte *command, byte **frame) {
	if (READ_LE_UINT16(command + TG_MOVIE_V1_2_COMMAND_INDEX) == 0) {
		if (!movieBegin(command, sprite))
			return 0;
	}

	uint16 index = READ_LE_UINT16(command + TG_MOVIE_V1_2_COMMAND_INDEX);
	if (!isRecordIndexValid(index, "spriteMovieService"))
		return 0;

	Movie_v1_2 *movie = movieFromHandle(_records[index].object);
	if (!movie)
		return 0;

	if (_version >= kVersionV1_2_5)
		WRITE_LE_UINT16(command + TG_MOVIE_V1_2_5_COMMAND_FRAME_READY, 0);
	else
		command[TG_MOVIE_V1_2_COMMAND_FRAME_READY] = 0;

	if (_version >= kVersionV1_2_6 && movie->speed == 0)
		return 1;

	uint32 now = getEngineMillis();

	if (movie->deadline > now)
		return 1;

	movie->frameUpdated = false;
	movie->stalled = false;

	if (movie->audioStarted)
		movie->skipping = movieCheckStartLeadIn(movie, now);
	else
		movie->skipping = false;

	for (;;) {
		if (movie->timelineRemaining == 0)
			return movieStop(movie);

		if (!movieStepFrame(movie))
			return 0;

		if (_version >= kVersionV1_2_5)
			WRITE_LE_UINT16(command + TG_MOVIE_V1_2_5_COMMAND_FRAME_READY, movie->frameUpdated ? 1 : 0);
		else
			command[TG_MOVIE_V1_2_COMMAND_FRAME_READY] = movie->frameUpdated ? 1 : 0;

		if (movie->frameUpdated)
			*frame = movie->bitmap;

		if (!movie->audioStarted)
			movie->deadline = now;

		if (_version >= kVersionV1_2_6 && movie->delay != 0 && movie->speed != 0)
			movie->delay = (uint16)((uint32)TG_MOVIE_V1_2_6_DEFAULT_SPEED * movie->delay / movie->speed);

		movie->deadline += movie->delay;

		if (movie->deadline >= now)
			return 1;

		if (movie->stalled)
			return 1;

		if (!movie->skipping)
			return 1;
	}
}

uint32 TopgunEngine_v1_2::movieAudioFill(uint16 movieHandle, byte *destination, uint32 count) {
	Movie_v1_2 *movie = movieFromHandle(movieHandle);
	if (!movie)
		return 0;

	if (movie->frame < movie->frameCount) {
		const byte *entry = movie->frameTable + movie->frame * TG_MOVIE_V1_2_FRAME_ENTRY_SIZE;

		if (entry[0] == kMovieChunkAudio)
			movieCopyAudioChunk(movie);
	}

	if (movie->audioBuffered < count)
		return 0;

	utilsLibrary()->copyHugeBytes(destination, movie->audioBuffer, count);
	movie->audioBuffered -= count;

	if (movie->audioBuffered != 0)
		utilsLibrary()->copyHugeBytes(movie->audioBuffer, movie->audioBuffer + count, movie->audioBuffered);

	return count;
}

} // End of namespace Topgun
