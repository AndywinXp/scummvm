#define FORBIDDEN_SYMBOL_EXCEPTION_setjmp
#define FORBIDDEN_SYMBOL_EXCEPTION_longjmp

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_sort.h"

#include "engines/grim/lua/lmem.h"
#include "engines/grim/lua/lstate.h"
#include "engines/grim/lua/lstring.h"
#include "engines/grim/lua/ltable.h"
#include "engines/grim/lua/ltask.h"
#include "engines/grim/lua/ltm.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

struct PtrID {
	uint32 serialNumber;
	void *pItem;
};

static bool bPtrsSorted = false;

static PtrID *aStringPtrs;
static uint32 stringPtrTotal;

static PtrID *aClosurePtrs;
static uint32 closurePtrTotal;

static PtrID *aTablePtrs;
static uint32 tablePtrTotal;

static PtrID *aProtoPtrs;
static uint32 protoPtrTotal;

// Userdata is inline in the ScummVM objects: its string table entries point here
static UserData *aUserdata;

static int32 loadInt32(tSaveRestoreAccess *restoreFunc) {
	uint32 data = 0;

	restoreFunc(&data, 4);

	return (int32)FROM_LE_32(data);
}

static int comparePtrIDs(const void *elem1, const void *elem2) {
	const PtrID *pElem1 = (const PtrID *)elem1;
	const PtrID *pElem2 = (const PtrID *)elem2;

	if (pElem1->serialNumber < pElem2->serialNumber)
		return -1;
	else if (pElem1->serialNumber > pElem2->serialNumber)
		return 1;

	return 0;
}

static void SerialToPtr(TObject *pObject) {
	PtrID *pPtrID, ptrid;
	uint32 serialNum = (uint32)pObject->value.i;

	if (!bPtrsSorted) {
		sort(aStringPtrs, stringPtrTotal, sizeof(PtrID), comparePtrIDs);
		sort(aClosurePtrs, closurePtrTotal, sizeof(PtrID), comparePtrIDs);
		sort(aTablePtrs, tablePtrTotal, sizeof(PtrID), comparePtrIDs);
		sort(aProtoPtrs, protoPtrTotal, sizeof(PtrID), comparePtrIDs);

		bPtrsSorted = true;
	}

	memset(&pObject->value, 0, sizeof(Value));

	if (serialNum == 0)
		return;

	ptrid.serialNumber = serialNum;
	ptrid.pItem = nullptr;

	switch (pObject->ttype) {
	case LUA_T_USERDATA:
	case LUA_T_STRING:
		pPtrID = (PtrID *)binarySearch(&ptrid, aStringPtrs, stringPtrTotal, sizeof(PtrID), comparePtrIDs);
		break;

	case LUA_T_CLOSURE:
		pPtrID = (PtrID *)binarySearch(&ptrid, aClosurePtrs, closurePtrTotal, sizeof(PtrID), comparePtrIDs);
		break;

	case LUA_T_ARRAY:
		pPtrID = (PtrID *)binarySearch(&ptrid, aTablePtrs, tablePtrTotal, sizeof(PtrID), comparePtrIDs);
		break;

	case LUA_T_PROTO:
	case LUA_T_PMARK:
		pPtrID = (PtrID *)binarySearch(&ptrid, aProtoPtrs, protoPtrTotal, sizeof(PtrID), comparePtrIDs);
		break;

	default:
		pPtrID = nullptr;
	}

	if (!pPtrID)
		return;

	switch (pObject->ttype) {
	case LUA_T_USERDATA:
		pObject->value.ud = *(UserData *)pPtrID->pItem;
		break;

	case LUA_T_STRING:
		pObject->value.ts = (TaggedString *)pPtrID->pItem;
		break;

	case LUA_T_CLOSURE:
		pObject->value.cl = (Closure *)pPtrID->pItem;
		break;

	case LUA_T_ARRAY:
		pObject->value.a = (Hash *)pPtrID->pItem;
		break;

	default:
		pObject->value.tf = (TProtoFunc *)pPtrID->pItem;
	}
}

