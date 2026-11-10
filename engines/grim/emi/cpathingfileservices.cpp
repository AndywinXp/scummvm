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

#include "engines/grim/emi/cpathingfileservices.h"

namespace Grim {

struct TypeDirectory {
	const char *extension;
	const char *directory;
};

static const TypeDirectory kTypeDirectories[20] = {
	{ ".ani", ".\\Animation\\" },
	{ ".til", ".\\Backgrounds\\" },
	{ ".cos", ".\\Costumes\\" },
	{ ".laf", ".\\Fonts\\" },
	{ ".tga", ".\\Materials\\" },
	{ ".pcx", ".\\Materials\\" },
	{ ".mes", ".\\Meshes\\" },
	{ ".spr", ".\\Meshes\\" },
	{ ".snm", ".\\Movies\\" },
	{ ".lua", ".\\Scripts\\" },
	{ ".set", ".\\Sets\\" },
	{ ".aif", ".\\Sfx\\" },
	{ ".skl", ".\\Skeletons\\" },
	{ ".wav", ".\\Voice\\" },
	{ ".wvc", ".\\Voice\\" },
	{ ".lip", ".\\Lip\\" },
	{ ".sur", ".\\Materials\\" },
	{ ".mov", ".\\Movies\\" },
	{ ".bik", ".\\Movies\\" },
	{ ".m4b", ".\\" }
};

static char gCdPath[260] = { 0 };
static char gSuspendedCdPath[260] = { 0 };

const char *getTypeDirectory(const char *extension) {
	for (int i = 0; i < 20; i++) {
		if (scumm_strnicmp(extension, kTypeDirectories[i].extension, 4) == 0)
			return kTypeDirectories[i].directory;
	}

	return nullptr;
}

char *getCdPath() {
	return gCdPath[0] != 0 ? gCdPath : nullptr;
}

void suspendCdPath() {
	Common::strcpy_s(gSuspendedCdPath, gCdPath);
	gCdPath[0] = 0;
}

void resumeCdPath() {
	Common::strcpy_s(gCdPath, gSuspendedCdPath);
}

void CPathingFileServices::openInto(LFile *file, const char *path, const char *mode) {
	const char *current = path;
	bool leadingDot = false;

	if (!file) {
		warning("CPathingFileServices::openInto(): null file");

		return;
	}

	if (*path == '.') {
		LFileServices::openInto(file, path, mode);
		if (file->isOpen())
			return;

		current = path[1] ? path + 2 : path + 1;
		leadingDot = true;
	}

	if (!strchr(current, ':')) {
		LString resolved;

		if (resolvePath(resolved, current))
			LFileServices::openInto(file, resolved.c_str(), mode);
	}

	if (!file->isOpen() && !leadingDot)
		LFileServices::openInto(file, current, mode);
}

LFile *CPathingFileServices::open(const char *path, const char *mode) {
	const char *current = path;
	LFile *file = nullptr;
	bool leadingDot = false;

	if (*path == '.') {
		file = LFileServices::open(path, mode);
		if (file)
			return file;

		current = path[1] ? path + 2 : path + 1;
		leadingDot = true;
	}

	if (!strchr(current, ':')) {
		LString resolved;

		if (resolvePath(resolved, current))
			file = LFileServices::open(resolved.c_str(), mode);

		if (file)
			return file;
	}

	if (!leadingDot)
		return LFileServices::open(current, mode);

	return file;
}

LFile *CPathingFileServices::openExtended(const char *path, const char *mode, int unused1, int unused2) {
	return open(path, mode);
}

bool CPathingFileServices::exists(const char *path) {
	LString resolved;

	if (strchr(path, ':'))
		return false;

	if (!resolvePath(resolved, path))
		return false;

	return LFileServices::exists(resolved.c_str());
}

int CPathingFileServices::getModTime(const char *path) {
	LString resolved;

	if (strchr(path, ':'))
		return 0;

	if (!resolvePath(resolved, path))
		return 0;

	return LFileServices::getModTime(resolved.c_str());
}

bool CPathingFileServices::resolvePath(LString &outPath, const char *name) {
	const char *extension = strrchr(name, '.');
	if (!extension)
		return false;

	const char *typeDirectory = getTypeDirectory(extension);
	if (!typeDirectory)
		return false;

	outPath = gCdPath[0] ? gCdPath : ".";
	outPath += typeDirectory + 1;
	outPath += name;

	return true;
}

} // End of namespace Grim
