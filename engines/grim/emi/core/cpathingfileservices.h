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

#ifndef GRIM_EMI_CPATHINGFILESERVICES_H
#define GRIM_EMI_CPATHINGFILESERVICES_H

#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class CPathingFileServices : public LFileServices {
public:
	explicit CPathingFileServices(EmiEngine *engine);

	void openInto(LFile *file, const char *path, const char *mode) override;
	LFile *open(const char *path, const char *mode) override;
	LFile *openExtended(const char *path, const char *mode, int unused1, int unused2) override;
	bool exists(const char *path) override;
	int getModTime(const char *path) override;
	bool resolvePath(LString &outPath, const char *name) override;

	static const char *getTypeDirectory(const char *extension);
	char *getCdPath();
	void setCdPath(const char *path);
	void suspendCdPath();
	void resumeCdPath();

private:
	char _cdPath[260];
	char _suspendedCdPath[260];
};

} // End of namespace Grim

#endif
