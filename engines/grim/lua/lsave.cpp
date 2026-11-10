#define FORBIDDEN_SYMBOL_EXCEPTION_setjmp
#define FORBIDDEN_SYMBOL_EXCEPTION_longjmp

#include "common/array.h"
#include "common/endian.h"
#include "common/hashmap.h"
#include "common/textconsole.h"

#include "engines/grim/lua/lopcodes.h"
#include "engines/grim/lua/lstate.h"
#include "engines/grim/lua/lstring.h"
#include "engines/grim/lua/ltask.h"
#include "engines/grim/lua/ltm.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

PointerId makeIdFromPointer(void *ptr) {
	PointerId pointer;
	pointer.id = (uintptr)ptr;

	return pointer;
}

void *makePointerFromId(PointerId ptr) {
	return (void *)ptr.id;
}

static const byte opcodeSizes[] = {
	1, 2, 1, 2, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1,
	1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3,
	1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	3, 2, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 3, 1, 2, 3, 2, 4, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, 1,
	3, 2, 2, 2, 2, 3, 2, 1, 1
};

static_assert(sizeof(opcodeSizes) == POP1 + 1, "opcode size table does not match the opcodes");

int32 luaExeCodeSize(const byte *code, int32 limit) {
	int32 pos = 2;

	while (pos < limit) {
		byte op = code[pos];

		if (op >= sizeof(opcodeSizes))
			return -1;

		pos += opcodeSizes[op];

		if (pos > limit)
			return -1;

		if (op == ENDCODE)
			return pos;
	}

	return -1;
}

// Userdata is inline in the ScummVM objects: the first pass collects it
static bool bCollectUserdata;
static Common::HashMap<uint64, uint32> userdataIndices;
static Common::Array<UserData> userdataEntries;

static int nullWriter(void *buffer, int size) {
	return 0;
}

static uint32 getSerial(const void *ptr) {
	return (uint32)(uintptr)ptr;
}

static void saveInt32(tSaveRestoreAccess *saveFunc, int32 value) {
	uint32 data = TO_LE_32((uint32)value);

	saveFunc(&data, 4);
}

static void saveByte(tSaveRestoreAccess *saveFunc, bool value) {
	byte data = value ? 1 : 0;

	saveFunc(&data, 1);
}

static uint32 getUserdataSerial(const UserData *pUserdata) {
	uint64 key = ((uint64)(uint32)pUserdata->tag << 32) | (uint32)pUserdata->id;
	uint32 index;

	if (!userdataIndices.tryGetVal(key, index)) {
		index = userdataEntries.size();
		userdataIndices.setVal(key, index);
		userdataEntries.push_back(*pUserdata);
	}

	if (bCollectUserdata)
		return 0;

	return getSerial(&userdataEntries[index]);
}

static void SaveObject(const TObject *pObject, tSaveRestoreAccess *saveFunc) {
	int32 ttype = pObject->ttype;
	uint32 serialNum = 0;

	switch (ttype) {
	case LUA_T_CPROTO:
		if (lua_FuncPtrHasID(pObject->value.f)) {
			serialNum = lua_FuncPtrToID(pObject->value.f);
		} else {
			ttype = LUA_T_NIL;
		}
		break;

	case LUA_T_USERDATA:
		serialNum = getUserdataSerial(&pObject->value.ud);
		break;

	default:
		memcpy(&serialNum, &pObject->value, 4);
	}

	saveInt32(saveFunc, ttype);
	saveInt32(saveFunc, (int32)serialNum);
}

static void luaS_writestrings(tSaveRestoreAccess *saveFunc) {
	for (int32 i = 0; i < NUM_HASHS; i++) {
		stringtable *tb = &string_root[i];

		for (int32 j = 0; j < tb->size; j++) {
			if (tb->hash[j] != nullptr && tb->hash[j] != &EMPTY) {
				TaggedString *pTString = tb->hash[j];

				saveInt32(saveFunc, (int32)getSerial(pTString));
				saveInt32(saveFunc, pTString->constindex);
				SaveObject(&pTString->globalval, saveFunc);

				int32 length = strlen(pTString->str);

				saveInt32(saveFunc, length);
				saveFunc(pTString->str, length);
			}
		}
	}
}

static void luaS_writeuserdata(tSaveRestoreAccess *saveFunc) {
	for (uint i = 0; i < userdataEntries.size(); i++) {
		UserData *pUserdata = &userdataEntries[i];
		int32 value = pUserdata->id;

		saveInt32(saveFunc, (int32)getSerial(pUserdata));
		saveInt32(saveFunc, -1);

		if (lua_savehook)
			value = lua_savehook(pUserdata->tag, value, saveFunc);

		saveInt32(saveFunc, pUserdata->tag);
		saveInt32(saveFunc, value);
	}
}

