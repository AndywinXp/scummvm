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

#include "topgun/console.h"
#include "topgun/libraries/resource/resource.h"
#include "topgun/topgun.h"

namespace Topgun {

#define TG_CONSOLE_MAX_LISTED_TYPES   16
#define TG_CONSOLE_MAX_LISTED_ENTRIES 20
#define TG_CONSOLE_MAX_PALETTE_BYTES  0xFFFF

Console::Console() : GUI::Debugger() {
	registerCmd("test",   WRAP_METHOD(Console, Cmd_test));
	registerCmd("listresources", WRAP_METHOD(Console, Cmd_listResources));
}

Console::~Console() {
}

bool Console::Cmd_test(int argc, const char **argv) {
	debugPrintf("Test\n");
	return true;
}

bool Console::Cmd_listResources(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: %s <file.bin>\n", argv[0]);
		return true;
	}

	ResourceLibrary *library = g_engine->resourceLibrary();

	uint16 file = library->resOpenFile(argv[1]);
	if (file == 0) {
		debugPrintf("Cannot open '%s'\n", argv[1]);
		return true;
	}

	uint16 entryCount = library->resCountResources(file);
	debugPrintf("Entries: %d\n", entryCount);
	debugPrintf("Variables: %d\n", library->resCountVariables(file));

	byte *palette = new byte[TG_CONSOLE_MAX_PALETTE_BYTES];
	debugPrintf("Palette colors (nDwords): %d\n", library->resLoadPalette(file, palette));
	delete[] palette;

	uint32 typeCounts[TG_CONSOLE_MAX_LISTED_TYPES];
	for (int i = 0; i < TG_CONSOLE_MAX_LISTED_TYPES; i++)
		typeCounts[i] = 0;

	uint32 otherTypes = 0;
	for (uint16 i = 0; i < entryCount; i++) {
		uint16 type = library->resGetType(file, i);
		if (type < TG_CONSOLE_MAX_LISTED_TYPES)
			typeCounts[type]++;
		else
			otherTypes++;
	}

	for (int i = 0; i < TG_CONSOLE_MAX_LISTED_TYPES; i++) {
		if (typeCounts[i] != 0)
			debugPrintf("Type %d: %u\n", i, typeCounts[i]);
	}

	if (otherTypes != 0)
		debugPrintf("Other types: %u\n", otherTypes);

	debugPrintf("Index Type Size\n");
	for (uint16 i = 0; i < entryCount && i < TG_CONSOLE_MAX_LISTED_ENTRIES; i++)
		debugPrintf("%5d %4d %u\n", i, library->resGetType(file, i), library->resGetSize(file, i));

	library->resCloseFile(file);
	return true;
}

} // End of namespace Topgun
