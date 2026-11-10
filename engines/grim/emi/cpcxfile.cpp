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

#include "engines/grim/emi/cpcxfile.h"
#include "engines/grim/emi/lbitmap.h"
#include "engines/grim/emi/lcolorformat.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const uint kHeaderSize = 128;
static const int kMaxPlanes = 3;

CPCXFile::CPCXFile() {
	bitmap = nullptr;
}

bool CPCXFile::readImage(LFile *file, LBitmap *image, int unused) {
	PcxReader reader;
	PcxHeader header;
	bool result = false;

	bitmap = image;
	ioPcxLoadFile(file, &reader);

	if (ioPcxReadHeader(&reader, &header)) {
		int imageWidth = header.xMax - header.xMin + 1;
		int imageHeight = header.yMax - header.yMin + 1;
		int numPlanes = header.numPlanes;

		if (imageWidth <= 0 || imageHeight <= 0 || numPlanes > kMaxPlanes) {
			warning("CPCXFile::readImage(): unsupported image geometry");

			if (gpConsoleServices)
				gpConsoleServices->logError("PCX::ReadImage: FAIL!!!\n");
		} else {
			int rowBytes = (int16)(header.bytesPerLine * header.numPlanes);
			byte *rowBuffer = (byte *)gpMemoryServices->malloc((uint)rowBytes * sizeof(byte));
			int bytesPerPixel = header.numPlanes * header.bitsPerPixel / 8;

			bitmap->create(imageWidth, imageHeight, gColorFormat6, nullptr);

			for (int row = 0; row < imageHeight; row++) {
				if (reader.cursor >= reader.end)
					break;

				byte *pixels = (byte *)bitmap->getPixels();

				if (!pixels || !rowBuffer) {
					warning("CPCXFile::readImage(): missing buffer");

					break;
				}

				int decoded = ioPcxDecodeLine(&reader, rowBuffer, rowBytes);

				for (int plane = 0; plane < numPlanes; plane++) {
					int planeStart = plane * imageWidth;
					int index = planeStart;

					for (int x = 0; x < imageWidth; x++) {
						if (index >= decoded)
							break;

						pixels[plane + numPlanes * (x + row * imageWidth)] = rowBuffer[planeStart + x];
						index++;
					}
				}
			}

			if (reader.overrun && gpConsoleServices)
				gpConsoleServices->logWarning("PCX::ReadImage: EOF during read!\n");

			if (rowBuffer)
				gpMemoryServices->free(rowBuffer);

			result = true;

			if (bytesPerPixel == 1)
				bitmap->getColorFormat().setFormat(2);
			else
				bitmap->getColorFormat().setFormat(6);
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("PCX::ReadImage: FAIL!!!\n");
	}

	if (reader.base) {
		gpMemoryServices->free(reader.base);
		reader.base = nullptr;
	}

	bitmap = nullptr;

	return result;
}

int CPCXFile::readPalette(LFile *file, int *palette) {
	byte marker = 0;

	file->read(&marker, 1);

	if (marker != 0xC0) {
		if (gpConsoleServices)
			gpConsoleServices->logError("PCX::ReadPal: PCX Pal header not valid\n");

		return 0;
	}

	for (int i = 0; i < 256; i++) {
		byte red = 0;
		byte green = 0;
		byte blue = 0;

		file->read(&red, 1);
		file->read(&green, 1);
		file->read(&blue, 1);

		// The disasm ORs the three components together
		palette[i] = red | green | blue;

		if (i != 255 && file->eof()) {
			if (gpConsoleServices)
				gpConsoleServices->logError("PCX::ReadPal: Only got %d rgb triplets on the read\n", i);

			return 0;
		}
	}

	return 1;
}

int CPCXFile::writeImage(LFile *file, LBitmap *image, const int *palette) {
	PcxHeader header;

	bitmap = image;

	header.id = 10;
	header.version = 5;
	header.encoding = 1;
	header.bitsPerPixel = 8;
	header.xMin = 0;
	header.yMin = 0;
	header.xMax = bitmap->getWidth() - 1;
	header.yMax = bitmap->getHeight() - 1;
	header.hDpi = bitmap->getWidth() - 1;
	header.vDpi = bitmap->getHeight() - 1;
	memset(header.palette16, 0, sizeof(header.palette16));
	header.reserved64 = 0;
	header.numPlanes = 1;
	header.bytesPerLine = bitmap->getColorFormat().bytesPerPixel * bitmap->getWidth();
	header.paletteType = 2;
	header.screenWidth = bitmap->getWidth();
	header.screenHeight = bitmap->getHeight();
	memset(header.filler, 0, sizeof(header.filler));

	ioPcxWriteHeader(file, &header);

	byte *lineBuffer = (byte *)gpMemoryServices->malloc((uint)(2 * bitmap->getWidth()) * sizeof(byte));

	if (lineBuffer) {
		const byte *pixels = (const byte *)bitmap->getPixels();

		if (pixels) {
			for (int row = 0; row < bitmap->getHeight(); row++) {
				// The disasm advances by the width and not by the stride
				const byte *line = pixels + row * bitmap->getWidth();
				int encoded = ioPcxEncodeLine(line, lineBuffer, bitmap->getWidth());

				file->write(lineBuffer, encoded);
			}
		} else {
			warning("CPCXFile::writeImage(): bitmap has no pixels");
		}

		gpMemoryServices->free(lineBuffer);
	}

	if (palette) {
		byte marker = 12;

		file->write(&marker, 1);

		for (int i = 0; i < 256; i++) {
			byte component[3];

			component[0] = palette[i] & 0xFF;
			component[1] = (palette[i] >> 8) & 0xFF;
			component[2] = (palette[i] >> 16) & 0xFF;
			file->write(component, 3);
		}
	}

	bitmap = nullptr;

	return 1;
}

bool ioPcxReadHeader(PcxReader *reader, PcxHeader *header) {
	if (reader->end - reader->cursor < (int)kHeaderSize) {
		warning("ioPcxReadHeader(): file too short");

		return false;
	}

	byte *data = reader->cursor;

	header->id = data[0];
	reader->cursor += 1;

	if (header->id != 10) {
		if (gpConsoleServices)
			gpConsoleServices->logError("PCX::ReadHeader: Header ID number not valid\n");

		return false;
	}

	header->version = data[1];
	reader->cursor += 1;

	if (header->version != 0 && header->version != 2 && header->version != 3 && header->version != 4 && header->version != 5) {
		if (gpConsoleServices)
			gpConsoleServices->logError("PCX::ReadHeader: <%d> is not a valid pcx version number.\n", header->version);

		return false;
	}

	header->encoding = data[2];
	header->bitsPerPixel = data[3];
	header->xMin = READ_LE_UINT16(data + 4);
	header->yMin = READ_LE_UINT16(data + 6);
	header->xMax = READ_LE_UINT16(data + 8);
	header->yMax = READ_LE_UINT16(data + 10);
	header->hDpi = READ_LE_UINT16(data + 12);
	header->vDpi = READ_LE_UINT16(data + 14);
	memcpy(header->palette16, data + 16, sizeof(header->palette16));
	header->reserved64 = data[64];
	header->numPlanes = data[65];
	header->bytesPerLine = READ_LE_UINT16(data + 66);
	header->paletteType = READ_LE_UINT16(data + 68);
	header->screenWidth = READ_LE_UINT16(data + 70);
	header->screenHeight = READ_LE_UINT16(data + 72);
	memcpy(header->filler, data + 74, sizeof(header->filler));
	reader->cursor = data + kHeaderSize;

	return true;
}

int ioPcxWriteHeader(LFile *file, const PcxHeader *header) {
	byte data[kHeaderSize];

	data[0] = header->id;
	data[1] = header->version;
	data[2] = header->encoding;
	data[3] = header->bitsPerPixel;
	WRITE_LE_UINT16(data + 4, header->xMin);
	WRITE_LE_UINT16(data + 6, header->yMin);
	WRITE_LE_UINT16(data + 8, header->xMax);
	WRITE_LE_UINT16(data + 10, header->yMax);
	WRITE_LE_UINT16(data + 12, header->hDpi);
	WRITE_LE_UINT16(data + 14, header->vDpi);
	memcpy(data + 16, header->palette16, sizeof(header->palette16));
	data[64] = header->reserved64;
	data[65] = header->numPlanes;
	WRITE_LE_UINT16(data + 66, header->bytesPerLine);
	WRITE_LE_UINT16(data + 68, header->paletteType);
	WRITE_LE_UINT16(data + 70, header->screenWidth);
	WRITE_LE_UINT16(data + 72, header->screenHeight);
	memcpy(data + 74, header->filler, sizeof(header->filler));

	file->write(data, kHeaderSize);

	return 1;
}

int ioPcxEncodeLine(const byte *source, byte *dest, int length) {
	int written = 0;

	if (!length)
		return 0;

	do {
		int run = 1;
		bool encodeRun = false;

		if (length > 1) {
			do {
				if (run >= 63)
					break;

				if (source[0] != source[run])
					break;

				run++;
			} while (run < length);

			if (run > 1)
				encodeRun = true;
		}

		if (!encodeRun && source[0] >= 0xC0)
			encodeRun = true;

		if (encodeRun) {
			length -= run;
			dest[0] = run | 0xC0;
			dest[1] = source[0];
			dest += 2;
			source += run;
			written += 2;
		} else {
			*dest++ = *source++;
			length--;
			written++;
		}
	} while (length);

	return written;
}

int ioPcxDecodeLine(PcxReader *reader, byte *dest, int length) {
	int decoded = 0;

	while (decoded < length) {
		if (reader->cursor >= reader->end)
			break;

		byte value = *reader->cursor;
		reader->cursor++;

		int run = 1;

		if ((value & 0xC0) == 0xC0) {
			run = value & 0x3F;

			if (reader->cursor < reader->end) {
				value = *reader->cursor;
				reader->cursor++;
			} else {
				value = 0;
				reader->overrun = true;
			}

			if (run == 0)
				continue;
		}

		while (decoded < length) {
			dest[decoded] = value;
			run--;
			decoded++;

			if (!run)
				break;
		}

		if (run)
			break;
	}

	if (reader->overrun && gpConsoleServices)
		gpConsoleServices->logWarning("PCX::DecodeRow: EOF during read!\n");

	return decoded;
}

void ioPcxLoadFile(LFile *file, PcxReader *reader) {
	int position = file->tell();

	file->seek(0, SEEK_END);
	reader->size = file->tell();
	file->seek(0, SEEK_SET);

	reader->overrun = false;
	reader->base = (byte *)gpMemoryServices->malloc(reader->size * sizeof(byte));

	if (!reader->base) {
		warning("ioPcxLoadFile(): cannot allocate %u bytes", reader->size);

		reader->cursor = nullptr;
		reader->end = nullptr;

		return;
	}

	if (position < 0 || (uint)position > reader->size) {
		warning("ioPcxLoadFile(): invalid file position %d", position);
		position = 0;
	}

	reader->cursor = reader->base + position;
	reader->end = reader->base + reader->size;
	file->read(reader->base, reader->size);
}

} // End of namespace Grim
