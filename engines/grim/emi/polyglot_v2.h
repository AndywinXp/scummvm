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

#ifndef GRIM_EMI_POLYGLOT_V2_H
#define GRIM_EMI_POLYGLOT_V2_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"
#include "engines/grim/polyglot.h"

namespace Grim {

class Polyglot_v2 : public Polyglot {
public:
	Polyglot_v2();
	~Polyglot_v2() override;

	void shutdown() override;
	void configure(const char *tabFile) override;
	const char *getTranslation(const char *msg) override;

private:
	int nextKeyByte();
	LFile *openFileNoBundle(const char *path, const char *mode);

	char *_table;
	char **_lines;
	int _lineCount;
	uint32 _randomSeed;
};

} // End of namespace Grim

#endif
