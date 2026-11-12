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

#ifndef GRIM_TEXT_H
#define GRIM_TEXT_H

#include "common/scummsys.h"

#include "engines/grim/text/blasttext.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class Text {
public:
	Text();

	void setBlastText(BlastText *pBlastText) { _pBlastText = pBlastText; }

	int sysInitialize(SysStruct *pHS);
	void sysTerminate();
	int sysStartup();
	int sysShutdown();
	int sysSave(Common::WriteStream *pStream);
	int sysRestore(Common::ReadStream *pStream);
	BlastTextFont *getSystemFont();
	void freeFont(BlastTextFont *&pFont);
	int getNextChar(const char **ppText);
	int getCharKernedWidth(const TextParams *pParams, int c);
	int getStringWidth(const TextParams *pParams, const char *pText);
	int getStringHeight(const TextParams *pParams, const char *pText);
	int printText(TextParams *pParams, const char *format, ...);

	friend class BlastText;

private:
	BlastText *_pBlastText;
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	BlastTextFont *_pSystemFont;
};

} // End of namespace Grim

#endif
