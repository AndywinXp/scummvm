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

void TopgunEngine_v0::syncBackgroundDimensions(uint16 bitmapId) {
	warning("syncBackgroundDimensions UNIMPLEMENTED");
}

void TopgunEngine_v0::commitBackgroundChangeAndRepaint(const Common::Rect &rect) {
	warning("commitBackgroundChangeAndRepaint UNIMPLEMENTED");
}

void TopgunEngine_v0::applyBackgroundToSpriteLib(uint16 bitmapId, Common::Rect *rectOut) {
	warning("applyBackgroundToSpriteLib UNIMPLEMENTED");
}

void TopgunEngine_v0::clearBackground() {
	warning("clearBackground UNIMPLEMENTED");
}

bool TopgunEngine_v0::setBackgroundFromBitmapResource(uint16 bitmapId) {
	warning("setBackgroundFromBitmapResource UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::copyAndPaintBackgroundRect(const Common::Rect &rect) {
	warning("copyAndPaintBackgroundRect UNIMPLEMENTED");
}

void TopgunEngine_v0::initDissolve() {
	warning("initDissolve UNIMPLEMENTED");
}

void TopgunEngine_v0::dissolveOneTile() {
	warning("dissolveOneTile UNIMPLEMENTED");
}

void TopgunEngine_v0::computeInsetRingRect(uint16 k, Common::Rect *out, uint16 n) {
	warning("computeInsetRingRect UNIMPLEMENTED");
}

void TopgunEngine_v0::paintFrameBandsTopBottom(const Common::Rect &newer, const Common::Rect &older) {
	warning("paintFrameBandsTopBottom UNIMPLEMENTED");
}

void TopgunEngine_v0::wipeEffectFrameRingsInsideOut() {
	warning("wipeEffectFrameRingsInsideOut UNIMPLEMENTED");
}

void TopgunEngine_v0::wipeEffectFrameRingsOutsideIn() {
	warning("wipeEffectFrameRingsOutsideIn UNIMPLEMENTED");
}

void TopgunEngine_v0::advanceSceneFadeStep() {
	warning("advanceSceneFadeStep UNIMPLEMENTED");
}

void TopgunEngine_v0::sceneTransitionIn() {
	warning("sceneTransitionIn UNIMPLEMENTED");
}

void TopgunEngine_v0::sceneTransitionOut() {
	warning("sceneTransitionOut UNIMPLEMENTED");
}

} // End of namespace Topgun
