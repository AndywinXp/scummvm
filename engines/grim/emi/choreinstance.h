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

#ifndef GRIM_EMI_CHOREINSTANCE_H
#define GRIM_EMI_CHOREINSTANCE_H

#include "common/array.h"
#include "common/hashmap.h"

#include "engines/grim/emi/choreset.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class Actor;
class ComponentInstance;

class ChoreInstance : public LResourceHandle<ChoreSet> {
public:
	enum {
		kPlaying = 0x01,
		kLooping = 0x02
	};

	static const uint kFinished = 0x80000000U;

	enum {
		kRestoreBufferSize = 512
	};

	ChoreInstance(Actor *actor, uint instanceId);
	ChoreInstance(Actor *actor, const LString &newChoreName, const LString &choreSetName);
	~ChoreInstance() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	void bind(const LString &newChoreName, const LString &choreSetName);
	bool isPlaying();
	bool isFinished();
	bool isLooping();
	bool setPlaying();
	bool fadeIn(float seconds);
	bool fadeOut(float seconds);
	bool stop();
	bool pause();
	bool complete();
	bool update(float deltaTime, bool replay);
	bool setLooping(bool looping);
	bool save(tSaveRestoreAccess *access);
	bool restore(tSaveRestoreAccess *access);

	static uint newInstanceWithId(Actor *actor, uint instanceId);
	static uint newInstance(Actor *actor, const LString &newChoreName, const LString &choreSetName);
	static ChoreInstance *getInstance(uint instanceId);
	static bool isInstanceValid(uint instanceId);
	static bool startup();
	static void shutdown();

	uint flags;
	float time;
	float stopTime;
	LString choreName;
	Common::Array<ComponentInstance *> instances;
	uint id;
	Actor *owner;

private:
	friend class Actor;

	typedef Common::HashMap<uint, ChoreInstance *> InstanceMap;

	static InstanceMap _registry;
	static uint _nextId;
	static bool _started;
};

} // End of namespace Grim

#endif
