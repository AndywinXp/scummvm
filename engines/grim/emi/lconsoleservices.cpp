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

#include "common/debug.h"
#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

LConsoleServices *gpConsoleServices = nullptr;

static LConsoleServices **gConsoles = nullptr;
static uint gNConsoles = 0;
static uint gConsolesArraySize = 0;

static bool gLogToFileEnabled = false;
static char gLogToFileName[32] = { 0 };

LConsoleServices::LConsoleServices() {
	if (!gpConsoleServices) {
		registerConsole(this);
		gpConsoleServices = this;
	}
}

LConsoleServices::~LConsoleServices() {
	if (gpConsoleServices == this)
		gpConsoleServices = nullptr;
}

void LConsoleServices::deleteAllConsoles() {
	uint i;

	for (i = 0; i < gNConsoles; i++) {
		LConsoleServices *console = gConsoles[i];

		if (console) {
			delete console;

			for (uint j = i; j < gNConsoles; j++) {
				if (gConsoles[j] == console)
					gConsoles[j] = nullptr;
			}
		}
	}

	gpMemoryServices->free(gConsoles);
	gConsoles = nullptr;
	gpConsoleServices = nullptr;
}

int LConsoleServices::registerConsole(LConsoleServices *console) {
	LConsoleServices **oldConsoles = gConsoles;
	int result;
	uint i;

	if (oldConsoles) {
		result = gNConsoles;

		if (gNConsoles >= gConsolesArraySize) {
			if (gNConsoles == gConsolesArraySize) {
				gConsoles = (LConsoleServices **)gpMemoryServices->malloc((gConsolesArraySize + 4) * sizeof(LConsoleServices *));

				for (i = 0; i < gConsolesArraySize; i++)
					gConsoles[i] = oldConsoles[i];

				gConsolesArraySize += 4;
				gpMemoryServices->free(oldConsoles);
				gConsoles[gNConsoles] = console;
				gpConsoleServices = console;
				gNConsoles++;

				return gNConsoles;
			} else {
				gpConsoleServices = console;
			}
		} else {
			gConsoles[gNConsoles] = console;
			result = gNConsoles + 1;
			gpConsoleServices = console;
			gNConsoles++;
		}
	} else {
		gConsolesArraySize = 4;
		gConsoles = (LConsoleServices **)gpMemoryServices->malloc(gConsolesArraySize * sizeof(LConsoleServices *));
		gNConsoles = 1;
		gConsoles[0] = console;
		result = gNConsoles;
		gpConsoleServices = console;
	}

	return result;
}

int LConsoleServices::logStatus(const char *format, ...) {
	char output[1024];
	va_list args;

	output[0] = 'S';
	output[1] = ':';

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	uint copyLength = message.size();
	if (copyLength > 1021) {
		warning("LConsoleServices::logStatus(): message too long");
		copyLength = 1021;
	}

	memcpy(output + 2, message.c_str(), copyLength);
	output[2 + copyLength] = 0;

	debugN("%s", output);
	logToFile("%s", output);

	return message.size();
}

int LConsoleServices::logWarning(const char *format, ...) {
	char output[1024];
	va_list args;

	output[0] = 'W';
	output[1] = ':';

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	uint copyLength = message.size();
	if (copyLength > 1021) {
		warning("LConsoleServices::logWarning(): message too long");
		copyLength = 1021;
	}

	memcpy(output + 2, message.c_str(), copyLength);
	output[2 + copyLength] = 0;

	debugN("%s", output);
	logToFile("%s", output);

	return message.size();
}

int LConsoleServices::logError(const char *format, ...) {
	char output[1024];
	va_list args;

	output[0] = 'E';
	output[1] = ':';

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	uint copyLength = message.size();
	if (copyLength > 1021) {
		warning("LConsoleServices::logError(): message too long");
		copyLength = 1021;
	}

	memcpy(output + 2, message.c_str(), copyLength);
	output[2 + copyLength] = 0;

	debugN("%s", output);
	logToFile("%s", output);

	return message.size();
}

int LConsoleServices::logDebug(const char *format, ...) {
	char output[1024];
	va_list args;

	output[0] = 'D';
	output[1] = ':';

	va_start(args, format);
	Common::String message = Common::String::vformat(format, args);
	va_end(args);

	uint copyLength = message.size();
	if (copyLength > 1021) {
		warning("LConsoleServices::logDebug(): message too long");
		copyLength = 1021;
	}

	memcpy(output + 2, message.c_str(), copyLength);
	output[2 + copyLength] = 0;

	debugN("%s", output);
	logToFile("%s", output);

	return message.size();
}

int LConsoleServices::logToFile(const char *format, ...) {
	va_list args;

	if (!gLogToFileEnabled)
		return 0;

	Common::DumpFile file;
	if (file.open(Common::Path(gLogToFileName))) {
		va_start(args, format);
		Common::String text = Common::String::vformat(format, args);
		va_end(args);

		file.writeString(text);
		file.close();
	}

	return 0;
}

} // End of namespace Grim
