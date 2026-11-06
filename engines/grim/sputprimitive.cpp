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

#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"

namespace Grim {

SputPrimitive::SputPrimitive(RenderDroid *pRd) : _pSputRender(nullptr), _pHS(nullptr), _bInitialized(0), _bStarted(0), _current(pRd) {
}

SputPrimitive::~SputPrimitive() {
	_current.purge();
}

void SputPrimitive::setSputRender(SputRender *pSputRender) {
	_pSputRender = pSputRender;
	_current.setSputRender(pSputRender);
}

int SputPrimitive::initialize(SysStruct *pHS) {
	_pHS = pHS;
	_bInitialized = 1;

	return 0;
}

void SputPrimitive::pause() {
	_current.markAllDirty(3);
	_pSputRender->setSkipPrimitives(1);
}

void SputPrimitive::resume() {
	_pSputRender->setSkipPrimitives(0);
}

int SputPrimitive::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int SputPrimitive::startup() {
	if (_bStarted)
		return 1;

	// TODO: register the primitives Lua functions
	_bStarted = 1;

	return 0;
}

int SputPrimitive::shutdown() {
	_current.purge();
	_bStarted = 0;

	return 0;
}

int SputPrimitive::save(Common::WriteStream *pStream) {
	int numPrimitives = 0;

	for (SputPrimitiveObject *pObj = _current.getHead(); pObj; pObj = pObj->pNext)
		numPrimitives++;

	pStream->writeSint32LE(numPrimitives);

	if (pStream->err()) {
		warning("SputPrimitive::save(): failed to write the primitive count");
		return 1;
	}

	for (SputPrimitiveObject *pObj = _current.getHead(); pObj; pObj = pObj->pNext) {
		pStream->writeSint32LE(pObj->shape);
		pStream->writeSint32LE(pObj->id);
		pStream->writeSint32LE(pObj->layer);
		pStream->writeSint32LE(pObj->x1);
		pStream->writeSint32LE(pObj->y1);
		pStream->writeSint32LE(pObj->x2);
		pStream->writeSint32LE(pObj->y2);
		pStream->writeSint32LE(pObj->color);
		pStream->writeSint32LE(pObj->pattern);
		pStream->writeSint32LE(pObj->bFilled);

		if (pStream->err()) {
			warning("SputPrimitive::save(): failed to write primitive %d", pObj->id);
			return 1;
		}
	}

	return 0;
}

int SputPrimitive::restore(Common::ReadStream *pStream) {
	_current.purge();

	int numPrimitives = pStream->readSint32LE();

	if (pStream->err() || pStream->eos()) {
		warning("SputPrimitive::restore(): failed to read the primitive count");
		return 1;
	}

	if (numPrimitives < 0) {
		warning("SputPrimitive::restore(): negative primitive count");
		return 1;
	}

	for (int i = 0; i < numPrimitives; i++) {
		int shape = pStream->readSint32LE();
		int id = pStream->readSint32LE();
		int layer = pStream->readSint32LE();
		int x1 = pStream->readSint32LE();
		int y1 = pStream->readSint32LE();
		int x2 = pStream->readSint32LE();
		int y2 = pStream->readSint32LE();
		int color = pStream->readSint32LE();
		int pattern = pStream->readSint32LE();
		int bFilled = pStream->readSint32LE();

		if (pStream->err() || pStream->eos()) {
			warning("SputPrimitive::restore(): failed to read primitive %d of %d", i + 1, numPrimitives);
			return 1;
		}

		SputPrimitiveObject *pObj = _current.addObject(id, layer);

		pObj->shape = shape;
		pObj->x1 = x1;
		pObj->y1 = y1;
		pObj->x2 = x2;
		pObj->y2 = y2;
		pObj->color = color;
		pObj->pattern = pattern;
		pObj->bFilled = bFilled;

		_current._nextId = id + 1;
	}

	return 0;
}

} // End of namespace Grim
