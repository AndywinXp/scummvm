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

#include "engines/grim/text.h"

namespace Grim {

static void freeFontMemory(BlastTextFont *&pFont) {
	delete[] pFont->aCharMap;
	pFont->aCharMap = nullptr;
	delete[] pFont->aGlyphs;
	pFont->aGlyphs = nullptr;
	delete[] pFont->pPixels;
	pFont->pPixels = nullptr;
	delete pFont;
	pFont = nullptr;
}

Text::Text() : _pBlastText(nullptr), _pHS(nullptr), _bInitialized(0), _bStarted(0), _pSystemFont(nullptr) {
}

int Text::sysInitialize(SysStruct *pHS) {
	_pHS = pHS;

	if (!_bInitialized) {
		_bInitialized = 1;

		return 0;
	}

	return 1;
}

void Text::sysTerminate() {
	_bInitialized = 0;
}

int Text::sysStartup() {
	if (_bInitialized && !_bStarted) {
		_bStarted = 1;

		for (int i = 0; i < 255; i++)
			_pBlastText->_aCharMap[i] = i;

		return _pBlastText->initFont(_pSystemFont);
	}

	return 1;
}

int Text::sysShutdown() {
	if (_pSystemFont)
		freeFontMemory(_pSystemFont);

	_bStarted = 0;

	return 0;
}

int Text::sysSave(Common::WriteStream *pStream) {
	pStream->writeSint32LE(MKTAG('B', 't', 'x', 'T'));
	pStream->writeSint32LE(1);
	pStream->writeSint32LE(0);

	return 0;
}

int Text::sysRestore(Common::ReadStream *pStream) {
	int tag = pStream->readSint32LE();

	if (tag == MKTAG('B', 't', 'x', 'T')) {
		int version = pStream->readSint32LE();

		if (version == 1) {
			int value = pStream->readSint32LE();

			return value != 0;
		}
	}

	return 1;
}

BlastTextFont *Text::getSystemFont() {
	return _pSystemFont;
}

void Text::freeFont(BlastTextFont *&pFont) {
	if (pFont && pFont != _pSystemFont)
		freeFontMemory(pFont);
}

int Text::getNextChar(const char **ppText) {
	if (!*ppText) {
		warning("Text::getNextChar(): no text");
		return 0;
	}

	int c = *(const byte *)*ppText;

	*ppText = (c != 0) ? *ppText + 1 : nullptr;

	return c;
}

int Text::getCharKernedWidth(const TextParams *pParams, int c) {
	BlastTextFont *pFont = pParams->pFont;

	if (!pFont) {
		warning("Text::getCharKernedWidth(): no font");
		return 0;
	}

	return _pBlastText->charPositionHelper(pParams, pFont, (byte)c);
}

int Text::getStringWidth(const TextParams *pParams, const char *pText) {
	TextParams params = *pParams;
	BlastTextFont *pFont = pParams->pFont;
	int maxWidth = 0;
	int width = 0;

	if (!pFont) {
		warning("Text::getStringWidth(): no font");
		return 0;
	}

	do {
		pText = _pBlastText->measureString(&params, pFont, pText, &width, nullptr, 1);

		if (maxWidth <= width)
			maxWidth = width;
	} while (pText);

	return maxWidth;
}

int Text::getStringHeight(const TextParams *pParams, const char *pText) {
	TextParams params = *pParams;
	BlastTextFont *pFont = pParams->pFont;
	int height = 0;

	if (!pFont) {
		warning("Text::getStringHeight(): no font");
		return 0;
	}

	do {
		pText = _pBlastText->measureString(&params, pFont, pText, nullptr, nullptr, 1);
		height += (pParams->scaleY * (uint)pFont->lineHeight + 0xFFFF) >> 16;
	} while (pText);

	return height;
}

int Text::printText(TextParams *pParams, const char *format, ...) {
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

	BlastTextFont *pFont = pParams->pFont;

	if (!pFont) {
		warning("Text::printText(): no font");
		return written;
	}

	_pBlastText->drawString(pParams, pFont, buffer);

	return written;
}

} // End of namespace Grim
