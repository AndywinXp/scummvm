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

#ifndef GRIM_LUABASE_H
#define GRIM_LUABASE_H

#include "common/str.h"
#include "common/list.h"

namespace Grim {

typedef uint32 lua_Object; // from lua/lua.h

/**
 * Declare a Lua opcode in the current class.
 */
#define DECLARE_LUA_OPCODE(func) \
	public:\
	inline static void static_##func() {\
		static_cast<LuaClass *>(LuaBase::instance())->func();\
	}\
	protected:\
	virtual void func()
/**
 * Retrieve the opcode "func" of the class "class".
 */
#define LUA_OPCODE(class, func) \
	class::static_##func

/**
 * @brief A list of arguments to be passed to a Lua function.
 *
 * This is a convenience class to pass arguments to a Lua function, using
 * LuaBase::callback(const char *name, const LuaObjects &objects).
 */
class LuaObjects {
public:
	void add(float number);
	void add(int number);
	void add(const char *str);
	void addNil();

private:
	/**
	 * Pushes all the objects to Lua, in the same order as they were added.
	 */
	void pushObjects() const;

	/**
	 * The struct wrapping the value and the type of the objects.
	 */
	struct Obj {
		enum {
			Nil,
			Number,
			String
		} _type;
		union {
			float number;
			const char *string;
		} _value;
	};
	Common::List<Obj> _objects;

	friend class LuaBase;
};

class LuaBase {
public:
	typedef LuaBase LuaClass;

	LuaBase();
	virtual ~LuaBase();
	inline static LuaBase *instance() { return s_instance; }

	int dofile(const char *filename);

	void update(int frameTime, int movieTime);
	void setFrameTime(float frameTime);
	void setMovieTime(float movieTime);
	void recognizeOpcodes();

	// Force the demo flag
	virtual void forceDemo();

	/**
	 * Call a Lua function in the system table.
	 *
	 * @param name The name of the function.
	 */
	bool callback(const char *name);
	/**
	 * Call a Lua function in the system table passing the specified arguments.
	 *
	 * @param name The name of the function;
	 * @param objects The arguments to be passed to the function.
	 */
	bool callback(const char *name, const LuaObjects &objects);

protected:
	bool getbool(int num);
	void pushbool(bool val);

	void registerTypeOverride();
	void registerSectorConstants();
	void registerConcatFallback();

	DECLARE_LUA_OPCODE(dummyHandler);
	DECLARE_LUA_OPCODE(typeOverride);
	DECLARE_LUA_OPCODE(concatFallback);

	int refSystemTable;
	int refTypeOverride;
	int refOldConcatFallback;

private:
	unsigned int _frameTimeCollection;

	static LuaBase *s_instance;

	friend class LuaObjects;
};

} // end of namespace Grim

#endif
