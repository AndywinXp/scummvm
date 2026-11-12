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

#include "engines/grim/emi/ctgafile.h"
#include "engines/grim/emi/lbitmap.h"
#include "engines/grim/emi/lcolorformat.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const uint kHeaderSize = 18;
static const uint kFooterSize = 26;
static const uint kExtensionSize = 495;
static const char kFooterSignature[] = "TRUEVISION-XFILE";
static const uint kSignatureSize = 16;

CTGAFile::CTGAFile() {
	init();
}

void CTGAFile::init() {
	bitmap = nullptr;
	flipVertical = false;
	flipHorizontal = false;
	hasGamma = false;
	gamma = 0.0f;
	pixelAspect = 0.0f;
}

bool CTGAFile::readImage(LFile *file, LBitmap *image, int unused) {
	if (!readFooter(file))
		return false;

	bitmap = image;
	file->seek(0, SEEK_SET);

	if (readHeader(file)) {
		byte originalDescriptor = descriptor;

		descriptor = originalDescriptor & 0xCF;
		flipHorizontal = (originalDescriptor & 0x10) != 0;
		flipVertical = (originalDescriptor & 0x20) == 0;

		int bitmapWidth = (int16)width;
		int bitmapHeight = (int16)height;

		if (pixelDepth == 16)
			bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::format5, nullptr);
		else if (pixelDepth == 24)
			bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::format6, nullptr);
		else if (pixelDepth == 32)
			bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::format9, nullptr);

		if (pixelDepth == 16) {
			if (descriptor <= 1) {
				if (!readPixels16(file)) {
					bitmap = nullptr;

					return false;
				}

				bitmap->getColorFormat().setFormat(5);
				flipImage(2);
			}
		} else if (pixelDepth == 24) {
			if (descriptor == 0) {
				if (!readPixels24(file)) {
					bitmap = nullptr;

					return false;
				}

				bitmap->getColorFormat().setFormat(6);
				flipImage(3);
			}
		} else if (pixelDepth == 32) {
			if (!readPixels32(file)) {
				bitmap = nullptr;

				return false;
			}

			bitmap->getColorFormat().setFormat(9);
			flipImage(4);
		}
	}

	bitmap = nullptr;

	return true;
}

int CTGAFile::readPalette(LFile *file, int *palette) {
	return 0;
}

int CTGAFile::writeImage(LFile *file, LBitmap *image, const byte *palette) {
	bitmap = image;
	file->seek(0, SEEK_SET);

	int formatId = bitmap->getColorFormat().formatId;

	if (formatId < 5 || formatId > 6) {
		bitmap = nullptr;

		return 0;
	}

	pixelDepth = 24;

	int bitmapHeight = bitmap->getHeight();
	int bitmapWidth = bitmap->getWidth();
	uint dataSize = 3 * bitmapWidth * bitmapHeight;

	width = bitmapWidth;
	height = bitmapHeight;
	imageType = 2;
	colorMapType = 0;
	idLength = 0;
	colorMapFirst = 0;
	colorMapLength = 0;
	colorMapDepth = 0;
	xOrigin = 0;
	yOrigin = 0;
	descriptor = 0x20;

	byte header[kHeaderSize];

	header[0] = idLength;
	header[1] = colorMapType;
	header[2] = imageType;
	WRITE_LE_UINT16(header + 3, colorMapFirst);
	WRITE_LE_UINT16(header + 5, colorMapLength);
	header[7] = colorMapDepth;
	WRITE_LE_UINT16(header + 8, xOrigin);
	WRITE_LE_UINT16(header + 10, yOrigin);
	WRITE_LE_UINT16(header + 12, width);
	WRITE_LE_UINT16(header + 14, height);
	header[16] = pixelDepth;
	header[17] = descriptor;

	file->write(header, kHeaderSize);

	byte *buffer = (byte *)gpMemoryServices->mallocImpl(dataSize * sizeof(byte));

	if (!buffer) {
		warning("CTGAFile::writeImage(): cannot allocate %u bytes", dataSize);

		return 0;
	}

	const byte *source = (const byte *)bitmap->getPixels();
	byte *dest = buffer;

	if (formatId == 5) {
		for (uint i = 0; i < dataSize / 3; i++) {
			uint16 pixel = READ_LE_UINT16(source);

			source += 2;
			dest[0] = (pixel << 3) & 0xFF;
			dest[1] = (pixel >> 3) & 0xFC;
			dest[2] = (pixel >> 8) & 0xF8;
			dest += 3;
		}
	} else {
		for (uint i = 0; i < dataSize / 3; i++) {
			dest[0] = source[2];
			dest[1] = source[1];
			dest[2] = source[0];
			source += 3;
			dest += 3;
		}
	}

	if (file->write(buffer, dataSize) == dataSize) {
		gpMemoryServices->freeImpl(buffer);
		bitmap = nullptr;

		return 1;
	}

	gpMemoryServices->freeImpl(buffer);

	return 0;
}