static uint32 luaS_count(uint32 *maxstrlen) {
	uint32 count = 0;

	*maxstrlen = 0;

	for (int32 i = 0; i < NUM_HASHS; i++) {
		stringtable *tb = &string_root[i];

		for (int32 j = 0; j < tb->size; j++) {
			if (tb->hash[j] != nullptr && tb->hash[j] != &EMPTY) {
				count++;

				uint32 length = strlen(tb->hash[j]->str);

				if (length > *maxstrlen)
					*maxstrlen = length;
			}
		}
	}

	return count;
}

static void luaH_save(tSaveRestoreAccess *saveFunc, Hash *pHash) {
	int32 numItems = 0;

	saveInt32(saveFunc, pHash->nhash);

	for (int32 i = 0; i < pHash->nhash; i++) {
		Node *n = &pHash->node[i];

		if (n->ref.ttype != LUA_T_NIL && n->val.ttype != LUA_T_NIL)
			numItems++;
	}

	saveInt32(saveFunc, numItems);
	saveInt32(saveFunc, pHash->htag);

	for (int32 i = 0; i < pHash->nhash; i++) {
		Node *n = &pHash->node[i];

		if (n->ref.ttype != LUA_T_NIL && n->val.ttype != LUA_T_NIL) {
			SaveObject(&n->ref, saveFunc);
			SaveObject(&n->val, saveFunc);
		}
	}
}

static void SaveGlobals(tSaveRestoreAccess *saveFunc) {
	uint32 maxstrlen, globalCount = 0, protoCount = 0, clCount = 0, tableCount = 0, stringCount = 0;

	stringCount = luaS_count(&maxstrlen) + userdataEntries.size();

	for (GCnode *pNode = rootcl.next; pNode; pNode = pNode->next)
		clCount++;

	for (GCnode *pNode = roottable.next; pNode; pNode = pNode->next)
		tableCount++;

	for (GCnode *pNode = rootproto.next; pNode; pNode = pNode->next)
		protoCount++;

	for (GCnode *pNode = rootglobal.next; pNode; pNode = pNode->next)
		globalCount++;

	saveInt32(saveFunc, stringCount);
	saveInt32(saveFunc, clCount);
	saveInt32(saveFunc, tableCount);
	saveInt32(saveFunc, protoCount);
	saveInt32(saveFunc, globalCount);
	saveInt32(saveFunc, maxstrlen);

	luaS_writestrings(saveFunc);
	luaS_writeuserdata(saveFunc);

	for (Closure *pClosure = (Closure *)rootcl.next; pClosure; pClosure = (Closure *)pClosure->head.next) {
		saveInt32(saveFunc, (int32)getSerial(pClosure));
		saveInt32(saveFunc, pClosure->nelems);

		for (int32 i = 0; i <= pClosure->nelems; i++)
			SaveObject(&pClosure->consts[i], saveFunc);
	}

	for (Hash *pHash = (Hash *)roottable.next; pHash; pHash = (Hash *)pHash->head.next) {
		saveInt32(saveFunc, (int32)getSerial(pHash));
		luaH_save(saveFunc, pHash);
	}

	for (TProtoFunc *pProto = (TProtoFunc *)rootproto.next; pProto; pProto = (TProtoFunc *)pProto->head.next) {
		int32 numVars = 0;

		saveInt32(saveFunc, (int32)getSerial(pProto));
		saveInt32(saveFunc, (int32)getSerial(pProto->fileName));
		saveInt32(saveFunc, pProto->lineDefined);
		saveInt32(saveFunc, pProto->nconsts);

		for (int32 i = 0; i < pProto->nconsts; i++)
			SaveObject(&pProto->consts[i], saveFunc);

		if (pProto->locvars) {
			while (pProto->locvars[numVars++].line != -1) {
			}
		}

		saveInt32(saveFunc, numVars);

		for (int32 i = 0; i < numVars; i++) {
			saveInt32(saveFunc, (int32)getSerial(pProto->locvars[i].varname));
			saveInt32(saveFunc, pProto->locvars[i].line);
		}

		int32 numBytes = luaExeCodeSize(pProto->code, 0x7fffffff);

		saveInt32(saveFunc, numBytes);
		saveFunc(pProto->code, numBytes);
	}

	for (GCnode *pTString = rootglobal.next; pTString; pTString = pTString->next)
		saveInt32(saveFunc, (int32)getSerial(pTString));

	if (luaIsEmi)
		saveByte(saveFunc, true);
	else
		saveInt32(saveFunc, 1);

	SaveObject(&errorim, saveFunc);

	saveInt32(saveFunc, IMtable_size);

	for (int32 i = 0; i < IMtable_size; i++) {
		for (int32 j = 0; j < IM_N; j++)
			SaveObject(&IMtable[i].int_method[j], saveFunc);
	}

	saveInt32(saveFunc, last_tag);
	saveInt32(saveFunc, refSize);

	for (int32 i = 0; i < refSize; i++) {
		SaveObject(&refArray[i].o, saveFunc);
		saveInt32(saveFunc, (int32)refArray[i].status);
	}

	saveInt32(saveFunc, GCthreshold);
	saveInt32(saveFunc, nblocks);

	saveInt32(saveFunc, Mbuffsize);
	saveFunc(Mbuffer, Mbuffsize);

	int32 Mbasepos = Mbuffbase - Mbuffer;

	saveInt32(saveFunc, Mbasepos);
	saveInt32(saveFunc, Mbuffnext);

	saveInt32(saveFunc, globalTaskSerialId);
}

