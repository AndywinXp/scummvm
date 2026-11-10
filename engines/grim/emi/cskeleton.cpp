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

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cskeleton.h"
#include "engines/grim/emi/lformatparser.h"

namespace Grim {

enum {
	kKeywordNumNodes = 1,
	kKeywordName = 2,
	kKeywordParent = 3,
	kKeywordPos = 4,
	kKeywordQuat = 5
};

static const int kTokenSize = 256;
static const int kBinaryNodeSize = 92;

class CSkeletonParser : public LFormatParser {
public:
	CSkeletonParser() {
		addKeyword("NUMNODES", kKeywordNumNodes, 1);
		addKeyword("NAME", kKeywordName, 3);
		addKeyword("PARENT", kKeywordParent, 3);
		addKeyword("POS", kKeywordPos, 42);
		addKeyword("QUAT", kKeywordQuat, 170);
	}
};

static void copyNodeName(char *dest, const char *source) {
	if (strlen(source) >= (size_t)SkeletonNode::kNameSize)
		warning("CSkeleton: node name '%s' is too long", source);

	Common::strlcpy(dest, source, SkeletonNode::kNameSize);
}

SkeletonNode::SkeletonNode() {
	memset(name, 0, sizeof(name));
	memset(parent, 0, sizeof(parent));
}

CSkeleton::CSkeleton() {
}

CSkeleton::~CSkeleton() {
}

bool CSkeleton::save(LFile *file) {
	CSkeletonParser parser;

	parser.setFile(file);
	parser.writeVersion(0, 0);
	parser.writeEntry(kKeywordNumNodes, (int)nodes.size());

	for (uint i = 0; i < nodes.size(); i++) {
		parser.writeComment("Node:");
		parser.writeEntry(kKeywordName, nodes[i].name);
		parser.writeEntry(kKeywordParent, nodes[i].parent);
		parser.writeEntry(kKeywordPos, nodes[i].pos.x, nodes[i].pos.y, nodes[i].pos.z);
		parser.writeEntry(kKeywordQuat, nodes[i].quat.x, nodes[i].quat.y, nodes[i].quat.z, nodes[i].quat.w);
	}

	parser.writeEndOfSection();

	return true;
}

bool CSkeleton::restore(LFile *file) {
	CSkeletonParser parser;
	int count = 0;
	char token[kTokenSize];

	parser.setFile(file);
	parser.readVersion(0, 0);
	parser.readEntry(kKeywordNumNodes, &count);

	if (count < 0) {
		warning("CSkeleton::restore(): invalid node count %d", count);

		return false;
	}

	nodes.resize(count);

	for (int i = 0; i < count; i++) {
		parser.readEntry(kKeywordName, token);
		copyNodeName(nodes[i].name, token);

		parser.readEntry(kKeywordParent, token);
		copyNodeName(nodes[i].parent, token);

		parser.readEntry(kKeywordPos, &nodes[i].pos.x, &nodes[i].pos.y, &nodes[i].pos.z);
		parser.readEntry(kKeywordQuat, &nodes[i].quat.x, &nodes[i].quat.y, &nodes[i].quat.z, &nodes[i].quat.w);
	}

	parser.readEndOfSection();

	return true;
}

bool CSkeleton::loadBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "rb");

	if (!file)
		return false;

	byte countBytes[4];

	if (file->read(countBytes, sizeof(countBytes)) != sizeof(countBytes)) {
		warning("CSkeleton::loadBinary(): cannot read the node count");
		gpFileServices->close(file, true);

		return false;
	}

	int count = READ_LE_INT32(countBytes);
	int fileSize = gpFileServices->getSize(path);

	if (count < 0 || (fileSize > 0 && count > (fileSize - 4) / kBinaryNodeSize)) {
		warning("CSkeleton::loadBinary(): invalid node count %d", count);
		gpFileServices->close(file, true);

		return false;
	}

	nodes.resize(count);

	for (int i = 0; i < count; i++) {
		byte record[kBinaryNodeSize];

		if (file->read(record, sizeof(record)) != sizeof(record)) {
			warning("CSkeleton::loadBinary(): unexpected end of file");
			gpFileServices->close(file, true);

			return false;
		}

		memcpy(nodes[i].name, record, SkeletonNode::kNameSize);
		memcpy(nodes[i].parent, record + 32, SkeletonNode::kNameSize);
		nodes[i].pos.x = READ_LE_FLOAT32(record + 64);
		nodes[i].pos.y = READ_LE_FLOAT32(record + 68);
		nodes[i].pos.z = READ_LE_FLOAT32(record + 72);
		nodes[i].quat.x = READ_LE_FLOAT32(record + 76);
		nodes[i].quat.y = READ_LE_FLOAT32(record + 80);
		nodes[i].quat.z = READ_LE_FLOAT32(record + 84);
		nodes[i].quat.w = READ_LE_FLOAT32(record + 88);
	}

	gpFileServices->close(file, true);

	return true;
}

bool CSkeleton::saveBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "wb");

	if (!file)
		return false;

	byte record[kBinaryNodeSize];

	WRITE_LE_INT32(record, (int)nodes.size());
	file->write(record, 4);

	for (uint i = 0; i < nodes.size(); i++) {
		memcpy(record, nodes[i].name, SkeletonNode::kNameSize);
		memcpy(record + 32, nodes[i].parent, SkeletonNode::kNameSize);
		WRITE_LE_FLOAT32(record + 64, nodes[i].pos.x);
		WRITE_LE_FLOAT32(record + 68, nodes[i].pos.y);
		WRITE_LE_FLOAT32(record + 72, nodes[i].pos.z);
		WRITE_LE_FLOAT32(record + 76, nodes[i].quat.x);
		WRITE_LE_FLOAT32(record + 80, nodes[i].quat.y);
		WRITE_LE_FLOAT32(record + 84, nodes[i].quat.z);
		WRITE_LE_FLOAT32(record + 88, nodes[i].quat.w);
		file->write(record, sizeof(record));
	}

	gpFileServices->close(file, true);

	return true;
}

} // End of namespace Grim
