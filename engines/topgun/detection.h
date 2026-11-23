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

#ifndef TOPGUN_DETECTION_H
#define TOPGUN_DETECTION_H

#include "engines/advancedDetector.h"

namespace Topgun {

enum TopgunDebugChannels {
	kDebugGraphics = 1,
	kDebugPath,
	kDebugScan,
	kDebugFilePath,
	kDebugScript,
};

extern const PlainGameDescriptor topgunGames[];

extern const char *const directoryGlobs[];

extern const ADGameDescription gameDescriptions[];

#define GAMEOPTION_ORIGINAL_SAVELOAD GUIO_GAMEOPTIONS1
#define GAMEOPTION_SCREEN_800X600 GUIO_GAMEOPTIONS2
#define GAMEOPTION_HIGH_BIT_DEPTH GUIO_GAMEOPTIONS3

} // End of namespace Topgun

class TopgunMetaEngineDetection : public AdvancedMetaEngineDetection<ADGameDescription> {
	static const DebugChannelDef debugFlagList[];

public:
	TopgunMetaEngineDetection();
	~TopgunMetaEngineDetection() override {}

	const char *getName() const override {
		return "topgun";
	}

	const char *getEngineName() const override {
		return "Topgun";
	}

	const char *getOriginalCopyright() const override {
		return "Topgun (C)";
	}

	const DebugChannelDef *getDebugChannels() const override {
		return debugFlagList;
	}
};

#endif // TOPGUN_DETECTION_H
