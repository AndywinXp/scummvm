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
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/background.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/tiledscene.h"
#include "engines/grim/emi/zfile.h"

namespace Grim {

Background::Background() {
	bmData = nullptr;

	for (int i = 0; i < kMaxTextures; i++)
		textures[i] = nullptr;
}

Background::~Background() {
	if (bmData)
		BmData::free(_engine, bmData);

	bmData = nullptr;

	CDrawContext *drawContext = _engine->getDrawContext();

	for (int i = 0; i < kMaxTextures; i++) {
		if (!textures[i])
			continue;

		if (drawContext && textures[i] == drawContext->defaultTexture)
			warning("Background::~Background(): texture is the default texture");
		else
			delete textures[i];

		textures[i] = nullptr;
	}
}

uint Background::resSizeOfItem() {
	uint size = BmData::getSize(bmData) + sizeof(Background);

	for (int i = 0; i < kMaxTextures; i++) {
		if (textures[i])
			size += textures[i]->getSize();
	}

	return size;
}

CTexture *Background::getTexture(int index) {
	int slot = index % kMaxTextures;

	if (slot < 0) {
		warning("Background::getTexture(): negative texture index %d", index);

		return nullptr;
	}

	return textures[slot];
}

bool Background::resLoadItem(const char *itemName) {
	if (strlen(itemName) < 2) {
		warning("Background::resLoadItem(): invalid resource name");

		return false;
	}

	CDrawContext *drawContext = _engine->getDrawContext();
	LConsoleServices *console = _engine->getConsoleServices();
	ZFileServices *zFileServices = _engine->getZFileServices();

	if (!drawContext) {
		warning("Background::resLoadItem(): no draw context");

		return false;
	}

	bool isPS2 = _engine->getGamePlatform() == Common::kPlatformPS2;

	// PS2 doesn't update sounds while loading backgrounds
	if (!isPS2) {
		Sounds *sounds = _engine->getSounds();

		if (sounds->getAudioMixAhead() < 64) {
			int16 mixAhead = 64;

			sounds->setAudioMixAhead(&mixAhead, false);
		}

		bool forceUpdate = false;

		sounds->soundsUpdateThrottled(&forceUpdate);
	}

	const char *fileName = itemName + 2;
	ZFile *file = zFileServices->open(fileName, "rb");

	if (!file) {
		if (isPS2 && console)
			console->logWarning("Cannot find file %s.\n", fileName);

		if (scumm_stricmp(fileName, "dfltroom.til") != 0) {
			if (isPS2 && console)
				console->logWarning("Trying default .TIL file %s.\n", "dfltroom.til");

			return resLoadItem("XXdfltroom.til");
		}

		if (console)
			console->logError("Failed miserably.\n");
		else
			warning("Background::resLoadItem(): failed miserably");

		return false;
	}

	byte signature[4];

	if (file->read(signature, sizeof(signature)) != sizeof(signature)) {
		warning("Background::resLoadItem(): unexpected end of file");
		zFileServices->closeZFile(file);

		return false;
	}

	file->seek(0, 0);

	if (READ_LE_UINT32(signature) != TiledScene::kSignature) {
		zFileServices->closeZFile(file);

		return false;
	}

	byte headerBytes[TiledScene::kHeaderBytes];

	if (file->read(headerBytes, sizeof(headerBytes)) != sizeof(headerBytes)) {
		warning("Background::resLoadItem(): unexpected end of file");
		zFileServices->closeZFile(file);

		return false;
	}

	file->seek(READ_LE_INT32(headerBytes + 4 * TiledScene::kHeaderTablesSize), 0);

	bmData = (BmData *)_engine->getMemoryServices()->mallocImpl(sizeof(BmData));

	if (bmData) {
		Common::strlcpy((char *)bmData->itemName, itemName, sizeof(bmData->itemName));

		char *extension = strrchr((char *)bmData->itemName, '.');

		if (extension)
			*extension = 0;

		if (!bmData->load(_engine, file)) {
			_engine->getMemoryServices()->freeImpl(bmData);
			bmData = nullptr;
		}
	}

	zFileServices->closeZFile(file);

	if (!bmData) {
		if (isPS2 && console)
			console->logDebug("No background art available for this setup!\n");

		return false;
	}

	for (uint i = 0; i < bmData->numImages; i++) {
		if (i == kMaxTextures)
			break;

		if (textures[i])
			continue;

		if (isPS2 && bmData->bitsPerPixel != 32) {
			int width = bmData->images[i].width;
			int height = bmData->images[i].height;
			LABitmap bitmap(_engine, width, height, LColorFormat::format9, nullptr, false);

			convertPs2Pixels(bmData->images[i].data, width * height, (byte *)bitmap.getPixels());
			textures[i] = drawContext->createTexture(&bitmap);
		} else {
			LABitmap bitmap(_engine, bmData->images[i].width, bmData->images[i].height, LColorFormat::format9, bmData->images[i].data, false);

			if (drawContext->displayMode.bpp == 16 && _engine->_layer16BitFix)
				bitmap.ditherPixels();

			textures[i] = drawContext->createTexture(&bitmap);
		}

		if (textures[i]) {
			textures[i]->flags = isPS2 ? 0x04 : 0x84;
			textures[i]->applyFlags();
		}
	}

	BmData::free(_engine, bmData);

	return true;
}

// PS2 pixels are 16-bit GS textures: red in the low bits, alpha in bit 15
void Background::convertPs2Pixels(const byte *source, int count, byte *dest) {
	if (!source || !dest)
		return;

	for (int i = 0; i < count; i++) {
		uint pixel = READ_LE_UINT16(source + i * 2);

		dest[i * 4 + 0] = (pixel & 0x1F) << 3;
		dest[i * 4 + 1] = ((pixel >> 5) & 0x1F) << 3;
		dest[i * 4 + 2] = ((pixel >> 10) & 0x1F) << 3;
		dest[i * 4 + 3] = (pixel & 0x8000) ? 0xFF : 0x00;
	}
}

} // End of namespace Grim