static void LoadObject(TObject *pObject, tSaveRestoreAccess *restoreFunc) {
	pObject->ttype = (lua_Type)loadInt32(restoreFunc);

	uint32 raw = (uint32)loadInt32(restoreFunc);

	memset(&pObject->value, 0, sizeof(Value));
	memcpy(&pObject->value, &raw, 4);
}

static void ResolveObject(TObject *pObject) {
	switch (pObject->ttype) {
	case LUA_T_CPROTO:
		{
			uint32 id = (uint32)pObject->value.i;

			memset(&pObject->value, 0, sizeof(Value));
			pObject->value.f = lua_IDToFuncPtr(id);
		}
		break;

	case LUA_T_NUMBER:
	case LUA_T_LINE:
	case LUA_T_TASK:
	case LUA_T_NIL:
		break;

	default:
		SerialToPtr(pObject);
	}
}

static void LoadResolvedObject(TObject *pObject, tSaveRestoreAccess *restoreFunc) {
	LoadObject(pObject, restoreFunc);
	ResolveObject(pObject);
}

static void LoadSerial(TObject *pObject, lua_Type ttype, tSaveRestoreAccess *restoreFunc) {
	uint32 raw = (uint32)loadInt32(restoreFunc);

	pObject->ttype = ttype;
	memset(&pObject->value, 0, sizeof(Value));
	memcpy(&pObject->value, &raw, 4);
}

static void luaH_forcerehash(Hash *t) {
	Node *vold = t->node;

	t->node = hashnodecreate(t->nhash);

	for (int32 i = 0; i < t->nuse; i++) {
		Node *n = vold + i;

		if (n->ref.ttype != LUA_T_NIL && n->val.ttype != LUA_T_NIL)
			*node(t, present(t, &n->ref)) = *n;
	}

	luaM_free(vold);
}

static Hash *luaH_load(tSaveRestoreAccess *restoreFunc) {
	Hash *pHash = luaM_new(Hash);

	pHash->nhash = loadInt32(restoreFunc);
	pHash->nuse = loadInt32(restoreFunc);
	pHash->htag = loadInt32(restoreFunc);
	pHash->node = hashnodecreate(pHash->nhash);
	luaO_insertlist(&roottable, (GCnode *)pHash);

	for (int32 i = 0; i < pHash->nuse; i++) {
		LoadObject(&pHash->node[i].ref, restoreFunc);
		LoadObject(&pHash->node[i].val, restoreFunc);
	}

	return pHash;
}

static void luaS_resolvestrings() {
	for (int32 i = 0; i < NUM_HASHS; i++) {
		stringtable *tb = &string_root[i];

		for (int32 j = 0; j < tb->size; j++) {
			TaggedString *pTString = tb->hash[j];

			if (pTString != nullptr && pTString != &EMPTY && pTString->constindex != -1)
				ResolveObject(&pTString->globalval);
		}
	}
}

static void GlobalEndRestore() {
	if (aStringPtrs) {
		luaM_free(aStringPtrs);
		stringPtrTotal = 0;
		aStringPtrs = nullptr;
	}

	if (aUserdata) {
		luaM_free(aUserdata);
		aUserdata = nullptr;
	}

	if (aClosurePtrs) {
		luaM_free(aClosurePtrs);
		closurePtrTotal = 0;
		aClosurePtrs = nullptr;
	}

	if (aTablePtrs) {
		luaM_free(aTablePtrs);
		tablePtrTotal = 0;
		aTablePtrs = nullptr;
	}

	if (aProtoPtrs) {
		luaM_free(aProtoPtrs);
		protoPtrTotal = 0;
		aProtoPtrs = nullptr;
	}

	bPtrsSorted = false;
}

