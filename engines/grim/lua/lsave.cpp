#define FORBIDDEN_SYMBOL_EXCEPTION_setjmp
#define FORBIDDEN_SYMBOL_EXCEPTION_longjmp

#include "common/endian.h"
#include "common/debug.h"

#include "engines/grim/savegame.h"

#include "engines/grim/lua/ltask.h"
#include "engines/grim/lua/lauxlib.h"
#include "engines/grim/lua/lmem.h"
#include "engines/grim/lua/ldo.h"
#include "engines/grim/lua/ltm.h"
#include "engines/grim/lua/ltable.h"
#include "engines/grim/lua/lvm.h"
#include "engines/grim/lua/lopcodes.h"
#include "engines/grim/lua/lstring.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

static tSaveRestoreAccess *pSaveFunc;
static SaveGame *pSaveGame;

static int saveGameWriter(void *data, int size) {
	pSaveGame->write(data, size);

	return 0;
}

static void writeBytes(const void *data, int size) {
	pSaveFunc(const_cast<void *>(data), size);
}

static void writeLEUint64(uint64 data) {
	data = TO_LE_64(data);
	pSaveFunc(&data, 8);
}

static void writeLEUint32(uint32 data) {
	data = TO_LE_32(data);
	pSaveFunc(&data, 4);
}

static void writeLESint32(int32 data) {
	writeLEUint32((uint32)data);
}

static void writeByte(byte data) {
	pSaveFunc(&data, 1);
}

static void writeBool(bool data) {
	writeByte(data);
}

static void writeFloat(float data) {
	uint32 v;
	memcpy(&v, &data, 4);
	writeLEUint32(v);
}

PointerId makeIdFromPointer(void *ptr) {
	PointerId pointer;
	pointer.id = (uintptr)ptr;

	return pointer;
}

void *makePointerFromId(PointerId ptr) {
	return (void *)ptr.id;
}

static void saveObjectValue(TObject *object) {
	int32 userId = 0;

	if (object->ttype == LUA_T_USERDATA) {
		userId = object->value.ud.id;
		if (lua_savehook)
			userId = lua_savehook(object->value.ud.tag, userId, pSaveFunc);
	}

	writeLESint32(object->ttype);

	switch (object->ttype) {
		case LUA_T_CPROTO:
		case LUA_T_CMARK:
			{
				luaL_libList *list = list_of_libs;
				int32 idObj = 0;
				while (list) {
					for (int32 l = 0; l < list->number; l++) {
						if (list->list[l].func == object->value.f) {
							idObj = (idObj << 16) | l;
							writeLESint32(idObj);
							writeLESint32(0);
							return;
						}
					}
					list = list->next;
					idObj++;
				}
				assert(0);
				break;
			}
		case LUA_T_NUMBER:
		case LUA_T_TASK:
			{
				writeFloat(object->value.n);
			}
			break;
		case LUA_T_NIL:
			break;
		case LUA_T_ARRAY:
			{
				writeLEUint64(makeIdFromPointer(object->value.a).id);
			}
			break;
		case LUA_T_USERDATA:
			{
				writeLESint32(userId);
				writeLESint32(object->value.ud.tag);
			}
			break;
		case LUA_T_STRING:
			{
				writeLEUint64(makeIdFromPointer(object->value.ts).id);
			}
			break;
		case LUA_T_PROTO:
		case LUA_T_PMARK:
			{
				writeLEUint64(makeIdFromPointer(object->value.tf).id);
			}
			break;
		case LUA_T_CLOSURE:
		case LUA_T_CLMARK:
			{
				writeLEUint64(makeIdFromPointer(object->value.cl).id);
			}
			break;
		case LUA_T_LINE:
			{
				writeLESint32(object->value.i);
			}
			break;
		default:
			writeLEUint64(makeIdFromPointer(object->value.ts).id);
	}
}

static int32 opcodeSizeTable[] = {
	1, 2, 1, 2, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1,
	1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3,
	1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	3, 2, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 3, 1, 2, 3, 2, 4, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1, 1,
	3, 2, 2, 2, 2, 3, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 3, 2, 1, 1,
	1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 3, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 2, 3, 2, 4, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 3, 2, 3,
	2, 3, 2, 3, 2, 3, 2, 1, 1, 3, 2, 2, 2, 2, 3, 2, 1, 1
};

