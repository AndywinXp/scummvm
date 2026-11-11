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

#include "common/savefile.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "graphics/pixelformat.h"
#include "graphics/surface.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/lcolorformat.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/textevent.h"
#include "engines/grim/emi/zfile.h"
#include "engines/grim/legacy_savegames/legacy_savegames.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

struct SpecScreenRect {
	int x;
	int y;
	int width;
	int height;
};

static const int kSavedTextBufferSize = 512;
static const int kThumbnailTextureWidth = 256;
static const int kThumbnailTextureHeight = 128;
static const int kThumbnailImageWidth = 160;
static const int kThumbnailImageHeight = 120;
static const int kScreenWidth = 640;
static const int kScreenBufferHeight = 512;
static const int kNumSpecScreenTextures = 8;
static const uint kSpecScreenTextureFlags = 775;
static const uint kMaxSaveTableSize = 0x10000;

static const SpecScreenRect kSpecScreenRects[kNumSpecScreenTextures] = {
	{ 0, 0, 256, 256 },
	{ 256, 0, 256, 256 },
	{ 512, 0, 128, 128 },
	{ 512, 128, 128, 128 },
	{ 0, 256, 256, 256 },
	{ 256, 256, 256, 256 },
	{ 512, 256, 128, 128 },
	{ 512, 384, 128, 128 }
};

ZFile *EmiEngine::_saveFile = nullptr;
LABitmap *EmiEngine::_screenBitmap = nullptr;

Common::Error EmiEngine::loadGameState(int slot) {
	assert(slot >= 0);

	loadLib(Common::String::format("Saves/efmi%03d.gsv", slot).c_str());

	return Common::kNoError;
}

Common::String EmiEngine::getSaveFileName(const char *path) {
	Common::String name = path;

	for (uint i = 0; i < name.size(); i++) {
		if (name[i] == '\\')
			name.setChar('/', i);
	}

	while (name.hasPrefix("./"))
		name = name.substr(2);

	if (name.hasPrefixIgnoreCase("saves/"))
		name = name.substr(6);

	return name;
}

LFile *EmiEngine::openSaveFile(const char *fileName, const char *mode) {
	if (!fileName || !*fileName) {
		warning("EmiEngine::openSaveFile(): no file name");

		return nullptr;
	}

	Common::String saveName = getSaveFileName(fileName);
	Common::SaveFileManager *saveFileManager = g_system->getSavefileManager();
	LFile *file = new LFile();

	if (strchr(mode, 'w')) {
		file->writeStream = saveFileManager->openForSaving(saveName, false);
		file->openFlags = 18;
	} else {
		file->readStream = saveFileManager->openRawFile(saveName);
	}

	if (!file->isOpen()) {
		delete file;

		return nullptr;
	}

	file->fileName = saveName;

	ZFile *zFile = ZFileServices::get()->createZFile(file, mode);

	if (!zFile) {
		delete file;

		return nullptr;
	}

	return zFile;
}

void EmiEngine::closeSaveFile(LFile *file) {
	if (file)
		ZFileServices::get()->closeZFile(static_cast<ZFile *>(file));
}

int EmiEngine::writeSaveData(void *data, int bytes) {
	if (bytes) {
		if (!_saveFile) {
			warning("EmiEngine::writeSaveData(): no save file");

			return 1;
		}

		_saveFile->writeCompressed(data, bytes);
	}

	return 1;
}

int EmiEngine::readSaveData(void *data, int bytes) {
	if (bytes) {
		if (!_saveFile) {
			warning("EmiEngine::readSaveData(): no save file");

			return 1;
		}

		_saveFile->read(data, bytes);
	}

	return 1;
}

bool EmiEngine::readSavedText(char *buffer, int bufferSize) {
	int length = 0;

	readSaveData(&length, sizeof(length));

	if (length < 0 || length > bufferSize) {
		warning("EmiEngine::readSavedText(): invalid text length %d", length);
		buffer[0] = 0;

		return false;
	}

	readSaveData(buffer, length);

	if (length == 0) {
		buffer[0] = 0;
	} else if (buffer[length - 1] != 0) {
		warning("EmiEngine::readSavedText(): the text is not terminated");
		buffer[length - 1] = 0;
	}

	return true;
}

LABitmap *EmiEngine::getScreenBitmap() {
	if (!_screenBitmap)
		_screenBitmap = new LABitmap();

	return _screenBitmap;
}

