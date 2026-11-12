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

#include "engines/grim/emi/nodes/cprimitive.h"
#include "engines/grim/emi/core/lformatparser.h"

namespace Grim {

enum {
	kKeywordName = 0,
	kKeywordSphere = 1,
	kKeywordBox = 2
};

static const int kTokenSize = 256;

class CPrimitiveParser : public LFormatParser {
public:
	explicit CPrimitiveParser(EmiEngine *engine) : LFormatParser(engine) {
		addKeyword("NAME", kKeywordName, 3);
		addKeyword("SPHERE", kKeywordSphere, 170);
		addKeyword("BOX", kKeywordBox, 2730);
	}
};

CPrimitive::CPrimitive() {
	radius = 0.0f;
	name = "<unnamed primitive>";
}

CPrimitive::~CPrimitive() {
}

bool CPrimitive::save(LFile *file) {
	CPrimitiveParser parser(file->getEngine());

	parser.setFile(file);
	parser.writeVersion(1, 0);
	parser.writeEntry(kKeywordName, name.c_str());
	parser.writeComment("sphere - radius,x,y,z");
	parser.writeEntry(kKeywordSphere, radius, center.x, center.y, center.z);
	parser.writeComment("box - min x,y,z  max x,y,z");
	parser.writeEntry(kKeywordBox, box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z);
	parser.writeEndOfSection();

	return true;
}

bool CPrimitive::restore(LFile *file) {
	CPrimitiveParser parser(file->getEngine());
	char token[kTokenSize];

	parser.setFile(file);

	if (!parser.readVersion(1, 0))
		return false;

	token[0] = 0;
	parser.readEntry(kKeywordName, token);
	name = token;
	parser.readEntry(kKeywordSphere, &radius, &center.x, &center.y, &center.z);
	parser.readEntry(kKeywordBox, &box.min.x, &box.min.y, &box.min.z, &box.max.x, &box.max.y, &box.max.z);
	parser.readEndOfSection();

	return true;
}

void CPrimitive::computeBounds() {
}

} // End of namespace Grim
