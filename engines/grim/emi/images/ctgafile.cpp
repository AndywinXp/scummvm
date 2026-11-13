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

#include "engines/grim/emi/images/ctgafile.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/images/lbitmap.h"
#include "engines/grim/emi/graphics/lcolorformat.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

static const uint kHeaderSize = 18;
static const uint kFooterSize = 26;
static const uint kExtensionSize = 495;
static const char kFooterSignature[] = "TRUEVISION-XFILE";
static const uint kSignatureSize = 16;

CTGAFile::CTGAFile(EmiEngine *engine) : _engine(engine) {
	init();
}

void CTGAFile::init() {
	_bitmap = nullptr;
	_flipVertical = false;
	_flipHorizontal = false;
	_hasGamma = false;
	_gamma = 0.0f;
	_pixelAspect = 0.0f;
}

bool CTGAFile::readImage(LFile *file, LBitmap *image, int unused) {
	if (!readFooter(file)) {
		if (_engine->isDemo())
			warning("CTGAFile::readImage(): empty or truncated image file");

		return false;
	}

	_bitmap = image;
	file->seek(0, SEEK_SET);

	if (readHeader(file)) {
		if (_engine->isDemo() && _pixelDepth != 16 && _pixelDepth != 24 && _pixelDepth != 32) {
			warning("CTGAFile::readImage(): unsupported pixel depth %d", _pixelDepth);
			_bitmap = nullptr;

			return false;
		}

		byte originalDescriptor = _descriptor;

		_descriptor = originalDescriptor & 0xCF;
		_flipHorizontal = (originalDescriptor & 0x10) != 0;
		_flipVertical = (originalDescriptor & 0x20) == 0;

		int bitmapWidth = (int16)_width;
		int bitmapHeight = (int16)_height;

		if (_pixelDepth == 16)
			_bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::kFormat5, nullptr);
		else if (_pixelDepth == 24)
			_bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::kFormat6, nullptr);
		else if (_pixelDepth == 32)
			_bitmap->create(bitmapWidth, bitmapHeight, LColorFormat::kFormat9, nullptr);

		if (_pixelDepth == 16) {
			if (_descriptor <= 1) {
				if (!readPixels16(file)) {
					_bitmap = nullptr;

					return false;
				}

				_bitmap->getColorFormat().setFormat(5);
				flipImage(2);
			}
		} else if (_pixelDepth == 24) {
			if (_descriptor == 0) {
				if (!readPixels24(file)) {
					_bitmap = nullptr;

					return false;
				}

				_bitmap->getColorFormat().setFormat(6);
				flipImage(3);
			}
		} else if (_pixelDepth == 32) {
			if (!readPixels32(file)) {
				_bitmap = nullptr;

				return false;
			}

			_bitmap->getColorFormat().setFormat(9);
			flipImage(4);
		}
	}

	_bitmap = nullptr;

	return true;
}

int CTGAFile::readPalette(LFile *file, int *palette) {
	return 0;
}

int CTGAFile::writeImage(LFile *file, LBitmap *image, const byte *palette) {
	_bitmap = image;
	file->seek(0, SEEK_SET);

	int formatId = _bitmap->getColorFormat()._formatId;

	if (formatId < 5 || formatId > 6) {
		_bitmap = nullptr;

		return 0;
	}

	_pixelDepth = 24;

	int bitmapHeight = _bitmap->getHeight();
	int bitmapWidth = _bitmap->getWidth();
	uint dataSize = 3 * bitmapWidth * bitmapHeight;

	_width = bitmapWidth;
	_height = bitmapHeight;
	_imageType = 2;
	_colorMapType = 0;
	_idLength = 0;
	_colorMapFirst = 0;
	_colorMapLength = 0;
	_colorMapDepth = 0;
	_xOrigin = 0;
	_yOrigin = 0;
	_descriptor = 0x20;

	byte header[kHeaderSize];

	header[0] = _idLength;
	header[1] = _colorMapType;
	header[2] = _imageType;
	WRITE_LE_UINT16(header + 3, _colorMapFirst);
	WRITE_LE_UINT16(header + 5, _colorMapLength);
	header[7] = _colorMapDepth;
	WRITE_LE_UINT16(header + 8, _xOrigin);
	WRITE_LE_UINT16(header + 10, _yOrigin);
	WRITE_LE_UINT16(header + 12, _width);
	WRITE_LE_UINT16(header + 14, _height);
	header[16] = _pixelDepth;
	header[17] = _descriptor;

	file->write(header, kHeaderSize);

	byte *buffer = (byte *)_engine->getMemoryServices()->mallocImpl(dataSize * sizeof(byte));

	if (!buffer) {
		warning("CTGAFile::writeImage(): cannot allocate %u bytes", dataSize);

		return 0;
	}

	const byte *source = (const byte *)_bitmap->getPixels();
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
		_engine->getMemoryServices()->freeImpl(buffer);
		_bitmap = nullptr;

		return 1;
	}

	_engine->getMemoryServices()->freeImpl(buffer);

	return 0;
}