static void GlobalBeginRestore(tSaveRestoreAccess *restoreFunc) {
	uint32 maxstrlen, globalCount;
	uint32 numUserdata = 0;
	PtrID *pPtr;
	char *pBuffer;

	stringPtrTotal = loadInt32(restoreFunc);
	closurePtrTotal = loadInt32(restoreFunc);
	tablePtrTotal = loadInt32(restoreFunc);
	protoPtrTotal = loadInt32(restoreFunc);
	globalCount = loadInt32(restoreFunc);

	pPtr = aStringPtrs = luaM_newvector(stringPtrTotal, PtrID);
	aUserdata = luaM_newvector(stringPtrTotal, UserData);

	maxstrlen = loadInt32(restoreFunc);
	pBuffer = (char *)luaM_malloc(maxstrlen + 5);

	for (uint32 i = 0; i < stringPtrTotal; i++) {
		pPtr->serialNumber = loadInt32(restoreFunc);

		int32 constindex = loadInt32(restoreFunc);
		int32 tag = loadInt32(restoreFunc);
		int32 v = loadInt32(restoreFunc);

		if (constindex != -1) {
			uint32 length = loadInt32(restoreFunc);

			restoreFunc(pBuffer, length);
			pBuffer[length] = '\0';

			TaggedString *pTString = luaS_new(pBuffer);

			memset(&pTString->globalval.value, 0, sizeof(Value));
			pTString->globalval.value.i = v;
			pTString->globalval.ttype = (lua_Type)tag;
			pTString->constindex = constindex;

			pPtr->pItem = pTString;
		} else {
			UserData *pUserdata = &aUserdata[numUserdata++];

			pUserdata->tag = tag;
			pUserdata->id = v;

			if (lua_restorehook)
				pUserdata->id = lua_restorehook(tag, v, restoreFunc);

			pPtr->pItem = pUserdata;
		}

		pPtr++;
	}

	luaM_free(pBuffer);

	pPtr = aClosurePtrs = luaM_newvector(closurePtrTotal, PtrID);

	for (uint32 i = 0; i < closurePtrTotal; i++) {
		pPtr->serialNumber = loadInt32(restoreFunc);

		uint32 nelems = loadInt32(restoreFunc);
		Closure *pClosure = (Closure *)luaM_malloc(sizeof(Closure) + nelems * sizeof(TObject));

		luaO_insertlist(&rootcl, (GCnode *)pClosure);
		pClosure->nelems = nelems;

		for (int32 j = 0; j <= pClosure->nelems; j++)
			LoadObject(&pClosure->consts[j], restoreFunc);

		pPtr->pItem = pClosure;
		pPtr++;
	}

	pPtr = aTablePtrs = luaM_newvector(tablePtrTotal, PtrID);

	for (uint32 i = 0; i < tablePtrTotal; i++) {
		pPtr->serialNumber = loadInt32(restoreFunc);
		pPtr->pItem = luaH_load(restoreFunc);
		pPtr++;
	}

	pPtr = aProtoPtrs = luaM_newvector(protoPtrTotal, PtrID);

	for (uint32 i = 0; i < protoPtrTotal; i++) {
		pPtr->serialNumber = loadInt32(restoreFunc);

		TProtoFunc *pPFunc = luaM_new(TProtoFunc);

		luaO_insertlist(&rootproto, (GCnode *)pPFunc);
		pPFunc->fileName = (TaggedString *)(uintptr)(uint32)loadInt32(restoreFunc);
		pPFunc->lineDefined = loadInt32(restoreFunc);
		pPFunc->nconsts = loadInt32(restoreFunc);
		pPFunc->consts = luaM_newvector(pPFunc->nconsts, TObject);

		for (int32 j = 0; j < pPFunc->nconsts; j++)
			LoadObject(&pPFunc->consts[j], restoreFunc);

		uint32 numVars = loadInt32(restoreFunc);

		if (numVars)
			pPFunc->locvars = luaM_newvector(numVars, LocVar);
		else
			pPFunc->locvars = nullptr;

		for (uint32 j = 0; j < numVars; j++) {
			pPFunc->locvars[j].varname = (TaggedString *)(uintptr)(uint32)loadInt32(restoreFunc);
			pPFunc->locvars[j].line = loadInt32(restoreFunc);
		}

		uint32 numBytes = loadInt32(restoreFunc);

		pPFunc->code = (byte *)luaM_malloc(numBytes);
		restoreFunc(pPFunc->code, numBytes);
		pPtr->pItem = pPFunc;
		pPtr++;
	}

	luaS_resolvestrings();

	for (TProtoFunc *pPFunc = (TProtoFunc *)rootproto.next; pPFunc; pPFunc = (TProtoFunc *)pPFunc->head.next) {
		TObject tempObject;

		tempObject.ttype = LUA_T_STRING;
		memset(&tempObject.value, 0, sizeof(Value));
		tempObject.value.i = (int32)(uintptr)pPFunc->fileName;

		ResolveObject(&tempObject);

		pPFunc->fileName = tempObject.value.ts;

		for (int32 i = 0; i < pPFunc->nconsts; i++)
			ResolveObject(&pPFunc->consts[i]);

		if (pPFunc->locvars) {
			int32 i = 0;

			while (pPFunc->locvars[i].line != -1) {
				tempObject.ttype = LUA_T_STRING;
				memset(&tempObject.value, 0, sizeof(Value));
				tempObject.value.i = (int32)(uintptr)pPFunc->locvars[i].varname;

				ResolveObject(&tempObject);

				pPFunc->locvars[i].varname = tempObject.value.ts;
				i++;
			}
		}
	}

	for (Hash *pHash = (Hash *)roottable.next; pHash; pHash = (Hash *)pHash->head.next) {
		for (int32 i = 0; i < pHash->nuse; i++) {
			ResolveObject(&pHash->node[i].ref);
			ResolveObject(&pHash->node[i].val);
		}

		luaH_forcerehash(pHash);
	}

	for (Closure *pClosure = (Closure *)rootcl.next; pClosure; pClosure = (Closure *)pClosure->head.next) {
		for (int32 i = 0; i <= pClosure->nelems; i++)
			ResolveObject(&pClosure->consts[i]);
	}

	GCnode *pPrevString = &rootglobal;

	for (uint32 i = 0; i < globalCount; i++) {
		TObject tempObject;

		LoadSerial(&tempObject, LUA_T_STRING, restoreFunc);
		ResolveObject(&tempObject);

		TaggedString *pTString = tempObject.value.ts;

		if (!pTString)
			error("LState::GlobalBeginRestore(): a global string is missing");

		pPrevString->next = &pTString->head;
		pPrevString = &pTString->head;
	}

	pPrevString->next = nullptr;

	loadInt32(restoreFunc);
	LoadResolvedObject(&errorim, restoreFunc);

	IMtable_size = loadInt32(restoreFunc);
	IMtable = luaM_newvector(IMtable_size, IM);

	for (int32 i = 0; i < IMtable_size; i++) {
		IM *pIM = &IMtable[i];

		for (int32 j = 0; j < IM_N; j++)
			LoadResolvedObject(&pIM->int_method[j], restoreFunc);
	}

	last_tag = loadInt32(restoreFunc);
	refSize = loadInt32(restoreFunc);
	refArray = luaM_newvector(refSize, struct ref);

	for (int32 i = 0; i < refSize; i++) {
		LoadResolvedObject(&refArray[i].o, restoreFunc);
		refArray[i].status = (Status)loadInt32(restoreFunc);
	}

	GCthreshold = loadInt32(restoreFunc);
	nblocks = loadInt32(restoreFunc);

	Mbuffsize = loadInt32(restoreFunc);
	Mbuffer = (char *)luaM_malloc(Mbuffsize);
	restoreFunc(Mbuffer, Mbuffsize);

	int32 Mbasepos = loadInt32(restoreFunc);

	Mbuffbase = Mbasepos + Mbuffer;
	Mbuffnext = loadInt32(restoreFunc);

	globalTaskSerialId = loadInt32(restoreFunc);
}

