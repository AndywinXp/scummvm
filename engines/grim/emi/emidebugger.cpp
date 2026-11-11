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

#include "common/str.h"

#include "engines/grim/emi/emidebugger.h"
#include "engines/grim/grim.h"
#include "engines/grim/md5check.h"

namespace Grim {

EmiDebugger::EmiDebugger() : GUI::Debugger() {
	registerCmd("check_gamedata", WRAP_METHOD(EmiDebugger, cmdCheckFiles));
	registerCmd("lua_do", WRAP_METHOD(EmiDebugger, cmdLuaDo));
}

EmiDebugger::~EmiDebugger() {
}

bool EmiDebugger::cmdCheckFiles(int argc, const char **argv) {
	if (MD5Check::checkFiles())
		debugPrintf("All files are ok.\n");
	else
		debugPrintf("Some files are corrupted or missing.\n");

	return true;
}

bool EmiDebugger::cmdLuaDo(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: lua_do <lua command>\n");

		return true;
	}

	Common::String command;

	for (int i = 1; i < argc; i++) {
		command += argv[i];
		command += " ";
	}

	command.deleteLastChar();
	debugPrintf("Executing command: <%s>\n", command.c_str());
	command = Common::String::format("__temp_fn__ = function()\n%s\nend\nstart_script(__temp_fn__)", command.c_str());
	g_grim->debugLua(command);

	return true;
}

} // End of namespace Grim
