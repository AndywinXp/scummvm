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

#include "engines/grim/emi/nodes/cskeleton.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lformatparser.h"

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
	explicit CSkeletonParser(EmiEngine *engine) : LFormatParser(engine) {
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
	memset(_name, 0, sizeof(_name));
	memset(_parent, 0, sizeof(_parent));
}

CSkeleton::CSkeleton(EmiEngine *engine) : _engine(engine) {
}

CSkeleton::~CSkeleton() {
}

bool CSkeleton::save(LFile *file) {
	CSkeletonParser parser(_engine);

	parser.setFile(file);
	parser.writeVersion(0, 0);
	parser.writeEntry(kKeywordNumNodes, (int)_nodes.size());

	for (uint i = 0; i < _nodes.size(); i++) {
		parser.writeComment("Node:");
		parser.writeEntry(kKeywordName, _nodes[i]._name);
		parser.writeEntry(kKeywordParent, _nodes[i]._parent);
		parser.writeEntry(kKeywordPos, _nodes[i]._pos._x, _nodes[i]._pos._y, _nodes[i]._pos._z);
		parser.writeEntry(kKeywordQuat, _nodes[i]._quat._x, _nodes[i]._quat._y, _nodes[i]._quat._z, _nodes[i]._quat._w);
	}

	parser.writeEndOfSection();

	return true;
}

bool CSkeleton::restore(LFile *file) {
	CSkeletonParser parser(_engine);
	int count = 0;
	char token[kTokenSize];

	parser.setFile(file);
	parser.readVersion(0, 0);
	parser.readEntry(kKeywordNumNodes, &count);

	if (count < 0) {
		warning("CSkeleton::restore(): invalid node count %d", count);

		return false;
	}

	_nodes.resize(count);

	for (int i = 0; i < count; i++) {
		parser.readEntry(kKeywordName, token);
		copyNodeName(_nodes[i]._name, token);

		parser.readEntry(kKeywordParent, token);
		copyNodeName(_nodes[i]._parent, token);

		parser.readEntry(kKeywordPos, &_nodes[i]._pos._x, &_nodes[i]._pos._y, &_nodes[i]._pos._z);
		parser.readEntry(kKeywordQuat, &_nodes[i]._quat._x, &_nodes[i]._quat._y, &_nodes[i]._quat._z, &_nodes[i]._quat._w);
	}

	parser.readEndOfSection();

	return true;
}

bool CSkeleton::loadBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "rb");

	if (!file)
		return false;

	byte countBytes[4];

	if (file->read(countBytes, sizeof(countBytes)) != sizeof(countBytes)) {
		warning("CSkeleton::loadBinary(): cannot read the node count");
		_engine->getFileServices()->close(file, true);

		return false;
	}

	int count = READ_LE_INT32(countBytes);
	int fileSize = _engine->getFileServices()->getSize(path);

	if (count < 0 || (fileSize > 0 && count > (fileSize - 4) / kBinaryNodeSize)) {
		warning("CSkeleton::loadBinary(): invalid node count %d", count);
		_engine->getFileServices()->close(file, true);

		return false;
	}

	_nodes.resize(count);

	for (int i = 0; i < count; i++) {
		byte record[kBinaryNodeSize];

		if (file->read(record, sizeof(record)) != sizeof(record)) {
			warning("CSkeleton::loadBinary(): unexpected end of file");
			_engine->getFileServices()->close(file, true);

			return false;
		}

		memcpy(_nodes[i]._name, record, SkeletonNode::kNameSize);
		memcpy(_nodes[i]._parent, record + 32, SkeletonNode::kNameSize);
		_nodes[i]._pos._x = READ_LE_FLOAT32(record + 64);
		_nodes[i]._pos._y = READ_LE_FLOAT32(record + 68);
		_nodes[i]._pos._z = READ_LE_FLOAT32(record + 72);
		_nodes[i]._quat._x = READ_LE_FLOAT32(record + 76);
		_nodes[i]._quat._y = READ_LE_FLOAT32(record + 80);
		_nodes[i]._quat._z = READ_LE_FLOAT32(record + 84);
		_nodes[i]._quat._w = READ_LE_FLOAT32(record + 88);
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

bool CSkeleton::saveBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "wb");

	if (!file)
		return false;

	byte record[kBinaryNodeSize];

	WRITE_LE_INT32(record, (int)_nodes.size());
	file->write(record, 4);

	for (uint i = 0; i < _nodes.size(); i++) {
		memcpy(record, _nodes[i]._name, SkeletonNode::kNameSize);
		memcpy(record + 32, _nodes[i]._parent, SkeletonNode::kNameSize);
		WRITE_LE_FLOAT32(record + 64, _nodes[i]._pos._x);
		WRITE_LE_FLOAT32(record + 68, _nodes[i]._pos._y);
		WRITE_LE_FLOAT32(record + 72, _nodes[i]._pos._z);
		WRITE_LE_FLOAT32(record + 76, _nodes[i]._quat._x);
		WRITE_LE_FLOAT32(record + 80, _nodes[i]._quat._y);
		WRITE_LE_FLOAT32(record + 84, _nodes[i]._quat._z);
		WRITE_LE_FLOAT32(record + 88, _nodes[i]._quat._w);
		file->write(record, sizeof(record));
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

} // End of namespace Grim
