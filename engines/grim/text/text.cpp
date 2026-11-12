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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/text/text.h"

namespace Grim {

static void freeFontMemory(BlastTextFont *&font) {
	delete[] font->charMap;
	font->charMap = nullptr;
	delete[] font->glyphs;
	font->glyphs = nullptr;
	delete[] font->pixels;
	font->pixels = nullptr;
	delete font;
	font = nullptr;
}

Text::Text() : _blastText(nullptr), _hs(nullptr), _initialized(0), _started(0), _systemFont(nullptr) {
}

int Text::sysInitialize(SysStruct *hs) {
	_hs = hs;

	if (!_initialized) {
		_initialized = 1;

		return 0;
	}

	return 1;
}

void Text::sysTerminate() {
	_initialized = 0;
}

int Text::sysStartup() {
	if (_initialized && !_started) {
		_started = 1;

		for (int i = 0; i < 255; i++)
			_blastText->_charMap[i] = i;

		return _blastText->initFont(_systemFont);
	}

	return 1;
}

int Text::sysShutdown() {
	if (_systemFont)
		freeFontMemory(_systemFont);

	_started = 0;

	return 0;
}

int Text::sysSave(Common::WriteStream *stream) {
	stream->writeSint32LE(MKTAG('B', 't', 'x', 'T'));
	stream->writeSint32LE(1);
	stream->writeSint32LE(0);

	return 0;
}

int Text::sysRestore(Common::ReadStream *stream) {
	int tag = stream->readSint32LE();

	if (tag == MKTAG('B', 't', 'x', 'T')) {
		int version = stream->readSint32LE();

		if (version == 1) {
			int value = stream->readSint32LE();

			return value != 0;
		}
	}

	return 1;
}

BlastTextFont *Text::getSystemFont() {
	return _systemFont;
}

void Text::freeFont(BlastTextFont *&font) {
	if (font && font != _systemFont)
		freeFontMemory(font);
}

int Text::getNextChar(const char **text) {
	if (!*text) {
		warning("Text::getNextChar(): no text");
		return 0;
	}

	int c = *(const byte *)*text;

	*text = (c != 0) ? *text + 1 : nullptr;

	return c;
}

int Text::getCharKernedWidth(const TextParams *params, int c) {
	BlastTextFont *font = params->font;

	if (!font) {
		warning("Text::getCharKernedWidth(): no font");
		return 0;
	}

	return _blastText->charPositionHelper(params, font, (byte)c);
}

int Text::getStringWidth(const TextParams *paramsPointer, const char *text) {
	TextParams params = *paramsPointer;
	BlastTextFont *font = paramsPointer->font;
	int maxWidth = 0;
	int width = 0;

	if (!font) {
		warning("Text::getStringWidth(): no font");
		return 0;
	}

	do {
		text = _blastText->measureString(&params, font, text, &width, nullptr, 1);

		if (maxWidth <= width)
			maxWidth = width;
	} while (text);

	return maxWidth;
}

int Text::getStringHeight(const TextParams *paramsPointer, const char *text) {
	TextParams params = *paramsPointer;
	BlastTextFont *font = paramsPointer->font;
	int height = 0;

	if (!font) {
		warning("Text::getStringHeight(): no font");
		return 0;
	}

	do {
		text = _blastText->measureString(&params, font, text, nullptr, nullptr, 1);
		height += (paramsPointer->scaleY * (uint)font->lineHeight + 0xFFFF) >> 16;
	} while (text);

	return height;
}

int Text::printText(TextParams *params, const char *format, ...) {
	char buffer[1004];

	if (!format)
		return 0;

	va_list args;

	va_start(args, format);
	int written = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (written >= (int)sizeof(buffer)) {
		warning("Text::printText(): text too long, truncated");
		written = sizeof(buffer) - 1;
	}

	BlastTextFont *font = params->font;

	if (!font) {
		warning("Text::printText(): no font");
		return written;
	}

	_blastText->drawString(params, font, buffer);

	return written;
}

} // End of namespace Grim