int CTGAFile::readFooter(LFile *file) {
	byte footer[kFooterSize];
	byte extension[kExtensionSize];

	memset(_authorName, 0, sizeof(_authorName));
	memset(_authorComments, 0, sizeof(_authorComments));
	_stampMonth = 1;
	_stampDay = 1;
	_stampYear = 1980;
	_stampHour = 0;
	_stampMinute = 0;
	_stampSecond = 0;
	memset(_jobName, 0, sizeof(_jobName));
	_jobHours = 0;
	_jobMinutes = 0;
	_jobSeconds = 0;
	memset(_softwareId, 0, sizeof(_softwareId));
	_softwareVersion = 0;
	_softwareLetter = ' ';
	_keyColor[0] = 0;
	_keyColor[1] = 0;
	_keyColor[2] = 0;
	_keyColor[3] = 0;
	_pixelAspectNum = 1;
	_pixelAspectDen = 1;
	_gammaNum = 1;
	_gammaDen = 1;
	_colorCorrectionOffset = 0;
	_thumbnailOffset = 0;
	_scanLineOffset = 0;
	_attributesType = 4;

	file->seek(-(int)kFooterSize, SEEK_END);

	if (file->read(footer, kFooterSize) != kFooterSize)
		return 0;

	// The disasm takes the second dword of the footer as the extension offset
	_extensionOffset = READ_LE_UINT32(footer + 4);
	memcpy(_signature, footer + 8, sizeof(_signature));
	_footerTail = READ_LE_UINT16(footer + 24);

	if (memcmp(_signature, kFooterSignature, kSignatureSize) != 0)
		return 1;

	if (_extensionOffset == 0)
		return 1;

	file->seek((int)_extensionOffset, SEEK_SET);

	if (file->read(extension, kExtensionSize) != kExtensionSize)
		return 0;

	_extensionSize = READ_LE_UINT16(extension);
	memcpy(_authorName, extension + 2, sizeof(_authorName));
	memcpy(_authorComments, extension + 43, sizeof(_authorComments));
	_stampMonth = READ_LE_UINT16(extension + 367);
	_stampDay = READ_LE_UINT16(extension + 369);
	_stampYear = READ_LE_UINT16(extension + 371);
	_stampHour = READ_LE_UINT16(extension + 373);
	_stampMinute = READ_LE_UINT16(extension + 375);
	_stampSecond = READ_LE_UINT16(extension + 377);
	memcpy(_jobName, extension + 379, sizeof(_jobName));
	_jobHours = READ_LE_UINT16(extension + 420);
	_jobMinutes = READ_LE_UINT16(extension + 422);
	_jobSeconds = READ_LE_UINT16(extension + 424);
	memcpy(_softwareId, extension + 426, sizeof(_softwareId));
	_softwareVersion = READ_LE_UINT16(extension + 467);
	_softwareLetter = extension[469];
	_keyColor[0] = extension[470];
	_keyColor[1] = extension[471];
	_keyColor[2] = extension[472];
	_keyColor[3] = extension[473];
	_pixelAspectNum = READ_LE_UINT16(extension + 474);
	_pixelAspectDen = READ_LE_UINT16(extension + 476);
	_gammaNum = READ_LE_UINT16(extension + 478);
	_gammaDen = READ_LE_UINT16(extension + 480);
	_colorCorrectionOffset = READ_LE_UINT32(extension + 482);
	_thumbnailOffset = READ_LE_UINT32(extension + 486);
	_scanLineOffset = READ_LE_UINT32(extension + 490);
	_attributesType = extension[494];

	if (_gammaDen != 0) {
		_hasGamma = true;
		_gamma = (float)((double)(int16)_gammaNum / (double)(int16)_gammaDen);
	}

	if (_pixelAspectDen != 0)
		_pixelAspect = (float)((double)(int16)_pixelAspectNum / (double)(int16)_pixelAspectDen);

	return 1;
}

