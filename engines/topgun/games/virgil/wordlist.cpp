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

#include "common/path.h"
#include "common/stream.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/games/virgil/wordlist.h"

namespace Topgun {

enum {
	kFunctionWordListInit,
	kFunctionWordListGetNumWords,
	kFunctionWordListGetScrollPos,
	kFunctionWordListGetScrollState,
	kFunctionWordListPush,
	kFunctionWordListPop,
	kFunctionWordListScroll,
	kFunctionWordListSearch,
	kFunctionWordListSetList,
	kFunctionWordListSetRowInfo,
	kFunctionWordListSetScrollPos,
	kFunctionWordListSetString,
	kFunctionWordListTerm,
	kFunctionCount
};

WordListPlugin::WordListPlugin(PluginHost *host, const Common::String &fileName) : _host(host), _fileName(fileName) {
}

WordListPlugin::~WordListPlugin() {
	freeState();
}

bool WordListPlugin::init() {
	freeState();

	return _resources.loadFromEXE(Common::Path(_fileName));
}

void WordListPlugin::free() {
	freeState();
}

int WordListPlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("WordList_Init"))
		return kFunctionWordListInit;
	if (name.equalsIgnoreCase("WordList_GetNumWords"))
		return kFunctionWordListGetNumWords;
	if (name.equalsIgnoreCase("WordList_GetScrollPos"))
		return kFunctionWordListGetScrollPos;
	if (name.equalsIgnoreCase("WordList_GetScrollState"))
		return kFunctionWordListGetScrollState;
	if (name.equalsIgnoreCase("WordList_Push"))
		return kFunctionWordListPush;
	if (name.equalsIgnoreCase("WordList_Pop"))
		return kFunctionWordListPop;
	if (name.equalsIgnoreCase("WordList_Scroll"))
		return kFunctionWordListScroll;
	if (name.equalsIgnoreCase("WordList_Search"))
		return kFunctionWordListSearch;
	if (name.equalsIgnoreCase("WordList_SetList"))
		return kFunctionWordListSetList;
	if (name.equalsIgnoreCase("WordList_SetRowInfo"))
		return kFunctionWordListSetRowInfo;
	if (name.equalsIgnoreCase("WordList_SetScrollPos"))
		return kFunctionWordListSetScrollPos;
	if (name.equalsIgnoreCase("WordList_SetString"))
		return kFunctionWordListSetString;
	if (name.equalsIgnoreCase("WordList_Term"))
		return kFunctionWordListTerm;

	return -1;
}

