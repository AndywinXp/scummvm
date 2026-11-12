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

#include "engines/grim/emi/color.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/primitivequeue_v2.h"

namespace Grim {

static const int32 kColorTag = (int32)MKTAG('R', 'L', 'O', 'C');
static const int kDefaultPrimitiveLayer = 2;
static const int kMaxPolygonPoints = 250;
static const float kNoChange = -10000.0f;

lua_Object Lua_V2::getTableField(lua_Object table, const char *key) {
	lua_pushobject(table);
	lua_pushstring(key);

	return lua_gettable();
}

bool Lua_V2::getColorField(lua_Object table, Color &color) {
	lua_Object colorObj = getTableField(table, "color");

	if (!lua_isuserdata(colorObj) || lua_tag(colorObj) != kColorTag)
		return false;

	color = Color((uint)lua_getuserdata(colorObj));

	return true;
}

bool Lua_V2::getLayerField(lua_Object table, int *layer) {
	lua_Object layerObj = getTableField(table, "layer");

	if (!lua_isnumber(layerObj))
		return false;

	*layer = luaFtol(lua_getnumber(layerObj));

	return true;
}

bool Lua_V2::getFloatPairFields(lua_Object table, const char *firstKey, const char *secondKey, float defaultValue, float *first, float *second) {
	lua_Object firstObj = getTableField(table, firstKey);
	lua_Object secondObj = getTableField(table, secondKey);

	if (!lua_isnumber(firstObj) && !lua_isnumber(secondObj))
		return false;

	*first = defaultValue;
	*second = defaultValue;

	if (lua_isnumber(firstObj))
		*first = lua_getnumber(firstObj);

	if (lua_isnumber(secondObj))
		*second = lua_getnumber(secondObj);

	return true;
}

void Lua_V2::Draw3DLine() {
	Color lineColor(Color::kWhite);
	int layer = kDefaultPrimitiveLayer;
	lua_Object coordObjs[6];
	float coords[6];

	for (int i = 0; i < 6; i++)
		coordObjs[i] = lua_getparam(i + 1);

	lua_Object optionsObj = lua_getparam(7);

	for (int i = 0; i < 6; i++) {
		if (!lua_isnumber(coordObjs[i])) {
			lua_pushnil();

			return;
		}
	}

	for (int i = 0; i < 6; i++)
		coords[i] = lua_getnumber(coordObjs[i]);

	if (lua_istable(optionsObj)) {
		getColorField(optionsObj, lineColor);
		getLayerField(optionsObj, &layer);
	}

	// The disasm queues the line with Color(0), not with the color read above
	int primitiveId = _engine->getPrimitiveQueue()->add3DLine(coords[0], coords[1], coords[2], coords[3], coords[4], coords[5], Color(0u), layer);

	lua_pushnumber((float)(uint)primitiveId);
}

void Lua_V2::DrawLine() {
	Color lineColor(Color::kWhite);
	int layer = kDefaultPrimitiveLayer;
	lua_Object coordObjs[4];
	float coords[4];

	for (int i = 0; i < 4; i++)
		coordObjs[i] = lua_getparam(i + 1);

	lua_Object optionsObj = lua_getparam(5);

	for (int i = 0; i < 4; i++) {
		if (!lua_isnumber(coordObjs[i])) {
			lua_pushnil();

			return;
		}
	}

	for (int i = 0; i < 4; i++)
		coords[i] = lua_getnumber(coordObjs[i]);

	if (lua_istable(optionsObj)) {
		getColorField(optionsObj, lineColor);
		getLayerField(optionsObj, &layer);
	}

	int primitiveId = _engine->getPrimitiveQueue()->addLine(coords[0], coords[1], coords[2], coords[3], lineColor, layer);

	lua_pushnumber((float)(uint)primitiveId);
}

void Lua_V2::DrawPolygon() {
	Color polygonColor(Color::kWhite);
	int layer = kDefaultPrimitiveLayer;
	lua_Object pointsObj = lua_getparam(1);
	lua_Object optionsObj = lua_getparam(2);

	if (!lua_istable(pointsObj)) {
		lua_pushnil();

		return;
	}

	if (lua_istable(optionsObj)) {
		getColorField(optionsObj, polygonColor);
		getLayerField(optionsObj, &layer);
	}

	int numPoints = 0;
	int index = 1;

	do {
		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);

		lua_Object pointObj = lua_gettable();

		if (lua_isnil(pointObj))
			break;

		index += 2;
		numPoints++;
	} while (numPoints < kMaxPolygonPoints);

