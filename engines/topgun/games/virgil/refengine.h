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

#ifndef TOPGUN_GAMES_VIRGIL_REFENGINE_H
#define TOPGUN_GAMES_VIRGIL_REFENGINE_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Topgun {

enum ReStatus {
	kReStatusOk = 0,
	kReStatusFailed = 8
};

enum ReObjectType {
	kReTypeApplication = 510,
	kReTypeDatabase = 520,
	kReTypeIndexList = 530,
	kReTypeEntryList = 540,
	kReTypeTokenSet = 555
};

enum ReOperation {
	kReOperationNext = 430,
	kReOperationOpen = 470,
	kReOperationClose = 480
};

enum ReOption {
	kReOptionDefault = 100,
	kReOptionIndexList = 180,
	kReOptionEntryList = 240,
	kReOptionTokens = 495
};

struct ReObject {
	virtual ~ReObject() {}
};

struct ReApplication : public ReObject {
};

struct ReDatabase : public ReObject {
	uint16 unk_04 = 0;
	uint32 unk_06 = 0;
	int16 id = 0;
};

struct ReEntry {
	uint32 address = 0;
	byte unk_04[14] = {};
};

struct ReEntryList : public ReObject {
	Common::String word;
	byte unk_06 = 0;
	int16 database = 0;
	uint32 unk_09 = 0;
	uint16 count = 0;
	const ReEntry *entries = nullptr;
};

struct ReIndexList : public ReObject {
	Common::String word;
	int16 database = 0;
	uint32 unk_0D = 0;
	uint16 *unk_19 = nullptr;
	uint16 count = 0;
	const char *terms = nullptr;
};

struct ReToken {
	char *text = nullptr;
	byte length = 0;
	byte type = 0;
	byte unk_06 = 0;
	byte flags = 0;
	byte attribute = 0;
};

struct ReTokenSet : public ReObject {
	uint32 entry = 0;
	int16 database = 0;
	byte unk_0B = 0;
	uint32 unk_0E = 0;
	int16 count = 0;
	ReToken *tokens = nullptr;
};

class ReferenceEngine {
public:
	~ReferenceEngine();

	uint16 reMemory(ReObject *&object, const ReObject *parent, uint16 type, uint16 operation, uint16 count);
	uint16 reApp(ReApplication *application, uint16 option, uint16 operation);
	uint16 reDb(ReDatabase *database, uint16 option, uint16 operation);
	uint16 reEntLst(ReEntryList *list, uint16 option, uint16 operation);
	uint16 reIdxLst(ReIndexList *list, uint16 option, uint16 operation);
	uint16 reToken(ReTokenSet *tokenSet, uint16 option, uint16 operation);

private:
	Common::Array<ReObject *> _objects;
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_VIRGIL_REFENGINE_H
