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
	//struct tagWAVEOUTCAPSA pwoc;

	waveapi_sampleRate = sampleRate;
	/*waveOutGetDevCapsA(-1, (LPWAVEOUTCAPSA)&pwoc, 52);
	if (pwoc.dwFormats & 1) {
		waveapi_bytesPerSample = 1;
	}
	if (pwoc.dwFormats & 2) {
		waveapi_bytesPerSample = 2;
	}
	if (waveapi_sampleRate > 40000 && pwoc.dwFormats & 1) {
		waveapi_bytesPerSample = 2;
	}*/
	waveapi_bytesPerSample = 2;
	waveapi_zeroLevel = 128;
	//waveapi_numChannels = pwoc.wChannels;
	waveapi_numChannels = 2;

	if (waveapi_bytesPerSample != 1) {
		waveapi_zeroLevel = 0;
	}
	waveapi_outBuf = (int *)malloc(waveapi_numChannels * waveapi_bytesPerSample * 9216);

	waveapi_waveFormat.nChannels = waveapi_numChannels;
	waveapi_waveFormat.wFormatTag = 1;
	waveapi_waveFormat.nSamplesPerSec = waveapi_sampleRate;
	waveapi_mixBuf = waveapi_outBuf + (waveapi_numChannels * waveapi_bytesPerSample * 8192);
	waveapi_waveFormat.nAvgBytesPerSec = waveapi_bytesPerSample * waveapi_sampleRate * waveapi_numChannels;
	waveapi_waveFormat.nBlockAlign = waveapi_numChannels * waveapi_bytesPerSample;
	waveapi_waveFormat.wBitsPerSample = waveapi_bytesPerSample * 8;

	/*
	if (waveOutOpen((LPHWAVEOUT)&waveHandle, -1, (LPCWAVEFORMATEX)&waveapi_waveFormat.wFormatTag, 0, 0, 0)) {
		warning("iWIN init: waveOutOpen failed\n");
		return -1;
	}*/

	waveapi_waveOutParams.bytesPerSample = waveapi_bytesPerSample * 8;
	waveapi_waveOutParams.numChannels = waveapi_numChannels;
	waveapi_waveOutParams.offsetBeginMixBuf = (waveapi_bytesPerSample * waveapi_numChannels) * 1024;
	waveapi_waveOutParams.sizeSampleKB = 0;
	waveapi_waveOutParams.mixBuf = waveapi_mixBuf;

	// Init the buffer at volume zero
	memset(waveapi_outBuf, waveapi_zeroLevel, (unsigned int)waveapi_numChannels * (unsigned int)waveapi_bytesPerSample * 9216);

	/*
	*waveHeaders = (LPWAVEHDR)malloc(32 * sizeof(LPWAVEHDR));
	for (int l = 0; l < NUM_HEADERS; l++) {
		waveHeaders[l]->lpData = waveapi_outBuf + (waveapi_numChannels * waveapi_bytesPerSample * l * 1024);
		waveHeaders[l]->dwBufferLength = waveapi_bytesPerSample * waveapi_numChannels * 1024;
		waveHeaders[l]->dwFlags = 0;
		waveHeaders[l]->dwLoops = 0;
		if (waveOutPrepareHeader(waveHandle, waveHeaders[l], 32)) {
			debug(5, "iWIN init: waveOutPrepareHeader failed.\n");
			for (l = 0; l < 8; l++) {
				waveOutUnprepareHeader(waveHandle, waveHeaders[l], 32);
				waveHeaders[l] = NULL;
			}
			free(waveHeaders);
			return -1;
		}
	}
	for (int l = 0; l < NUM_HEADERS; l++) {
		if (waveOutWrite(waveHandle, waveHeaders[l], 32)) {
			debug(5, "iWIN init: waveOutWrite failed.\n");
			for (int r = 0; r < NUM_HEADERS; r++) {
				waveOutUnprepareHeader(waveHandle, waveHeaders[r], 32);
				waveHeaders[r] = NULL;
			}
			free(waveHeaders);
			waveapi_disableWrite = 0;
			return -1;
		}
	}*/

	waveapi_disableWrite = 0;
	return 0;
}

// Validated

void DiMUSE_v2::waveapi_write(char *lpData, int *feedSize, int *sampleRate) {
	if (waveapi_disableWrite)
		return;
	/*
	if (waveHeaders == NULL)
		return;
	*/
	if (_vm->_game.id == GID_DIG) {
		waveapi_xorTrigger ^= 1;
		if (!waveapi_xorTrigger)
			return;
	}
	
	*feedSize = 0;
	/*
	LPWAVEHDR headerToUse = waveHeaders[waveapi_writeIndex];
	if ((headerToUse->dwFlags & 1) == (headerToUse->lpData))
		return;
	if (waveOutUnprepareHeader(waveHandle, headerToUse, 32))
		return;
	LPWAVEHDR *ptrToWriteIndex = *(&waveHeaders + waveapi_writeIndex);
	if (!ptrToWriteIndex || !*ptrToWriteIndex)
		return;
	
	ptrToWriteIndex = waveapi_outBuf + ((waveapi_numChannels * waveapi_bytesPerSample * waveapi_writeIndex) << 10);
	*lpData = waveHeaders[waveapi_writeIndex]->lpData; // Is it right?
	waveOutPrepareHeader(waveHeaders, headerToUse, 32);
	waveOutWrite(waveHeaders, headerToUse, 32);*/
	*sampleRate = waveapi_sampleRate;
	*feedSize = 1024;
	waveapi_writeIndex = (waveapi_writeIndex + 1) % 8;
}

// Validated
int DiMUSE_v2::waveapi_free() {
	waveapi_disableWrite = 1;
	for (int l = 0; l < NUM_HEADERS; l++) {
		//waveOutUnprepareHeader(waveHandle, waveHeaders[l], 32);
		//waveHeaders[l] = NULL;
	}
	//free(waveHeaders);
	//waveOutReset(waveHandle);
	//waveOutClose(waveHandle);
	free(waveapi_outBuf);
	waveapi_outBuf = NULL;
	return 0;
}

// Validated
void DiMUSE_v2::waveapi_callback() {
	if (!wvSlicingHalted) {
		tracks_callback();
	}
}

// Validated
void DiMUSE_v2::waveapi_increaseSlice() {
	wvSlicingHalted++;
}

// Validated
int DiMUSE_v2::waveapi_decreaseSlice() {
	int result = wvSlicingHalted;
	if (wvSlicingHalted--)
		result = wvSlicingHalted - 1;
	return result;
}

} // End of namespace Scumm
