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

#include "topgun/display.h"
#include "topgun/libraries/spritelibrary.h"
#include "topgun/libraries/utilslibrary.h"
#include "topgun/topgun.h"

namespace Topgun {

SpriteLibrary::SpriteLibrary(TopgunEngine *engine) : _engine(engine) {
	memset(_queueCommandSizeTable, 0, sizeof(_queueCommandSizeTable));
}

SpriteLibrary::~SpriteLibrary() {
}

void SpriteLibrary::spriteAnimate() {
	warning("spriteAnimate UNIMPLEMENTED");
}

bool SpriteLibrary::spriteAppendBitmap(uint16 sprite, byte *frame) {
	warning("spriteAppendBitmap UNIMPLEMENTED");
	return false;
}

bool SpriteLibrary::spriteAppendBitmaps(uint16 sprite, byte **frames, uint16 count) {
	warning("spriteAppendBitmaps UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteBreakLoops(uint16 sprite) {
	warning("spriteBreakLoops UNIMPLEMENTED");
}

bool SpriteLibrary::spriteChangePalette(uint16 first, uint16 count, const byte *entries) {
	warning("spriteChangePalette UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteClearUpdate(uint16 sprite) {
	warning("spriteClearUpdate UNIMPLEMENTED");
}

uint16 SpriteLibrary::spriteCopy(uint16 sprite) {
	warning("spriteCopy UNIMPLEMENTED");
	return 0;
}

void SpriteLibrary::spriteCopyRect(Common::Rect &rect, const byte *source, byte *destination) {
	warning("spriteCopyRect UNIMPLEMENTED");
}

uint16 SpriteLibrary::spriteCreate(byte *frame) {
	warning("spriteCreate UNIMPLEMENTED");
	return 0;
}

byte *SpriteLibrary::spriteDibCreate(uint16 width, uint16 height, uint16 flags) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + kBitmapHeaderSize;

	byte *bitmap = _engine->utilsLibrary()->tgMalloc(size, flags);
	if (bitmap == nullptr)
		return nullptr;

	memset(bitmap, 0, kBitmapHeaderSize);
	WRITE_LE_UINT16(bitmap, kBitmapHeaderSize);
	WRITE_LE_UINT16(bitmap + 8, (size >> 16) != 0 ? 1 : 0);
	WRITE_LE_UINT16(bitmap + 2, width);
	WRITE_LE_UINT16(bitmap + 4, pitch);
	WRITE_LE_UINT16(bitmap + 6, height);
	return bitmap;
}

void SpriteLibrary::spriteEmptyQueue(uint16 sprite) {
	warning("spriteEmptyQueue UNIMPLEMENTED");
}

void SpriteLibrary::spriteEnable(uint16 sprite, uint16 enable) {
	warning("spriteEnable UNIMPLEMENTED");
}

void SpriteLibrary::spriteExitLibrary() {
	warning("spriteExitLibrary UNIMPLEMENTED");
}

uint16 SpriteLibrary::spriteDestroy(uint16 sprite, bool hideRefresh, bool freeFrames) {
	warning("spriteDestroy UNIMPLEMENTED");
	return 0;
}

void SpriteLibrary::spriteDestroyAll(bool hideRefresh) {
	warning("spriteDestroyAll UNIMPLEMENTED");
}

bool SpriteLibrary::spriteFadePalette(uint16 mult, uint16 divisor, uint16 first, uint16 count) {
	warning("spriteFadePalette UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteDibFree(byte *bitmap) {
	warning("spriteDibFree UNIMPLEMENTED");
}

uint16 SpriteLibrary::spriteGetId(uint16 sprite) {
	warning("spriteGetId UNIMPLEMENTED");
	return 0;
}

void SpriteLibrary::spriteGetInfo(uint16 sprite, SpriteInfo &info) {
	warning("spriteGetInfo UNIMPLEMENTED");
}

void SpriteLibrary::spriteGetPalette(byte *destination, uint16 first, uint16 count) {
	warning("spriteGetPalette UNIMPLEMENTED");
}

void SpriteLibrary::spriteHide(uint16 sprite, bool refresh) {
	warning("spriteHide UNIMPLEMENTED");
}

bool SpriteLibrary::spriteInitLibrary() {
	_inTick = false;
	_bkgTransparent = false;
	_debugProc = 0;
	_messageProcSet = false;
	_serviceProcSet = false;
	_composeBuffer = nullptr;
	_composeBufferCapacity = 0;
	_backgroundBitmap = nullptr;
	_topmostSprite = 0;
	_spriteCount = 0;
	_clipRect = Common::Rect();
	_scrollX = 0;
	_scrollY = 0;

	_composeBuffer = spriteDibCreate(kComposeBufferWidth, kComposeBufferHeight, 0);
	if (_composeBuffer == nullptr)
		return false;

	_composeBufferCapacity = (uint32)READ_LE_UINT16(_composeBuffer + 4) * READ_LE_UINT16(_composeBuffer + 6) + kBitmapHeaderSize;

	Display *display = _engine->display();
	int16 screenWidth = display->width();
	int16 screenHeight = display->height();

	display->setViewportOrigin(screenWidth / 2, screenHeight / 2);
	_clipRect.right = screenWidth;
	_clipRect.bottom = screenHeight;
	_scrollX = (screenWidth - 1) / 2;
	_scrollY = (screenHeight - 1) / 2;
	_clipRect.translate(-_scrollX, -_scrollY);

	initQueueRecordSizeTable();
	_spriteTableEnd = 0;
	_tickCursor = 0;
	return true;
}

void SpriteLibrary::initQueueRecordSizeTable() {
	_queueCommandSizeTable[1] = 12;
	_queueCommandSizeTable[2] = 14;
	_queueCommandSizeTable[3] = 18;
	_queueCommandSizeTable[4] = 2;
	_queueCommandSizeTable[5] = 18;
	_queueCommandSizeTable[6] = 6;
	_queueCommandSizeTable[7] = 7;
	_queueCommandSizeTable[8] = 6;
	_queueCommandSizeTable[9] = 3;
	_queueCommandSizeTable[10] = 6;
	_queueCommandSizeTable[11] = 8;
	_queueCommandSizeTable[12] = 6;
	_queueCommandSizeTable[13] = 4;
	_queueCommandSizeTable[14] = 2;
	_queueCommandSizeTable[15] = 12;
	_queueCommandSizeTable[16] = 4;
	_queueCommandSizeTable[17] = 2;
	_queueCommandSizeTable[18] = 4;
	_queueCommandSizeTable[19] = 6;
	_queueCommandSizeTable[20] = 4;
	_queueCommandSizeTable[21] = 11;
	_queueCommandSizeTable[22] = 4;
	_queueCommandSizeTable[23] = 6;
	_queueCommandSizeTable[24] = 3;
	_queueCommandSizeTable[25] = 7;
	_queueCommandSizeTable[26] = 9;
	_queueCommandSizeTable[27] = 4;
	_queueCommandSizeTable[28] = 4;
}

void SpriteLibrary::spritePaint(uint16 sprite) {
	warning("spritePaint UNIMPLEMENTED");
}

void SpriteLibrary::spritePaintRect(const Common::Rect &rect, bool backgroundCoordinates) {
	warning("spritePaintRect UNIMPLEMENTED");
}

uint16 SpriteLibrary::spritePicked(int16 x, int16 y) {
	warning("spritePicked UNIMPLEMENTED");
	return 0;
}

bool SpriteLibrary::spriteRealizePalette() {
	warning("spriteRealizePalette UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteSetBackground(byte *background, Common::Rect *outClipRect) {
	warning("spriteSetBackground UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetBkgTransparent(bool flag) {
	warning("spriteSetBkgTransparent UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetDebugProc(uint32 value) {
	warning("spriteSetDebugProc UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetId(uint16 sprite, uint16 id) {
	warning("spriteSetId UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetMessageProc() {
	warning("spriteSetMessageProc UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetMotion(uint16 sprite, uint32 milliseconds) {
	warning("spriteSetMotion UNIMPLEMENTED");
}

bool SpriteLibrary::spriteSetPalette(const byte *entries) {
	warning("spriteSetPalette UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteSetPosition(uint16 sprite, int16 x, int16 y) {
	warning("spriteSetPosition UNIMPLEMENTED");
}

bool SpriteLibrary::spriteSetQueue(uint16 sprite, const byte *queue, uint16 size) {
	warning("spriteSetQueue UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteSetServiceProc() {
	warning("spriteSetServiceProc UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetSpeed(uint16 sprite, uint32 milliseconds) {
	warning("spriteSetSpeed UNIMPLEMENTED");
}

void SpriteLibrary::spriteSetTopMost(uint16 sprite) {
	warning("spriteSetTopMost UNIMPLEMENTED");
}

void SpriteLibrary::spriteShow(uint16 sprite, uint16 frame, bool refresh) {
	warning("spriteShow UNIMPLEMENTED");
}

bool SpriteLibrary::spriteSwapOrder(uint16 spriteA, uint16 spriteB, bool onlyIfBAbove, bool refresh) {
	warning("spriteSwapOrder UNIMPLEMENTED");
	return false;
}

void SpriteLibrary::spriteTransfer(uint16 source, uint16 destination) {
	warning("spriteTransfer UNIMPLEMENTED");
}

void SpriteLibrary::spriteCallback(uint16 sprite, bool notifyOnFrame) {
	warning("spriteCallback UNIMPLEMENTED");
}

} // End of namespace Topgun