void EmiEngine::halveBitmap(LABitmap *source, LABitmap *destination) {
	LColorFormat rgbFormat(6);
	int bytesPerPixel = rgbFormat.bytesPerPixel;
	int sourceStride = bytesPerPixel * source->getWidth();
	int halfWidth = (int)((uint)source->getWidth() >> 1);
	int halfHeight = (int)((uint)source->getHeight() >> 1);

	destination->create(halfWidth, halfHeight, rgbFormat, nullptr);

	const byte *first = (const byte *)source->getPixels();
	byte *target = (byte *)destination->getPixels();

	if (halfHeight > 0 && (!first || !target)) {
		warning("EmiEngine::halveBitmap(): the bitmap has no pixels");

		return;
	}

	for (int y = 0; y < halfHeight; y++) {
		if (halfWidth > 0) {
			const byte *second = first + sourceStride;

			for (int x = 0; x < halfWidth; x++) {
				const byte *firstNext = first + bytesPerPixel;
				const byte *secondNext = second + bytesPerPixel;

				int alpha = rgbFormat.getAlpha(first);
				int red = rgbFormat.getRed(first);
				int green = rgbFormat.getGreen(first);
				int blue = rgbFormat.getBlue(first);

				alpha += rgbFormat.getAlpha(second);
				red += rgbFormat.getRed(second);
				green += rgbFormat.getGreen(second);
				blue += rgbFormat.getBlue(second);

				alpha += rgbFormat.getAlpha(firstNext);
				red += rgbFormat.getRed(firstNext);
				green += rgbFormat.getGreen(firstNext);
				blue += rgbFormat.getBlue(firstNext);

				alpha += rgbFormat.getAlpha(secondNext);
				red += rgbFormat.getRed(secondNext);
				green += rgbFormat.getGreen(secondNext);
				blue += rgbFormat.getBlue(secondNext);

				first = firstNext + bytesPerPixel;
				second = secondNext + bytesPerPixel;

				rgbFormat.packPixel(target, ((uint)alpha + 1) >> 2, ((uint)red + 1) >> 2, ((uint)green + 1) >> 2, ((uint)blue + 1) >> 2);
				target += bytesPerPixel;
			}
		}

		first += sourceStride;
	}
}

bool EmiEngine::grabScreen() {
	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext) {
		warning("EmiEngine::grabScreen(): no draw context");

		return false;
	}

	drawContext->readBackBuffer(getScreenBitmap());

	return true;
}

bool EmiEngine::makeThumbnailTexture(int index, bool fromScreen) {
	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext) {
		warning("EmiEngine::makeThumbnailTexture(): no draw context");

		return false;
	}

	LABitmap screen;
	LColorFormat rgbFormat(6);

	drawContext->readBackBuffer(&screen);

	if (fromScreen)
		grabScreen();

	screen.convertFormat(rgbFormat);

	LABitmap halved;

	halveBitmap(&screen, &halved);
	halveBitmap(&halved, &screen);

	int width = 1;

	while (width < screen.width)
		width *= 2;

	int height = 1;

	while (height < screen.height)
		height *= 2;

	LABitmap texture(width, height, screen.colorFormat, nullptr, false);
	const byte *sourcePixels = (const byte *)screen.pixels;
	byte *targetPixels = (byte *)texture.pixels;

	if (screen.height > 0 && (!sourcePixels || !targetPixels)) {
		warning("EmiEngine::makeThumbnailTexture(): the bitmap has no pixels");

		return false;
	}

	for (int row = 0; row < screen.height; row++)
		memcpy(targetPixels + row * texture.stride, sourcePixels + row * screen.stride, screen.stride);

	return drawContext->setCachedTexture(index, &texture);
}

bool EmiEngine::loadThumbnailTexture(int index, const char *fileName) {
	LABitmap bitmap;
	LColorFormat rgbFormat(6);

	bitmap.create(kThumbnailTextureWidth, kThumbnailTextureHeight, rgbFormat, nullptr);

	_saveFile = static_cast<ZFile *>(openSaveFile(fileName, "rb"));

	if (!_saveFile)
		return false;

	int tableSize = 0;
	char version[kSavedTextBufferSize + 1];

	readSaveData(&tableSize, sizeof(tableSize));
	_saveFile->seek(tableSize, 1);
	readSavedText(version, kSavedTextBufferSize);

	uint imageSize = 0;

	readSaveData(&imageSize, sizeof(imageSize));

	if (imageSize > (uint)bitmap.sizeBytes) {
		warning("EmiEngine::loadThumbnailTexture(): invalid image size %u", imageSize);
		imageSize = bitmap.sizeBytes;
	}

	readSaveData(bitmap.pixels, imageSize);
	closeSaveFile(_saveFile);
	_saveFile = nullptr;

	if (!CDrawContext::context) {
		warning("EmiEngine::loadThumbnailTexture(): no draw context");

		return false;
	}

	CDrawContext::context->setCachedTexture(index, &bitmap);

	return true;
}