int CTGAFile::readFooter(LFile *file) {
	byte footer[kFooterSize];
	byte extension[kExtensionSize];

	memset(authorName, 0, sizeof(authorName));
	memset(authorComments, 0, sizeof(authorComments));
	stampMonth = 1;
	stampDay = 1;
	stampYear = 1980;
	stampHour = 0;
	stampMinute = 0;
	stampSecond = 0;
	memset(jobName, 0, sizeof(jobName));
	jobHours = 0;
	jobMinutes = 0;
	jobSeconds = 0;
	memset(softwareId, 0, sizeof(softwareId));
	softwareVersion = 0;
	softwareLetter = ' ';
	keyColor[0] = 0;
	keyColor[1] = 0;
	keyColor[2] = 0;
	keyColor[3] = 0;
	pixelAspectNum = 1;
	pixelAspectDen = 1;
	gammaNum = 1;
	gammaDen = 1;
	colorCorrectionOffset = 0;
	thumbnailOffset = 0;
	scanLineOffset = 0;
	attributesType = 4;

	file->seek(-(int)kFooterSize, SEEK_END);

	if (file->read(footer, kFooterSize) != kFooterSize)
		return 0;

	// The disasm takes the second dword of the footer as the extension offset
	extensionOffset = READ_LE_UINT32(footer + 4);
	memcpy(signature, footer + 8, sizeof(signature));
	footerTail = READ_LE_UINT16(footer + 24);

	if (memcmp(signature, kFooterSignature, kSignatureSize) != 0)
		return 1;

	if (extensionOffset == 0)
		return 1;

	file->seek((int)extensionOffset, SEEK_SET);

	if (file->read(extension, kExtensionSize) != kExtensionSize)
		return 0;

	extensionSize = READ_LE_UINT16(extension);
	memcpy(authorName, extension + 2, sizeof(authorName));
	memcpy(authorComments, extension + 43, sizeof(authorComments));
	stampMonth = READ_LE_UINT16(extension + 367);
	stampDay = READ_LE_UINT16(extension + 369);
	stampYear = READ_LE_UINT16(extension + 371);
	stampHour = READ_LE_UINT16(extension + 373);
	stampMinute = READ_LE_UINT16(extension + 375);
	stampSecond = READ_LE_UINT16(extension + 377);
	memcpy(jobName, extension + 379, sizeof(jobName));
	jobHours = READ_LE_UINT16(extension + 420);
	jobMinutes = READ_LE_UINT16(extension + 422);
	jobSeconds = READ_LE_UINT16(extension + 424);
	memcpy(softwareId, extension + 426, sizeof(softwareId));
	softwareVersion = READ_LE_UINT16(extension + 467);
	softwareLetter = extension[469];
	keyColor[0] = extension[470];
	keyColor[1] = extension[471];
	keyColor[2] = extension[472];
	keyColor[3] = extension[473];
	pixelAspectNum = READ_LE_UINT16(extension + 474);
	pixelAspectDen = READ_LE_UINT16(extension + 476);
	gammaNum = READ_LE_UINT16(extension + 478);
	gammaDen = READ_LE_UINT16(extension + 480);
	colorCorrectionOffset = READ_LE_UINT32(extension + 482);
	thumbnailOffset = READ_LE_UINT32(extension + 486);
	scanLineOffset = READ_LE_UINT32(extension + 490);
	attributesType = extension[494];

	if (gammaDen != 0) {
		hasGamma = true;
		gamma = (float)((double)(int16)gammaNum / (double)(int16)gammaDen);
	}

	if (pixelAspectDen != 0)
		pixelAspect = (float)((double)(int16)pixelAspectNum / (double)(int16)pixelAspectDen);

	return 1;
}

