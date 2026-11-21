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
		GUIO1(GUIO_NONE)
	},

	{
		"tuneland",
		"Demo",
		AD_ENTRY1s("SAMPLER.BIN", "9f25f15a70effb7f4c0c1d93cfa884e8", 120268),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO1(GUIO_NONE)
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

	AD_TABLE_END_MARKER
};

} // End of namespace Topgun