bool EmiEngine::buildSpecScreenTextures(int index) {
	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext) {
		warning("EmiEngine::buildSpecScreenTextures(): no draw context");

		return false;
	}

	int bytesPerPixel = drawContext->displayMode.bpp / 8;
	uint bufferSize = (uint)(kScreenWidth * kScreenBufferHeight * bytesPerPixel) * sizeof(char);
	char *buffer = (char *)gpMemoryServices->malloc(bufferSize);

	if (!buffer) {
		warning("EmiEngine::buildSpecScreenTextures(): out of memory");

		return false;
	}

	// The disasm leaves the rows below the back buffer uninitialized
	memset(buffer, 0, bufferSize);
	drawContext->readBackBufferRaw(buffer, bufferSize);

	LColorFormat format(bytesPerPixel == 4 ? LColorFormat::format9 : LColorFormat::format5);

	for (int i = 0; i < kNumSpecScreenTextures; i++) {
		const SpecScreenRect &rect = kSpecScreenRects[i];

		drawContext->setCachedTexture(index + i, rect.width, rect.height, format, nullptr, "SpecScreenTex");

		CTexture *texture = drawContext->getCachedTexture(index + i);

		if (texture) {
			texture->updateTexture(&format, rect.width, rect.height, kScreenWidth * bytesPerPixel,
					(const byte *)&buffer[bytesPerPixel * (rect.x + kScreenWidth * rect.y)]);
			texture->flags = kSpecScreenTextureFlags;
			texture->applyFlags();
		}
	}

	gpMemoryServices->free(buffer);

	return true;
}

bool EmiEngine::clearTextureSlot(int index) {
	if (!CDrawContext::context) {
		warning("EmiEngine::clearTextureSlot(): no draw context");

		return false;
	}

	LABitmap bitmap;
	LColorFormat format(9);

	bitmap.create(2, 2, format, nullptr);

	uint32 *pixels = (uint32 *)bitmap.pixels;

	if (pixels) {
		for (int i = 0; i < 4; i++)
			pixels[i] = 0xFFFFFFFF;
	}

	return CDrawContext::context->setCachedTexture(index, &bitmap);
}

void EmiEngine::saveGame(CDrawContext *drawContext) {
	_libSaveRequest = 0;
	setSaveLoadIdle(false);

	_saveFile = static_cast<ZFile *>(openSaveFile(_libSaveGameFileName.c_str(), "wb"));

	if (_saveFile) {
		Lua_V2::saveGameCallback(writeSaveData);

		LString version;

		Lua_V2::getVersion(version);

		int length = (int)version.size() + 1;

		writeSaveData(&length, sizeof(length));
		writeSaveData((void *)version.c_str(), length);

		LColorFormat rgbFormat(6);
		LABitmap *screen = getScreenBitmap();

		screen->convertFormat(rgbFormat);

		LABitmap halved;

		halveBitmap(screen, &halved);
		halveBitmap(&halved, screen);

		int width = 1;

		while (width < screen->width)
			width *= 2;

		int height = 1;

		while (height < screen->height)
			height *= 2;

		LABitmap thumbnail(width, height, screen->colorFormat, nullptr, false);
		const byte *sourcePixels = (const byte *)screen->pixels;
		byte *targetPixels = (byte *)thumbnail.pixels;

		if (screen->height > 0 && (!sourcePixels || !targetPixels)) {
			warning("EmiEngine::saveGame(): the screenshot has no pixels");
		} else {
			for (int row = 0; row < screen->height; row++)
				memcpy(targetPixels + row * thumbnail.stride, sourcePixels + row * screen->stride, screen->stride);
		}

		int imageSize = thumbnail.sizeBytes;

		writeSaveData(&imageSize, sizeof(imageSize));
		writeSaveData(thumbnail.pixels, imageSize);

		length = (int)_island.size() + 1;

		writeSaveData(&length, sizeof(length));
		writeSaveData((void *)_island.c_str(), length);

		Room_v2::saveCurrentRoom(writeSaveData);
		Actor::saveActors(writeSaveData);
		TextEvent::saveAll(writeSaveData);
		Sounds::saveSounds(writeSaveData);
		Sounds::saveMusicState(writeSaveData);
		Lua_V2::saveLocks(writeSaveData);
		lua_Save(writeSaveData);
	}

	closeSaveFile(_saveFile);
	_saveFile = nullptr;
	setSaveLoadIdle(true);
}

