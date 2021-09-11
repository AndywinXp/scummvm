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

int DiMUSE_v2::waveapi_moduleInit(int sampleRate, waveOutParams *waveoutParamStruct) {
	waveapi_sampleRate = sampleRate;

	waveapi_bytesPerSample = 2;
	waveapi_numChannels = 2;
	waveapi_zeroLevel = 0;

	// Nine buffers (1024 * 4 bytes each), one will be used for the mixer
	waveapi_outBuf = (uint8 *)malloc(waveapi_numChannels * waveapi_bytesPerSample * 512 * 9);
	waveapi_mixBuf = waveapi_outBuf + (waveapi_numChannels * waveapi_bytesPerSample * 512 * 8); // 9-th buffer

	waveoutParamStruct->bytesPerSample = waveapi_bytesPerSample * 8;
	waveoutParamStruct->numChannels = waveapi_numChannels;
	waveoutParamStruct->mixBufSize = 2048;// (waveapi_bytesPerSample * waveapi_numChannels) << 10; // 4096
	waveoutParamStruct->sizeSampleKB = 0;
	waveoutParamStruct->mixBuf = waveapi_mixBuf;

	// Init the buffer at volume zero
	memset(waveapi_outBuf, waveapi_zeroLevel, waveapi_numChannels * waveapi_bytesPerSample * 512 * 9);

	waveapi_disableWrite = 0;
	return 0;
}

void DiMUSE_v2::waveapi_write(uint8 **audioData, int *feedSize, int *sampleRate) {
	uint8 *curBufferBlock;
	if (waveapi_disableWrite)
		return;

	if (_vm->_game.id == GID_DIG) {
		waveapi_xorTrigger ^= 1;
		if (!waveapi_xorTrigger)
			return;
	}
	
	*feedSize = 0;
	if (_mixer->isReady()) {
		curBufferBlock = &waveapi_outBuf[512 * waveapi_writeIndex * waveapi_bytesPerSample * waveapi_numChannels];

		*audioData = curBufferBlock;

		*sampleRate = waveapi_sampleRate;
		*feedSize = 512;
		waveapi_writeIndex = (waveapi_writeIndex + 1) % 8;

		byte *ptr = (byte *)malloc(iMUSE_feedSize * 4);
		memcpy(ptr, curBufferBlock, iMUSE_feedSize * 4);
		_diMUSEMixer->_stream->queueBuffer(ptr, iMUSE_feedSize * 4, DisposeAfterUse::YES, Audio::FLAG_16BITS | Audio::FLAG_STEREO | Audio::FLAG_LITTLE_ENDIAN);
		//debug(5, "Num of blocks queued: %d", _diMUSEMixer->_stream->numQueuedStreams());
	}
}

int DiMUSE_v2::waveapi_free() {
	waveapi_disableWrite = 1;
	free(waveapi_outBuf);
	return 0;
}

void DiMUSE_v2::waveapi_callback() {
	if (!wvSlicingHalted) {
		tracks_callback();
	}
}

void DiMUSE_v2::waveapi_increaseSlice() {
	wvSlicingHalted++;
}

void DiMUSE_v2::waveapi_decreaseSlice() {
	wvSlicingHalted--;
}

} // End of namespace Scumm
