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

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::loadStats(Fighter *f) {
	uint16 *host = _host->variableTable();

	f->fightAttacks = 0;
	f->fightHits = 0;

	if (f->player == 0) {
		f->totalAttacks = host[kVarStatAttacks0];
		f->totalHits = host[kVarStatHits0];
	} else {
		f->totalAttacks = host[kVarStatAttacks1];
		f->totalHits = host[kVarStatHits1];
	}
}

void BattlePlugin::storeStats(Fighter *f) {
	uint16 *host = _host->variableTable();

	uint16 percentage = 0;
	if (f->totalAttacks != 0)
		percentage = (uint16)((f->totalHits * 100) / f->totalAttacks);

	if (f->player == 0) {
		host[kVarStatAttacks0] = (uint16)f->totalAttacks;
		host[kVarStatHits0] = (uint16)f->totalHits;
		host[kVarStatPercentage0] = percentage;
	} else {
		host[kVarStatAttacks1] = (uint16)f->totalAttacks;
		host[kVarStatHits1] = (uint16)f->totalHits;
		host[kVarStatPercentage1] = percentage;
	}
}

uint16 BattlePlugin::scoreAttack(Fighter *f, uint16 hit, uint16 move, uint16 value) {
	uint16 *host = _host->variableTable();

	setAttackActive(f, 0);

	f->fightAttacks++;
	f->totalAttacks++;

	if (hit == 0) {
		f->combo = 0;
	} else {
		f->fightHits++;
		f->totalHits++;
		_ctx->lastScorer = f->player;

		if (g_system->getMillis() > f->comboEnd)
			f->combo = 0;

		f->combo = (f->combo << 8) | f->move;

		const ComboRecord *record = f->comboTab;
		if (record->pattern != 0xFFFFFFFF) {
			do {
				if ((record->mask & f->combo) == record->pattern) {
					uint16 code;
					if (record->mask == 0x000000FF)
						code = 1;
					else if (record->mask == 0x0000FFFF)
						code = 2;
					else if (record->mask == 0x00FFFFFF)
						code = 3;
					else
						code = 4;

					_host->callScript(kScriptCombo, f->player, code);
					break;
				}

				record++;
			} while (record->pattern != 0xFFFFFFFF);
		}

		_host->callScript(kScriptAddScore, f->player, value);

		if ((f->player == 0 && (host[kVarOptions2] & 0x1000)) || (f->player == 1 && (host[kVarOptions2] & 0x2000)))
			_host->callScript(kScriptAddScore, f->player, value);
	}

	aiEvent(f, hit == 0 ? kAiEventAttackMissed : kAiEventHitLanded);
	return hit;
}

void BattlePlugin::beastSaveRestore(uint16 save, uint16 nameVar) {
	char section[0x80];

	const char *name = _host->getString(nameVar);
	if (name == nullptr) {
		warning("BattlePlugin::beastSaveRestore(): string %d does not exist", nameVar);
		name = "";
	}
	Common::strlcpy(section, name, sizeof(section));

	Fighter *f = &_ctx->fighters[0];
	Fighter *g = &_ctx->fighters[1];

	if (save != 0) {
		Common::String text = Common::String::format("%d", (int16)f->fightAttacks);
		_host->writePrivateProfileString(section, "LSA11", text.c_str());

		text = Common::String::format("%d", (int16)f->fightHits);
		_host->writePrivateProfileString(section, "LSH11", text.c_str());

		text = Common::String::format("%d", (int16)f->totalAttacks);
		_host->writePrivateProfileString(section, "LTA11", text.c_str());

		text = Common::String::format("%d", (int16)f->totalAttacks);
		_host->writePrivateProfileString(section, "LTH11", text.c_str());

		text = Common::String::format("%d", (int16)g->fightAttacks);
		_host->writePrivateProfileString(section, "LSA11", text.c_str());

		text = Common::String::format("%d", (int16)g->fightHits);
		_host->writePrivateProfileString(section, "LSH11", text.c_str());

		text = Common::String::format("%d", (int16)g->totalAttacks);
		_host->writePrivateProfileString(section, "LTA11", text.c_str());

		text = Common::String::format("%d", (int16)g->totalAttacks);
		_host->writePrivateProfileString(section, "LTH11", text.c_str());
	} else {
		f->fightAttacks = _host->getPrivateProfileInt(section, "LSA11", 0);
		f->fightHits = _host->getPrivateProfileInt(section, "LSH11", 0);
		f->totalAttacks = _host->getPrivateProfileInt(section, "LTA11", 0);
		f->totalHits = _host->getPrivateProfileInt(section, "LTH11", 0);

		g->fightAttacks = _host->getPrivateProfileInt(section, "LSA12", 0);
		g->fightHits = _host->getPrivateProfileInt(section, "LSH12", 0);
		g->totalAttacks = _host->getPrivateProfileInt(section, "LTA12", 0);
		g->totalHits = _host->getPrivateProfileInt(section, "LTH12", 0);
	}
}

} // End of namespace Topgun