static void RestoreExecutionState(lua_Task *pExecState, Stack *pStack, uint32 numChildren, tSaveRestoreAccess *restoreFunc) {
	TObject tempObject;

	if (numChildren) {
		pExecState->next = luaM_new(lua_Task);
		lua_taskinit(pExecState->next, nullptr, 0, 0);
		RestoreExecutionState(pExecState->next, pStack, numChildren - 1, restoreFunc);
	}

	pExecState->S = pStack;

	LoadSerial(&tempObject, LUA_T_CLOSURE, restoreFunc);
	ResolveObject(&tempObject);
	pExecState->cl = tempObject.value.cl;

	LoadSerial(&tempObject, LUA_T_PROTO, restoreFunc);
	ResolveObject(&tempObject);
	pExecState->tf = tempObject.value.tf;

	pExecState->base = loadInt32(restoreFunc);
	pExecState->initBase = loadInt32(restoreFunc);
	pExecState->initResults = loadInt32(restoreFunc);
	pExecState->executed = loadInt32(restoreFunc) != 0;

	int32 pcOffset = loadInt32(restoreFunc);

	pExecState->pc = pExecState->tf->code + pcOffset;
	pExecState->aux = loadInt32(restoreFunc);
	pExecState->consts = pExecState->tf->consts;
}

