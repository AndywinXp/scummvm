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

#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcontrol.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/sputtime.h"

namespace Grim {

int GrimEngine::sputInitialize(SysStruct *pHS) {
	_pSputHS = pHS;

	int result = callRegisterModules(pHS, 0, kNumBaseModules);

	if (!result)
		result = callRegisterModules(pHS, kNumBaseModules, kNumMainModules);

	_bSputInitialized = result == 0;

	return result;
}

int GrimEngine::sputTerminate() {
	_pSputHS = nullptr;

	if (_bSputInitialized) {
		callRelease(kNumBaseModules, kNumMainModules);
		callRelease(0, kNumBaseModules);
		_bSputInitialized = 0;
	}

	return 0;
}

int GrimEngine::sputStartup() {
	if (_bSputStarted)
		return 1;

	int result = runInitTable(0, kNumBaseModules);

	if (!result) {
		int physicalRam = 0x7FFFFFFF; // TODO: stdPlatform_GetPhysicalRam()

		if (physicalRam > 0x1000000) {
			// TODO: iMUSE_DeclareBufferSizes with 9 tracks
			int memoryLimit = _pRes->getMemoryLimit();

			if (memoryLimit >= (int)(physicalRam - 0x800000))
				memoryLimit = physicalRam - 0x800000;

			_pSputResource->setMemoryUsage(memoryLimit, 0);
		} else {
			// TODO: iMUSE_DeclareBufferSizes with 6 tracks
			_pSputResource->setMemoryUsage(10485760, 0);
		}

		result = runInitTable(kNumBaseModules, kNumMainModules);
	}

	_bSputStarted = result == 0;

	return result;
}

int GrimEngine::sputShutdown() {
	callTableFunc(kNumBaseModules, kNumMainModules);

	int result = callTableFunc(0, kNumBaseModules);

	_bSputStarted = 0;

	return result;
}

int GrimEngine::callRegisterModules(SysStruct *pHS, int firstModule, int count) {
	for (int i = 0; i < count; i++) {
		if (initializeModule(firstModule + i, pHS)) {
			for (int j = i - 1; j >= 0; j--)
				terminateModule(firstModule + j);

			return 1;
		}
	}

	return 0;
}

int GrimEngine::runInitTable(int firstModule, int count) {
	for (int i = 0; i < count; i++) {
		if (startupModule(firstModule + i)) {
			for (int j = i - 1; j >= 0; j--)
				shutdownModule(firstModule + j);

			return 1;
		}
	}

	return 0;
}

int GrimEngine::callTableFunc(int firstModule, int count) {
	for (int i = count - 1; i >= 0; i--)
		shutdownModule(firstModule + i);

	return 0;
}

int GrimEngine::callRelease(int firstModule, int count) {
	for (int i = count - 1; i >= 0; i--)
		terminateModule(firstModule + i);

	return 0;
}

int GrimEngine::initializeModule(int module, SysStruct *pHS) {
	switch (module) {
	case kSputMainModuleRoom:
		return _pSputRoom->initialize(pHS);
	case kSputMainModuleTime:
		return _pSputTime->initialize(pHS);
	case kSputMainModuleLua:
		// TODO: sputLua_Initialize
		return 0;
	case kSputMainModuleCostume:
		return _pSputCostume->initialize(pHS);
	case kSputMainModuleActor:
		return _pSputActor->initialize(pHS);
	case kSputMainModuleResource:
		return _pSputResource->initialize(pHS);
	case kSputMainModuleControl:
		return _pSputControl->initialize(pHS);
	case kSputMainModuleSounds:
		// TODO: SoundsInitialize (IMuse is created by the engine)
		return 0;
	case kSputMainModuleRender:
		return _pSputRender->initialize(pHS);
	case kSputMainModuleText:
		return _pSputText->initialize(pHS);
	case kSputMainModulePrimitive:
		return _pSputPrimitive->initialize(pHS);
	case kSputMainModuleSmush:
		return _pSputSmush->initialize(pHS);
	default:
		return 0;
	}
}

int GrimEngine::startupModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		return _pSputRoom->startup();
	case kSputMainModuleTime:
		return _pSputTime->startup();
	case kSputMainModuleLua:
		// TODO: sputLua_Startup
		return 0;
	case kSputMainModuleCostume:
		return _pSputCostume->startup();
	case kSputMainModuleActor:
		return _pSputActor->startup();
	case kSputMainModuleResource:
		return _pSputResource->startup();
	case kSputMainModuleControl:
		return _pSputControl->startup();
	case kSputMainModuleSounds:
		// TODO: sounds_startup (IMuse is started by the engine)
		return 0;
	case kSputMainModuleRender:
		return _pSputRender->startup();
	case kSputMainModuleText:
		return _pSputText->startup();
	case kSputMainModulePrimitive:
		return _pSputPrimitive->startup();
	case kSputMainModuleSmush:
		return _pSputSmush->startup();
	default:
		return 0;
	}
}

