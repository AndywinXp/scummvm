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

#include "common/scummsys.h"
#include "common/mutex.h"
#include "common/serializer.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "scumm/dimuse.h"
#include "scumm/dimuse_v2/dimuse_v2.h"
#include "scumm/dimuse_v2/dimuse_v2_internalmixer.h"

namespace Scumm {

DiMUSE_InternalMixer::DiMUSE_InternalMixer(Audio::Mixer *mixer) {
	_stream = Audio::makeQueuingAudioStream(22050, true);
	_mixer = mixer;
	memset(mixer_mixBuf, 0, sizeof(mixer_mixBuf));
}

DiMUSE_InternalMixer::~DiMUSE_InternalMixer() {
	free(mixer_amp8Table);
	free(mixer_amp12Table);
	free(mixer_softLTable);
	mixer_amp8Table = NULL;
	mixer_amp12Table = NULL;
	mixer_softLTable = NULL;
}

int DiMUSE_InternalMixer::mixer_initModule(int bytesPerSample, int numChannels, int *mixBuf, int offsetBeginMixBuf, int sizeSampleKB, int mixChannelsNum) {
	int amplitudeValue;
	int waveMixChannelsCount;
	int softLdenominator;
	int softLnumerator;
	int softLcurValue;
	int zeroCenterOffset;

	mixer_outWordSize = bytesPerSample;
	mixer_outChannelCount = numChannels;
	//mixer_mixBuf = mixBuf;
	mixer_mixBufSize = offsetBeginMixBuf;
	mixer_stereoReverseFlag = sizeSampleKB;

	//mixer_amp8Table = (int *)malloc(2176 * sizeof(int));
	//memset(mixer_amp8Table, 0, 2176 * sizeof(int));
	//mixer_amp12Table = (int *)malloc(34816 * sizeof(int));
	//memset(mixer_amp12Table, 0, 34816 * sizeof(int));

	// Equal power volume lookup table, populated from inside-out 
	// (hence the need for softLMID)
	//mixer_softLTable = (int *)malloc(8192 * 2 * sizeof(int)); //8192
	//memset(mixer_softLTable, 0, 8192 * 2 * sizeof(int));
	// mixer_softLMID = &mixer_softLTable[8192];
	int *tableMalloc = (int *)malloc(213504);
	mixer_amp8Table = tableMalloc;
	mixer_amp12Table = tableMalloc + 2176;
	mixer_softLTable = tableMalloc + 36992;
	mixer_softLMID = tableMalloc + 45184;
	

	waveMixChannelsCount = mixChannelsNum;

	if (tableMalloc) {
		zeroCenterOffset = 0;
		for (int i = 0; i < 17; i++) {
			amplitudeValue = -2048 * zeroCenterOffset;
			for (int j = 0; j < 128; j++) {
				mixer_amp8Table[(i * 128) + j] = amplitudeValue / 127;
				amplitudeValue += 16 * zeroCenterOffset;
			}

			zeroCenterOffset += 8;
			if (zeroCenterOffset == 8)
				zeroCenterOffset = 7;
			
		}

		zeroCenterOffset = 0;
		for (int i = 0; i < 17; i++) {
			amplitudeValue = -2048 * zeroCenterOffset;
			for (int j = 0; j < 2048; j++) {
				mixer_amp12Table[(i * 2048) + j] = amplitudeValue / 127;
				amplitudeValue += zeroCenterOffset;
			}
			
			zeroCenterOffset += 8;
			if (zeroCenterOffset == 8)
				zeroCenterOffset = 7;
		}
		if (mixer_outWordSize == 8) {
			if (waveMixChannelsCount * 1024 > 0) {
				softLnumerator = 0;
				softLdenominator = 2047 * waveMixChannelsCount;
				for (int i = 0; i < waveMixChannelsCount * 2048; i++) {
					softLcurValue = softLnumerator / softLdenominator + 1;
					softLdenominator += waveMixChannelsCount - 1;
					softLnumerator += 254 * waveMixChannelsCount;
					softLcurValue /= 2;

					mixer_softLMID[i] = softLcurValue + 128;
					mixer_softLMID[-i] = 127 - softLcurValue;
				}
			}
		} else if (waveMixChannelsCount * 1024 > 0) {
			softLdenominator = 2047 * waveMixChannelsCount;
			softLnumerator = 0;
			for (int i = 0; i < waveMixChannelsCount * 1024; i++) {
				softLcurValue = softLnumerator / softLdenominator + 1;
				softLcurValue /= 2;
				softLdenominator += waveMixChannelsCount - 1;
				softLnumerator += 65534 * waveMixChannelsCount;
				mixer_softLMID[i] = softLcurValue;
				mixer_softLMID[-i - 1] = -1 - softLcurValue;
			}
		}
		_mixer->playStream(Audio::Mixer::kMusicSoundType, &_channelHandle, _stream, -1, 127, false);
		return 0;
	} else {
		debug(5, "DiMUSE_InternalMixer::mixer_initModule(): ERROR: allocating mixer buffers");
		return -1;
	}
	
}

void DiMUSE_InternalMixer::mixer_setRadioChatter() {
	mixer_radioChatter = 1;
}
void DiMUSE_InternalMixer::mixer_clearRadioChatter() {
	mixer_radioChatter = 0;
}

int DiMUSE_InternalMixer::mixer_clearMixBuff() {
	if (!mixer_mixBuf)
		return -1;

	memset(mixer_mixBuf, 0, mixer_mixBufSize * 4);
	//memset((mixer_mixBuf + 4 * mixer_mixBufSize / 4), 1, mixer_mixBufSize & 3);

	return 0;
}

void DiMUSE_InternalMixer::mixer_mix(uint8 *srcBuf, int inFrameCount, int wordSize, int channelCount, int feedSize, int mixBufStartIndex, int volume, int pan) {
	int *ampTable;
	int rightChannelVolume;
	int leftChannelVolume;
	int channelVolume;
	int channelPan;

	if (mixer_mixBuf) {
		if (srcBuf) {
			if (inFrameCount) {
				if (channelCount == 1 && mixer_outChannelCount == 2) {
					channelVolume = volume / 8;
					if (volume)
						++channelVolume;
					if (channelVolume >= 17)
						channelVolume = 16;
					channelPan = (pan / 8) - 8;
					if (pan > 64)
						++channelPan;
					channelPan = channelVolume == 0 ? 0 : channelPan;

					// Linear volume quantization from the lookup table
					rightChannelVolume = mixer_table[17 * channelVolume + channelPan];
					leftChannelVolume = mixer_table[17 * channelVolume - channelPan];
					if (wordSize == 8) {
						mixer_mixBits8ConvertToStereo(
							srcBuf,
							inFrameCount,
							feedSize,
							mixBufStartIndex,
							&mixer_amp8Table[leftChannelVolume * 128],
							&mixer_amp8Table[rightChannelVolume * 128]);
					} else {
						if (wordSize == 12) {
							mixer_mixBits12ConvertToStereo(
								srcBuf,
								inFrameCount,
								feedSize,
								mixBufStartIndex,
								&mixer_amp12Table[leftChannelVolume * 2048],
								&mixer_amp12Table[rightChannelVolume * 2048]);
						} else {
							mixer_mixBits16ConvertToStereo(
								(uint16 *)srcBuf,
								inFrameCount,
								feedSize,
								mixBufStartIndex,
								&mixer_amp12Table[leftChannelVolume * 8192],
								&mixer_amp12Table[rightChannelVolume * 8192]);
						}
					}
				} else {
					channelVolume = volume / 8;
					if (volume)
						channelVolume++;
					if (channelVolume >= 17)
						channelVolume = 16;
					channelVolume = 16;
					if (wordSize == 8)
						ampTable = &mixer_amp8Table[channelVolume * 128];
					else
						ampTable = &mixer_amp12Table[channelVolume * 2048];

					if (mixer_outChannelCount == 1) {
						if (channelCount == 1) {
							if (wordSize == 8) {
								mixer_mixBits8Mono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							} else if (wordSize == 12) {
								mixer_mixBits12Mono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							} else {
								mixer_mixBits16Mono((uint16 *)srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							}
						} else if (wordSize == 8) {
							mixer_mixBits8ConvertToMono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						} else if (wordSize == 12) {
							mixer_mixBits12ConvertToMono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						} else {
							mixer_mixBits16ConvertToMono((uint16 *)srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						}
					} else if (wordSize == 8) {
						mixer_mixBits8Stereo(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					} else if (wordSize == 12) {
						mixer_mixBits12Stereo(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					} else {
						mixer_mixBits16Stereo((uint16 *)srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					}
				}
			}
		}
	}
}

int DiMUSE_InternalMixer::mixer_loop(uint8 *destBuffer, int len) {
	int16 *destBuffer16Bit = (int16 *)destBuffer;
	int16 *mixBuffer = (int16 *)mixer_mixBuf;

	if (!mixer_mixBuf || !destBuffer || !len)
		return -1;

	if (mixer_outChannelCount == 2)
		len *= 2;

	if (!mixer_stereoReverseFlag || mixer_outChannelCount == 1) {
		if (mixer_outWordSize != 16) {
			if (len) {
				for (int i = 0, j = len - 1; j > 0; i++, j--) {
					destBuffer[i] = (uint8)mixer_softLMID[mixBuffer[i]];
				}
			}
			return 0;
		}

		if (len) {
			for (int i = 0, j = len - 1; j > 0; i++, j--) {
				int num = (uint16)mixer_softLMID[2 * mixBuffer[i]];
				destBuffer16Bit[i] = num;
			}
		}
		return 0;
	} else {
		len /= 2;
		if (mixer_outWordSize == 16) {
			if (len) {
				for (int i = 0; i < len; i += 2) {
					destBuffer16Bit[i] = (uint16)mixer_softLMID[2 * mixBuffer[i + 1]];
					destBuffer16Bit[i + 1] = (uint16)mixer_softLMID[2 * mixBuffer[i]];
				}
			}
			return 0;
		}

		if (len) {
			for (int i = 0; i < len; i += 2) {
				destBuffer[i] = (uint8)mixer_softLMID[mixBuffer[i + 1]];
				destBuffer[i + 1] = (uint8)mixer_softLMID[mixBuffer[i]];
			}
		}
		return 0;
	}
}

void DiMUSE_InternalMixer::mixer_mixBits8Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	uint8 *ptr;
	int residualLength;
	int value;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	srcBuf_ptr = srcBuf;
	if (inFrameCount == feedSize) {
		if (mixer_radioChatter) {
			ptr = srcBuf + 4;
			value = srcBuf[0] - 128 + srcBuf[1] - 128 + srcBuf[2] - 128 + srcBuf[3] - 128;
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += 4 * (uint16)ampTable[2 * (srcBuf_ptr[i] - value / 4)];
					value += ptr[i] - srcBuf_ptr[i];
				}
			}
		} else {
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16)ampTable[2 * srcBuf_ptr[i]];
				}
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		if (inFrameCount - 1 != 0) {
			for (int i = 0, j = 0; i < inFrameCount - 1; i++, j += 2) {
				mixBufCurCell[j] += (uint16)ampTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[j + 1] += ((int16)ampTable[2 * srcBuf_ptr[i]] + (int16)ampTable[2 * srcBuf_ptr[i + 1]]) / 2;
			}
		}
		mixBufCurCell[inFrameCount] += (uint16)ampTable[2 * srcBuf_ptr[inFrameCount]];
		mixBufCurCell[inFrameCount + 1] += (uint16)ampTable[2 * srcBuf_ptr[inFrameCount]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 2) {
				mixBufCurCell[i] += (uint16)ampTable[2 * srcBuf_ptr[j]];
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				// Seek the next srcBuf element until there's excess length
				for (residualLength += inFrameCount; residualLength >= 0; ++srcBuf_ptr)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits12Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int value;
	int xorFlag;
	int residualLength;
	int term_1;
	int term_2;

	if ((inFrameCount & 1) != 0) {
		inFrameCount &= 0xFFFFFFFE;
		debug(5, "DiMUSE_InternalMixer::mixer_mixBits12Mono(): WARNING: odd inFrameCount with 12-bit data");
	}

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (inFrameCount / 2) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < inFrameCount / 2; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8)];
				mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if ((inFrameCount / 2) - 1) {
			for (int i = 0; i < (inFrameCount / 2) - 1; i++) {
				value = (uint16)ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];

				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8)];
				mixBufCurCell[1] += ((int16)value + (int16)ampTable[2 * value]) / 2;
				mixBufCurCell[2] += value;
				mixBufCurCell[3] += ((int16)value + (int16)ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF) << 8))]) / 2;
				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
		mixBufCurCell[1] += ((uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
			+ (uint16)ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))]) / 2;
		mixBufCurCell[2] += (uint16)ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
		mixBufCurCell[3] += (uint16)ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += (uint16)ampTable[2 * (srcBuf_ptr[0] | (srcBuf_ptr[1] & 0xF) << 8)];
				srcBuf_ptr += 3;
			}
		}
	} else {
		xorFlag = 0;
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				if (xorFlag) {
					term_1 = (srcBuf_ptr[1] & 0xF0) << 4;
					term_2 = srcBuf_ptr[2];
				} else {
					term_1 = (srcBuf_ptr[1] & 0xF) << 8;
					term_2 = srcBuf_ptr[0];
				}

				mixBufCurCell[i] += (uint16)ampTable[2 * (term_2 | term_1)];
				residualLength += inFrameCount;
				while (residualLength >= 0) {
					residualLength -= feedSize;
					xorFlag ^= 1u;
					if (!xorFlag)
						srcBuf_ptr += 3;
				}
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits16Mono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += (uint16)ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		int i = 0; // TODO: check
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += ((int16)ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096]
					+ (int16)ampTable[(int16)(srcBuf_ptr[i + 1] & 0xFFF7) / 8 + 4096]) / 2;
				mixBufCurCell += 2;
			}
		}
		mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
	} else {
		if (2 * feedSize == inFrameCount) {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
					srcBuf_ptr += 2;
				}
			}
		} else {
			residualLength = -inFrameCount;

			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];

					for (residualLength += inFrameCount; residualLength >= 0; ++srcBuf_ptr)
						residualLength -= feedSize;
				}
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits8ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint8 *srcBuf_ptr;
	int residualLength;
	uint16 *mixBufCurCell;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	srcBuf_ptr = srcBuf;
	if (inFrameCount == feedSize) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 2) {
				mixBufCurCell[i] += (2 * (int16)ampTable[2 * srcBuf_ptr[j]]) / 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		if (inFrameCount != 1) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
				int term_1 = ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[3]]) / 2;
				int term_2 = (int16)ampTable[2 * srcBuf_ptr[0]] + (int16)ampTable[2 * srcBuf_ptr[2]];
				mixBufCurCell[1] += ((term_2 / 2) + term_1) / 2;

				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
		mixBufCurCell[0] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
		mixBufCurCell[1] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 4) {
				mixBufCurCell[i] += (2 * (int16)ampTable[2 * srcBuf_ptr[j]]) / 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[2 * srcBuf_ptr[0]] + (int16)ampTable[2 * srcBuf_ptr[1]]) / 2;
				// Skip srcBuf elements until there's excess length
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits12ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint8 *srcBuf_ptr;
	int residualLength;
	uint16 *mixBufCurCell;
	int term_1;
	int term_2;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				term_1 = srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8);
				term_2 = (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				mixBufCurCell[i] += ((int16)ampTable[2 * term_1] + term_2) / 2;
				srcBuf_ptr += 3;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				term_1 = (int16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF0) << 8))];
				term_2 = (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF) << 4))];

				mixBufCurCell[0] += (term_1 + term_2) / 2;
				mixBufCurCell[1] += ((term_1 + (int16)ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF) << 8))]) / 2)
					+ ((term_2 + (int16)ampTable[2 * (srcBuf_ptr[5] | ((srcBuf_ptr[4] & 0xF0) << 4))]) / 2) / 2;

				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}

		mixBufCurCell[0] += ((int16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
			+ (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;
		mixBufCurCell[1] += ((int16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
			+ (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;


	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {

				mixBufCurCell[i] += ((int16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
					+ (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;

				srcBuf_ptr += 6;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
					+ (int16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;

				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 3)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits16ConvertToMono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]
					+ (int16)ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]) / 2;
				srcBuf_ptr += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += ((int16)ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]
					+ (int16)ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]) / 2;

				mixBufCurCell[1] += ((((int16)ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]
					+ (int16)ampTable[((int16)(srcBuf_ptr[2] & 0xFFF7) / 8) + 4096]) / 2)
					+ (((int16)ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]
						+ (int16)ampTable[((int16)(srcBuf_ptr[3] & 0xFFF7) / 8) + 4096]) / 2)) / 2;
				mixBufCurCell += 2;
				srcBuf_ptr += 2;
			}
		}

		mixBufCurCell[0] += ((int16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]
			+ (int16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]) / 2;
		mixBufCurCell[1] += ((int16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]
			+ (int16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]) / 2;
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
					+ (int16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]) / 2;
				srcBuf_ptr += 4;
			}
		}
	} else {
		residualLength = -inFrameCount;

		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
					+ (int16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]) / 2;

				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits8ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	uint8 *ptr;
	int value;
	int residualLength;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (mixer_radioChatter) {
			srcBuf_ptr = srcBuf;
			ptr = srcBuf + 4;
			value = srcBuf[0] - 128 + srcBuf[1] - 128 + srcBuf[2] - 128 + srcBuf[3] - 128;
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += 4 * (uint16)leftAmpTable[2 * (srcBuf_ptr[i] - (value / 4))];
					mixBufCurCell[1] += 4 * (uint16)rightAmpTable[2 * (srcBuf_ptr[i] - (value / 4))];
					value += ptr[i] - srcBuf_ptr[i];
					mixBufCurCell += 2;
				}
			}
		} else {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[i]];
					mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[i]];
					mixBufCurCell += 2;
				}
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		int i = 0; // TODO: check
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[2] += ((int16)leftAmpTable[2 * srcBuf_ptr[i]] + (int16)leftAmpTable[2 * srcBuf_ptr[i]]) / 2;
				mixBufCurCell[3] += ((int16)rightAmpTable[2 * srcBuf_ptr[i]] + (int16)rightAmpTable[2 * srcBuf_ptr[i]]) / 2;
			}
		}
		mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[2] += (uint16)leftAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[3] += (uint16)rightAmpTable[2 * srcBuf_ptr[i]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[1]];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell += 2;
				srcBuf_ptr += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell += 2;

				for (residualLength += inFrameCount; residualLength > 0; ++srcBuf_ptr)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits12ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;

	int xorFlag;
	int residualLength;

	int term_1;
	int term_2;
	int term_3;
	int term_4;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (inFrameCount / 2) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < (inFrameCount / 2); i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[2] += (uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
				mixBufCurCell[3] += (uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if ((inFrameCount / 2) - 1 != 0) {
			for (int i = 0; i < (inFrameCount / 2) - 1; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];

				mixBufCurCell[2] += ((uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]
					+ (uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]) / 2;

				mixBufCurCell[3] += ((uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]
					+ (uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]) / 2;

				mixBufCurCell[4] += (uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
				mixBufCurCell[5] += (uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				mixBufCurCell[6] += ((uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))] +
					(int16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[4] & 0xF) << 8))]) / 2;

				mixBufCurCell[7] += ((uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))] +
					(int16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]) / 2;

				srcBuf_ptr += 3;
				mixBufCurCell += 8;
			}
		}

		mixBufCurCell[0] += (uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
		mixBufCurCell[1] += (uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];

		mixBufCurCell[2] += ((uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
			+ (uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;

		mixBufCurCell[3] += ((uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
			+ (uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]) / 2;

		mixBufCurCell[4] += (uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
		mixBufCurCell[5] += (uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
		mixBufCurCell[6] += (uint16)leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
		mixBufCurCell[7] += (uint16)rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else {
		xorFlag = 0;
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				if (xorFlag) {
					term_2 = (srcBuf_ptr[1] & 0xF0) << 4;
					term_1 = srcBuf_ptr[2];
				} else {
					term_2 = (srcBuf_ptr[1] & 0xF) << 8;
					term_1 = srcBuf_ptr[0];
				}
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * (term_1 | term_2)];

				if (xorFlag) {
					term_4 = (srcBuf_ptr[1] & 0xF0) << 4;
					term_3 = srcBuf_ptr[2];
				} else {
					term_4 = (srcBuf_ptr[1] & 0xF) << 8;
					term_3 = srcBuf_ptr[0];
				}
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * (term_3 | term_4)];

				residualLength += inFrameCount;
				while (residualLength > 0) {
					residualLength -= feedSize;
					xorFlag ^= 1u;
					if (!xorFlag)
						srcBuf_ptr += 3;
				}

				mixBufCurCell += 2;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits16ConvertToStereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16* mixBufCurCell;
	uint16 *srcBuf_tmp;
	int residualLength;

	mixBufCurCell = (uint16*)&mixer_mixBuf[mixBufStartIndex];

	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_tmp = srcBuf;
		int i = 0; // TODO: check
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];

				mixBufCurCell[2] += ((int16)leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096]
					+ (int16)leftAmpTable[(int16)(srcBuf_tmp[i + 1] & 0xFFF7) / 8 + 4096]) / 2;

				mixBufCurCell[3] += ((int16)rightAmpTable[((int16)(srcBuf_tmp[i] & 0xFFF7) / 8) + 4096]
					+ (int16)rightAmpTable[((int16)(srcBuf_tmp[i + 1] & 0xFFF7) / 8) + 4096]) / 2;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[2] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[3] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				srcBuf_tmp += 2;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)rightAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];

				for (residualLength += inFrameCount; residualLength > 0; ++srcBuf_tmp)
					residualLength -= feedSize;

				mixBufCurCell += 2;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits8Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16*)&mixer_mixBuf[mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
				mixBufCurCell[2] += ((int16)ampTable[2 * srcBuf_ptr[0]] + (int16)ampTable[2 * srcBuf_ptr[2]]) / 2;
				mixBufCurCell[3] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[3]]) / 2;
				mixBufCurCell += 4;
				srcBuf_ptr += 2;
			}
		}
		mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
		mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
		mixBufCurCell[2] += (uint16)ampTable[2 * srcBuf_ptr[0]];
		mixBufCurCell[3] += (uint16)ampTable[2 * srcBuf_ptr[1]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
				srcBuf_ptr += 4;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
				mixBufCurCell += 2;
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits12Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];

				mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				mixBufCurCell[2] += ((uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))]
					+ (int16)ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF) << 8))]) / 2;

				mixBufCurCell[3] += ((uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))]
					+ (int16)ampTable[2 * (srcBuf_ptr[5] | ((srcBuf_ptr[4] & 0xF0) << 4))]) / 2;

				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
		mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
		mixBufCurCell[2] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
		mixBufCurCell[3] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				srcBuf_ptr += 6;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) << 8))];
				mixBufCurCell[1] += (uint16)ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) << 4))];

				mixBufCurCell += 2;
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 3)
					residualLength -= feedSize;
			}
		}
	}
}

void DiMUSE_InternalMixer::mixer_mixBits16Stereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;


	mixBufCurCell = (uint16*)&mixer_mixBuf[mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;

		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];

				mixBufCurCell[2] += ((int16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
					+ (int16)ampTable[(int16)(srcBuf_ptr[2] & 0xFFF7) / 8 + 4096]) / 2;

				mixBufCurCell[4] += ((int16)ampTable[((int16)((*((uint8*)srcBuf_ptr + 2) | (*((uint8 *)srcBuf_ptr + 3) << 8)) & 0xFFF7) / 8) + 4096]
					+ (int16)ampTable[((int16)(srcBuf_ptr[3] & 0xFFF7) / 8) + 4096]) / 2;
				mixBufCurCell += 4;
				srcBuf_ptr += 2;
			}
		}
		mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[2] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[3] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
	} else {
		if (2 * feedSize == inFrameCount) {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
					mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
					mixBufCurCell += 2;
					srcBuf_ptr += 4;
				}
			}
		} else {
			residualLength = -inFrameCount;
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += (uint16)ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
					mixBufCurCell[1] += (uint16)ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];

					for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
						residualLength -= feedSize;
				}
			}
		}
	}
}

} // End of namespace Scumm