int CTGAFile::readHeader(LFile *file) {
	byte header[kHeaderSize];

	if (file->read(header, kHeaderSize) != kHeaderSize)
		return 0;

	idLength = header[0];
	colorMapType = header[1];
	imageType = header[2];
	colorMapFirst = READ_LE_UINT16(header + 3);
	colorMapLength = READ_LE_UINT16(header + 5);
	colorMapDepth = header[7];
	xOrigin = READ_LE_UINT16(header + 8);
	yOrigin = READ_LE_UINT16(header + 10);
	width = READ_LE_UINT16(header + 12);
	height = READ_LE_UINT16(header + 14);
	pixelDepth = header[16];
	descriptor = header[17];

	return 1;
}

bool CTGAFile::readPixels16(LFile *file) {
	byte *pixels = (byte *)bitmap->getPixels();
	int pixelCount = (int16)width * (int16)height;

	if (!pixels) {
		warning("CTGAFile::readPixels16(): bitmap has no pixels");

		return false;
	}

	if (idLength)
		file->seek(idLength, SEEK_CUR);

	if (colorMapLength)
		file->seek(colorMapLength * ((colorMapDepth + 7) / 8), SEEK_CUR);

	if (pixelCount <= 0)
		return true;

	if (imageType == 2) {
		file->read(pixels, pixelCount * (pixelDepth >> 3));

		return true;
	}

	if (imageType != 10)
		return true;

	int written = 0;

	do {
		byte packet;

		if (file->read(&packet, 1) != 1) {
			warning("CTGAFile::readPixels16(): unexpected end of file");

			break;
		}

		int count;

		if (packet <= 0x7F) {
			count = packet + 1;

			if (count > pixelCount - written) {
				warning("CTGAFile::readPixels16(): run beyond the end of the image");
				count = pixelCount - written;
			}

			file->read(pixels + written * 2, count * 2);
		} else {
			count = packet - 127;

			byte value[2];

			if (file->read(value, 2) != 2) {
				warning("CTGAFile::readPixels16(): unexpected end of file");

				break;
			}

			if (count > pixelCount - written) {
				warning("CTGAFile::readPixels16(): run beyond the end of the image");
				count = pixelCount - written;
			}

			for (int i = 0; i < count; i++) {
				pixels[(written + i) * 2] = value[0];
				pixels[(written + i) * 2 + 1] = value[1];
			}
		}

		written += count;
	} while (written < pixelCount);

	return true;
}

bool CTGAFile::readPixels24(LFile *file) {
	byte *pixels = (byte *)bitmap->getPixels();
	int pixelCount = (int16)width * (int16)height;

	if (!pixels) {
		warning("CTGAFile::readPixels24(): bitmap has no pixels");

		return false;
	}

	if (idLength)
		file->seek(idLength, SEEK_CUR);

	if (colorMapLength)
		file->seek(colorMapLength * ((colorMapDepth + 7) / 8), SEEK_CUR);

	if (imageType == 2 && pixelCount > 0) {
		file->read(pixels, pixelCount * (pixelDepth >> 3));
	} else if (imageType == 10 && pixelCount > 0) {
		int written = 0;

		do {
			byte packet;

			if (file->read(&packet, 1) != 1) {
				warning("CTGAFile::readPixels24(): unexpected end of file");

				break;
			}

			int count;

			if (packet <= 0x7F) {
				count = packet + 1;

				if (count > pixelCount - written) {
					warning("CTGAFile::readPixels24(): run beyond the end of the image");
					count = pixelCount - written;
				}

				file->read(pixels + written * 3, count * 3);
			} else {
				count = packet - 127;

				byte value[3];

				if (file->read(value, 3) != 3) {
					warning("CTGAFile::readPixels24(): unexpected end of file");

					break;
				}

				if (count > pixelCount - written) {
					warning("CTGAFile::readPixels24(): run beyond the end of the image");
					count = pixelCount - written;
				}

				for (int i = 0; i < count; i++) {
					pixels[(written + i) * 3] = value[0];
					pixels[(written + i) * 3 + 1] = value[1];
					pixels[(written + i) * 3 + 2] = value[2];
				}
			}

			written += count;
		} while (written < pixelCount);
	}

	for (int i = 0; i < pixelCount; i++) {
		byte swap = pixels[i * 3];

		pixels[i * 3] = pixels[i * 3 + 2];
		pixels[i * 3 + 2] = swap;
	}

	return true;
}

