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

#ifndef GRIM_EMI_CSND_H
#define GRIM_EMI_CSND_H

#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/resources/lresource.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/emi/sound/lsound.h"

namespace Grim {

class CSnd : virtual public LResource {
public:
	CSnd();
	~CSnd() override;

	LResourceHandle<LSound> *getSoundHandle();
	void setSoundHandle(const LResourceHandle<LSound> &handle);

	float getPrio();
	float getPan();
	float getVolume();
	float getMvol();
	float getFreq();
	float getCutoff0();
	float getCutoff1();
	float getField24();
	float getField28();
	float getStDelay();
	float getRndDelay0();
	float getRndDelay1();
	int getFlags();
	int getGroup();

	void setPrio(float value);
	void setPan(float value);
	void setVolume(float value);
	void setMvol(float value);
	void setFreq(float value);
	int setCutoff0(float value);
	int setCutoff1(float value);
	float setField24(float value);
	float setField28(float value);
	void setStDelay(float value);
	void setRndDelay0(float value);
	void setRndDelay1(float value);
	void setFlags(int value);
	void setGroup(int value);

	bool save(LFile *file);

	float volume;
	float mvol;
	float prio;
	float pan;
	float field24;
	float field28;
	float cutoff0;
	float cutoff1;
	int flags;
	int group;
	float stDelay;
	float rndDelay0;
	float rndDelay1;
	float freq;
	LResourceHandle<LSound> sound;

	static const LResourceType sndType;

protected:
	bool resLoadItem(const char *itemName) override;
};

} // End of namespace Grim

#endif
