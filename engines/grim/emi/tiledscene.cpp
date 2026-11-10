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

#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/tiledscene.h"
#include "engines/grim/emi/zfile.h"

namespace Grim {

QuadRange::QuadRange(float firstValue, float secondValue) {
	u = firstValue;
	v = secondValue;
}

TiledScene::TiledScene() {
	data = nullptr;
	dataSize = 0;
	frames = nullptr;
	batches = nullptr;
	quads = nullptr;
	uvs = nullptr;
	vertices.count = 0;
	vertices.positions = nullptr;
	vertices.normals = nullptr;
	vertices.colors = nullptr;
	vertices.uvs = nullptr;

	for (int i = 0; i < kHeaderSize; i++)
		header[i] = 0;
}

TiledScene::~TiledScene() {
	if (data)
		gpMemoryServices->free(data);

	data = nullptr;
	dataSize = 0;

	if (quads)
		gpMemoryServices->free(quads);

	quads = nullptr;

	if (uvs)
		gpMemoryServices->free(uvs);

	uvs = nullptr;
}

uint TiledScene::resSizeOfItem() {
	return vertices.getDataSize() + dataSize + sizeof(TiledScene);
}

int TiledScene::getFrameCount() {
	return header[kHeaderNumFrames];
}

bool TiledScene::resLoadItem(const char *itemName) {
	if (strlen(itemName) < 2) {
		warning("TiledScene::resLoadItem(): invalid resource name");

		return false;
	}

	const char *fileName = itemName + 2;
	ZFile *file = ZFileServices::get()->open(fileName, "rb");

	if (!file) {
		if (scumm_stricmp(fileName, "dfltroom.til") != 0)
			return resLoadItem("XXdfltroom.til");

		if (gpConsoleServices)
			gpConsoleServices->logError("Failed miserably.\n");
		else
			warning("TiledScene::resLoadItem(): failed miserably");

		return false;
	}

	byte signature[4];

	if (file->read(signature, sizeof(signature)) != sizeof(signature)) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		ZFileServices::get()->closeZFile(file);

		return false;
	}

	file->seek(0, 0);

	if (READ_LE_UINT32(signature) != kSignature) {
		warning("TiledScene::resLoadItem(): wrong signature");
		ZFileServices::get()->closeZFile(file);

		return true;
	}

	byte headerBytes[kHeaderBytes];

	if (file->read(headerBytes, sizeof(headerBytes)) != sizeof(headerBytes)) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		ZFileServices::get()->closeZFile(file);

		return false;
	}

	for (int i = 0; i < kHeaderSize; i++)
		header[i] = READ_LE_INT32(headerBytes + 4 * i);

	file->seek(0, 0);

	int numTiles = header[kHeaderNumVertices];
	int numFrames = header[kHeaderNumFrames];
	int numBatches = header[kHeaderNumBatches];

	if (header[kHeaderTablesSize] < kHeaderBytes || numTiles < 0 || numFrames < 0 || numBatches < 0) {
		warning("TiledScene::resLoadItem(): invalid header");
		ZFileServices::get()->closeZFile(file);

		return false;
	}

	dataSize = header[kHeaderTablesSize] - header[kHeaderTablesSize] % 16 + 16;

	if ((int64)kHeaderBytes + 16 * (int64)numTiles + 8 * (int64)numFrames + 12 * (int64)numBatches > dataSize) {
		warning("TiledScene::resLoadItem(): the tables do not fit in the file");
		ZFileServices::get()->closeZFile(file);

		return false;
	}

	data = (byte *)gpMemoryServices->malloc(dataSize * sizeof(byte));

	if (file->read(data, dataSize) != (uint)dataSize) {
		warning("TiledScene::resLoadItem(): unexpected end of file");
		ZFileServices::get()->closeZFile(file);

		return false;
	}

	ZFileServices::get()->closeZFile(file);

	quads = (float *)gpMemoryServices->malloc(numTiles * 3 * sizeof(float));
	uvs = (QuadRange *)gpMemoryServices->malloc(numTiles * sizeof(QuadRange));
	frames = (int *)(data + kHeaderBytes + 16 * numTiles);
	batches = (int *)(data + kHeaderBytes + 16 * numTiles + 8 * numFrames);

#if defined(SCUMM_BIG_ENDIAN)
	for (int i = 0; i < 2 * numFrames; i++)
		frames[i] = FROM_LE_32(frames[i]);

	for (int i = 0; i < 3 * numBatches; i++)
		batches[i] = FROM_LE_32(batches[i]);
#endif

	for (int i = 0; i < numTiles; i++) {
		const byte *tile = data + kHeaderBytes + 16 * i;

		quads[3 * i] = READ_LE_FLOAT32(tile);
		quads[3 * i + 1] = READ_LE_FLOAT32(tile + 4);
		quads[3 * i + 2] = 1.0f;
		uvs[i] = QuadRange(READ_LE_FLOAT32(tile + 8), READ_LE_FLOAT32(tile + 12));
	}

	vertices.count = numTiles;
	vertices.positions = quads;
	vertices.normals = nullptr;
	vertices.colors = nullptr;
	vertices.uvs = uvs ? &uvs[0].u : nullptr;

	return true;
}

} // End of namespace Grim
