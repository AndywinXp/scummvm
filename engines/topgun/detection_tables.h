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

namespace Topgun {

const PlainGameDescriptor topgunGames[] = {
	{ "tuneland", "TuneLand Starring Howie Mandel" },
	{ "tunesplayer", "TuneLand Tunes Player" },
	{ "pythonwaste", "Monty Python's Complete Waste of Time" },
	{ "battlebeast", "Battle Beast" },
	{ "junglegames", "Timon & Pumbaa's Jungle Games" },
	{ 0, 0 }
};

const ADGameDescription gameDescriptions[] = {
	{
		"tuneland",
		nullptr,
		AD_ENTRY1s("HERALD.BIN", "9e811ce530048d37d1412f9ec64f0097", 162259),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"Demo",
		AD_ENTRY1s("SAMPLER.BIN", "9f25f15a70effb7f4c0c1d93cfa884e8", 120268),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tunesplayer",
		nullptr,
		AD_ENTRY1s("TUNES.EXE", "59193f3574a3b9bd6cde56aa73415db2", 410656),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO1(GUIO_NONE)
	},

	{
		"pythonwaste",
		nullptr,
		AD_ENTRY1s("HERALD.BIN", "7c57a11624726d2be3c0487716adf33d", 287772),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"battlebeast",
		nullptr,
		AD_ENTRY1s("BEAST.BIN", "f501fdf6a9a6ac7c141abf9b5c5a41e2", 4214131),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		nullptr,
		AD_ENTRY1s("JUNGMAIN.BIN", "49e89089db55c65b0f507f2718720254", 7148491),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		nullptr,
		AD_ENTRY1s("JUNGMAIN.BIN", "2848632c56f4775b20e6af82f99a3bfe", 7224580),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		nullptr,
		AD_ENTRY1s("JUNGMAIN.BIN", "76aecef25e36dd95e663b8cd3aa619fd", 7187831),
		Common::FR_FRA,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		nullptr,
		AD_ENTRY1s("JUNGMAIN.BIN", "446413cef0cea2fc4fbbaf10d838caa4", 7074945),
		Common::JA_JPN,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace Topgun