static void saveState() {
	lua_collectgarbage(0);
	int32 i, l;
	int32 countElements = 0;
	int32 maxStringLength = 0;

	// Check for max length for strings and count them
	for (i = 0; i < NUM_HASHS; i++) {
		stringtable *tempStringTable = &string_root[i];
		for (l = 0; l < tempStringTable->size; l++) {
			if (tempStringTable->hash[l] && tempStringTable->hash[l] != &EMPTY) {
				countElements++;
				if (tempStringTable->hash[l]->constindex != -1) {
					int len = strlen(tempStringTable->hash[l]->str);
					if (maxStringLength < len) {
						maxStringLength = len;
					}
				}
			}
		}
	}
	// save number of strings
	writeLESint32(countElements);

	// save number of closures
	countElements = 0;
	GCnode *tempNode;
	tempNode = rootcl.next;
	while (tempNode) {
		countElements++;
		tempNode = tempNode->next;
	}
	writeLESint32(countElements);

	// save number of tables
	countElements = 0;
	tempNode = roottable.next;
	while (tempNode) {
		countElements++;
		tempNode = tempNode->next;
	}
	writeLESint32(countElements);

	// save number of prototypes
	countElements = 0;
	tempNode = rootproto.next;
	while (tempNode) {
		countElements++;
		tempNode = tempNode->next;
	}
	writeLESint32(countElements);

	// save number of global strings
	countElements = 0;
	tempNode = rootglobal.next;
	while (tempNode) {
		countElements++;
		tempNode = tempNode->next;
	}
	writeLESint32(countElements);

	// save maximum length for string
	writeLESint32(maxStringLength);

	// save hash tables for strings and user data
	TaggedString *tempString;
	for (i = 0; i < NUM_HASHS; i++) {
		stringtable *tempStringTable = &string_root[i];
		for (l = 0; l < tempStringTable->size; l++) {
			if (tempStringTable->hash[l] && tempStringTable->hash[l] != &EMPTY) {
				tempString = tempStringTable->hash[l];
				writeLEUint64(makeIdFromPointer(tempString).id);
				writeLESint32(tempString->constindex);
				if (tempString->constindex != -1) {
					saveObjectValue(&tempString->globalval);
					int len = strlen(tempString->str);
					writeLESint32(len);
					writeBytes(tempString->str, len);
				}
			}
		}
	}

	Closure *tempClosure = (Closure *)rootcl.next;
	while (tempClosure) {
		writeLEUint64(makeIdFromPointer(tempClosure).id);
		writeLESint32(tempClosure->nelems);
		for (i = 0; i <= tempClosure->nelems; i++) {
			saveObjectValue(&tempClosure->consts[i]);
		}
		tempClosure = (Closure *)tempClosure->head.next;
	}

	Hash *tempHash = (Hash *)roottable.next;
	while (tempHash) {
		writeLEUint64(makeIdFromPointer(tempHash).id);
		writeLESint32(tempHash->nhash);
		int32 countUsedHash = 0;
		for (i = 0; i < tempHash->nhash; i++) {
			Node *newNode = &tempHash->node[i];
			if (newNode->ref.ttype != LUA_T_NIL && newNode->val.ttype != LUA_T_NIL) {
				countUsedHash++;
			}
		}
		writeLESint32(countUsedHash);
		writeLESint32(tempHash->htag);
		for (i = 0; i < tempHash->nhash; i++) {
			Node *newNode = &tempHash->node[i];
			if (newNode->ref.ttype != LUA_T_NIL && newNode->val.ttype != LUA_T_NIL) {
				saveObjectValue(&tempHash->node[i].ref);
				saveObjectValue(&tempHash->node[i].val);
			}
		}
		tempHash = (Hash *)tempHash->head.next;
	}

	TProtoFunc *tempProtoFunc = (TProtoFunc *)rootproto.next;
	while (tempProtoFunc) {
		writeLEUint64(makeIdFromPointer(tempProtoFunc).id);
		writeLEUint64(makeIdFromPointer(tempProtoFunc->fileName).id);
		writeLESint32(tempProtoFunc->lineDefined);
		writeLESint32(tempProtoFunc->nconsts);
		for (i = 0; i < tempProtoFunc->nconsts; i++) {
			saveObjectValue(&tempProtoFunc->consts[i]);
		}
		int32 countVariables = 0;
		if (tempProtoFunc->locvars) {
			for (; tempProtoFunc->locvars[countVariables++].line != -1;) { }
		}

		writeLESint32(countVariables);
		for (i = 0; i < countVariables; i++) {
			writeLEUint64(makeIdFromPointer(tempProtoFunc->locvars[i].varname).id);
			writeLESint32(tempProtoFunc->locvars[i].line);
		}

		byte *codePtr = tempProtoFunc->code + 2;
		byte *tmpPtr = codePtr;
		int32 opcodeId;
		do {
			opcodeId = *tmpPtr;
			tmpPtr += opcodeSizeTable[opcodeId];
		} while (opcodeId != ENDCODE);
		int32 codeSize = (tmpPtr - codePtr) + 2;
		writeLESint32(codeSize);
		writeBytes(tempProtoFunc->code, codeSize);
		tempProtoFunc = (TProtoFunc *)tempProtoFunc->head.next;
	}

	tempString = (TaggedString *)rootglobal.next;
	while (tempString) {
		writeLEUint64(makeIdFromPointer(tempString).id);
		tempString = (TaggedString *)tempString->head.next;
	}

	saveObjectValue(&errorim);

	IM *tempIm = IMtable;
	writeLESint32(IMtable_size);
	for (i = 0; i < IMtable_size; i++) {
		for (l = 0; l < IM_N; l++) {
			saveObjectValue(&tempIm->int_method[l]);
		}
		tempIm++;
	}

	writeLESint32(last_tag);
	writeLESint32(refSize);
	for (i = 0 ; i < refSize; i++) {
		saveObjectValue(&refArray[i].o);
		writeLESint32(refArray[i].status);
	}

	writeLESint32(GCthreshold);
	writeLESint32(nblocks);

	writeLESint32(Mbuffsize);
	writeBytes(Mbuffer, Mbuffsize);
	int32 MbaseOffset = Mbuffbase - Mbuffer;
	writeLESint32(MbaseOffset);
	writeLESint32(Mbuffnext);

	writeLESint32(globalTaskSerialId);

	int32 countStates = 0, currentState = 0;
	LState *state = lua_rootState;
	while (state) {
		if (lua_state == state)
			currentState = countStates;
		countStates++;
		state = state->next;
	}
	writeLESint32(countStates);
	writeLESint32(currentState);

	state = lua_rootState;
	while (state) {
		lua_Task *task = state->task;
		int32 countTasks = 0, n = -1;
		while (task) {
			if (state->prevTask && state->prevTask == task)
				n = countTasks;
			countTasks++;
			task = task->next;
		}
		writeLESint32(countTasks);
		task = state->task;
		while (task) {
			writeLEUint64(makeIdFromPointer(task->cl).id);
			writeLEUint64(makeIdFromPointer(task->tf).id);
			writeLESint32(task->base);
			writeLESint32(task->initBase);
			writeLESint32(task->initResults);
			writeBool(task->executed);
			int32 pcOffset = task->pc - task->tf->code;
			writeLESint32(pcOffset);
			writeLESint32(task->aux);
			task = task->next;
		}

		writeLESint32(n);

		writeBool(state->updated);

		byte pauseState = 0;
		pauseState = state->all_paused & LUA_SG_ALL_PAUSED;
		pauseState |= state->paused ? LUA_SG_PAUSED : 0;
		writeByte(pauseState);

		writeLESint32(state->preventBreakCounter);
		writeLESint32(state->callLevelCounter);

		int32 stackLastSize = (state->stack.last - state->stack.stack) + 1;
		writeLESint32(stackLastSize);
		int32 stackTopSize = state->stack.top - state->stack.stack;
		writeLESint32(stackTopSize);
		for (i = 0; i < stackTopSize; i++) {
			saveObjectValue(&state->stack.stack[i]);
		}

		writeLESint32(state->Cstack.base);
		writeLESint32(state->Cstack.lua2C);
		writeLESint32(state->Cstack.num);

		writeLESint32(state->numCblocks);
		for (i = 0; i < state->numCblocks; i++) {
			writeLESint32(state->Cblocks[i].base);
			writeLESint32(state->Cblocks[i].lua2C);
			writeLESint32(state->Cblocks[i].num);
		}

		writeLEUint32(state->sleepFor);
		writeLEUint32(state->id);
		saveObjectValue(&state->taskFunc);

		state = state->next;
	}
}

int lua_Save(tSaveRestoreAccess *saveFunc) {
	pSaveFunc = saveFunc;
	saveState();

	return 0;
}

void lua_Save(SaveGame *savedState) {
	savedState->beginSection('LUAS');

	pSaveGame = savedState;
	pSaveFunc = saveGameWriter;
	saveState();

	savedState->endSection();
}

} // end of namespace Grim