uint16 WordListPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("WordListPlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 0;
	switch (function) {
	case kFunctionWordListPush:
	case kFunctionWordListScroll:
	case kFunctionWordListSearch:
	case kFunctionWordListSetScrollPos:
		expected = 1;
		break;
	case kFunctionWordListInit:
	case kFunctionWordListSetList:
	case kFunctionWordListSetString:
		expected = 2;
		break;
	case kFunctionWordListSetRowInfo:
		expected = 3;
		break;
	}

	if (count < expected)
		warning("WordListPlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[3];
	for (int i = 0; i < 3; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	if (_terminated && function != kFunctionWordListInit)
		warning("WordListPlugin::callFunction(): function %d uses the word list after it has been terminated", function);

	switch (function) {
	case kFunctionWordListInit:
		return wordListInit(values[0], values[1]);
	case kFunctionWordListGetNumWords:
		return wordListGetNumWords();
	case kFunctionWordListGetScrollPos:
		return wordListGetScrollPos();
	case kFunctionWordListGetScrollState:
		return wordListGetScrollState();
	case kFunctionWordListPush:
		return wordListPush(values[0]);
	case kFunctionWordListPop:
		return wordListPop();
	case kFunctionWordListScroll:
		return wordListScroll(values[0]);
	case kFunctionWordListSearch:
		return wordListSearch(values[0]);
	case kFunctionWordListSetList:
		return wordListSetList(values[0], values[1]);
	case kFunctionWordListSetRowInfo:
		return wordListSetRowInfo(values[0], values[1], values[2]);
	case kFunctionWordListSetScrollPos:
		return wordListSetScrollPos(values[0]);
	case kFunctionWordListSetString:
		return wordListSetString(values[0], values[1]);
	case kFunctionWordListTerm:
		return wordListTerm();
	}

	return 0;
}

void WordListPlugin::refreshRows() {
	uint16 visible = 0;

	if (_current) {
		if ((uint16)(_current->top + _rows) <= _current->count)
			visible = _rows;
		else
			visible = (uint16)(_current->count - _current->top);
	}

	uint16 row = 0;

	for (; row < visible; row++) {
		if (row >= _rows) {
			warning("WordListPlugin::refreshRows(): the top word %d is past the end of the list", _current->top);
			break;
		}

		if (_rowTable[row].textObject == 0)
			continue;

		uint16 index = (uint16)(_current->top + row);
		if (index >= _current->count) {
			warning("WordListPlugin::refreshRows(): word %d does not exist", index);
			continue;
		}

		_host->setText(_rowTable[row].textObject, _current->entries[index]);
	}

	for (; row < _rows; row++) {
		if (_rowTable[row].textObject != 0)
			_host->setText(_rowTable[row].textObject, "");
	}
}

void WordListPlugin::refreshTitle() {
	if (_titleObject == 0)
		return;

	const char *title = "";

	if (_current)
		title = _current->title;
	else
		warning("WordListPlugin::refreshTitle(): there is no current list");

	_host->setText(_titleObject, title);
}

WordListLevel *WordListPlugin::buildChildLevel(const char *record) {
	if (!record)
		return nullptr;

	int8 depth = (int8)(record[1] + 1);
	record += strlen(record) + 1;

	uint16 wordCount = 0;

	for (const char *next = record; *next != 0; next += strlen(next) + 1) {
		if ((int8)next[1] < depth)
			break;

		if ((int8)next[1] == depth)
			wordCount++;
	}

	if (wordCount == 0)
		return nullptr;

	WordListLevel *level = new WordListLevel();
	level->count = wordCount;
	level->parent = _current;

	for (const char *next = record; *next != 0; next += strlen(next) + 1) {
		if ((int8)next[1] < depth)
			break;

		if ((int8)next[1] == depth)
			level->entries.push_back(next + 2);
	}

	return level;
}

WordListLevel *WordListPlugin::buildRootLevel() {
	uint16 wordCount = 0;

	for (uint16 i = 0; i < _listCount; i++) {
		if (!_listData[i]) {
			warning("WordListPlugin::buildRootLevel(): list %d is missing", i);
			continue;
		}

		for (const char *record = (const char *)_listData[i]; *record != 0; record += strlen(record) + 1) {
			if ((int8)record[1] < '0')
				break;

			if (record[1] == '0')
				wordCount++;
		}
	}

	if (wordCount == 0)
		return nullptr;

	WordListLevel *level = new WordListLevel();
	level->count = wordCount;
	level->parent = _current;

	for (uint16 i = 0; i < _listCount; i++) {
		if (!_listData[i])
			continue;

		for (const char *record = (const char *)_listData[i]; *record != 0; record += strlen(record) + 1) {
			if ((int8)record[1] < '0')
				break;

			if (record[1] == '0')
				level->entries.push_back(record + 2);
		}
	}

	return level;
}

void WordListPlugin::freeLevels(WordListLevel *level) {
	if (!level)
		return;

	freeLevels(level->child);

	if (level->parent)
		level->parent->child = nullptr;

	delete level;
}

void WordListPlugin::unloadLists() {
	for (uint16 i = 0; i < _listCount; i++) {
		if (_externalList) {
			_host->globalFree(_externalHandle);
			_externalHandle = 0;
		} else {
			_resourceData[i].clear();
		}

		_listData[i] = nullptr;
	}
}

uint16 WordListPlugin::wordListInit(uint16 rows, uint16 titleObject) {
	freeLevels(_root);

	_rows = rows;
	_rowTable.clear();
	_rowTable.resize(rows);
	_titleObject = titleObject;
	_current = nullptr;
	_root = nullptr;
	_terminated = false;

	return 1;
}

uint16 WordListPlugin::wordListGetNumWords() const {
	if (!_current)
		return 0;

	return _current->count;
}

uint16 WordListPlugin::wordListGetScrollPos() const {
	if (!_current)
		return 0;

	return (uint16)_current->top;
}

uint16 WordListPlugin::wordListGetScrollState() const {
	if (!_current)
		return 0;

	uint16 state = 0;

	if (_current->top > 0)
		state = 1;

	if ((int16)(_current->count - _rows) > _current->top)
		state |= 2;

	return state;
}

uint16 WordListPlugin::wordListPush(uint16 row) {
	if (!_current)
		return 0;

	uint16 index = (uint16)(_current->top + row);
	if (index >= _current->count)
		return 0;

	const char *entry = _current->entries[index];

	WordListLevel *level = buildChildLevel(entry - 2);
	if (!level)
		return 0;

	if (strlen(entry) >= kWordListTitleSize)
		warning("WordListPlugin::wordListPush(): the title '%s' is too long", entry);

	Common::strlcpy(level->title, entry, kWordListTitleSize);

	if (_titleObject != 0)
		_host->setText(_titleObject, entry);

	_current->child = level;
	_current = level;

	refreshRows();
	refreshTitle();

	return 1;
}

uint16 WordListPlugin::wordListPop() {
	if (_current && _current->parent) {
		WordListLevel *parent = _current->parent;

		freeLevels(_current);
		_current = parent;

		refreshRows();
		refreshTitle();
	}

	return kWordListNonZeroResult;
}

uint16 WordListPlugin::wordListScroll(uint16 command) {
	if (!_current)
		return 0;

	switch (command) {
	case kWordListScrollUp:
		if (_current->top > 0)
			_current->top--;
		break;
	case kWordListScrollDown:
		if ((int16)(_current->count - _rows) > _current->top)
			_current->top++;
		break;
	case kWordListScrollPageUp:
		_current->top = (int16)(_current->top - _rows);
		if (_current->top < 0)
			_current->top = 0;
		break;
	case kWordListScrollHome:
		_current->top = 0;
		break;
	case kWordListScrollPageDown:
		_current->top = (int16)(_current->top + _rows);
		if ((int16)(_current->count - _rows) < _current->top)
			_current->top = MAX<int16>((int16)(_current->count - _rows), 0);
		break;
	case kWordListScrollEnd:
		_current->top = MAX<int16>((int16)(_current->count - _rows), 0);
		break;
	}

	refreshRows();

	return (uint16)_current->top;
}

uint16 WordListPlugin::wordListSearch(uint16 stringId) {
	if (!_current)
		return 0;

	const char *text = _host->getString(stringId);
	if (!text) {
		warning("WordListPlugin::wordListSearch(): string %d does not exist", stringId);
		return 0;
	}

	int16 low = 0;
	int16 high = (int16)(_current->count - 1);
	int16 middle = kWordListSearchStartIndex;

	while (high >= low) {
		middle = (int16)((int16)(high + low) / 2);

		int16 result = _host->compareStringsIgnoreCase(text, _current->entries[middle]);
		if (result == 0)
			break;

		if (result < 0)
			high = (int16)(middle - 1);
		else
			low = (int16)(middle + 1);
	}

	if (_host->compareStringsIgnoreCase(text, _current->entries[middle]) > 0)
		middle++;

	_current->top = middle;

	if ((int16)(_current->count - _rows) < _current->top)
		_current->top = MAX<int16>((int16)(_current->count - _rows), 0);

	refreshRows();

	return (uint16)(middle - _current->top);
}

uint16 WordListPlugin::wordListSetList(uint16 titleString, uint16 handle) {
	freeLevels(_root);
	unloadLists();

	if (handle == 0) {
		_externalList = false;
		_listCount = 0;

		for (uint16 i = 0; i < kWordListListCount; i++) {
			_resourceData[i].clear();
			_listData[i] = nullptr;

			Common::String name = Common::String::format("Category%d", i + 1);

			Common::SeekableReadStream *stream = _resources.getResource(Common::kWinRCData, Common::WinResourceID(name));
			if (!stream)
				continue;

			_resourceData[i].resize(stream->size());
			if (!_resourceData[i].empty())
				stream->read(_resourceData[i].data(), _resourceData[i].size());

			delete stream;

			if (!isListTerminated(_resourceData[i].data(), _resourceData[i].size())) {
				warning("WordListPlugin::wordListSetList(): the list '%s' has no end", name.c_str());
				continue;
			}

			_listData[i] = _resourceData[i].data();
			_listCount++;
		}
	} else {
		uint32 size = 0;

		_externalHandle = handle;
		_listData[0] = _host->globalLock(handle, size);

		if (_listData[0] && !isListTerminated(_listData[0], size)) {
			warning("WordListPlugin::wordListSetList(): the list of the handle %d has no end", handle);
			_listData[0] = nullptr;
		}

		_listCount = 1;
		_externalList = true;
	}

	_current = nullptr;
	_root = buildRootLevel();
	_current = _root;

	const char *title = _host->getString(titleString);

	if (_current) {
		if (!title) {
			warning("WordListPlugin::wordListSetList(): string %d does not exist", titleString);
		} else {
			if (strlen(title) >= kWordListTitleSize)
				warning("WordListPlugin::wordListSetList(): the title '%s' is too long", title);

			Common::strlcpy(_current->title, title, kWordListTitleSize);
		}
	}

	refreshRows();
	refreshTitle();

	return _root ? 1 : 0;
}

uint16 WordListPlugin::wordListSetRowInfo(uint16 row, uint16 first, uint16 textObject) {
	if (_rows <= row)
		return row;

	_rowTable[row].unk_00 = first;
	_rowTable[row].textObject = textObject;

	return textObject;
}

uint16 WordListPlugin::wordListSetScrollPos(uint16 position) {
	if (!_current)
		return kWordListNonZeroResult;

	uint16 last = (uint16)(_current->count - _rows);
	if (last < position)
		return last;

	_current->top = (int16)position;
	refreshRows();

	return kWordListNonZeroResult;
}

uint16 WordListPlugin::wordListSetString(uint16 row, uint16 stringId) {
	if (!_current)
		return 0;

	uint16 index = (uint16)(_current->top + row);
	if (index >= _current->count)
		return 0;

	_host->setString(stringId, _current->entries[index]);

	return 1;
}

uint16 WordListPlugin::wordListTerm() {
	_rowTable.clear();
	_rows = 0;

	freeLevels(_root);
	_root = nullptr;
	_current = nullptr;

	unloadLists();
	_terminated = true;

	return kWordListNonZeroResult;
}

void WordListPlugin::freeState() {
	freeLevels(_root);
	_root = nullptr;
	_current = nullptr;

	for (uint16 i = 0; i < kWordListListCount; i++) {
		_resourceData[i].clear();
		_listData[i] = nullptr;
	}

	_rowTable.clear();
	_rows = 0;
	_externalHandle = 0;
	_listCount = 0;
	_externalList = false;
	_titleObject = 0;
	_terminated = false;
}

bool WordListPlugin::isListTerminated(const byte *data, uint32 size) {
	uint32 position = 0;

	while (position < size && data[position] != 0) {
		while (position < size && data[position] != 0)
			position++;

		position++;
	}

	return position < size;
}

} // End of namespace Topgun