static void RestoreState(LState *pState, tSaveRestoreAccess *restoreFunc) {
	uint32 numExecStates = loadInt32(restoreFunc);

	if (numExecStates) {
		pState->task = luaM_new(lua_Task);
		lua_taskinit(pState->task, nullptr, 0, 0);
		RestoreExecutionState(pState->task, &pState->stack, numExecStates - 1, restoreFunc);
	} else {
		pState->task = nullptr;
	}

	int32 previousExecState = loadInt32(restoreFunc);

	if (previousExecState >= 0) {
		lua_Task *pExecState = pState->task;

		while (previousExecState--)
			pExecState = pExecState->next;

		pState->prevTask = pExecState;
	} else {
		pState->prevTask = nullptr;
	}

	pState->preventBreakCounter = loadInt32(restoreFunc);
	loadInt32(restoreFunc);
	pState->updated = loadInt32(restoreFunc) != 0;
	pState->all_paused = loadInt32(restoreFunc) ? 1 : 0;
	pState->paused = false;
	pState->callLevelCounter = loadInt32(restoreFunc);

	int32 stackSize = loadInt32(restoreFunc);

	luaM_free(pState->stack.stack);
	pState->stack.stack = luaM_newvector(stackSize, TObject);
	pState->stack.last = pState->stack.stack + stackSize - 1;

	int32 stackTop = loadInt32(restoreFunc);

	pState->stack.top = pState->stack.stack + stackTop;

	for (int32 i = 0; i < stackTop; i++)
		LoadResolvedObject(&pState->stack.stack[i], restoreFunc);

	pState->Cstack.base = loadInt32(restoreFunc);
	pState->Cstack.lua2C = loadInt32(restoreFunc);
	pState->Cstack.num = loadInt32(restoreFunc);

	pState->numCblocks = loadInt32(restoreFunc);

	for (int32 j = 0; j < pState->numCblocks; j++) {
		pState->Cblocks[j].base = loadInt32(restoreFunc);
		pState->Cblocks[j].lua2C = loadInt32(restoreFunc);
		pState->Cblocks[j].num = loadInt32(restoreFunc);
	}

	pState->id = loadInt32(restoreFunc);
	LoadResolvedObject(&pState->taskFunc, restoreFunc);
}

int lua_Restore(tSaveRestoreAccess *restoreFunc) {
	if (lua_isopen())
		lua_close();

	lua_rootState = lua_state = luaM_new(LState);
	lua_stateinit(lua_state);
	lua_resetglobals();

	GlobalBeginRestore(restoreFunc);

	int32 numThreads = globalTaskSerialId;

	uint32 numTasks = loadInt32(restoreFunc);
	uint32 currentTaskNumber = loadInt32(restoreFunc);

	RestoreState(lua_rootState, restoreFunc);

	LState *pLState = lua_rootState;

	for (uint32 i = 1; i < numTasks; i++) {
		LState *pNewState = luaM_new(LState);

		lua_stateinit(pNewState);
		RestoreState(pNewState, restoreFunc);

		pNewState->prev = pLState;
		pLState->next = pNewState;
		pLState = pNewState;
	}

	lua_state = lua_rootState;

	while (currentTaskNumber--)
		lua_state = lua_state->next;

	GlobalEndRestore();

	globalTaskSerialId = numThreads;

	return 0;
}

} // end of namespace Grim
