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
#include "common/textconsole.h"

#include "topgun/libraries/resource/resource_v1_2_3.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

uint32 ResourceLibrary_v1_2::decodeSegmentedBitmapStream(LzwState *state, const byte *source, byte *destination, DecodeLimits &limits) {
	uint32 written = 0;

	while (!limits.failed) {
		if (!limits.sourceAvailable(source, 2)) {
			warning("ResourceLibrary_v1_2::decodeSegmentedBitmapStream(): stream overruns its buffer");
			limits.failed = true;
			break;
		}

		uint16 tag = READ_LE_UINT16(source);
		uint16 mode = tag & TG_RES_V1_2_SEGMENT_MODE_MASK;
		uint16 length = tag & TG_RES_V1_2_SEGMENT_LENGTH_MASK;
		if (mode == TG_RES_V1_2_SEGMENT_END)
			break;

		source += 2;

		if (!limits.sourceAvailable(source, length)) {
			warning("ResourceLibrary_v1_2::decodeSegmentedBitmapStream(): segment overruns its buffer");
			limits.failed = true;
			break;
		}

		if (mode == TG_RES_V1_2_SEGMENT_RAW) {
			if (!limits.destinationAvailable(destination + written, length)) {
				warning("ResourceLibrary_v1_2::decodeSegmentedBitmapStream(): segment overruns its destination");
				limits.failed = true;
				break;
			}

			if (length != 0)
				memcpy(destination + written, source, length);

			written += length;
		} else if (mode == TG_RES_V1_2_SEGMENT_RLE) {
			written += decodeRleSegment(destination + written, source, length, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_10_BITS) {
			lzwInitCodeWidth(state, 10);
			written += lzwDecodeSubChunks(state, length, source, destination + written, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_11_BITS) {
			lzwInitCodeWidth(state, 11);
			written += lzwDecodeSubChunks(state, length, source, destination + written, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_12_BITS) {
			lzwInitCodeWidth(state, 12);
			written += lzwDecodeSubChunks(state, length, source, destination + written, limits);
		}

		source += length;
	}

	return written;
}

void ResourceLibrary_v1_2::lzwInitCodeWidth(LzwState *state, uint16 bits) {
	static const uint16 maskTable[4][TG_RES_V1_2_LZW_MASK_COUNT] = {
		{ 0x0000, 0x7F00, 0x3F00, 0x1F00, 0x0F00, 0x0700, 0x0300, 0x0100 },
		{ 0x0000, 0x3F00, 0x0F00, 0x0300, 0x0000, 0x3F00, 0x0F00, 0x0300 },
		{ 0x0000, 0x1F00, 0x0300, 0x7F00, 0x0F00, 0x0100, 0x3F00, 0x0700 },
		{ 0x0000, 0x0F00, 0x0000, 0x0F00, 0x0000, 0x0F00, 0x0000, 0x0F00 }
	};

	if (bits < TG_RES_V1_2_LZW_MIN_CODE_WIDTH || bits > TG_RES_V1_2_LZW_MAX_CODE_WIDTH)
		bits = TG_RES_V1_2_LZW_DEFAULT_CODE_WIDTH;

	for (uint16 i = 0; i < TG_RES_V1_2_LZW_MASK_COUNT; i++)
		state->masks[i] = maskTable[bits - TG_RES_V1_2_LZW_MIN_CODE_WIDTH][i];

	state->codeWidth = bits;
	state->endCode = (1 << bits) - 1;
	state->maxEntry = state->endCode - 1;
	state->leftover = 0;
	state->unusedBits = 0;
	state->firstChar = 0;
	state->currentCode = 0;
	state->previousCode = 0;
	state->nextEntry = 0;
}

uint16 ResourceLibrary_v1_2::lzwReadCode(LzwState *state, const byte *&source, DecodeLimits &limits) {
	if (!limits.sourceAvailable(source, 1)) {
		warning("ResourceLibrary_v1_2::lzwReadCode(): stream overruns its buffer");
		limits.failed = true;
		return state->endCode;
	}

	uint32 accumulator = (((uint32)state->leftover << 8) & state->masks[state->maskIndex]) | *source++;
	state->maskIndex = (state->maskIndex + 1) & (TG_RES_V1_2_LZW_MASK_COUNT - 1);

	uint16 shift = state->unusedBits + 8;
	if (shift < state->codeWidth) {
		if (!limits.sourceAvailable(source, 1)) {
			warning("ResourceLibrary_v1_2::lzwReadCode(): stream overruns its buffer");
			limits.failed = true;
			return state->endCode;
		}

		accumulator = (accumulator << 8) | *source++;
		shift += 8;
	}

	shift -= state->codeWidth;
	state->leftover = (byte)accumulator;
	state->unusedBits = shift;
	return (uint16)(accumulator >> shift);
}

uint16 ResourceLibrary_v1_2::lzwWriteString(LzwState *state, uint16 code, byte *&output, DecodeLimits &limits) {
	byte length = state->length[code];
	if (length == 0) {
		warning("ResourceLibrary_v1_2::lzwWriteString(): dictionary entry %d has no length", code);
		limits.failed = true;
		return 0;
	}

	if (!limits.destinationAvailable(output, (uint32)length + 1)) {
		warning("ResourceLibrary_v1_2::lzwWriteString(): string overruns its destination");
		limits.failed = true;
		return 0;
	}

	// The counter is a signed byte
	byte *position = output + length;
	int16 counter = (int8)length;
	uint16 current = code;

	do {
		*position-- = state->suffix[current];
		current = state->prefix[current];
		counter--;
	} while (counter > 0);

	*position = (byte)current;
	output += (uint32)length + 1;
	return current;
}

uint32 ResourceLibrary_v1_2::lzwDecodeChunk(LzwState *state, const byte *source, byte *destination, DecodeLimits &limits) {
	byte *output = destination;

	memset(state->length, 0, ((uint32)state->tableSize + 3) / 4 * 4);
	state->nextEntry = TG_RES_V1_2_LZW_FIRST_ENTRY;
	state->unusedBits = 0;
	state->maskIndex = 0;
	state->leftover = 0;

	uint16 code = lzwReadCode(state, source, limits);
	if (limits.failed)
		return 0;

	state->previousCode = code;
	state->firstChar = code;

	if (!limits.destinationAvailable(output, 1)) {
		warning("ResourceLibrary_v1_2::lzwDecodeChunk(): stream overruns its destination");
		limits.failed = true;
		return 0;
	}

	*output++ = (byte)code;

	while (true) {
		code = lzwReadCode(state, source, limits);
		if (limits.failed || code == state->endCode)
			break;

		state->currentCode = code;

		uint16 firstChar;
		if ((int16)code < (int16)state->nextEntry) {
			if (code > 0xFF) {
				firstChar = lzwWriteString(state, code, output, limits);
				if (limits.failed)
					break;
			} else {
				if (!limits.destinationAvailable(output, 1)) {
					warning("ResourceLibrary_v1_2::lzwDecodeChunk(): stream overruns its destination");
					limits.failed = true;
					break;
				}

				*output++ = (byte)code;
				firstChar = code;
			}
		} else {
			uint16 previous = state->previousCode;
			if (previous > 0xFF) {
				firstChar = lzwWriteString(state, previous, output, limits);
				if (limits.failed)
					break;
			} else {
				if (!limits.destinationAvailable(output, 1)) {
					warning("ResourceLibrary_v1_2::lzwDecodeChunk(): stream overruns its destination");
					limits.failed = true;
					break;
				}

				*output++ = (byte)previous;
				firstChar = previous;
			}

			if (!limits.destinationAvailable(output, 1)) {
				warning("ResourceLibrary_v1_2::lzwDecodeChunk(): stream overruns its destination");
				limits.failed = true;
				break;
			}

			*output++ = (byte)state->firstChar;
		}

		state->firstChar = firstChar;

		if ((int16)state->nextEntry <= (int16)state->maxEntry) {
			state->suffix[state->nextEntry] = (byte)state->firstChar;
			state->length[state->nextEntry] = (byte)(state->length[state->previousCode] + 1);
			state->prefix[state->nextEntry] = state->previousCode;
			state->nextEntry++;
		}

		state->previousCode = state->currentCode;
	}

	return output - destination;
}

uint32 ResourceLibrary_v1_2::lzwDecodeSubChunks(LzwState *state, int32 length, const byte *source, byte *destination, DecodeLimits &limits) {
	byte *output = destination;

	while (length > 0) {
		if (!limits.sourceAvailable(source, 2)) {
			warning("ResourceLibrary_v1_2::lzwDecodeSubChunks(): stream overruns its buffer");
			limits.failed = true;
			break;
		}

		uint32 chunkLength = READ_LE_UINT16(source);
		source += 2;

		if (!limits.sourceAvailable(source, chunkLength)) {
			warning("ResourceLibrary_v1_2::lzwDecodeSubChunks(): chunk overruns its buffer");
			limits.failed = true;
			break;
		}

		const byte *next = source + chunkLength;
		length -= (int32)(chunkLength + 2);

		output += lzwDecodeChunk(state, source, output, limits);
		if (limits.failed)
			break;

		source = next;
	}

	return output - destination;
}

uint32 ResourceLibrary_v1_2::decodeRleSegment(byte *destination, const byte *source, uint32 length, DecodeLimits &limits) {
	byte *output = destination;
	const byte *input = source;
	const byte *end = source + length;

	do {
		if (!limits.sourceAvailable(input, 1)) {
			warning("ResourceLibrary_v1_2::decodeRleSegment(): stream overruns its buffer");
			limits.failed = true;
			break;
		}

		uint32 command = *input++;
		uint32 count;
		bool fill;

		if (command == 0) {
			if (!limits.sourceAvailable(input, 2)) {
				warning("ResourceLibrary_v1_2::decodeRleSegment(): stream overruns its buffer");
				limits.failed = true;
				break;
			}

			uint16 word = READ_LE_UINT16(input);
			input += 2;

			if ((int16)word < 0) {
				count = (uint16)(0 - word);
				fill = false;
			} else {
				count = word;
				fill = true;
			}
		} else if (command <= TG_RES_MAX_RUN_LENGTH) {
			count = command;
			fill = true;
		} else {
			count = command - TG_RES_MAX_RUN_LENGTH;
			fill = false;
		}

		if (!limits.destinationAvailable(output, count)) {
			warning("ResourceLibrary_v1_2::decodeRleSegment(): stream overruns its destination");
			limits.failed = true;
			break;
		}

		if (fill) {
			if (!limits.sourceAvailable(input, 1)) {
				warning("ResourceLibrary_v1_2::decodeRleSegment(): stream overruns its buffer");
				limits.failed = true;
				break;
			}

			memset(output, *input, count);
			input++;
		} else {
			if (!limits.sourceAvailable(input, count)) {
				warning("ResourceLibrary_v1_2::decodeRleSegment(): stream overruns its buffer");
				limits.failed = true;
				break;
			}

			memcpy(output, input, count);
			input += count;
		}

		output += count;
	} while (input < end);

	return output - destination;
}

void ResourceLibrary_v1_2::rleExpandBitmap(byte *destination, uint32 destinationSize, const byte *source, uint32 sourceSize) {
	uint32 headerLength = READ_LE_UINT16(source);
	uint32 padding = (uint16)(READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_PITCH_OFFSET) - READ_LE_UINT16(source + TG_RES_V1_2_2_1_BITMAP_WIDTH_OFFSET));

	// The line offset table is skipped
	uint32 tableSize = (uint16)(READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_HEIGHT_OFFSET) << 1);

	const byte *sourceEnd = (sourceSize == TG_RES_V1_2_SIZE_UNKNOWN) ? nullptr : source + sourceSize;
	DecodeLimits limits(sourceEnd, destination + destinationSize);

	if (!limits.sourceAvailable(source, headerLength + tableSize)) {
		warning("ResourceLibrary_v1_2::rleExpandBitmap(): bitmap header overruns its buffer");
		return;
	}

	memcpy(destination, source, headerLength);

	const byte *input = source + headerLength + tableSize;
	byte *output = destination + headerLength;

	while (true) {
		if (!limits.sourceAvailable(input, 1)) {
			warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its buffer");
			return;
		}

		uint32 command = *input++;

		if (command == 0) {
			if (!limits.destinationAvailable(output, padding)) {
				warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its destination");
				return;
			}

			output += padding;

			if (!limits.sourceAvailable(input, 1)) {
				warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its buffer");
				return;
			}

			if (*input == 0)
				return;

			continue;
		}

		if (command <= 0x7F) {
			if (!limits.sourceAvailable(input, 1) || !limits.destinationAvailable(output, command)) {
				warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its buffer");
				return;
			}

			memset(output, *input, command);
			input++;
			output += command;
			continue;
		}

		uint32 count = (command ^ 0xFF) & 0xFF;
		if (count == 0) {
			if (!limits.sourceAvailable(input, 1)) {
				warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its buffer");
				return;
			}

			count = *input++;
		}

		if (!limits.sourceAvailable(input, count) || !limits.destinationAvailable(output, count)) {
			warning("ResourceLibrary_v1_2::rleExpandBitmap(): stream overruns its buffer");
			return;
		}

		memcpy(output, input, count);
		input += count;
		output += count;
	}
}

bool ResourceLibrary_v1_2::decodeSegmentedBitmapStream(LzwState *state, const byte *source, byte *destination, uint32 remaining, const byte *sourceEnd) {
	DecodeLimits limits(sourceEnd, destination + remaining);
	byte *output = destination;

	while (true) {
		if (!limits.sourceAvailable(source, 2)) {
			warning("ResourceLibrary_v1_2::decodeSegmentedBitmapStream(): stream overruns its buffer");
			return false;
		}

		uint16 tag = READ_LE_UINT16(source);
		uint16 mode = tag & TG_RES_V1_2_SEGMENT_MODE_MASK;
		uint16 length = tag & TG_RES_V1_2_SEGMENT_LENGTH_MASK;
		source += 2;

		if (mode == TG_RES_V1_2_SEGMENT_END)
			return remaining == 0;

		if (!limits.sourceAvailable(source, length)) {
			warning("ResourceLibrary_v1_2::decodeSegmentedBitmapStream(): segment overruns its buffer");
			return false;
		}

		uint32 written;

		if (mode == TG_RES_V1_2_SEGMENT_RAW) {
			if (remaining < length)
				return false;

			memcpy(output, source, length);
			written = length;
		} else if (mode == TG_RES_V1_2_SEGMENT_RLE && _engine->_version < kVersionV1_2_5) {
			written = decodeRleSegment(output, source, length, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_10_BITS) {
			lzwInitCodeWidth(state, 10);
			written = lzwDecodeSubChunks(state, length, source, output, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_11_BITS) {
			lzwInitCodeWidth(state, 11);
			written = lzwDecodeSubChunks(state, length, source, output, limits);
		} else if (mode == TG_RES_V1_2_SEGMENT_LZW_12_BITS) {
			lzwInitCodeWidth(state, 12);
			written = lzwDecodeSubChunks(state, length, source, output, limits);
		} else {
			return false;
		}

		if (limits.failed || written == 0)
			return false;

		output += written;
		source += length;
		remaining -= written;
	}
}

} // End of namespace Topgun
