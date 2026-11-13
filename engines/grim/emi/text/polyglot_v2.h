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

#include "common/array.h"
#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/text/polyglot.h"

namespace Grim {

class Polyglot_v2 : public Polyglot {
public:
	explicit Polyglot_v2(EmiEngine *engine);
	~Polyglot_v2() override;

	void shutdown() override;
	void configure(const char *tabFile) override;
	const char *getTranslation(const char *msg) override;

	// Table of the Korean and Chinese versions. The result points into the table or into
	// a buffer of this class, so it is only valid until the next call: copy it at once.
	const uint16 *getCjkTranslation(const char *msg, int mode);
	bool isCjkTable() const { return _cjkTable != nullptr; }

private:
	enum {
		kDefaultBufferSize = 512
	};

	int nextKeyByte();
	LFile *openFileNoBundle(const char *path, const char *mode);
	void configureCjk(const char *tabFile);
	const uint16 *toCjkUnits(const char *text);
	static int compareCjkLines(const void *a, const void *b);

	EmiEngine *_engine;
	char *_table;
	char **_lines;
	int _lineCount;
	uint32 _randomSeed;
	uint16 *_cjkTable;
	uint16 **_cjkLines;
	Common::Array<uint16> _cjkModeBuffer;
	uint16 _defaultBuffer[kDefaultBufferSize];
};

} // End of namespace Grim

#endif
