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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/fileutil.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/lua/lauxlib.h"
#include "engines/grim/lua/luadebug.h"

namespace Grim {

static const char *const fieldNames[] = {
	"x", "y", "font", "width", "height", "hicolor", "fgcolor", "bgcolor",
	"fxcolor", "duration", "layer", "center", "ljustify", "rjustify", "volume", "pan"
};

static struct luaL_reg monkeyMainOpcodes[] = {
	{ "EnginePause", LUA_OPCODE(Lua_V2, EnginePause) },
	{ "EngineResume", LUA_OPCODE(Lua_V2, EngineResume) },
	{ "EngineDisplay", LUA_OPCODE(Lua_V2, EmptyFunction) },
	{ "SpewStartup", LUA_OPCODE(Lua_V2, EmptyFunction) },
	{ "SpewShutdown", LUA_OPCODE(Lua_V2, EmptyFunction) },
	{ "GetCurrentScript", LUA_OPCODE(Lua_V1, GetCurrentScript) },
	{ "GetPhysicalBytes", LUA_OPCODE(Lua_V2, GetPhysicalBytes) },
	{ "GetMemoryUsage", LUA_OPCODE(Lua_V2, GetMemoryUsage) },
	{ "SetMemoryUsage", LUA_OPCODE(Lua_V2, SetMemoryUsage) },
	{ "GetDiskFreeSpace", LUA_OPCODE(Lua_V1, GetDiskFreeSpace) },
	{ "SetActiveCD", LUA_OPCODE(Lua_V2, SetActiveCD) },
	{ "GetActiveCD", LUA_OPCODE(Lua_V2, GetActiveCD) },
	{ "AreWeInternational", LUA_OPCODE(Lua_V2, AreWeInternational) },
	{ "FileFindFirst", LUA_OPCODE(Lua_V2, FileFindFirst) },
	{ "FileFindNext", LUA_OPCODE(Lua_V2, FileFindNext) },
	{ "FileFindDispose", LUA_OPCODE(Lua_V2, FileFindDispose) },
	{ "TextFileGetLineCount", LUA_OPCODE(Lua_V2, TextFileGetLineCount) },
	{ "TextFileGetLine", LUA_OPCODE(Lua_V2, TextFileGetLine) },
	{ "InputDialog", LUA_OPCODE(Lua_V2, InputDialog) },
	{ "QueryDialog", LUA_OPCODE(Lua_V2, QueryDialog) },
	{ "WriteRegistryValue", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ReadRegistryValue", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "Load", LUA_OPCODE(Lua_V1, Load) },
	{ "Save", LUA_OPCODE(Lua_V1, Save) },
	{ "ScreenShot", LUA_OPCODE(Lua_V2, ScreenShot) },
	{ "MakeScreenTextures", LUA_OPCODE(Lua_V2, MakeScreenTextures) },
	{ "SubmitSaveGameData", LUA_OPCODE(Lua_V2, SubmitSaveGameData) },
	{ "GetSaveGameData", LUA_OPCODE(Lua_V2, GetSaveGameData) },
	{ "JustLoaded", LUA_OPCODE(Lua_V1, JustLoaded) },
	{ "ScreenshotForSavegame", LUA_OPCODE(Lua_V2, ScreenshotForSavegame) },
	{ "ThumbnailFromScreen", LUA_OPCODE(Lua_V2, ThumbnailFromScreen) },
	{ "ThumbnailFromFile", LUA_OPCODE(Lua_V2, ThumbnailFromFile) },
	{ "ClearSpecialtyTexture", LUA_OPCODE(Lua_V2, ClearSpecialtyTexture) },
	{ "LoadActor", LUA_OPCODE(Lua_V2, LoadActor) },
	{ "UnloadActor", LUA_OPCODE(Lua_V2, UnloadActor) },
	{ "GetActorTimeScale", LUA_OPCODE(Lua_V2, GetActorTimeScale) },
	{ "SetActorTimeScale", LUA_OPCODE(Lua_V2, SetActorTimeScale) },
	{ "SetActorWalkDominate", LUA_OPCODE(Lua_V2, SetActorWalkDominate) },
	{ "SetSelectedActor", LUA_OPCODE(Lua_V2, SetSelectedActor) },
	{ "SetActorWalkChore", LUA_OPCODE(Lua_V2, SetActorWalkChore) },
	{ "SetActorTurnChores", LUA_OPCODE(Lua_V2, SetActorTurnChores) },
	{ "SetActorRestChore", LUA_OPCODE(Lua_V2, SetActorRestChore) },
	{ "SetActorMumblechore", LUA_OPCODE(Lua_V2, SetActorMumblechore) },
	{ "SetActorTalkChore", LUA_OPCODE(Lua_V2, SetActorTalkChore) },
	{ "SetActorWalkRate", LUA_OPCODE(Lua_V2, SetActorWalkRate) },
	{ "GetActorWalkRate", LUA_OPCODE(Lua_V2, GetActorWalkRate) },
	{ "SetActorTurnRate", LUA_OPCODE(Lua_V2, SetActorTurnRate) },
	{ "GetActorTurnRate", LUA_OPCODE(Lua_V2, GetActorTurnRate) },
	{ "GetActorPos", LUA_OPCODE(Lua_V2, GetActorPos) },
	{ "GetActorWorldPos", LUA_OPCODE(Lua_V2, GetActorWorldPos) },
	{ "GetActorPuckVector", LUA_OPCODE(Lua_V2, GetActorPuckVector) },
	{ "GetActorYawToPoint", LUA_OPCODE(Lua_V2, GetActorYawToPoint) },
	{ "SetActorReflection", LUA_OPCODE(Lua_V2, SetActorReflection) },
	{ "PutActorInSet", LUA_OPCODE(Lua_V2, PutActorInSet) },
	{ "PutActorAt", LUA_OPCODE(Lua_V2, PutActorAt) },
	{ "PutActorInOverworld", LUA_OPCODE(Lua_V2, PutActorInOverworld) },
	{ "RemoveActorFromOverworld", LUA_OPCODE(Lua_V2, RemoveActorFromOverworld) },
	{ "ClearOverworld", LUA_OPCODE(Lua_V2, ClearOverworld) },
	{ "ToggleOverworld", LUA_OPCODE(Lua_V2, ToggleOverworld) },
	{ "WalkActorVector", LUA_OPCODE(Lua_V2, WalkActorVector) },
	{ "WalkActorForward", LUA_OPCODE(Lua_V2, WalkActorForward) },
	{ "ActorStopMoving", LUA_OPCODE(Lua_V2, ActorStopMoving) },
	{ "WalkActorTo", LUA_OPCODE(Lua_V2, WalkActorTo) },
	{ "WalkActorToAvoiding", LUA_OPCODE(Lua_V2, WalkActorToAvoiding) },
	{ "ActorLookAt", LUA_OPCODE(Lua_V2, ActorLookAt) },
	{ "IsActorLooking", LUA_OPCODE(Lua_V2, IsActorLooking) },
	{ "SetActorLookRate", LUA_OPCODE(Lua_V2, SetActorLookRate) },
	{ "GetActorLookRate", LUA_OPCODE(Lua_V2, GetActorLookRate) },
	{ "SetActorFOV", LUA_OPCODE(Lua_V2, SetActorFOV) },
	{ "GetActorFOV", LUA_OPCODE(Lua_V2, GetActorFOV) },
	{ "GetVisibleThings", LUA_OPCODE(Lua_V2, GetVisibleThings) },
	{ "SetActorLighting", LUA_OPCODE(Lua_V2, SetActorLighting) },
	{ "SetActorHead", LUA_OPCODE(Lua_V2, SetActorHead) },
	{ "SetActorHeadLimits", LUA_OPCODE(Lua_V2, SetActorHeadLimits) },
	{ "SetActorVisibility", LUA_OPCODE(Lua_V2, SetActorVisibility) },
	{ "SetActorFollowBoxes", LUA_OPCODE(Lua_V2, SetActorFollowBoxes) },
	{ "ActorActivateShadow", LUA_OPCODE(Lua_V2, ActorActivateShadow) },
	{ "ShutUpActor", LUA_OPCODE(Lua_V2, ShutUpActor) },
	{ "EnableActorPuck", LUA_OPCODE(Lua_V2, EnableActorPuck) },
	{ "SetActorGlobalAlpha", LUA_OPCODE(Lua_V2, SetActorGlobalAlpha) },
	{ "SetActorLocalAlpha", LUA_OPCODE(Lua_V2, SetActorLocalAlpha) },
	{ "SetActorCollisionMode", LUA_OPCODE(Lua_V2, SetActorCollisionMode) },
	{ "SetActorCollisionScale", LUA_OPCODE(Lua_V2, SetActorCollisionScale) },
	{ "SetActorSortOrder", LUA_OPCODE(Lua_V2, SetActorSortOrder) },
	{ "GetActorSortOrder", LUA_OPCODE(Lua_V2, GetActorSortOrder) },
	{ "AttachActor", LUA_OPCODE(Lua_V2, AttachActor) },
	{ "DetachActor", LUA_OPCODE(Lua_V2, DetachActor) },
	{ "IsActorInSector", LUA_OPCODE(Lua_V2, IsActorInSector) },
	{ "GetActorSector", LUA_OPCODE(Lua_V2, GetActorSector) },
	{ "IsPointInSector", LUA_OPCODE(Lua_V2, IsPointInSector) },
	{ "GetPointSector", LUA_OPCODE(Lua_V2, GetPointSector) },
	{ "TurnActor", LUA_OPCODE(Lua_V2, TurnActor) },
	{ "GetActorRot", LUA_OPCODE(Lua_V2, GetActorRot) },
	{ "SetActorRot", LUA_OPCODE(Lua_V2, SetActorRot) },
	{ "SetActorPitch", LUA_OPCODE(Lua_V2, SetActorPitch) },
	{ "SetActorRoll", LUA_OPCODE(Lua_V2, SetActorRoll) },
	{ "IsActorTurning", LUA_OPCODE(Lua_V2, IsActorTurning) },
	{ "PointActorAt", LUA_OPCODE(Lua_V2, PointActorAt) },
	{ "TurnActorTo", LUA_OPCODE(Lua_V2, TurnActorTo) },
	{ "PlayActorChore", LUA_OPCODE(Lua_V2, PlayActorChore) },
	{ "PlayActorChoreLooping", LUA_OPCODE(Lua_V2, PlayActorChoreLooping) },
	{ "IsActorMoving", LUA_OPCODE(Lua_V2, IsActorMoving) },
	{ "IsActorResting", LUA_OPCODE(Lua_V2, IsActorResting) },
	{ "GetActorChores", LUA_OPCODE(Lua_V2, GetActorChores) },
	{ "StopActorChores", LUA_OPCODE(Lua_V2, StopActorChores) },
	{ "IsChoreValid", LUA_OPCODE(Lua_V2, IsChoreValid) },
	{ "IsChorePlaying", LUA_OPCODE(Lua_V2, IsChorePlaying) },
	{ "IsChoreLooping", LUA_OPCODE(Lua_V2, IsChoreLooping) },
	{ "SetChoreLooping", LUA_OPCODE(Lua_V2, SetChoreLooping) },
	{ "PlayChore", LUA_OPCODE(Lua_V2, PlayChore) },
	{ "FadeInChore", LUA_OPCODE(Lua_V2, FadeInChore) },
	{ "FadeOutChore", LUA_OPCODE(Lua_V2, FadeOutChore) },
	{ "StopChore", LUA_OPCODE(Lua_V2, StopChore) },
	{ "PauseChore", LUA_OPCODE(Lua_V2, PauseChore) },
	{ "AdvanceChore", LUA_OPCODE(Lua_V2, AdvanceChore) },
	{ "CompleteChore", LUA_OPCODE(Lua_V2, CompleteChore) },
	{ "LockChore", LUA_OPCODE(Lua_V2, LockChore) },
	{ "UnlockChore", LUA_OPCODE(Lua_V2, UnlockChore) },
	{ "LockChoreSet", LUA_OPCODE(Lua_V2, LockChoreSet) },
	{ "UnlockChoreSet", LUA_OPCODE(Lua_V2, UnlockChoreSet) },
	{ "exit", LUA_OPCODE(Lua_V2, Exit) },
	{ "FunctionName", LUA_OPCODE(Lua_V2, FunctionName) },
	{ "EnableControl", LUA_OPCODE(Lua_V1, EnableControl) },
	{ "DisableControl", LUA_OPCODE(Lua_V1, DisableControl) },
	{ "GetControlState", LUA_OPCODE(Lua_V2, GetControlState) },
	{ "PrintMessage", LUA_OPCODE(Lua_V2, PrintMessage) },
	{ "PrintError", LUA_OPCODE(Lua_V2, PrintError) },
	{ "PrintWarning", LUA_OPCODE(Lua_V2, PrintWarning) },
	{ "PrintDebug", LUA_OPCODE(Lua_V2, PrintDebug) },
	{ "MakeCurrentSet", LUA_OPCODE(Lua_V2, MakeCurrentSet) },
	{ "LockSet", LUA_OPCODE(Lua_V2, LockSet) },
	{ "UnLockSet", LUA_OPCODE(Lua_V2, UnLockSet) },
	{ "LockBackground", LUA_OPCODE(Lua_V2, LockBackground) },
	{ "UnLockBackground", LUA_OPCODE(Lua_V2, UnLockBackground) },
	{ "MakeCurrentSetup", LUA_OPCODE(Lua_V2, MakeCurrentSetup) },
	{ "GetCurrentSetup", LUA_OPCODE(Lua_V2, GetCurrentSetup) },
	{ "NextSetup", LUA_OPCODE(Lua_V2, NextSetup) },
	{ "PreviousSetup", LUA_OPCODE(Lua_V2, PreviousSetup) },
	{ "StartMovie", LUA_OPCODE(Lua_V2, StartMovie) },
	{ "StopMovie", LUA_OPCODE(Lua_V2, StopMovie) },
	{ "PauseMovie", LUA_OPCODE(Lua_V2, PauseMovie) },
	{ "EscapeMovie", LUA_OPCODE(Lua_V2, EscapeMovie) },
	{ "IsMoviePlaying", LUA_OPCODE(Lua_V2, IsMoviePlaying) },
	{ "PlaySound", LUA_OPCODE(Lua_V2, PlaySound) },
	{ "StopSound", LUA_OPCODE(Lua_V2, StopSound) },
	{ "StopAllSounds", LUA_OPCODE(Lua_V2, StopAllSounds) },
	{ "LoadSound", LUA_OPCODE(Lua_V2, LoadSound) },
	{ "FreeSound", LUA_OPCODE(Lua_V2, FreeSound) },
	{ "PlayLoadedSound", LUA_OPCODE(Lua_V2, PlayLoadedSound) },
	{ "IsSoundPlaying", LUA_OPCODE(Lua_V2, IsSoundPlaying) },
	{ "GetGroupVolume", LUA_OPCODE(Lua_V2, GetGroupVolume) },
	{ "SetGroupVolume", LUA_OPCODE(Lua_V2, SetGroupVolume) },
	{ "GetSoundVolume", LUA_OPCODE(Lua_V2, GetSoundVolume) },
	{ "SetSoundVolume", LUA_OPCODE(Lua_V2, SetSoundVolume) },
	{ "EnableAudioGroup", LUA_OPCODE(Lua_V2, EnableAudioGroup) },
	{ "EnableVoiceFX", LUA_OPCODE(Lua_V2, EnableVoiceFX) },
	{ "SetAudioMixAhead", LUA_OPCODE(Lua_V2, SetAudioMixAhead) },
	{ "PlaySoundFrom", LUA_OPCODE(Lua_V2, PlaySoundFrom) },
	{ "PlayLoadedSoundFrom", LUA_OPCODE(Lua_V2, PlayLoadedSoundFrom) },
	{ "SetReverb", LUA_OPCODE(Lua_V2, SetReverb) },
	{ "UpdateSoundPosition", LUA_OPCODE(Lua_V2, UpdateSoundPosition) },
	{ "ImSetState", LUA_OPCODE(Lua_V2, ImSetState) },
	{ "ImSelectSet", LUA_OPCODE(Lua_V2, ImSelectSet) },
	{ "ImStateHasLooped", LUA_OPCODE(Lua_V2, ImStateHasLooped) },
	{ "ImStateHasEnded", LUA_OPCODE(Lua_V2, ImStateHasEnded) },
	{ "ImPushState", LUA_OPCODE(Lua_V2, ImPushState) },
	{ "ImPopState", LUA_OPCODE(Lua_V2, ImPopState) },
	{ "ImFlushStack", LUA_OPCODE(Lua_V2, ImFlushStack) },
	{ "ImGetMillisecondPosition", LUA_OPCODE(Lua_V2, ImGetMillisecondPosition) },
	{ "IsSectorActive", LUA_OPCODE(Lua_V2, IsSectorActive) },
	{ "MakeSectorActive", LUA_OPCODE(Lua_V2, MakeSectorActive) },
	{ "GetSectorOppositeEdge", LUA_OPCODE(Lua_V2, GetSectorOppositeEdge) },
	{ "GetSectorName", LUA_OPCODE(Lua_V2, GetSectorName) },
	{ "WorldToScreen", LUA_OPCODE(Lua_V2, WorldToScreen) },
	{ "GetCameraPosition", LUA_OPCODE(Lua_V2, GetCameraPosition) },
	{ "SetCameraPosition", LUA_OPCODE(Lua_V2, SetCameraPosition) },
	{ "CameraLookAt", LUA_OPCODE(Lua_V2, CameraLookAt) },
	{ "GetCameraFOV", LUA_OPCODE(Lua_V2, GetCameraFOV) },
	{ "SetCameraFOV", LUA_OPCODE(Lua_V2, SetCameraFOV) },
	{ "GetCameraYaw", LUA_OPCODE(Lua_V2, GetCameraYaw) },
	{ "YawCamera", LUA_OPCODE(Lua_V2, YawCamera) },
	{ "GetCameraPitch", LUA_OPCODE(Lua_V2, GetCameraPitch) },
	{ "PitchCamera", LUA_OPCODE(Lua_V2, PitchCamera) },
	{ "GetCameraRoll", LUA_OPCODE(Lua_V2, GetCameraRoll) },
	{ "RollCamera", LUA_OPCODE(Lua_V2, RollCamera) },
	{ "GetCameraLookVector", LUA_OPCODE(Lua_V2, GetCameraLookVector) },
	{ "PerSecond", LUA_OPCODE(Lua_V2, PerSecond) },
	{ "SnapToPlane", LUA_OPCODE(Lua_V2, SnapToPlane) },
	{ "GetAngleBetweenVectors", LUA_OPCODE(Lua_V2, GetAngleBetweenVectors) },
	{ "GetAngleBetweenActors", LUA_OPCODE(Lua_V2, GetAngleBetweenActors) },
	{ "SetAmbientLight", LUA_OPCODE(Lua_V2, EmptyFunction) },
	{ "TurnLightOn", LUA_OPCODE(Lua_V2, TurnLightOn) },
	{ "IsLightOn", LUA_OPCODE(Lua_V2, IsLightOn) },
	{ "SetLightPosition", LUA_OPCODE(Lua_V2, SetLightPosition) },
	{ "GammaEnabled", LUA_OPCODE(Lua_V2, GammaEnabled) },
	{ "SetGamma", LUA_OPCODE(Lua_V2, SetGamma) },
	{ "SetPixelCenter", LUA_OPCODE(Lua_V2, SetPixelCenter) },
	{ "LoadBundle", LUA_OPCODE(Lua_V2, LoadBundle) },
	{ "RenderModeUser", LUA_OPCODE(Lua_V2, RenderModeUser) },
	{ "DimScreen", LUA_OPCODE(Lua_V2, DimScreen) },
	{ "DimRegion", LUA_OPCODE(Lua_V2, DimRegion) },
	{ "UndimAll", LUA_OPCODE(Lua_V2, UndimAll) },
	{ "UndimRegion", LUA_OPCODE(Lua_V2, UndimRegion) },
	{ "Display", LUA_OPCODE(Lua_V2, EmptyFunction) },
	{ "SetSpeechMode", LUA_OPCODE(Lua_V2, SetSpeechMode) },
	{ "GetSpeechMode", LUA_OPCODE(Lua_V2, GetSpeechMode) },
	{ "GetCPUSpeed", LUA_OPCODE(Lua_V2, GetCPUSpeed) },
	{ "NewLayer", LUA_OPCODE(Lua_V2, NewLayer) },
	{ "FreeLayer", LUA_OPCODE(Lua_V2, FreeLayer) },
	{ "GetLayerSortOrder", LUA_OPCODE(Lua_V2, GetLayerSortOrder) },
	{ "SetLayerSortOrder", LUA_OPCODE(Lua_V2, SetLayerSortOrder) },
	{ "GetLayerFrame", LUA_OPCODE(Lua_V2, GetLayerFrame) },
	{ "SetLayerFrame", LUA_OPCODE(Lua_V2, SetLayerFrame) },
	{ "AdvanceLayerFrame", LUA_OPCODE(Lua_V2, AdvanceLayerFrame) },
	{ "SetTextSpeed", LUA_OPCODE(Lua_V2, SetTextSpeed) },
	{ "GetTextSpeed", LUA_OPCODE(Lua_V2, GetTextSpeed) },
	{ "IsMessageGoing", LUA_OPCODE(Lua_V2, IsMessageGoing) },
	{ "SetSayLineDefaults", LUA_OPCODE(Lua_V2, SetSayLineDefaults) },
	{ "SetActorTalkColor", LUA_OPCODE(Lua_V2, SetActorTalkColor) },
	{ "GetActorTalkColor", LUA_OPCODE(Lua_V2, GetActorTalkColor) },
	{ "SayLine", LUA_OPCODE(Lua_V2, SayLine) },
	{ "MakeTextObject", LUA_OPCODE(Lua_V2, MakeTextObject) },
	{ "KillTextObject", LUA_OPCODE(Lua_V2, KillTextObject) },
	{ "ChangeTextObject", LUA_OPCODE(Lua_V2, ChangeTextObject) },
	{ "GetTextObjectDimensions", LUA_OPCODE(Lua_V2, GetTextObjectDimensions) },
	{ "BlastText", LUA_OPCODE(Lua_V2, BlastText) },
	{ "ExpireText", LUA_OPCODE(Lua_V2, ExpireText) },
	{ "PurgeText", LUA_OPCODE(Lua_V2, PurgeText) },
	{ "PushText", LUA_OPCODE(Lua_V2, PushText) },
	{ "PopText", LUA_OPCODE(Lua_V2, PopText) },
	{ "MakeColor", LUA_OPCODE(Lua_V2, MakeColor) },
	{ "GetColorComponents", LUA_OPCODE(Lua_V2, GetColorComponents) },
	{ "SetTranslationMode", LUA_OPCODE(Lua_V2, SetTranslationMode) },
	{ "GetTranslationMode", LUA_OPCODE(Lua_V2, GetTranslationMode) },
	{ "LocalizeString", LUA_OPCODE(Lua_V2, LocalizeString) },
	{ "GetFontDimensions", LUA_OPCODE(Lua_V2, GetFontDimensions) },
	{ "GetTextCharPosition", LUA_OPCODE(Lua_V2, GetTextCharPosition) },
	{ "SetOffscreenTextPos", LUA_OPCODE(Lua_V2, SetOffscreenTextPos) },
	{ "ReportAllResources", LUA_OPCODE(Lua_V2, ReportAllResources) },
	{ "ReportLRUResources", LUA_OPCODE(Lua_V2, ReportLRUResources) },
	{ "ReportUsedResources", LUA_OPCODE(Lua_V2, ReportUsedResources) },
	{ "ReportUnusedResources", LUA_OPCODE(Lua_V2, ReportUnusedResources) },
	{ "PurgeAmount", LUA_OPCODE(Lua_V2, PurgeAmount) },
	{ "PurgeAllResources", LUA_OPCODE(Lua_V2, PurgeAllResources) },
	{ "PurgeUnusedResources", LUA_OPCODE(Lua_V2, PurgeUnusedResources) },
	{ "PurgeUnlockedResources", LUA_OPCODE(Lua_V2, PurgeUnlockedResources) },
	{ "NukeAllScriptLocks", LUA_OPCODE(Lua_V2, NukeAllScriptLocks) },
	{ "LightMgrStartup", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "LightMgrShutdown", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "LightMgrSetChange", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDebugDraw", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawCameras", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawLights", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawSectors", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawBBoxes", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawFPS", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawPerformance", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ToggleDrawActorStats", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "WriteSetToFile", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditSelect", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditPlace", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditModify", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditDelete", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditInsert", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditSortAdd", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "SectEditForgetIt", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "FRUTEY_Begin", LUA_OPCODE(Lua_V2, FRUTEY_Begin) },
	{ "FRUTEY_End", LUA_OPCODE(Lua_V2, FRUTEY_End) },
	{ "ImGetSfxVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ImSetSfxVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ImGetVoiceVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ImSetVoiceVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ImGetMusicVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) },
	{ "ImSetMusicVol", LUA_OPCODE(Lua_V2, UnimplementedFunction) }
};

static struct luaL_reg monkeyEntryOpcodes[] = {
	{ "  concatfallback", LUA_OPCODE(LuaBase, concatFallback) },
	{ "  typeoverride", LUA_OPCODE(LuaBase, typeOverride) },
	{ "  dfltcontrol", LUA_OPCODE(Lua_V2, EmptyFunction) }
};

static struct luaL_reg monkeyPrimitiveOpcodes[] = {
	{ "Draw3DLine", LUA_OPCODE(Lua_V2, Draw3DLine) },
	{ "DrawLine", LUA_OPCODE(Lua_V2, DrawLine) },
	{ "DrawPolygon", LUA_OPCODE(Lua_V2, DrawPolygon) },
	{ "DrawRectangle", LUA_OPCODE(Lua_V2, DrawRectangle) },
	{ "ChangePrimitive", LUA_OPCODE(Lua_V2, ChangePrimitive) },
	{ "KillPrimitive", LUA_OPCODE(Lua_V2, KillPrimitive) },
	{ "PurgePrimitiveQueue", LUA_OPCODE(Lua_V2, PurgePrimitiveQueue) }
};

Lua_V2::Lua_V2() : Lua_V1(true), _pFind(nullptr), _gcTimer(0.0f), _bStarted(false) {
	for (int i = 0; i < kNumFieldRefs; i++)
		_fieldRefs[i] = 0;
}

void Lua_V2::registerMainOpcodes() {
	luaL_openlib(monkeyMainOpcodes, ARRAYSIZE(monkeyMainOpcodes));
}

void Lua_V2::registerPrimitiveOpcodes() {
	luaL_openlib(monkeyPrimitiveOpcodes, ARRAYSIZE(monkeyPrimitiveOpcodes));
}

void Lua_V2::recognizeOpcodes() {
	luaL_addlibtolist(monkeyEntryOpcodes, ARRAYSIZE(monkeyEntryOpcodes));
}

int Lua_V2::startup() {
	if (_bStarted)
		return 0;

	registerMainOpcodes();
	recognizeOpcodes();
	makeSystemTable();
	registerTypeOverride();
	registerSectorConstants();
	registerConcatFallback();

	lua_savehook = identityHook;
	lua_restorehook = identityHook;

	for (int i = 0; i < kNumFieldRefs; i++) {
		lua_pushstring(fieldNames[i]);
		_fieldRefs[i] = lua_ref(1);
	}

	_bStarted = true;

	return 1;
}

int Lua_V2::shutdown() {
	if (!_bStarted)
		return 0;

	for (int i = 0; i < kNumFieldRefs; i++)
		lua_unref(_fieldRefs[i]);

	if (_pFind)
		fileUtilDisposeFind(_pFind);

	lua_unref(refSystemTable);
	lua_savehook = nullptr;
	lua_restorehook = nullptr;
	_bStarted = false;

	return 0;
}

int Lua_V2::boot(int saveGameResume, const char *bootParam) {
	if (dofile("_system.lua"))
		return 0;

	lua_pushnil();

	if (bootParam)
		lua_pushstring(bootParam);
	else
		lua_pushnil();

	return lua_call("BOOT") == 0;
}

void Lua_V2::update(float seconds) {
	double timer = _gcTimer + seconds;

	_gcTimer = (float)timer;

	if (timer > 10.0) {
		lua_collectgarbage(0);
		_gcTimer = 0.0f;
	}

	float frameTime = (float)((double)seconds * 1000.0);

	lua_beginblock();
	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("frameTime");
	lua_pushnumber(frameTime);
	lua_settable();
	lua_endblock();

	lua_runtasks((int)frameTime);
}

int Lua_V2::dofile(const char *filename) {
	LFile *file = gpFileServices->open(filename, "rb");

	if (!file)
		return 2;

	file->seek(0, SEEK_END);

	int size = file->tell();

	file->seek(0, SEEK_SET);

	if (size < 0)
		size = 0;

	char *buffer = new char[size + 1];
	int count = file->read(buffer, size);

	buffer[count] = 0;
	gpFileServices->close(file, true);

	int result = lua_dobuffer(buffer, count, filename);

	delete[] buffer;

	return result;
}

int32 Lua_V2::identityHook(int32 tag, int32 hData, tSaveRestoreAccess *saveFunc) {
	return hData;
}

void Lua_V2::makeSystemTable() {
	lua_Object systemTable = lua_createtable();

	lua_pushobject(systemTable);
	lua_setglobal("system");

	lua_pushobject(systemTable);
	refSystemTable = lua_ref(1);

	registerControls();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("controls");

	lua_Object controlsTable = lua_gettable();

	lua_pushobject(controlsTable);
	lua_pushstring("joy_selected");
	lua_pushnumber(-1.0f);
	lua_settable();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("axisHandler");
	lua_pushcfunction(LUA_OPCODE(Lua_V2, EmptyFunction));
	lua_settable();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("buttonHandler");
	lua_pushcfunction(LUA_OPCODE(Lua_V2, EmptyFunction));
	lua_settable();
}

void Lua_V2::typeOverride() {
	lua_Object data = lua_getparam(1);

	if (lua_isuserdata(data)) {
		int32 tag = lua_tag(data);

		if (tag == MKTAG(' ', 'T', 'E', 'S')) {
			lua_pushstring("set");
			lua_pushnumber((float)tag);
			return;
		}

		if (tag == MKTAG('R', 'T', 'C', 'A')) {
			lua_pushstring("actor");
			lua_pushnumber((float)tag);
			return;
		}
	}

	lua_Object oldType = lua_getref(refTypeOverride);

	lua_pushobject(data);
	lua_callfunction(oldType);

	lua_Object result1 = lua_getresult(1);
	lua_Object result2 = lua_getresult(2);

	lua_pushobject(result1);
	lua_pushobject(result2);
}

void Lua_V2::concatFallback() {
	lua_Object params[2];
	char result[128];

	params[0] = lua_getparam(1);
	params[1] = lua_getparam(2);
	result[0] = 0;

	for (int i = 0; i < 2; i++) {
		lua_Object param = params[i];
		Common::String text;

		if (lua_isstring(param)) {
			text = lua_getstring(param);

			if (text.contains('%'))
				warning("Lua_V2::concatFallback(): the string contains a %% character");
		} else if (lua_isnil(param)) {
			text = "(nil)";
		} else if (lua_isuserdata(param) && lua_tag(param) == MKTAG('R', 'T', 'C', 'A')) {
			text = Common::String::format("(actor%d:)", lua_getuserdata(param));
		} else {
			lua_pushobject(params[0]);
			lua_pushobject(params[1]);
			lua_callfunction(lua_getref(refOldConcatFallback));
			lua_pushobject(lua_getresult(1));
			return;
		}

		if (Common::strlcat(result, text.c_str(), sizeof(result)) >= sizeof(result))
			warning("Lua_V2::concatFallback(): the result has been truncated");
	}

	lua_pushstring(result);
}

void Lua_V2::UnimplementedFunction() {
	lua_printstack(nullptr);
}

void Lua_V2::EmptyFunction() {
}

void Lua_V2::Draw3DLine() {
	warning("Lua_V2::Draw3DLine(): not implemented");
}

void Lua_V2::DrawLine() {
	warning("Lua_V2::DrawLine(): not implemented");
}

void Lua_V2::DrawPolygon() {
	warning("Lua_V2::DrawPolygon(): not implemented");
}

void Lua_V2::DrawRectangle() {
	warning("Lua_V2::DrawRectangle(): not implemented");
}

void Lua_V2::ChangePrimitive() {
	warning("Lua_V2::ChangePrimitive(): not implemented");
}

void Lua_V2::KillPrimitive() {
	warning("Lua_V2::KillPrimitive(): not implemented");
}

void Lua_V2::PurgePrimitiveQueue() {
	warning("Lua_V2::PurgePrimitiveQueue(): not implemented");
}

void Lua_V2::EnginePause() {
	warning("Lua_V2::EnginePause(): not implemented");
}

void Lua_V2::EngineResume() {
	warning("Lua_V2::EngineResume(): not implemented");
}

void Lua_V2::GetPhysicalBytes() {
	warning("Lua_V2::GetPhysicalBytes(): not implemented");
}

void Lua_V2::GetMemoryUsage() {
	warning("Lua_V2::GetMemoryUsage(): not implemented");
}

void Lua_V2::SetMemoryUsage() {
	warning("Lua_V2::SetMemoryUsage(): not implemented");
}

void Lua_V2::SetActiveCD() {
	warning("Lua_V2::SetActiveCD(): not implemented");
}

void Lua_V2::GetActiveCD() {
	warning("Lua_V2::GetActiveCD(): not implemented");
}

void Lua_V2::AreWeInternational() {
	warning("Lua_V2::AreWeInternational(): not implemented");
}

void Lua_V2::FileFindFirst() {
	warning("Lua_V2::FileFindFirst(): not implemented");
}

void Lua_V2::FileFindNext() {
	warning("Lua_V2::FileFindNext(): not implemented");
}

void Lua_V2::FileFindDispose() {
	warning("Lua_V2::FileFindDispose(): not implemented");
}

void Lua_V2::TextFileGetLineCount() {
	warning("Lua_V2::TextFileGetLineCount(): not implemented");
}

void Lua_V2::TextFileGetLine() {
	warning("Lua_V2::TextFileGetLine(): not implemented");
}

void Lua_V2::InputDialog() {
	warning("Lua_V2::InputDialog(): not implemented");
}

void Lua_V2::QueryDialog() {
	warning("Lua_V2::QueryDialog(): not implemented");
}

void Lua_V2::ScreenShot() {
	warning("Lua_V2::ScreenShot(): not implemented");
}

void Lua_V2::MakeScreenTextures() {
	warning("Lua_V2::MakeScreenTextures(): not implemented");
}

void Lua_V2::SubmitSaveGameData() {
	warning("Lua_V2::SubmitSaveGameData(): not implemented");
}

void Lua_V2::GetSaveGameData() {
	warning("Lua_V2::GetSaveGameData(): not implemented");
}

void Lua_V2::ScreenshotForSavegame() {
	warning("Lua_V2::ScreenshotForSavegame(): not implemented");
}

void Lua_V2::ThumbnailFromScreen() {
	warning("Lua_V2::ThumbnailFromScreen(): not implemented");
}

void Lua_V2::ThumbnailFromFile() {
	warning("Lua_V2::ThumbnailFromFile(): not implemented");
}

void Lua_V2::ClearSpecialtyTexture() {
	warning("Lua_V2::ClearSpecialtyTexture(): not implemented");
}

void Lua_V2::Exit() {
	warning("Lua_V2::Exit(): not implemented");
}

void Lua_V2::FunctionName() {
	warning("Lua_V2::FunctionName(): not implemented");
}

void Lua_V2::GetControlState() {
	warning("Lua_V2::GetControlState(): not implemented");
}

void Lua_V2::PrintMessage() {
	warning("Lua_V2::PrintMessage(): not implemented");
}

void Lua_V2::PrintError() {
	warning("Lua_V2::PrintError(): not implemented");
}

void Lua_V2::PrintWarning() {
	warning("Lua_V2::PrintWarning(): not implemented");
}

void Lua_V2::PrintDebug() {
	warning("Lua_V2::PrintDebug(): not implemented");
}

void Lua_V2::MakeCurrentSet() {
	warning("Lua_V2::MakeCurrentSet(): not implemented");
}

void Lua_V2::LockSet() {
	warning("Lua_V2::LockSet(): not implemented");
}

void Lua_V2::UnLockSet() {
	warning("Lua_V2::UnLockSet(): not implemented");
}

void Lua_V2::LockBackground() {
	warning("Lua_V2::LockBackground(): not implemented");
}

void Lua_V2::UnLockBackground() {
	warning("Lua_V2::UnLockBackground(): not implemented");
}

void Lua_V2::MakeCurrentSetup() {
	warning("Lua_V2::MakeCurrentSetup(): not implemented");
}

void Lua_V2::GetCurrentSetup() {
	warning("Lua_V2::GetCurrentSetup(): not implemented");
}

void Lua_V2::NextSetup() {
	warning("Lua_V2::NextSetup(): not implemented");
}

void Lua_V2::PreviousSetup() {
	warning("Lua_V2::PreviousSetup(): not implemented");
}

void Lua_V2::StartMovie() {
	warning("Lua_V2::StartMovie(): not implemented");
}

void Lua_V2::StopMovie() {
	warning("Lua_V2::StopMovie(): not implemented");
}

void Lua_V2::PauseMovie() {
	warning("Lua_V2::PauseMovie(): not implemented");
}

void Lua_V2::EscapeMovie() {
	warning("Lua_V2::EscapeMovie(): not implemented");
}

void Lua_V2::IsMoviePlaying() {
	warning("Lua_V2::IsMoviePlaying(): not implemented");
}

void Lua_V2::WorldToScreen() {
	warning("Lua_V2::WorldToScreen(): not implemented");
}

void Lua_V2::GetCameraPosition() {
	warning("Lua_V2::GetCameraPosition(): not implemented");
}

void Lua_V2::SetCameraPosition() {
	warning("Lua_V2::SetCameraPosition(): not implemented");
}

void Lua_V2::CameraLookAt() {
	warning("Lua_V2::CameraLookAt(): not implemented");
}

void Lua_V2::GetCameraFOV() {
	warning("Lua_V2::GetCameraFOV(): not implemented");
}

void Lua_V2::SetCameraFOV() {
	warning("Lua_V2::SetCameraFOV(): not implemented");
}

void Lua_V2::GetCameraYaw() {
	warning("Lua_V2::GetCameraYaw(): not implemented");
}

void Lua_V2::YawCamera() {
	warning("Lua_V2::YawCamera(): not implemented");
}

void Lua_V2::GetCameraPitch() {
	warning("Lua_V2::GetCameraPitch(): not implemented");
}

void Lua_V2::PitchCamera() {
	warning("Lua_V2::PitchCamera(): not implemented");
}

void Lua_V2::GetCameraRoll() {
	warning("Lua_V2::GetCameraRoll(): not implemented");
}

void Lua_V2::RollCamera() {
	warning("Lua_V2::RollCamera(): not implemented");
}

void Lua_V2::GetCameraLookVector() {
	warning("Lua_V2::GetCameraLookVector(): not implemented");
}

void Lua_V2::PerSecond() {
	warning("Lua_V2::PerSecond(): not implemented");
}

void Lua_V2::SnapToPlane() {
	warning("Lua_V2::SnapToPlane(): not implemented");
}

void Lua_V2::GetAngleBetweenVectors() {
	warning("Lua_V2::GetAngleBetweenVectors(): not implemented");
}

void Lua_V2::TurnLightOn() {
	warning("Lua_V2::TurnLightOn(): not implemented");
}

void Lua_V2::IsLightOn() {
	warning("Lua_V2::IsLightOn(): not implemented");
}

void Lua_V2::SetLightPosition() {
	warning("Lua_V2::SetLightPosition(): not implemented");
}

void Lua_V2::GammaEnabled() {
	warning("Lua_V2::GammaEnabled(): not implemented");
}

void Lua_V2::SetGamma() {
	warning("Lua_V2::SetGamma(): not implemented");
}

void Lua_V2::SetPixelCenter() {
	warning("Lua_V2::SetPixelCenter(): not implemented");
}

void Lua_V2::LoadBundle() {
	warning("Lua_V2::LoadBundle(): not implemented");
}

void Lua_V2::RenderModeUser() {
	warning("Lua_V2::RenderModeUser(): not implemented");
}

void Lua_V2::DimScreen() {
	warning("Lua_V2::DimScreen(): not implemented");
}

void Lua_V2::DimRegion() {
	warning("Lua_V2::DimRegion(): not implemented");
}

void Lua_V2::UndimAll() {
	warning("Lua_V2::UndimAll(): not implemented");
}

void Lua_V2::UndimRegion() {
	warning("Lua_V2::UndimRegion(): not implemented");
}

void Lua_V2::SetSpeechMode() {
	warning("Lua_V2::SetSpeechMode(): not implemented");
}

void Lua_V2::GetSpeechMode() {
	warning("Lua_V2::GetSpeechMode(): not implemented");
}

void Lua_V2::GetCPUSpeed() {
	warning("Lua_V2::GetCPUSpeed(): not implemented");
}

void Lua_V2::NewLayer() {
	warning("Lua_V2::NewLayer(): not implemented");
}

void Lua_V2::FreeLayer() {
	warning("Lua_V2::FreeLayer(): not implemented");
}

void Lua_V2::GetLayerSortOrder() {
	warning("Lua_V2::GetLayerSortOrder(): not implemented");
}

void Lua_V2::SetLayerSortOrder() {
	warning("Lua_V2::SetLayerSortOrder(): not implemented");
}

void Lua_V2::GetLayerFrame() {
	warning("Lua_V2::GetLayerFrame(): not implemented");
}

void Lua_V2::SetLayerFrame() {
	warning("Lua_V2::SetLayerFrame(): not implemented");
}

void Lua_V2::AdvanceLayerFrame() {
	warning("Lua_V2::AdvanceLayerFrame(): not implemented");
}

void Lua_V2::SetTextSpeed() {
	warning("Lua_V2::SetTextSpeed(): not implemented");
}

void Lua_V2::GetTextSpeed() {
	warning("Lua_V2::GetTextSpeed(): not implemented");
}

void Lua_V2::IsMessageGoing() {
	warning("Lua_V2::IsMessageGoing(): not implemented");
}

void Lua_V2::SetSayLineDefaults() {
	warning("Lua_V2::SetSayLineDefaults(): not implemented");
}

void Lua_V2::SetActorTalkColor() {
	warning("Lua_V2::SetActorTalkColor(): not implemented");
}

void Lua_V2::GetActorTalkColor() {
	warning("Lua_V2::GetActorTalkColor(): not implemented");
}

void Lua_V2::SayLine() {
	warning("Lua_V2::SayLine(): not implemented");
}

void Lua_V2::MakeTextObject() {
	warning("Lua_V2::MakeTextObject(): not implemented");
}

void Lua_V2::KillTextObject() {
	warning("Lua_V2::KillTextObject(): not implemented");
}

void Lua_V2::ChangeTextObject() {
	warning("Lua_V2::ChangeTextObject(): not implemented");
}

void Lua_V2::GetTextObjectDimensions() {
	warning("Lua_V2::GetTextObjectDimensions(): not implemented");
}

void Lua_V2::BlastText() {
	warning("Lua_V2::BlastText(): not implemented");
}

void Lua_V2::ExpireText() {
	warning("Lua_V2::ExpireText(): not implemented");
}

void Lua_V2::PurgeText() {
	warning("Lua_V2::PurgeText(): not implemented");
}

void Lua_V2::PushText() {
	warning("Lua_V2::PushText(): not implemented");
}

void Lua_V2::PopText() {
	warning("Lua_V2::PopText(): not implemented");
}

void Lua_V2::MakeColor() {
	warning("Lua_V2::MakeColor(): not implemented");
}

void Lua_V2::GetColorComponents() {
	warning("Lua_V2::GetColorComponents(): not implemented");
}

void Lua_V2::SetTranslationMode() {
	warning("Lua_V2::SetTranslationMode(): not implemented");
}

void Lua_V2::GetTranslationMode() {
	warning("Lua_V2::GetTranslationMode(): not implemented");
}

void Lua_V2::LocalizeString() {
	warning("Lua_V2::LocalizeString(): not implemented");
}

void Lua_V2::GetFontDimensions() {
	warning("Lua_V2::GetFontDimensions(): not implemented");
}

void Lua_V2::GetTextCharPosition() {
	warning("Lua_V2::GetTextCharPosition(): not implemented");
}

void Lua_V2::SetOffscreenTextPos() {
	warning("Lua_V2::SetOffscreenTextPos(): not implemented");
}

void Lua_V2::ReportAllResources() {
	warning("Lua_V2::ReportAllResources(): not implemented");
}

void Lua_V2::ReportLRUResources() {
	warning("Lua_V2::ReportLRUResources(): not implemented");
}

void Lua_V2::ReportUsedResources() {
	warning("Lua_V2::ReportUsedResources(): not implemented");
}

void Lua_V2::ReportUnusedResources() {
	warning("Lua_V2::ReportUnusedResources(): not implemented");
}

void Lua_V2::PurgeAmount() {
	warning("Lua_V2::PurgeAmount(): not implemented");
}

void Lua_V2::PurgeAllResources() {
	warning("Lua_V2::PurgeAllResources(): not implemented");
}

void Lua_V2::PurgeUnusedResources() {
	warning("Lua_V2::PurgeUnusedResources(): not implemented");
}

void Lua_V2::PurgeUnlockedResources() {
	warning("Lua_V2::PurgeUnlockedResources(): not implemented");
}

void Lua_V2::NukeAllScriptLocks() {
	warning("Lua_V2::NukeAllScriptLocks(): not implemented");
}

void Lua_V2::FRUTEY_Begin() {
	warning("Lua_V2::FRUTEY_Begin(): not implemented");
}

void Lua_V2::FRUTEY_End() {
	warning("Lua_V2::FRUTEY_End(): not implemented");
}

} // end of namespace Grim
