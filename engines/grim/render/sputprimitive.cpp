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

#include "engines/grim/lua_v1_opcodes/lua_v1.h"
#include "engines/grim/render/sputprimitive.h"
#include "engines/grim/render/sputrender.h"

namespace Grim {

SputPrimitive::SputPrimitive(RenderDroid *rd) : _sputRender(nullptr), _hs(nullptr), _initialized(0), _started(0), _current(rd) {
}

SputPrimitive::~SputPrimitive() {
	_current.purge();
}

void SputPrimitive::setSputRender(SputRender *sputRender) {
	_sputRender = sputRender;
	_current.setSputRender(sputRender);
}

int SputPrimitive::initialize(SysStruct *hs) {
	_hs = hs;
	_initialized = 1;

	return 0;
}

void SputPrimitive::pause() {
	_current.markAllDirty(3);
	_sputRender->setSkipPrimitives(1);
}

void SputPrimitive::resume() {
	_sputRender->setSkipPrimitives(0);
}

int SputPrimitive::terminate() {
	_hs = nullptr;

	if (_initialized)
		_initialized = 0;

	return 0;
}

int SputPrimitive::startup() {
	if (_started)
		return 1;

	_lua->registerPrimitiveOpcodes();
	_started = 1;

	return 0;
}

int SputPrimitive::shutdown() {
	_current.purge();
	_started = 0;

	return 0;
}

int SputPrimitive::save(Common::WriteStream *stream) {
	int numPrimitives = 0;

	for (SputPrimitiveObject *obj = _current.getHead(); obj; obj = obj->next)
		numPrimitives++;

	stream->writeSint32LE(numPrimitives);

	if (stream->err()) {
		warning("SputPrimitive::save(): failed to write the primitive count");
		return 1;
	}

	for (SputPrimitiveObject *obj = _current.getHead(); obj; obj = obj->next) {
		stream->writeSint32LE(obj->shape);
		stream->writeSint32LE(obj->id);
		stream->writeSint32LE(obj->layer);
		stream->writeSint32LE(obj->x1);
		stream->writeSint32LE(obj->y1);
		stream->writeSint32LE(obj->x2);
		stream->writeSint32LE(obj->y2);
		stream->writeSint32LE(obj->color);
		stream->writeSint32LE(obj->pattern);
		stream->writeSint32LE(obj->filled);

		if (stream->err()) {
			warning("SputPrimitive::save(): failed to write primitive %d", obj->id);
			return 1;
		}
	}

	return 0;
}

int SputPrimitive::restore(Common::ReadStream *stream) {
	_current.purge();

	int numPrimitives = stream->readSint32LE();

	if (stream->err() || stream->eos()) {
		warning("SputPrimitive::restore(): failed to read the primitive count");
		return 1;
	}

	if (numPrimitives < 0) {
		warning("SputPrimitive::restore(): negative primitive count");
		return 1;
	}

	for (int i = 0; i < numPrimitives; i++) {
		int shape = stream->readSint32LE();
		int id = stream->readSint32LE();
		int layer = stream->readSint32LE();
		int x1 = stream->readSint32LE();
		int y1 = stream->readSint32LE();
		int x2 = stream->readSint32LE();
		int y2 = stream->readSint32LE();
		int color = stream->readSint32LE();
		int pattern = stream->readSint32LE();
		int filled = stream->readSint32LE();

		if (stream->err() || stream->eos()) {
			warning("SputPrimitive::restore(): failed to read primitive %d of %d", i + 1, numPrimitives);
			return 1;
		}

		SputPrimitiveObject *obj = _current.addObject(id, layer);

		obj->shape = shape;
		obj->x1 = x1;
		obj->y1 = y1;
		obj->x2 = x2;
		obj->y2 = y2;
		obj->color = color;
		obj->pattern = pattern;
		obj->filled = filled;

		_current._nextId = id + 1;
	}

	return 0;
}

} // End of namespace Grim
