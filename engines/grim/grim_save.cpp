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
#include "common/savefile.h"
#include "common/serializer.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

static const StdColorFormat g_saveImageFormat = { 1, 16, 5, 6, 5, 11, 5, 0, 3, 2, 3, 0, 0, 0 };

static const int kSoundsSaveVersion = 30;

void GrimEngine::save() {
	_libSaveRequest = 0;
	_pSputRender->allowLoadImage(0);
	_pSputRender->drawProgressBar(0.1f);

	Common::String fileName = _libSaveGameFileName;

	if (fileName.empty())
		fileName = "grim.sav";

	_pSaveWriteStream = g_system->getSavefileManager()->openForSaving(fileName);

	if (!_pSaveWriteStream)
		return;

	// TODO: sputLua_SaveGameCallback(zwrite)

	StdVBuffer *pShot = _pSputRender->screenShot(250, 188);
	uint32 imageSize = 0;

	if (pShot) {
		StdColorFormat format = g_saveImageFormat;
		StdVBuffer *pImage = _pStdDisplay->vBufferConvertColorFormat(&format, pShot);

		if (pImage) {
			imageSize = TO_LE_32((uint32)pImage->tex.sizeInBytes);
			zwrite(&imageSize, 4);
			zwrite(pImage->pPixels, pImage->tex.sizeInBytes);
			_pStdDisplay->vBufferFree(pImage);
		}
	} else {
		warning("GrimEngine::save(): no screenshot for the savegame");
	}

	if (!imageSize)
		zwrite(&imageSize, 4);

	_pSputRender->drawProgressBar(0.2f);
	_pSputCostume->getCos()->save(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.3f);
	_pRes->sysSave(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.4f);
	_pSputText->save(_pSaveWriteStream);
	_pSputRoom->save(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.5f);
	_pSputActor->save(_pSaveWriteStream);
	_pSputRender->save(_pSaveWriteStream);
	_pSputPrimitive->save(_pSaveWriteStream);
	_pSputSmush->save(_pSaveWriteStream);

	uint32 prevMode = TO_LE_32((uint32)_sputPrevMode);

	zwrite(&prevMode, 4);
	_pSputRender->drawProgressBar(0.75f);
	// TODO: lua_Save(zwrite)
	_pSputRender->drawProgressBar(1.0f);
	soundsSave(_pSaveWriteStream);

	_pSaveWriteStream->finalize();
	delete _pSaveWriteStream;
	_pSaveWriteStream = nullptr;

	_pSputRender->allowLoadImage(1);
}

int GrimEngine::zwrite(const void *buffer, int size) {
	if (size && _pSaveWriteStream->write(buffer, size) != (uint32)size)
		error("GrimEngine::zwrite(): write error");

	return 0;
}

void GrimEngine::restore() {
	_libLoadRequest = 0;

	Common::String fileName = _libSaveGameFileName;

	if (fileName.empty())
		fileName = "grim.sav";

	_pSaveReadStream = g_system->getSavefileManager()->openForLoading(fileName);

	if (!_pSaveReadStream)
		return;

	_pSputRender->allowLoadImage(0);
	_pSputRender->drawProgressBar(0.1f);
	_pImuse->imStopAllSounds();
	_pSputSmush->stopMovie();
	_pSputSmush->periodic();
	_pRes->freeAll();
	_pSputActor->clearShadowMaterial();
	_pSputResource->stopPurge(1);

	uint32 blockSize = 0;

	zread(&blockSize, 4);
	_pSaveReadStream->seek((int)FROM_LE_32(blockSize), SEEK_CUR);

	uint32 imageSize = 0;

	zread(&imageSize, 4);
	_pSaveReadStream->seek((int)FROM_LE_32(imageSize), SEEK_CUR);

	_pSputRender->drawProgressBar(0.2f);
	_pSputCostume->getCos()->restore(_pSaveReadStream);
	_pSputRender->drawProgressBar(0.3f);
	_pRes->sysLoad(_pSaveReadStream);
	_pSputText->restore(_pSaveReadStream);
	_pSputRender->drawProgressBar(0.4f);
	_pSputRoom->restore(_pSaveReadStream);
	_pSputActor->restore(_pSaveReadStream);
	_pSputRender->restore(_pSaveReadStream);
	_pSputPrimitive->restore(_pSaveReadStream);
	_pSputSmush->restore(_pSaveReadStream);

	_libMode = 4;

	uint32 prevMode = 0;

	zread(&prevMode, 4);
	_sputPrevMode = (int)FROM_LE_32(prevMode);
	_pSputRender->drawProgressBar(0.75f);
	// TODO: lua_Restore(zread)
	_pSputRender->drawProgressBar(1.0f);
	soundsRestore(_pSaveReadStream);
	_pSputResource->stopPurge(0);

	Room *pRoom = _pSputRoom->getCurrentRoom();
	int curSetup = 0;

	if (pRoom)
		curSetup = pRoom->curSetup;

	_pSputRender->useSetup(pRoom, curSetup);

	delete _pSaveReadStream;
	_pSaveReadStream = nullptr;

	// TODO: lua_dofile("patch05.bin")
	_libJustLoaded = 1;
	_pSputRender->allowLoadImage(1);

	StdRect rect = { 0, 0, 640, 480 };

	_pSputRender->markAllPagesDirty(&rect, 1);
}

int GrimEngine::zread(void *buffer, int size) {
	if (size)
		_pSaveReadStream->read(buffer, size);

	return 0;
}

StdVBuffer *GrimEngine::getSaveGameImage(const char *fileName) {
	Common::SeekableReadStream *pFile = g_system->getSavefileManager()->openForLoading(fileName);

	if (!pFile)
		return nullptr;

	int blockSize = pFile->readSint32LE();

	pFile->seek(blockSize, SEEK_CUR);

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.pitchInPixels = 2;
	format.width = 250;
	format.height = 188;
	format.pitchInBytes = 500;
	format.format = g_saveImageFormat;

	StdVBuffer *pImage = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

	if (!pImage) {
		delete pFile;
		return nullptr;
	}

	int size = pFile->readSint32LE();

	if (size != pImage->tex.sizeInBytes) {
		warning("GrimEngine::getSaveGameImage(): unexpected image size %d", size);
		_pStdDisplay->vBufferFree(pImage);
		delete pFile;
		return nullptr;
	}

	pFile->read(pImage->pPixels, size);
	delete pFile;

	return pImage;
}

void GrimEngine::imuseSave() {
	Common::OutSaveFile *pFile = g_system->getSavefileManager()->openForSaving("grim.tmp");

	if (!pFile)
		return;

	soundsSave(pFile);
	pFile->finalize();
	delete pFile;
}

void GrimEngine::imuseRestore() {
	Common::InSaveFile *pFile = g_system->getSavefileManager()->openForLoading("grim.tmp");

	if (!pFile)
		return;

	_pImuse->imStopAllSounds();
	soundsRestore(pFile);
	delete pFile;
	g_system->getSavefileManager()->removeSavefile("grim.tmp");
}

void GrimEngine::soundsSave(Common::WriteStream *pStream) {
	Common::Serializer ser(nullptr, pStream);

	ser.setVersion(kSoundsSaveVersion);
	_pImuse->soundsSaveLoad(ser);
}

void GrimEngine::soundsRestore(Common::SeekableReadStream *pStream) {
	Common::Serializer ser(pStream, nullptr);

	ser.setVersion(kSoundsSaveVersion);
	_pImuse->soundsSaveLoad(ser);
}

} // End of namespace Grim
