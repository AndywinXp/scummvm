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

#include "common/textconsole.h"

#include "topgun/topgun_v0.h"

namespace Topgun {

void TopgunEngine_v0::freeBitmapBlobIfUnreferenced(uint16 id) {
	warning("freeBitmapBlobIfUnreferenced UNIMPLEMENTED");
}

void TopgunEngine_v0::destroyDataResourceObject(uint16 id) {
	warning("destroyDataResourceObject UNIMPLEMENTED");
}

bool TopgunEngine_v0::destroyGroupResourceObject(uint16 id) {
	warning("destroyGroupResourceObject UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::destroySpriteResourceObject(uint16 id) {
	warning("destroySpriteResourceObject UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::destroyWaveResourceObject(uint16 id) {
	warning("destroyWaveResourceObject UNIMPLEMENTED");
}

bool TopgunEngine_v0::ensureBitmapResourceLoaded(uint16 id) {
	warning("ensureBitmapResourceLoaded UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::loadOrGetResourceData(uint16 id) {
	warning("loadOrGetResourceData UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::ensureGroupResourceLoaded(uint16 id) {
	warning("ensureGroupResourceLoaded UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::loadTransPaletteResource(uint16 id) {
	warning("loadTransPaletteResource UNIMPLEMENTED");
	return false;
}

byte *TopgunEngine_v0::loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) {
	warning("loadResourceWithProcessingPause UNIMPLEMENTED");
	return nullptr;
}

bool TopgunEngine_v0::ensureSpriteResourceLoaded(uint16 id) {
	warning("ensureSpriteResourceLoaded UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count) {
	warning("attachResourceListToSprite UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::ensureWaveResourceLoaded(uint16 id) {
	warning("ensureWaveResourceLoaded UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::destroyResourceObjectByType(uint16 id, uint16 type) {
	warning("destroyResourceObjectByType UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::resetResourcesAndTransientState() {
	warning("resetResourcesAndTransientState UNIMPLEMENTED");
}

void TopgunEngine_v0::destroySceneMainObjects() {
	warning("destroySceneMainObjects UNIMPLEMENTED");
}

bool TopgunEngine_v0::validateAndGetResourceByType(uint16 id, uint16 type) {
	warning("validateAndGetResourceByType UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::isResourceObjectLoaded(uint16 id) {
	warning("isResourceObjectLoaded UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::decrementResourceRefCountAndMaybeDestroy(uint16 bitmapId) {
	warning("decrementResourceRefCountAndMaybeDestroy UNIMPLEMENTED");
}

void TopgunEngine_v0::refreshGamePalette(byte *buffer) {
	warning("refreshGamePalette UNIMPLEMENTED");
}

bool TopgunEngine_v0::openResourceFileAndInit(const Common::String &name) {
	warning("openResourceFileAndInit UNIMPLEMENTED");
	return false;
}

} // End of namespace Topgun
