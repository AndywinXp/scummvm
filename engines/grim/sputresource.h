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

#ifndef GRIM_SPUTRESOURCE_H
#define GRIM_SPUTRESOURCE_H

#include "common/endian.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"

namespace Grim {

// The tag values are the reversed characters
enum SputResourceTags {
	kSputTagCmap = MKTAG('P', 'A', 'M', 'C'),
	kSputTagCost = MKTAG('T', 'S', 'O', 'C'),
	kSputTagKeyf = MKTAG('F', 'Y', 'E', 'K'),
	kSputTagMat = MKTAG(' ', 'T', 'A', 'M'),
	kSputTagModl = MKTAG('L', 'D', 'O', 'M')
};

class SputResource : public ResHandler {
public:
	SputResource(Res *pRes, RenderDroid *pRd);

	int handler(ResHandlerMsg *pMsg);

	int stopPurge(int state);
	void *alloc(int resType, const char *name);
	int closeResources();
	int findResources();
	int addResourcePath(const char *path);
	int initialize(SysStruct *pHS);
	int logToFile(const char *path);
	int terminate();
	int startup();
	void saveVerifyCallback(const char *fileName);
	int shutdown();
	int registerHandlers();
	int setMemoryUsage(int totalAllowedLimit, int purgeAmount);
	int loadResCmap(ResHandlerMsg *pMsg);
	int loadResKeyf(ResHandlerMsg *pMsg);
	int loadResMat(ResHandlerMsg *pMsg);
	int loadResModl(ResHandlerMsg *pMsg);
	int cmap();
	RdMaterial *mat(char *path, int bVideoMemory, int bVideoMemoryOnly);
	int unlock(RdMaterial *pMaterial);
	int reportAllocations();

private:
	int addResourcePathInternal(const char *path);

	Res *_pRes;
	RenderDroid *_pRd;
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	int _bLoaderLocked;
	int _totalAllowedLimit;
	int _purgeAmount;
	Common::String _extraDataPath;
	RdMaterialLoaderFunc _oldMatLoader;
	RdMaterialUnloaderFunc _oldMatUnloader;
};

} // End of namespace Grim

#endif