bool CTGAFile::readPixels32(LFile *file) {
	byte *pixels = (byte *)bitmap->getPixels();
	int pixelCount = (int16)width * (int16)height;

	if (!pixels) {
		warning("CTGAFile::readPixels32(): bitmap has no pixels");

		return false;
	}

	if (idLength)
		file->seek(idLength, SEEK_CUR);

	if (colorMapLength)
		file->seek(colorMapLength * ((colorMapDepth + 7) / 8), SEEK_CUR);

	if (imageType == 2 && pixelCount > 0) {
		file->read(pixels, pixelCount * (pixelDepth >> 3));
	} else if (imageType == 10 && pixelCount > 0) {
		int written = 0;

		do {
			byte packet;

			if (file->read(&packet, 1) != 1) {
				warning("CTGAFile::readPixels32(): unexpected end of file");

				break;
			}

			int count;

			if (packet <= 0x7F) {
				count = packet + 1;

				if (count > pixelCount - written) {
					warning("CTGAFile::readPixels32(): run beyond the end of the image");
					count = pixelCount - written;
				}

				file->read(pixels + written * 4, count * 4);
			} else {
				count = packet - 127;

				byte value[4];

				if (file->read(value, 4) != 4) {
					warning("CTGAFile::readPixels32(): unexpected end of file");

					break;
				}

				if (count > pixelCount - written) {
					warning("CTGAFile::readPixels32(): run beyond the end of the image");
					count = pixelCount - written;
				}

				for (int i = 0; i < count; i++) {
					pixels[(written + i) * 4] = value[0];
					pixels[(written + i) * 4 + 1] = value[1];
					pixels[(written + i) * 4 + 2] = value[2];
					pixels[(written + i) * 4 + 3] = value[3];
				}
			}

			written += count;
		} while (written < pixelCount);
	}

	for (int i = 0; i < pixelCount; i++) {
		byte swap = pixels[i * 4];

		pixels[i * 4] = pixels[i * 4 + 2];
		pixels[i * 4 + 2] = swap;
	}

	return true;
}

void CTGAFile::flipImage(int bytesPerPixel) {
	int bitmapWidth = bitmap->getWidth();
	int bitmapHeight = bitmap->getHeight();
	byte *pixels = (byte *)bitmap->getPixels();

	if (!pixels)
		return;

	if (flipHorizontal) {
		if (flipVertical) {
			int last = bitmapWidth * bitmapHeight - 1;

			for (int first = 0; first < last; first++, last--) {
				for (int i = 0; i < bytesPerPixel; i++) {
					byte swap = pixels[first * bytesPerPixel + i];

					pixels[first * bytesPerPixel + i] = pixels[last * bytesPerPixel + i];
					pixels[last * bytesPerPixel + i] = swap;
				}
			}
		} else {
			for (int row = 0; row < bitmapHeight; row++) {
				byte *rowPixels = pixels + row * bitmapWidth * bytesPerPixel;
				int last = bitmapWidth - 1;

				for (int first = 0; first < last; first++, last--) {
					for (int i = 0; i < bytesPerPixel; i++) {
						byte swap = rowPixels[first * bytesPerPixel + i];

						rowPixels[first * bytesPerPixel + i] = rowPixels[last * bytesPerPixel + i];
						rowPixels[last * bytesPerPixel + i] = swap;
					}
				}
			}
		}
	} else if (flipVertical) {
		int rowBytes = bitmapWidth * bytesPerPixel;
		int firstRow = 0;
		int lastRow = bitmapHeight - 1;

		for (; firstRow < lastRow; firstRow++, lastRow--) {
			for (int i = 0; i < rowBytes; i++) {
				byte swap = pixels[firstRow * rowBytes + i];

				pixels[firstRow * rowBytes + i] = pixels[lastRow * rowBytes + i];
				pixels[lastRow * rowBytes + i] = swap;
			}
		}
	}
}

} // End of namespace Grim
