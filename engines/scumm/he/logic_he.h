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

#if !defined(SCUMM_HE_LOGIC_HE_H) && defined(ENABLE_HE)
#define SCUMM_HE_LOGIC_HE_H

#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/he/wiz_he.h"
#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/geotypes.h"

namespace Scumm {

class ScummEngine_v90he;
class ScummEngine_v100he;
class ResourceManager;

class LogicHE {
public:
	static LogicHE *makeLogicHE(ScummEngine_v90he *vm);

	virtual ~LogicHE();

	virtual void beforeBootScript() {}
	virtual void initOnce() {}
	virtual int startOfFrame() { return 1; }
	void endOfFrame() {}
	void processKeyStroke(int keyPressed) {}

	virtual int versionID();
	virtual int32 dispatch(int op, int numArgs, int32 *args);
	virtual bool userCodeProcessWizImageCmd(const WizImageCommand *icmdPtr) { return false; }
	virtual bool overrideImageHitTest(int *outValue, int globNum, int state, int x, int y, int32 flags) { return false; }
	virtual bool overrideImagePixelHitTest(int *outValue, int globNum, int state, int x, int y, int32 flags) { return false; }
	virtual bool getSpriteProperty(int sprite, int property, int *outValue) { return false; }
	virtual bool setSpriteProperty(int sprite, int property, int value) { return false; }
	virtual bool getGroupProperty(int group, int property, int *outValue) { return false; }
	virtual void spriteNewHook(int sprite) {}
	virtual void groupNewHook(int group) {}
	

protected:
	// Only to be used from makeLogicHE()
	LogicHE(ScummEngine_v90he *vm);

	ScummEngine_v90he *_vm;

	void writeScummVar(int var, int32 value);
	int getFromArray(int arg0, int idx2, int idx1);
	void putInArray(int arg0, int idx2, int idx1, int val);
	int32 scummRound(double arg) { return (int32)(arg + 0.5); }

	#define RAD2DEG (180 / M_PI)
	#define DEG2RAD (M_PI / 180)
};

// Logic declarations
LogicHE *makeLogicHErace(ScummEngine_v90he *vm);
LogicHE *makeLogicHEfunshop(ScummEngine_v90he *vm);
LogicHE *makeLogicHEfootball(ScummEngine_v90he *vm);
LogicHE *makeLogicHEfootball2002(ScummEngine_v90he *vm);
LogicHE *makeLogicHEsoccer(ScummEngine_v90he *vm);
LogicHE *makeLogicHEbaseball2001(ScummEngine_v90he *vm);
LogicHE *makeLogicHEbasketball(ScummEngine_v100he *vm);
LogicHE *makeLogicHEmoonbase(ScummEngine_v100he *vm);


// Declarations for Basketball logic live here
// since they are defined in multiple files
class LogicHEBasketball : public LogicHE {
public:
	LogicHEBasketball(ScummEngine_v100he *vm) : LogicHE(vm) { _vm1 = vm; }

	int versionID() override;
	int32 dispatch(int op, int numArgs, int32 *args) override;

protected:
	int U32_User_Init_Court(int courtID);
	int U32_User_Init_Ball(U32FltPoint3D &ballLocation, U32FltVector3D &bellVelocity, int radius, int ballID);
	int U32_User_Init_Virtual_Ball(U32FltPoint3D &ballLocation, U32FltVector3D &bellVelocity, int radius, int ballID);
	int U32_User_Init_Player(int playerID, U32FltPoint3D &playerLocation, int height, int radius, bool bPlayerIsInGame);
	int U32_User_Player_Off(int playerID);
	int U32_User_Player_On(int playerID);
	int U32_User_Detect_Ball_Collision(U32FltPoint3D &ballLocation, U32FltVector3D &ballVector, int recordCollision, int ballID);
	int U32_User_Detect_Player_Collision(int playerID, U32FltPoint3D &playerLocation, U32FltVector3D &playerVector, bool bPlayerHasBall);
	int U32_User_Get_Last_Ball_Collision(int ballID);
	int U32_User_Get_Last_Player_Collision(int playerID);
	int U32_User_Deinit_Court();
	int U32_User_Deinit_Ball();
	int U32_User_Deinit_Virtual_Ball();
	int U32_User_Deinit_Player(int playerID);

