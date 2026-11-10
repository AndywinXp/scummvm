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
#include "common/util.h"

#include "engines/grim/emi/bmutil.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const uint kHeaderSize = 128;
static const char kSignature[] = "BM  ";
static const uint kSignatureSize = 4;
static const uint kVersion = 70;
static const int kCodecRaw = 0;
static const int kCodecLz = 3;

namespace {

struct Codec3State {
	const byte *source;
	uint sourceSize;
	uint position;
	uint word;
	int bitsLeft;
	bool failed;

	void loadWord() {
		if (position + 2 > sourceSize) {
			failed = true;
			word = 0;

			return;
		}

		word = READ_LE_UINT16(source + position);
		position += 2;
	}

	int readBit() {
		int bit = word & 1;

		word >>= 1;
		bitsLeft--;

		if (bitsLeft == 0) {
			bitsLeft = 16;
			loadWord();
		}

		return bit;
	}

	byte readByte() {
		if (position >= sourceSize) {
			failed = true;

			return 0;
		}

		return source[position++];
	}
};

} // End of anonymous namespace

bool bmutilLoad(LFile *file, BmData *bm) {
	byte header[kHeaderSize];

	if (file->read(header, kHeaderSize) != kHeaderSize)
		return false;

	if (memcmp(header, kSignature, kSignatureSize) != 0)
		return false;

	if (READ_LE_UINT32(header + 4) != kVersion)
		return false;

	bm->codec = READ_LE_INT32(header + 8);
	bm->field132 = READ_LE_INT32(header + 12);
	bm->field204 = READ_LE_INT32(header + 20);
	bm->field208 = READ_LE_INT32(header + 24);
	bm->field212 = READ_LE_INT32(header + 28);
	bm->formatField136 = READ_LE_INT32(header + 32);
	bm->bitsPerPixel = READ_LE_INT32(header + 36);

	for (uint i = 0; i < ARRAYSIZE(bm->formatRest); i++)
		bm->formatRest[i] = READ_LE_INT32(header + 40 + i * 4);

	bm->numImages = 0;
	bm->extraData = nullptr;

	for (uint i = 0; i < kBmMaxImages; i++)
		bm->images[i].data = nullptr;

	uint imageCount = READ_LE_UINT32(header + 16);

	for (uint i = 0; i < imageCount; i++) {
		byte dimensions[8];

		if (bm->numImages >= kBmMaxImages) {
			warning("bmutilLoad(): more than %u images", kBmMaxImages);

			return false;
		}

		if (file->read(dimensions, sizeof(dimensions)) != sizeof(dimensions))
			return false;

		int width = READ_LE_INT32(dimensions);
		int height = READ_LE_INT32(dimensions + 4);
		uint bytesPerPixel = (uint)bm->bitsPerPixel >> 3;

		if (width < 0 || height < 0 || (uint64)width * (uint64)height * bytesPerPixel > 0x7FFFFFFF) {
			warning("bmutilLoad(): invalid image size %d x %d", width, height);

			return false;
		}

		uint size = bytesPerPixel * height * width;
		byte *pixels = (byte *)gpMemoryServices->malloc(size * sizeof(byte));

		if (!pixels)
			return false;

		if (bm->codec == kCodecRaw) {
			uint rowBytes = width * bytesPerPixel;
			byte *row = pixels;

			for (int y = 0; y < height; y++) {
				file->read(row, rowBytes);
				row += rowBytes;
			}
		} else if (bm->codec == kCodecLz) {
			int compressedSize;
			byte sizeBytes[4];

			file->read(sizeBytes, sizeof(sizeBytes));
			compressedSize = READ_LE_INT32(sizeBytes);

			if (compressedSize < 0) {
				warning("bmutilLoad(): invalid compressed size %d", compressedSize);

				return false;
			}

			if ((uint)compressedSize < size) {
				byte *compressed = (byte *)gpMemoryServices->malloc((uint)compressedSize * sizeof(byte));

				if (!compressed)
					return false;

				file->read(compressed, compressedSize);
				bmutilDecompressCodec3(pixels, size, compressed, compressedSize);

				// The disasm frees the decompressed pixels and keeps the compressed ones
				gpMemoryServices->free(pixels);
				pixels = compressed;
			} else {
				warning("bmutilLoad(): codec 3 image stored uncompressed, pixels not read");
			}
		} else {
			warning("bmutilLoad(): unknown codec %d", bm->codec);
		}

		bm->images[bm->numImages].width = width;
		bm->images[bm->numImages].height = height;
		bm->images[bm->numImages].data = pixels;
		bm->numImages++;
	}

	return true;
}

int bmutilGetSize(const BmData *bm) {
	int bits = 0;

	if (!bm)
		return 0;

	for (uint i = 0; i < bm->numImages; i++)
		bits += bm->bitsPerPixel * bm->images[i].height * bm->images[i].width;

	return (bits >> 3) + (int)sizeof(BmData);
}

void bmutilFree(BmData *&bm) {
	if (!bm)
		return;

	for (uint i = 0; i < bm->numImages; i++) {
		if (bm->images[i].data) {
			gpMemoryServices->free(bm->images[i].data);
			bm->images[i].data = nullptr;
		}
	}

	if (bm->extraData) {
		gpMemoryServices->free(bm->extraData);
		bm->extraData = nullptr;
	}

	gpMemoryServices->free(bm);
	bm = nullptr;
}

int bmutilDecompressCodec3(byte *dest, uint destSize, const byte *source, uint sourceSize) {
	Codec3State state;
	uint destPosition = 0;

	state.source = source;
	state.sourceSize = sourceSize;
	state.position = 0;
	state.word = 0;
	state.bitsLeft = 16;
	state.failed = false;
	state.loadWord();

	while (!state.failed) {
		if (state.readBit() == 1) {
			byte value = state.readByte();

			if (state.failed)
				break;

			if (destPosition >= destSize) {
				warning("bmutilDecompressCodec3(): output buffer too small");

				return -1;
			}

			dest[destPosition++] = value;

			continue;
		}

		int copyLength;
		int offset;

		if (state.readBit() == 0) {
			int high = state.readBit();
			int low = state.readBit();

			copyLength = (low | (high << 1)) + 3;
			offset = state.readByte() - 256;
		} else {
			byte first = state.readByte();
			byte second = state.readByte();

			offset = (((second & 0xF0) << 4) | first) - 4096;
			copyLength = (second & 0x0F) + 3;

			if ((second & 0x0F) == 0) {
				copyLength = state.readByte() + 1;

				if (copyLength == 1)
					return 0;
			}
		}

		if (state.failed)
			break;

		if (destPosition < (uint)(-offset)) {
			warning("bmutilDecompressCodec3(): copy from before the start of the output");

			return -1;
		}

		for (int i = 0; i < copyLength; i++) {
			if (destPosition >= destSize) {
				warning("bmutilDecompressCodec3(): output buffer too small");

				return -1;
			}

			dest[destPosition] = dest[destPosition + offset];
			destPosition++;
		}
	}

	warning("bmutilDecompressCodec3(): unexpected end of the compressed data");

	return -1;
}

} // End of namespace Grim
