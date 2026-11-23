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

#include "common/textconsole.h"

#include "topgun/games/ace/pathfind.h"

namespace Topgun {

enum {
	kFunctionInitBestPath,
	kFunctionFreeBestPath,
	kFunctionGetBestPath,
	kFunctionCount
};

PathFindPlugin::PathFindPlugin(PluginHost *host) : _host(host) {
}

PathFindPlugin::~PathFindPlugin() {
	freeState();
}

bool PathFindPlugin::init() {
	_xSize = 0;
	_ySize = 0;
	_nDirs = 0;
	_maxPathLen = 0;
	_maxBufLen = 0;
	_gridWords = 0;

	if (!initState()) {
		freeState();
		return false;
	}

	return true;
}

void PathFindPlugin::free() {
	freeState();
}

int PathFindPlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("InitBestPath"))
		return kFunctionInitBestPath;
	if (name.equalsIgnoreCase("FreeBestPath"))
		return kFunctionFreeBestPath;
	if (name.equalsIgnoreCase("GetBestPath"))
		return kFunctionGetBestPath;

	return -1;
}

uint16 PathFindPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("PathFindPlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 0;
	switch (function) {
	case kFunctionFreeBestPath:
		expected = 0;
		break;
	case kFunctionInitBestPath:
		expected = 5;
		break;
	case kFunctionGetBestPath:
		expected = 7;
		break;
	}

	if (count < expected)
		warning("PathFindPlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[7];
	for (int i = 0; i < 7; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionInitBestPath:
		return initBestPath(values[0], values[1], values[2], values[3], values[4]);
	case kFunctionFreeBestPath:
		return freeBestPath();
	case kFunctionGetBestPath:
		return (uint16)getBestPath((int16)values[0], (int16)values[1], (int16)values[2], (int16)values[3], values[4], values[5], values[6]);
	}

	return 0;
}

uint16 PathFindPlugin::tileIndex(int16 x, int16 y) const {
	return (uint16)(y * _xSize + x);
}

uint16 PathFindPlugin::initBestPath(uint16 xSize, uint16 ySize, uint16 maxPathLen, uint16 maxBufLen, uint16 nDirs) {
	_xSize = (int16)xSize;
	_ySize = (int16)ySize;
	_nDirs = (int16)nDirs;
	_maxPathLen = (int16)maxPathLen;
	_maxBufLen = (int16)maxBufLen;

	// Buffers of a previous call that was not followed by freeBestPath()
	delete[] _grid;
	_grid = nullptr;
	delete[] _open;
	_open = nullptr;
	delete[] _path;
	_path = nullptr;

	uint16 size = (uint16)((uint16)(_xSize * _ySize) << 1);
	if (size == 0)
		return 0;

	// The original overruns a grid that does not fit its 16-bit allocation size
	uint32 cells = (uint32)xSize * ySize;
	if (cells > kPathFindMaxGridCells) {
		warning("PathFindPlugin::initBestPath(): a grid of %d x %d tiles is too big", xSize, ySize);
		return 0;
	}

	_gridWords = (uint16)cells;
	_grid = new uint16[_gridWords]();

	size = (uint16)((uint16)_maxBufLen * sizeof(PathFindNode));
	if (size == 0) {
		delete[] _grid;
		_grid = nullptr;
		return 0;
	}

	// The search list is allocated with room for the nodes the original appends past its end
	uint32 nodes = (uint16)_maxBufLen;
	if (_nDirs > 0)
		nodes += (uint16)_nDirs;
	_open = new PathFindNode[nodes]();

	size = (uint16)((uint16)_maxBufLen * sizeof(PathFindEntry));
	if (size == 0) {
		delete[] _open;
		_open = nullptr;
		delete[] _grid;
		_grid = nullptr;
		return 0;
	}

	_path = new PathFindEntry[(uint16)_maxBufLen]();

	return 1;
}

uint16 PathFindPlugin::freeBestPath() {
	delete[] _path;
	_path = nullptr;

	delete[] _open;
	_open = nullptr;

	delete[] _grid;
	_grid = nullptr;

	return 1;
}

int16 PathFindPlugin::getBestPath(int16 xPos, int16 yPos, int16 xDes, int16 yDes, uint16 obstructionScript, uint16 returnScript, uint16 fullPath) {
	int16 shortDir = 0;
	uint16 tile = 0;
	uint16 len = 0;
	int16 step = 0;
	int16 shortStep = 0;
	int16 i = 0;
	int16 j = 0;
	int16 done = 0;
	int16 posX = 0;
	int16 posY = 0;
	int16 shortX = 0;
	int16 shortY = 0;
	int16 navDir = 0;
	uint16 tileOffset = 0;
	uint16 startOffset = 0;
	uint16 parentOffset = 0;
	uint16 parentCost = 0;

	if (!_grid || !_open || !_path)
		return kPathFindNoBuffers;

	if (xPos == xDes && yPos == yDes)
		return kPathFindSameTile;

	for (i = 0; i < (int16)(_xSize * _ySize); i++)
		_grid[i] = kPathFindUnvisited;

	for (uint16 node = 0; node < (uint16)_maxBufLen; node++)
		_open[node] = PathFindNode();

	for (i = 0; i < _maxBufLen; i++)
		_open[i].distance = kPathFindUnvisited;

	_openCount = 1;
	_open[0].x = xPos;
	_open[0].y = yPos;
	_open[0].distance = calcDist(xPos, yPos, xDes, yDes);

	startOffset = tileIndex(xPos, yPos);
	if (startOffset < _gridWords)
		_grid[startOffset] = 0;
	else
		warning("PathFindPlugin::getBestPath(): the start tile %d, %d is outside the grid", xPos, yPos);

	_foundPath = 0;
	done = 0;

	while (_foundPath == 0 && done != 1) {
		if (_openCount >= _maxBufLen)
			return kPathFindListFull;

		_shortest = 0;
		for (i = 0; i < (int16)(_openCount - 1); i++) {
			if (_open[i].distance < _open[_shortest].distance)
				_shortest = i;
		}

		posX = _open[_shortest].x;
		posY = _open[_shortest].y;
		_open[_shortest] = _open[_openCount - 1];
		_openCount--;

		_newX = posX;
		_newY = posY;
		_dir = 0;
		_newY = (int16)(_newY - 1);

		for (i = 0; i < _nDirs; i++) {
			tileOffset = tileIndex(_newX, _newY);
			tile = isObstructed(tileOffset, _newX, _newY, obstructionScript);
			if (tile == 0)
				len = _grid[tileOffset];

			if (len == kPathFindUnvisited && tile != 1) {
				parentOffset = tileIndex(posX, posY);
				if (parentOffset < _gridWords) {
					parentCost = _grid[parentOffset];
				} else {
					warning("PathFindPlugin::getBestPath(): the tile %d, %d is outside the grid", posX, posY);
					parentCost = 0;
				}

				_grid[tileOffset] = (uint16)(parentCost + _stepDist[_dir]);

				if (_openCount >= _maxBufLen)
					warning("PathFindPlugin::getBestPath(): the search list grows past its %d entries", _maxBufLen);

				_open[_openCount].x = _newX;
				_open[_openCount].y = _newY;
				_open[_openCount].dir = _dir;
				_open[_openCount].distance = calcDist(_newX, _newY, xDes, yDes);
				_openCount++;
			}

			if (_newX == xDes && _newY == yDes) {
				_foundPath = 1;
				break;
			}

			findNextDir();
		}

		if (_openCount == 0)
			done = 1;
	}

	if (_foundPath != 1)
		return kPathFindNoPath;

	step = 0;
	posX = xDes;
	posY = yDes;

	while (!(posX == xPos && posY == yPos) && step < _maxBufLen) {
		step++;

		_newX = posX;
		_newY = posY;
		shortX = posX;
		shortY = posY;
		shortStep = kPathFindUnvisited;
		_dir = 0;
		_newY = (int16)(_newY - 1);

		for (i = 0; i < _nDirs; i++) {
			if (_newX >= 0 && _newX < _xSize && _newY >= 0 && _newY < _ySize) {
				tileOffset = tileIndex(_newX, _newY);
				if ((int16)_grid[tileOffset] < shortStep) {
					shortStep = (int16)_grid[tileOffset];
					shortX = _newX;
					shortY = _newY;
					shortDir = _dir;
				}
			}

			findNextDir();
		}

		_newX = shortX;
		_newY = shortY;
		posX = _newX;
		posY = _newY;

		if (step - 1 >= kPathFindMaxSteps) {
			warning("PathFindPlugin::getBestPath(): the path has more than %d steps", kPathFindMaxSteps);
			return kPathFindListFull;
		}

		_pathDirs[step - 1] = _backTrack[shortDir];
	}

	posX = xPos;
	posY = yPos;

	for (i = 0; i < step; i++) {
		navDir = (int16)_pathDirs[step - 1 - i];
		posX = (int16)(posX + _delta[navDir][0]);
		posY = (int16)(posY + _delta[navDir][1]);

		_path[i].x = posX;
		_path[i].y = posY;
		_path[i].dir = navDir;
	}

	if (step == kPathFindMaxSteps)
		return kPathFindListFull;

	j = 0;
	for (i = 0; i < step; i++) {
		// The entry after the last one is never read
		if (i != step - 1 && _path[i].dir == _path[i + 1].dir && fullPath == 0)
			continue;

		if (j >= _maxPathLen)
			return j;

		savePathCoord(_path[i].x, _path[i].y, _path[i].dir, returnScript);
		j++;
	}

	return j;
}

void PathFindPlugin::findNextDir() {
	_newX = (int16)(_newX + _iso[_dir][0]);
	_newY = (int16)(_newY + _iso[_dir][1]);
	_dir = _nextDir[_dir];
}

int16 PathFindPlugin::calcDist(int16 x1, int16 y1, int16 x2, int16 y2) const {
	uint32 deltaY = (uint16)(y2 - y1);
	uint32 deltaX = (uint16)(x2 - x1);

	return (int16)(deltaY * deltaY + deltaX * deltaX);
}

uint16 PathFindPlugin::isObstructed(uint16 tileOffset, int16 x, int16 y, uint16 script) {
	if (x < 0 || x >= _xSize || y < 0 || y >= _ySize)
		return 1;

	uint16 arguments[3];
	arguments[0] = (uint16)x;
	arguments[1] = (uint16)y;
	arguments[2] = tileOffset;

	uint16 result = _host->runMessage(script, arguments, 3);
	if (result != 0)
		return 1;

	return 0;
}

void PathFindPlugin::savePathCoord(int16 x, int16 y, uint16 dir, uint16 script) {
	uint16 arguments[3];
	arguments[0] = (uint16)x;
	arguments[1] = (uint16)y;
	arguments[2] = dir;

	_host->runMessage(script, arguments, 3);
}

void PathFindPlugin::freeState() {
	delete[] _grid;
	_grid = nullptr;

	delete[] _open;
	_open = nullptr;

	delete[] _path;
	_path = nullptr;
}

bool PathFindPlugin::initState() {
	_grid = nullptr;
	_open = nullptr;
	_path = nullptr;

	return true;
}

} // End of namespace Topgun
