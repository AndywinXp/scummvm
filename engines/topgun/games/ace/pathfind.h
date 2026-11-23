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

#ifndef TOPGUN_GAMES_ACE_PATHFIND_H
#define TOPGUN_GAMES_ACE_PATHFIND_H

#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	// 29999: grid value of a tile the search has not reached, this is the exact number the game expects
	kPathFindUnvisited = 0x752F,
	// Entries of the array that holds the back-traced steps
	kPathFindMaxSteps = 2000,
	// 32767: the grid is allocated with 16-bit byte sizes, so it holds at most 65534 bytes
	kPathFindMaxGridCells = 0x7FFF,
	kPathFindNoBuffers = -10,
	kPathFindListFull = -20,
	kPathFindNoPath = -30,
	kPathFindSameTile = -50
};

struct PathFindNode {
	int16 x;
	int16 y;
	uint16 unk_04;
	uint16 unk_06;
	int16 distance;
	uint16 dir;
};

struct PathFindEntry {
	int16 x;
	int16 y;
	uint16 dir;
};

class PathFindPlugin : public Plugin {
public:
	PathFindPlugin(PluginHost *host);
	~PathFindPlugin();

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	PluginHost *_host;

	int16 _xSize = 0;
	int16 _ySize = 0;
	int16 _nDirs = 0;
	int16 _maxPathLen = 0;
	int16 _maxBufLen = 0;
	uint16 _gridWords = 0;
	uint16 *_grid = nullptr;
	PathFindNode *_open = nullptr;
	PathFindEntry *_path = nullptr;

	int16 _delta[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
	int16 _backTrack[8] = {4, 5, 6, 7, 0, 1, 2, 3};
	int16 _stepDist[8] = {1, 1, 1, 1, 1, 1, 1, 1};
	int16 _iso[8][2] = {{1, 1}, {0, 2}, {-1, 1}, {-2, 0}, {-1, -1}, {1, -2}, {0, -1}, {2, 0}};
	int16 _nextDir[8] = {2, 3, 4, 5, 6, 0, 7, 1};

	int16 _openCount = 0;
	int16 _foundPath = 0;
	int16 _shortest = 0;
	int16 _dir = 0;
	int16 _newX = 0;
	int16 _newY = 0;
	uint16 _pathDirs[kPathFindMaxSteps] = {};

	uint16 tileIndex(int16 x, int16 y) const;
	uint16 initBestPath(uint16 xSize, uint16 ySize, uint16 maxPathLen, uint16 maxBufLen, uint16 nDirs);
	uint16 freeBestPath();
	int16 getBestPath(int16 xPos, int16 yPos, int16 xDes, int16 yDes, uint16 obstructionScript, uint16 returnScript, uint16 fullPath);
	void findNextDir();
	int16 calcDist(int16 x1, int16 y1, int16 x2, int16 y2) const;
	uint16 isObstructed(uint16 tileOffset, int16 x, int16 y, uint16 script);
	void savePathCoord(int16 x, int16 y, uint16 dir, uint16 script);
	void freeState();
	bool initState();
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_ACE_PATHFIND_H
