/*
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/

#ifndef GRIM_LMEM_H
#define GRIM_LMEM_H

#include "common/scummsys.h"

namespace Grim {

// memory error messages
#define codeEM          "code size overflow"
#define constantEM      "constant table overflow"
#define refEM           "reference table overflow"
#define tableEM         "table overflow"
#define memEM           "not enough memory"

void *luaM_allocate (int32 size);
void luaM_release (void *block);
void *luaM_realloc (void *oldblock, int32 size);
int32 luaM_growaux (void **block, int32 nelems, int32 size, const char *errormsg, int32 limit);

#define luaM_free(b)                            luaM_release((b))
#define luaM_malloc(t)                          luaM_allocate((t))
#define luaM_new(t)                             ((t *)luaM_allocate(sizeof(t)))
#define luaM_newvector(n, t)                    ((t *)luaM_allocate((n) * sizeof(t)))
#define luaM_growvector(old, n, t, e, l)        (luaM_growaux((void **)old, n, sizeof(t), e, l))
#define luaM_reallocvector(v, n, t)             ((t *)luaM_realloc(v,(n) * sizeof(t)))

#ifdef LUA_DEBUG
extern int32 numblocks;
extern int32 totalmem;
#endif

} // end of namespace Grim

#endif