void GrimEngine::shutdownModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		_pSputRoom->shutdown();
		break;
	case kSputMainModuleTime:
		_pSputTime->shutdown();
		break;
	case kSputMainModuleLua:
		// TODO: sputLua_Shutdown
		break;
	case kSputMainModuleCostume:
		_pSputCostume->shutdown();
		break;
	case kSputMainModuleActor:
		_pSputActor->shutdown();
		break;
	case kSputMainModuleResource:
		_pSputResource->shutdown();
		break;
	case kSputMainModuleControl:
		_pSputControl->shutdown();
		break;
	case kSputMainModuleSounds:
		// TODO: sounds_shutdown (IMuse is destroyed by the engine)
		break;
	case kSputMainModuleRender:
		_pSputRender->shutdown();
		break;
	case kSputMainModuleText:
		_pSputText->shutdown();
		break;
	case kSputMainModulePrimitive:
		_pSputPrimitive->shutdown();
		break;
	case kSputMainModuleSmush:
		_pSputSmush->shutdown();
		break;
	default:
		break;
	}
}

void GrimEngine::terminateModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		_pSputRoom->terminate();
		break;
	case kSputMainModuleTime:
		_pSputTime->terminate();
		break;
	case kSputMainModuleLua:
		// TODO: sputLua_Terminate
		break;
	case kSputMainModuleCostume:
		_pSputCostume->terminate();
		break;
	case kSputMainModuleActor:
		_pSputActor->terminate();
		break;
	case kSputMainModuleResource:
		_pSputResource->terminate();
		break;
	case kSputMainModuleControl:
		_pSputControl->terminate();
		break;
	case kSputMainModuleSounds:
		// TODO: SoundsTerminate (IMuse is destroyed by the engine)
		break;
	case kSputMainModuleRender:
		_pSputRender->terminate();
		break;
	case kSputMainModuleText:
		_pSputText->release();
		break;
	case kSputMainModulePrimitive:
		_pSputPrimitive->terminate();
		break;
	case kSputMainModuleSmush:
		_pSputSmush->terminate();
		break;
	default:
		break;
	}
}

int GrimEngine::bundleDofile() {
	int bFound = 1; // TODO: sputLua_FindSystemFiles()

	return !bFound;
}

int GrimEngine::startEngine(int saveGameResume, const char *bootParam) {
	if (_bSputOpen)
		return 1;

	int bBooted = 1; // TODO: sputLua_Boot(saveGameResume, bootParam)

	if (!bBooted)
		return 1;

	_libMode = 2;
	_pSputRender->open();

	if (!_pSputControl->open())
		return 1;

	_bSputOpen = 1;

	return 0;
}

int GrimEngine::loop(int bRender, int manualTicks) {
	if (_libSaveRequest)
		save();

	if (_libLoadRequest)
		restore();

	setPerfCounter(0);
	setPerfCounter(10);
	_pImuse->soundsPeriodic();
	queryPerfCounterDiff(10);

	int result = _libMode;

	if (_libMode) {
		setPerfCounter(1);
		_pSputControl->update(_pSputTime->getFrameTimeFlex());
		queryPerfCounterDiff(1);

		int mode = _libMode;

		if (mode != 1 || manualTicks) {
			_loopCount++;

			if (manualTicks)
				_pSputTime->advanceManual(manualTicks);
			else
				_pSputTime->advance();

			if (_bRunLua) {
				setPerfCounter(2);
				// TODO: sputLua_Update()
				queryPerfCounterDiff(2);
			}

			mode = _libMode;

			if (mode != 3 && mode != 4) {
				setPerfCounter(3);
				_pSputActor->updateActors(_pSputTime->getFrameTimeFlex());
				queryPerfCounterDiff(3);
				mode = _libMode;
			}
		}

		if (bRender && (mode != 1 || manualTicks)) {
			_pSputRender->update();
			_bRenderPending = 1;
		}

		queryPerfCounterDiff(0);

		result = _libImuseState;

		if (result != -1) {
			result = _pImuse->imSetState(result);
			_libImuseState = -1;
		}
	}

	return result;
}

void GrimEngine::timedPageFlip() {
	setPerfCounter(8);

	switch (_libMode) {
	case 3:
		if (!_pSputRender->queryDisplayState())
			_pSputRender->pageFlip();

		break;
	case 2:
		_pSputRender->pageFlip();
		break;
	case 1:
		if (_bRenderPending) {
			_pSputRender->pageFlip();
			_bRenderPending = 0;
		}

		break;
	default:
		break;
	}

	queryPerfCounterDiff(8);
}

void GrimEngine::setPerfCounter(uint slot) {
	if (slot >= (uint)kNumPerfCounters)
		error("GrimEngine::setPerfCounter(): slot %u out of range", slot);

	_aPerfCounters[slot].start = (double)g_system->getMillis();
}

int GrimEngine::queryPerfCounterDiff(uint slot) {
	if (slot >= (uint)kNumPerfCounters)
		error("GrimEngine::queryPerfCounterDiff(): slot %u out of range", slot);

	double elapsed = (double)g_system->getMillis() - _aPerfCounters[slot].start;

	_aPerfCounters[slot].elapsed = elapsed;

	return (int)elapsed;
}

void GrimEngine::setPrevMode(int mode) {
	_sputPrevMode = mode;
}

int GrimEngine::getPrevMode() const {
	return _sputPrevMode;
}

} // End of namespace Grim
