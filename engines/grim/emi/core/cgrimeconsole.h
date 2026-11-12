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

#ifndef GRIM_EMI_CGRIMECONSOLE_H
#define GRIM_EMI_CGRIMECONSOLE_H

#include "common/scummsys.h"

#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class CGrimeConsole : public LConsoleServices {
public:
	explicit CGrimeConsole(EmiEngine *engine);

	int logStatus(const char *format, ...) override;
	int logWarning(const char *format, ...) override;
	int logError(const char *format, ...) override;
	int logDebug(const char *format, ...) override;

	void writeDebugLog(const char *text);
	void setDebugLog(bool enable);
	bool initDebugConsole();

private:
	int logPrintfStub(const char *text);

	bool _debugLogEnabled;
	LFile *_debugLogFile;
	bool _debugConsoleReady;
};

} // End of namespace Grim

#endif
