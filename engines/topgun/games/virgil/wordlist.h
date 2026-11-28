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

#ifndef TOPGUN_GAMES_VIRGIL_WORDLIST_H
#define TOPGUN_GAMES_VIRGIL_WORDLIST_H

#include "common/array.h"
#include "common/formats/winexe_ne.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	kWordListListCount = 5,
	kWordListTitleSize = 128,
	kWordListSearchStartIndex = 50,
	kWordListNonZeroResult = 1
};

enum WordListScrollCommand {
	kWordListScrollUp = 0,
	kWordListScrollDown = 1,
	kWordListScrollPageUp = 2,
	kWordListScrollPageDown = 3,
	kWordListScrollHome = 4,
	kWordListScrollEnd = 5
};

struct WordListRow {
	uint16 unk_00 = 0;
	uint16 textObject = 0;
};

struct WordListLevel {
	int16 top = 0;
	uint16 count = 0;
	Common::Array<const char *> entries;
	char title[kWordListTitleSize] = {};
	WordListLevel *parent = nullptr;
	WordListLevel *child = nullptr;
};

class WordListPlugin : public Plugin {
public:
	WordListPlugin(PluginHost *host, const Common::String &fileName);
	~WordListPlugin() override;

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	PluginHost *_host;
	Common::String _fileName;
	Common::NEResources _resources;

	uint16 _rows = 0;
	Common::Array<WordListRow> _rowTable;
	Common::Array<byte> _resourceData[kWordListListCount];
	const byte *_listData[kWordListListCount] = {};
	uint16 _externalHandle = 0;
	uint16 _listCount = 0;
	bool _externalList = false;
	uint16 _titleObject = 0;
	WordListLevel *_root = nullptr;
	WordListLevel *_current = nullptr;
	bool _terminated = false;

	void refreshRows();
	void refreshTitle();
	WordListLevel *buildChildLevel(const char *record);
	WordListLevel *buildRootLevel();
	void freeLevels(WordListLevel *level);
	void unloadLists();
	uint16 wordListInit(uint16 rows, uint16 titleObject);
	uint16 wordListGetNumWords() const;
	uint16 wordListGetScrollPos() const;
	uint16 wordListGetScrollState() const;
	uint16 wordListPush(uint16 row);
	uint16 wordListPop();
	uint16 wordListScroll(uint16 command);
	uint16 wordListSearch(uint16 stringId);
	uint16 wordListSetList(uint16 titleString, uint16 handle);
	uint16 wordListSetRowInfo(uint16 row, uint16 first, uint16 textObject);
	uint16 wordListSetScrollPos(uint16 position);
	uint16 wordListSetString(uint16 row, uint16 stringId);
	uint16 wordListTerm();
	void freeState();
	static bool isListTerminated(const byte *data, uint32 size);
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_VIRGIL_WORDLIST_H