void EmiEngine::loadGame(CDrawContext *drawContext) {
	_libLoadRequest = 0;
	_saveFile = static_cast<ZFile *>(openSaveFile(_libSaveGameFileName.c_str(), "rb"));

	if (_saveFile) {
		int tableSize = 0;
		char text[kSavedTextBufferSize + 1];

		readSaveData(&tableSize, sizeof(tableSize));
		_saveFile->seek(tableSize, 1);
		readSavedText(text, kSavedTextBufferSize);

		setSaveLoadIdle(false);
		Lua_V2::static_NukeAllScriptLocks();
		Sounds::stopAllMusic();
		Sounds::releaseAllSounds();

		ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

		if (resourceManager) {
			Overworld::getInstance()->clearActors();
			Actor::freeAllActors();
			Room_v2::loadRoom(nullptr);

			if (!_island.empty())
				CBundleServices::closeIslandBundles(_island.c_str());

			resourceManager->purgeAll();

			int imageSize = 0;

			readSaveData(&imageSize, sizeof(imageSize));
			_saveFile->seek(imageSize, 1);
			readSavedText(text, kSavedTextBufferSize);
			_island = text;

			if (!_island.empty())
				CBundleServices::openIslandBundles(_island.c_str());

			Room_v2::restoreCurrentRoom(readSaveData);
			Actor::restoreActors(readSaveData);
			TextEvent::restoreAll(readSaveData);
			Sounds::restoreSounds(readSaveData);
			Sounds::restoreMusicState(readSaveData);
			Lua_V2::restoreLocks(readSaveData);
			Sounds::releaseUnlockedSounds();
			lua_Restore(readSaveData);
			Lua_V2::updateJoySelected();
			_libJustLoaded = 1;
		}
	}

	closeSaveFile(_saveFile);
	_saveFile = nullptr;
	setSaveLoadIdle(true);
}

bool EmiEngine::isLegacySaveFile(const Common::String &fileName) {
	Common::InSaveFile *stream = g_system->getSavefileManager()->openForLoading(fileName);

	if (!stream)
		return false;

	bool legacy = LegacySaveFile::isLegacySave(stream);

	delete stream;

	return legacy;
}

bool EmiEngine::readSaveStrings(Common::SeekableReadStream *stream, Common::Array<Common::String> &strings) {
	strings.clear();

	if (LegacySaveFile::isLegacySave(stream))
		return false;

	int32 totalBytes = stream->readSint32LE();

	if (stream->err() || stream->eos() || totalBytes < 0 || (uint)totalBytes > kMaxSaveTableSize)
		return false;

	while (totalBytes > 0) {
		int32 length = stream->readSint32LE();

		if (stream->err() || stream->eos() || length < 1 || length > totalBytes - 4)
			return false;

		Common::Array<char> text(length);

		if (stream->read(text.data(), length) != (uint32)length)
			return false;

		text[length - 1] = '\0';
		strings.push_back(Common::String(text.data()));
		totalBytes -= length + 4;
	}

	return true;
}

Graphics::Surface *EmiEngine::readSaveThumbnail(Common::SeekableReadStream *stream) {
	int32 versionLength = stream->readSint32LE();

	if (stream->err() || stream->eos() || versionLength < 0 || versionLength > kSavedTextBufferSize)
		return nullptr;

	stream->seek(versionLength, SEEK_CUR);

	uint32 imageSize = stream->readUint32LE();
	uint32 expectedSize = kThumbnailTextureWidth * kThumbnailTextureHeight * 3;

	if (stream->err() || stream->eos() || imageSize != expectedSize)
		return nullptr;

	Common::Array<byte> pixels(imageSize);

	if (stream->read(pixels.data(), imageSize) != imageSize)
		return nullptr;

	Graphics::PixelFormat format(4, 8, 8, 8, 8, 24, 16, 8, 0);
	Graphics::Surface *thumbnail = new Graphics::Surface();

	thumbnail->create(kThumbnailImageWidth, kThumbnailImageHeight, format);

	for (int y = 0; y < kThumbnailImageHeight; y++) {
		const byte *source = &pixels[y * kThumbnailTextureWidth * 3];
		uint32 *target = (uint32 *)thumbnail->getBasePtr(0, y);

		for (int x = 0; x < kThumbnailImageWidth; x++)
			target[x] = format.RGBToColor(source[3 * x], source[3 * x + 1], source[3 * x + 2]);
	}

	return thumbnail;
}

Common::String EmiEngine::getSaveDescription(const Common::Array<Common::String> &strings, int slot) {
	if (strings.empty() || strings[0].empty())
		return Common::String::format("Slot %d", slot);

	return strings[0];
}

} // End of namespace Grim