	if (numPoints <= 0) {
		lua_pushnil();

		return;
	}

	float *points = (float *)_engine->getMemoryServices()->mallocImpl(numPoints * 3 * sizeof(float));

	if (!points) {
		warning("Lua_V2::DrawPolygon(): out of memory");
		lua_pushnil();

		return;
	}

	for (int i = 0; i < numPoints * 3; i++)
		points[i] = 0.0f;

	index = 1;

	for (int i = 0; i < numPoints; i++) {
		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);
		index++;

		lua_Object xObj = lua_gettable();

		points[i * 3] = lua_getnumber(xObj);

		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);
		index++;

		lua_Object yObj = lua_gettable();

		if (!lua_isnil(yObj))
			points[i * 3 + 1] = lua_getnumber(yObj);
		else
			points[i * 3 + 1] = 0.0f;
	}

	// The disasm never frees the points
	int primitiveId = _engine->getPrimitiveQueue()->addPolygon(numPoints, points, polygonColor, layer);

	lua_pushnumber((float)(uint)primitiveId);
}

void Lua_V2::DrawRectangle() {
	Color rectangleColor(Color::kWhite);
	bool filled = false;
	int layer = kDefaultPrimitiveLayer;
	lua_Object coordObjs[4];
	float coords[4];

	for (int i = 0; i < 4; i++)
		coordObjs[i] = lua_getparam(i + 1);

	lua_Object optionsObj = lua_getparam(5);

	for (int i = 0; i < 4; i++) {
		if (!lua_isnumber(coordObjs[i])) {
			lua_pushnil();

			return;
		}
	}

	for (int i = 0; i < 4; i++)
		coords[i] = lua_getnumber(coordObjs[i]);

	if (lua_istable(optionsObj)) {
		getColorField(optionsObj, rectangleColor);

		lua_Object filledObj = getTableField(optionsObj, "filled");

		if (!lua_isnil(filledObj))
			filled = true;

		getLayerField(optionsObj, &layer);
	}

	int primitiveId = _engine->getPrimitiveQueue()->addRectangle(coords[0], coords[1], coords[2], coords[3], rectangleColor, filled, layer);

	lua_pushnumber((float)(uint)primitiveId);
}

void Lua_V2::ChangePrimitive() {
	lua_Object primitiveObj = lua_getparam(1);
	lua_Object changesObj = lua_getparam(2);

	if (!lua_isnumber(primitiveObj) || !lua_istable(changesObj))
		return;

	int primitiveId = luaFtol(lua_getnumber(primitiveObj));
	Color newColor;
	int newLayer;
	float first;
	float second;

	if (getColorField(changesObj, newColor))
		_engine->getPrimitiveQueue()->setColor(primitiveId, newColor);

	if (getLayerField(changesObj, &newLayer))
		_engine->getPrimitiveQueue()->setLayer(primitiveId, newLayer);

	if (getFloatPairFields(changesObj, "xoffset", "yoffset", 0.0f, &first, &second))
		_engine->getPrimitiveQueue()->moveBy(primitiveId, first, second);

	if (getFloatPairFields(changesObj, "xmove", "ymove", kNoChange, &first, &second))
		_engine->getPrimitiveQueue()->moveTo(primitiveId, first, second);

	if (getFloatPairFields(changesObj, "x", "y", kNoChange, &first, &second))
		_engine->getPrimitiveQueue()->setEnd(primitiveId, first, second);

	if (getFloatPairFields(changesObj, "x2", "y2", kNoChange, &first, &second))
		_engine->getPrimitiveQueue()->setEnd(primitiveId, first, second);

	if (getFloatPairFields(changesObj, "width", "height", kNoChange, &first, &second))
		_engine->getPrimitiveQueue()->setEndRelative(primitiveId, first, second);
}

void Lua_V2::KillPrimitive() {
	lua_Object primitiveObj = lua_getparam(1);

	if (lua_isnumber(primitiveObj))
		_engine->getPrimitiveQueue()->kill(luaFtol(lua_getnumber(primitiveObj)));
}

void Lua_V2::PurgePrimitiveQueue() {
	_engine->getPrimitiveQueue()->clear();
	_engine->getPrimitiveQueue()->resetIds();
}

} // End of namespace Grim
