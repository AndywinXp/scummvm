/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if !defined(SCUMM_IMUSE_DIGI_V2_TRIGGERS_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_V2_TRIGGERS_H

#include "common/scummsys.h"
#include "common/textconsole.h"
#include "common/util.h"

namespace Scumm {
#define MAX_TRIGGERS 8
#define MAX_DEFERS 8

	typedef struct {
		int sound;
		char text[256];
		int opcode;
		int args_0_;
		int args_1_;
		int args_2_;
		int args_3_;
		int args_4_;
		int args_5_;
		int args_6_;
		int args_7_;
		int args_8_;
		int args_9_;
		int clearLater;
	} DiMUSETrigger;

	typedef struct {
		int counter;
		int opcode;
		int args_0_;
		int args_1_;
		int args_2_;
		int args_3_;
		int args_4_;
		int args_5_;
		int args_6_;
		int args_7_;
		int args_8_;
		int args_9_;
	} DiMUSEDefer;

	class DiMUSETriggersHandler {

	private:
		DiMUSE_v2 *_engine;
		DiMUSETrigger trigs[MAX_TRIGGERS];
		DiMUSEDefer defers[MAX_DEFERS];

		int  triggers_defersOn;
		int  triggers_midProcessing;
		char triggers_textBuffer[256];
		char triggers_empty_marker = '\0';
	public:
		DiMUSETriggersHandler(DiMUSE_v2 *engine);
		~DiMUSETriggersHandler();

		int  triggers_moduleInit();
		int  triggers_clear();
		int  triggers_save(int *buffer, int bufferSize);
		int  triggers_restore(int *buffer);
		int  triggers_setTrigger(int soundId, char *marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
		int  triggers_checkTrigger(int soundId, char *marker, int opcode);
		int  triggers_clearTrigger(int soundId, char *marker, int opcode);
		void triggers_processTriggers(int soundId, char *marker);
		int  triggers_deferCommand(int count, int opcode, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
		void triggers_loop();
		int  triggers_countPendingSounds(int soundId);
		int  triggers_moduleFree();

		
	};

} // End of namespace Scumm
#endif
