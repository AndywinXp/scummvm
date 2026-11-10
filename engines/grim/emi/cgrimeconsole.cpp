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
#include "common/textconsole.h"

#include "engines/grim/emi/cgrimeconsole.h"
#include "engines/grim/emi/lfile.h"

namespace Grim {

bool gDebugLogEnabled = false;

static LFile *gDebugLogFile = nullptr;
static bool gDebugConsoleReady = false;

static int logPrintfStub(const char *text) {
	return 0;
}

static int (*logPrintf)(const char *text) = nullptr;

int CGrimeConsole::logStatus(const char *format, ...) {
	va_list args;

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	int length = message.size();
	if (length > 1021) {
		warning("CGrimeConsole::logStatus(): message too long");
		message = Common::String(message.c_str(), 1021);
	}

	Common::String output = "S:" + message;
	logPrintf(output.c_str());
	writeDebugLog(output.c_str());

	return length;
}

int CGrimeConsole::logDebug(const char *format, ...) {
	va_list args;

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	int length = message.size();
	if (length > 1021) {
		warning("CGrimeConsole::logDebug(): message too long");
		message = Common::String(message.c_str(), 1021);
	}

	Common::String output = "D:" + message;
	logPrintf(output.c_str());
	writeDebugLog(output.c_str());

	return length;
}

int CGrimeConsole::logWarning(const char *format, ...) {
	va_list args;

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	int length = message.size();
	if (length > 1021) {
		warning("CGrimeConsole::logWarning(): message too long");
		message = Common::String(message.c_str(), 1021);
	}

	Common::String output = "W:" + message;
	logPrintf(output.c_str());
	writeDebugLog(output.c_str());

	return length;
}

int CGrimeConsole::logError(const char *format, ...) {
	va_list args;

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	int length = message.size();
	if (length > 1021) {
		warning("CGrimeConsole::logError(): message too long");
		message = Common::String(message.c_str(), 1021);
	}

	Common::String output = "E:" + message;
	logPrintf(output.c_str());
	writeDebugLog(output.c_str());

	return length;
}

void writeDebugLog(const char *text) {
	if (gDebugLogEnabled && gDebugLogFile)
		gDebugLogFile->writeFormat("%s", text);
}

void setDebugLog(bool enable) {
	if (enable) {
		if (gDebugConsoleReady)
			return;

		if (gDebugLogEnabled) {
			gDebugLogFile = gpFileServices->open("debug.log", "w");

			if (!gDebugLogFile)
				return;
		}

		initDebugConsole();
		gDebugConsoleReady = true;
	} else if (gDebugConsoleReady && gDebugLogFile) {
		gpFileServices->close(gDebugLogFile, true);
		gDebugLogFile = nullptr;
	}
}

bool initDebugConsole() {
	logPrintf = logPrintfStub;

	CGrimeConsole *console = new CGrimeConsole();

	LConsoleServices::registerConsole(console);

	return true;
}

} // End of namespace Grim