int CTGAFile::readHeader(LFile *file) {
	byte header[kHeaderSize];

	if (file->read(header, kHeaderSize) != kHeaderSize)
		return 0;

	_idLength = header[0];
	_colorMapType = header[1];
	_imageType = header[2];
	_colorMapFirst = READ_LE_UINT16(header + 3);
	_colorMapLength = READ_LE_UINT16(header + 5);
	_colorMapDepth = header[7];
	_xOrigin = READ_LE_UINT16(header + 8);
	_yOrigin = READ_LE_UINT16(header + 10);
	_width = READ_LE_UINT16(header + 12);
	_height = READ_LE_UINT16(header + 14);
	_pixelDepth = header[16];
	_descriptor = header[17];

	return 1;
}

bool CTGAFile::readPixels16(LFile *file) {
	byte *pixels = (byte *)_bitmap->getPixels();
	int pixelCount = (int16)_width * (int16)_height;

	if (!pixels) {
		warning("CTGAFile::readPixels16(): bitmap has no pixels");

		return false;
	}

	if (_idLength)
		file->seek(_idLength, SEEK_CUR);

	if (_colorMapLength)
		file->seek(_colorMapLength * ((_colorMapDepth + 7) / 8), SEEK_CUR);

	if (pixelCount <= 0)
		return true;

	if (_imageType == 2) {
		file->read(pixels, pixelCount * (_pixelDepth >> 3));

		return true;
	}

	if (_imageType != 10)
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
	byte *pixels = (byte *)_bitmap->getPixels();
	int pixelCount = (int16)_width * (int16)_height;

	if (!pixels) {
		warning("CTGAFile::readPixels24(): bitmap has no pixels");

		return false;
	}

	if (_idLength)
		file->seek(_idLength, SEEK_CUR);

	if (_colorMapLength)
		file->seek(_colorMapLength * ((_colorMapDepth + 7) / 8), SEEK_CUR);

	if (_imageType == 2 && pixelCount > 0) {
		file->read(pixels, pixelCount * (_pixelDepth >> 3));
	} else if (_imageType == 10 && pixelCount > 0) {
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
	byte *pixels = (byte *)_bitmap->getPixels();
	int pixelCount = (int16)_width * (int16)_height;

	if (!pixels) {
		warning("CTGAFile::readPixels32(): bitmap has no pixels");

		return false;
	}

	if (_idLength)
		file->seek(_idLength, SEEK_CUR);

	if (_colorMapLength)
		file->seek(_colorMapLength * ((_colorMapDepth + 7) / 8), SEEK_CUR);

	if (_imageType == 2 && pixelCount > 0) {
		file->read(pixels, pixelCount * (_pixelDepth >> 3));
	} else if (_imageType == 10 && pixelCount > 0) {
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
	int bitmapWidth = _bitmap->getWidth();
	int bitmapHeight = _bitmap->getHeight();
	byte *pixels = (byte *)_bitmap->getPixels();

	if (!pixels)
		return;

	if (_flipHorizontal) {
		if (_flipVertical) {
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
	} else if (_flipVertical) {
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