	int U32_User_Init_Screen_Translations(void);
	int U32_User_World_To_Screen_Translation(const U32FltPoint3D &worldPoint);
	int U32_User_Screen_To_World_Translation(const U32FltPoint2D &screenPoint);
	int U32_User_Get_Court_Dimensions();
	int U32_User_Compute_Points_For_Pixels(int pixels, int yPos);

	int U32_User_Compute_Trajectory_To_Target(const U32FltPoint3D &sourcePoint, const U32FltPoint3D &targetPoint, int speed);
	int U32_User_Compute_Launch_Trajectory(const U32FltPoint2D &sourcePoint, const U32FltPoint2D &targetPoint, int launchAngle, int iVelocity);
	int U32_User_Compute_Angle_Between_Vectors(const U32FltVector3D &vector1, const U32FltVector3D &vector2);

	// Court shields
	int U32_User_Raise_Shields(int shieldID);
	int U32_User_Lower_Shields(int shieldID);
	int U32_User_Are_Shields_Clear();

	// Player shields
	int U32_User_Shield_Player(int playerID, int shieldRadius);
	int U32_User_Clear_Player_Shield(int playerID);

	int U32_User_Compute_Angle_Of_Shot(int hDist, int vDist);
	int U32_User_Compute_Bank_Shot_Target(U32FltPoint3D basketLoc, int ballRadius);
	int U32_User_Compute_Swoosh_Target(const U32FltPoint3D &basketLoc, int ballRadius);
	int U32_User_Compute_Initial_Shot_Velocity(int theta, int hDist, int vDist, int gravity);
	int U32_User_Detect_Shot_Made(const U32Sphere &basketball, const U32IntVector3D &ballVector, int gravity, int whichBasket);
	int U32_User_Compute_Angle_Of_Pass(int iVelocity, int hDist, int vDist, int gravity);
	int U32_User_Compute_Angle_Of_Bounce_Pass(int iVelocity, int hDist, int currentZ, int destZ, int gravity);
	int U32_User_Hit_Moving_Target(U32FltPoint2D sourcePlayer, U32FltPoint2D targetPlayer, U32FltVector2D targetVelocity, int passSpeed);
	int U32_User_Get_Pass_Target(int playerID, const U32FltVector3D &aimVector);
	int U32_User_Detect_Pass_Blocker(int playerID, const U32FltPoint3D &targetPoint);
	int U32_User_Get_Ball_Intercept(int playerID, int ballID, int playerSpeed, int gravity);
	int U32_User_Get_Avoidance_Path(int playerID, const U32FltPoint2D &targetLocation, EAvoidanceType type);

	int U32_User_Get_Player_Closest_To_Ball(int teamIndex);
	int U32_User_Get_Player_Closest_To_Ball();
	int U32_User_Is_Player_In_Bounds(int playerID);
	int U32_User_Is_Ball_In_Bounds();
	int U32_User_Get_Open_Spot(int whichPlayer, U32FltVector2D upperLeft, U32FltVector2D lowerRight, U32FltVector2D from, bool attract, U32FltVector2D attract_point);

	int U32_User_Update_Cursor_Pos(int xScrollVal, int yScrollVal);
	int U32_User_Make_Cursor_Sticky(int lastCursorX, int lastCursorY);
	int U32_User_Cursor_Track_Moving_Object(int xChange, int yChange);
	int U32_User_Get_Cursor_Pos();

private:
	ScummEngine_v100he *_vm1;
};

} // End of namespace Scumm

#endif
