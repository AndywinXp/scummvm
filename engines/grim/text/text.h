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

	void setBlastText(BlastText *blastText) { _blastText = blastText; }

	int sysInitialize(SysStruct *hs);
	void sysTerminate();
	int sysStartup();
	int sysShutdown();
	int sysSave(Common::WriteStream *stream);
	int sysRestore(Common::ReadStream *stream);
	BlastTextFont *getSystemFont();
	void freeFont(BlastTextFont *&font);
	int getNextChar(const char **text);
	int getCharKernedWidth(const TextParams *params, int c);
	int getStringWidth(const TextParams *paramsPointer, const char *text);
	int getStringHeight(const TextParams *paramsPointer, const char *text);
	int printText(TextParams *params, const char *format, ...);

	friend class BlastText;

private:
	BlastText *_blastText;
	SysStruct *_hs;
	int _initialized;
	int _started;
	BlastTextFont *_systemFont;
};

} // End of namespace Grim

#endif
