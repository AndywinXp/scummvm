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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/graphics/tiledscene.h"
#include "engines/grim/emi/files/zfile.h"

namespace Grim {

QuadRange::QuadRange(float firstValue, float secondValue) {
	_u = firstValue;
	_v = secondValue;
}

TiledScene::TiledScene() {
	_data = nullptr;
	_dataSize = 0;
	_frames = nullptr;
	_batches = nullptr;
	_quads = nullptr;
	_uvs = nullptr;
	_vertices.count = 0;
	_vertices.positions = nullptr;
	_vertices.normals = nullptr;
	_vertices.colors = nullptr;
	_vertices.uvs = nullptr;

	for (int i = 0; i < kHeaderSize; i++)
		_header[i] = 0;
}

TiledScene::~TiledScene() {
	if (_data)
		_engine->getMemoryServices()->freeImpl(_data);

	_data = nullptr;
	_dataSize = 0;

	if (_quads)
		_engine->getMemoryServices()->freeImpl(_quads);

	_quads = nullptr;

	if (_uvs)
		_engine->getMemoryServices()->freeImpl(_uvs);

	_uvs = nullptr;
}

uint TiledScene::resSizeOfItem() {
	return _vertices.getDataSize() + _dataSize + sizeof(TiledScene);
}

int TiledScene::getFrameCount() {
	return _header[kHeaderNumFrames];
}

bool TiledScene::resLoadItem(const char *itemName) {
	if (strlen(itemName) < 2) {
		warning("TiledScene::resLoadItem(): invalid resource name");

		return false;
	}

	const char *fileName = itemName + 2;
	ZFileServices *zFileServices = _engine->getZFileServices();
	LConsoleServices *console = _engine->getConsoleServices();
	ZFile *file = zFileServices->open(fileName, "rb");

	if (!file) {
		if (scumm_stricmp(fileName, "dfltroom.til") != 0)
			return resLoadItem("XXdfltroom.til");

		if (console)
			console->logError("Failed miserably.\n");
		else
			warning("TiledScene::resLoadItem(): failed miserably");

		return false;
	}

	byte signature[4];

	if (file->read(signature, sizeof(signature)) != sizeof(signature)) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		zFileServices->closeZFile(file);

		return false;
	}

	file->seek(0, 0);

	if (READ_LE_UINT32(signature) != kSignature) {
		warning("TiledScene::resLoadItem(): wrong signature");
		zFileServices->closeZFile(file);

		return true;
	}

	byte headerBytes[kHeaderBytes];

	if (file->read(headerBytes, sizeof(headerBytes)) != sizeof(headerBytes)) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		zFileServices->closeZFile(file);

		return false;
	}

	for (int i = 0; i < kHeaderSize; i++)
		_header[i] = READ_LE_INT32(headerBytes + 4 * i);

	file->seek(0, 0);

	int numTiles = _header[kHeaderNumVertices];
	int numFrames = _header[kHeaderNumFrames];
	int numBatches = _header[kHeaderNumBatches];

	if (_header[kHeaderTablesSize] < kHeaderBytes || numTiles < 0 || numFrames < 0 || numBatches < 0) {
		warning("TiledScene::resLoadItem(): invalid header");
		zFileServices->closeZFile(file);

		return false;
	}

	_dataSize = _header[kHeaderTablesSize] - _header[kHeaderTablesSize] % 16 + 16;

	if ((int64)kHeaderBytes + 16 * (int64)numTiles + 8 * (int64)numFrames + 12 * (int64)numBatches > _dataSize) {
		warning("TiledScene::resLoadItem(): the tables do not fit in the file");
		zFileServices->closeZFile(file);

		return false;
	}

	_data = (byte *)_engine->getMemoryServices()->mallocImpl(_dataSize * sizeof(byte));

	if (file->read(_data, _dataSize) != (uint)_dataSize) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		zFileServices->closeZFile(file);

		return false;
	}

	zFileServices->closeZFile(file);

	_quads = (float *)_engine->getMemoryServices()->mallocImpl(numTiles * 3 * sizeof(float));
	_uvs = (QuadRange *)_engine->getMemoryServices()->mallocImpl(numTiles * sizeof(QuadRange));
	_frames = (int *)(_data + kHeaderBytes + 16 * numTiles);
	_batches = (int *)(_data + kHeaderBytes + 16 * numTiles + 8 * numFrames);

#if defined(SCUMM_BIG_ENDIAN)
	for (int i = 0; i < 2 * numFrames; i++)
		_frames[i] = FROM_LE_32(_frames[i]);

	for (int i = 0; i < 3 * numBatches; i++)
		_batches[i] = FROM_LE_32(_batches[i]);
#endif

	for (int i = 0; i < numTiles; i++) {
		const byte *tile = _data + kHeaderBytes + 16 * i;

		_quads[3 * i] = READ_LE_FLOAT32(tile);
		_quads[3 * i + 1] = READ_LE_FLOAT32(tile + 4);
		_quads[3 * i + 2] = 1.0f;
		_uvs[i] = QuadRange(READ_LE_FLOAT32(tile + 8), READ_LE_FLOAT32(tile + 12));
	}

	_vertices.count = numTiles;
	_vertices.positions = _quads;
	_vertices.normals = nullptr;
	_vertices.colors = nullptr;
	_vertices.uvs = _uvs ? &_uvs[0]._u : nullptr;

	return true;
}

} // End of namespace Grim