static void SaveExecutionState(lua_Task *pExecState, tSaveRestoreAccess *saveFunc) {
	if (pExecState->next)
		SaveExecutionState(pExecState->next, saveFunc);

	saveInt32(saveFunc, (int32)getSerial(pExecState->cl));
	saveInt32(saveFunc, (int32)getSerial(pExecState->tf));
	saveInt32(saveFunc, pExecState->base);
	saveInt32(saveFunc, pExecState->initBase);
	saveInt32(saveFunc, pExecState->initResults);
	if (luaIsEmi)
		saveByte(saveFunc, pExecState->executed);
	else
		saveInt32(saveFunc, pExecState->executed ? 1 : 0);

	saveInt32(saveFunc, pExecState->pc - pExecState->tf->code);
	saveInt32(saveFunc, pExecState->aux);
}

static void SaveState(LState *pState, tSaveRestoreAccess *saveFunc) {
	uint32 numExecStates = 0;
	int32 previousExecState = -1;

	for (lua_Task *pExecState = pState->task; pExecState; pExecState = pExecState->next) {
		if (pState->prevTask && pState->prevTask == pExecState)
			previousExecState = numExecStates;

		numExecStates++;
	}

	saveInt32(saveFunc, numExecStates);

	if (pState->task)
		SaveExecutionState(pState->task, saveFunc);

	saveInt32(saveFunc, previousExecState);

	if (luaIsEmi) {
		saveInt32(saveFunc, pState->sleepFor);
		saveInt32(saveFunc, pState->preventBreakCounter);
		saveByte(saveFunc, pState == lua_rootState);
		saveByte(saveFunc, pState->updated);
		saveByte(saveFunc, pState->paused);
		saveByte(saveFunc, pState->pauseSticky);
		saveInt32(saveFunc, pState->callLevelCounter);
	} else {
		saveInt32(saveFunc, pState->preventBreakCounter);
		saveInt32(saveFunc, pState == lua_rootState ? 1 : 0);
		saveInt32(saveFunc, pState->updated ? 1 : 0);
		saveInt32(saveFunc, (pState->all_paused || pState->paused) ? 1 : 0);
		saveInt32(saveFunc, pState->callLevelCounter);
	}

	int32 stackSize = (pState->stack.last - pState->stack.stack) + 1;

	saveInt32(saveFunc, stackSize);

	stackSize = pState->stack.top - pState->stack.stack;
	saveInt32(saveFunc, stackSize);

	for (int32 i = 0; i < stackSize; i++)
		SaveObject(&pState->stack.stack[i], saveFunc);

	saveInt32(saveFunc, pState->Cstack.base);
	saveInt32(saveFunc, pState->Cstack.lua2C);
	saveInt32(saveFunc, pState->Cstack.num);

	saveInt32(saveFunc, pState->numCblocks);

	for (int32 j = 0; j < pState->numCblocks; j++) {
		saveInt32(saveFunc, pState->Cblocks[j].base);
		saveInt32(saveFunc, pState->Cblocks[j].lua2C);
		saveInt32(saveFunc, pState->Cblocks[j].num);
	}

	saveInt32(saveFunc, pState->id);
	SaveObject(&pState->taskFunc, saveFunc);
}

static void saveLuaState(tSaveRestoreAccess *saveFunc) {
	uint32 numTasks = 0, currentTaskNumber = 0;

	SaveGlobals(saveFunc);

	for (LState *pState = lua_rootState; pState; pState = pState->next) {
		if (pState == lua_state)
			currentTaskNumber = numTasks;

		numTasks++;
	}

	saveInt32(saveFunc, numTasks);
	saveInt32(saveFunc, currentTaskNumber);

	for (LState *pState = lua_rootState; pState; pState = pState->next)
		SaveState(pState, saveFunc);
}

int lua_Save(tSaveRestoreAccess *saveFunc) {
	lua_collectgarbage(0);

	bCollectUserdata = true;
	userdataIndices.clear();
	userdataEntries.clear();
	saveLuaState(nullWriter);

	bCollectUserdata = false;
	saveLuaState(saveFunc);

	userdataIndices.clear();
	userdataEntries.clear();

	return 0;
}

} // end of namespace Grim
