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

#include "scumm/he/intern_he.h"
#include "scumm/he/logic_he.h"
#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/geotypes.h"

namespace Scumm {

// Opcodes
#define WORLD_TO_SCREEN_TRANSLATION           1006
#define WORLD_TO_SCREEN_TRANSLATION_PARAMS       3
#define SCREEN_TO_WORLD_TRANSLATION           1010
#define SCREEN_TO_WORLD_TRANSLATION_PARAMS       2
#define INIT_SCREEN_TRANSLATIONS              1011
#define INIT_SCREEN_TRANSLATIONS_PARAMS          0
#define GET_COURT_DIMENSIONS                  1012
#define GET_COURT_DIMENSIONS_PARAMS              0

#define COMPUTE_INITIAL_SHOT_VELOCITY         1030
#define COMPUTE_INITIAL_SHOT_VELOCITY_PARAMS     4
#define COMPUTE_TRAJECTORY_TO_TARGET          1031
#define COMPUTE_TRAJECTORY_TO_TARGET_PARAMS      7
#define COMPUTE_LAUNCH_TRAJECTORY             1032
#define COMPUTE_LAUNCH_TRAJECTORY_PARAMS         6
#define COMPUTE_ANGLE_OF_SHOT                 1033
#define COMPUTE_ANGLE_OF_SHOT_PARAMS             2
#define COMPUTE_ANGLE_OF_PASS                 1034
#define COMPUTE_ANGLE_OF_PASS_PARAMS             4
#define COMPUTE_POINTS_FOR_PIXELS             1035
#define COMPUTE_POINTS_FOR_PIXELS_PARAMS         2
#define COMPUTE_ANGLE_OF_BOUNCE_PASS          1036
#define COMPUTE_ANGLE_OF_BOUNCE_PASS_PARAMS      5
#define COMPUTE_BANK_SHOT_TARGET              1037
#define COMPUTE_BANK_SHOT_TARGET_PARAMS          4
#define COMPUTE_SWOOSH_TARGET                 1038
#define COMPUTE_SWOOSH_TARGET_PARAMS             4
#define DETECT_SHOT_MADE                      1039
#define DETECT_SHOT_MADE_PARAMS                  9
#define COMPUTE_ANGLE_BETWEEN_VECTORS         1040
#define COMPUTE_ANGLE_BETWEEN_VECTORS_PARAMS     6
#define HIT_MOVING_TARGET                     1041
#define HIT_MOVING_TARGET_PARAMS                 7
#define GET_PASS_TARGET                       1042
#define GET_PASS_TARGET_PARAMS                   3
#define DETECT_PASS_BLOCKER                   1043
#define DETECT_PASS_BLOCKER_PARAMS               3
#define GET_BALL_INTERCEPT                    1044
#define GET_BALL_INTERCEPT_PARAMS                4

#define INIT_COURT                            1050
#define INIT_COURT_PARAMS                        1
#define INIT_BALL                             1051
#define INIT_BALL_PARAMS                         8
#define INIT_PLAYER                           1052
#define INIT_PLAYER_PARAMS                       7
#define DEINIT_COURT                          1053
#define DEINIT_COURT_PARAMS                      0
#define DEINIT_BALL                           1054
#define DEINIT_BALL_PARAMS                       0
#define DEINIT_PLAYER                         1055
#define DEINIT_PLAYER_PARAMS                     1
#define DETECT_BALL_COLLISION                 1056
#define DETECT_BALL_COLLISION_PARAMS             8
#define DETECT_PLAYER_COLLISION               1057
#define DETECT_PLAYER_COLLISION_PARAMS           8
#define GET_LAST_BALL_COLLISION               1058
#define GET_LAST_BALL_COLLISION_PARAMS           1
#define GET_LAST_PLAYER_COLLISION             1059
#define GET_LAST_PLAYER_COLLISION_PARAMS         1
#define INIT_VIRTUAL_BALL                     1060
#define INIT_VIRTUAL_BALL_PARAMS                 8
#define DEINIT_VIRTUAL_BALL                   1061
#define DEINIT_VIRTUAL_BALL_PARAMS               0
#define PLAYER_OFF                            1062
#define PLAYER_OFF_PARAMS                        1
#define PLAYER_ON                             1063
#define PLAYER_ON_PARAMS                         1
#define RASIE_SHIELDS                         1064
#define RASIE_SHIELDS_PARAMS                     1
#define LOWER_SHIELDS                         1065
#define LOWER_SHIELDS_PARAMS                     1
#define FIND_PLAYER_CLOSEST_TO_BALL           1066
#define FIND_PLAYER_CLOSEST_TO_BALL_PARAMS       1
#define IS_PLAYER_IN_BOUNDS                   1067
#define IS_PLAYER_IN_BOUNDS_PARAMS               1
#define ARE_SHIELDS_CLEAR                     1068
#define ARE_SHIELDS_CLEAR_PARAMS                 0
#define SHIELD_PLAYER                         1069
#define SHIELD_PLAYER_PARAMS                     2
#define CLEAR_PLAYER_SHIELD                   1070
#define CLEAR_PLAYER_SHIELD_PARAMS               1
#define IS_BALL_IN_BOUNDS                     1071
#define IS_BALL_IN_BOUNDS_PARAMS                 0 
#define GET_AVOIDANCE_PATH                    1072
#define GET_AVOIDANCE_PATH_PARAMS                4
#define SET_BALL_LOCATION                     1073
#define SET_BALL_LOCATION_PARAMS                 4
#define GET_BALL_LOCATION                     1074
#define GET_BALL_LOCATION_PARAMS                 1
#define SET_PLAYER_LOCATION                   1075
#define SET_PLAYER_LOCATION_PARAMS               4
#define GET_PLAYER_LOCATION                   1076
#define GET_PLAYER_LOCATION_PARAMS               1
#define START_BLOCK                           1077
#define START_BLOCK_PARAMS                       3
#define HOLD_BLOCK                            1078
#define HOLD_BLOCK_PARAMS                        1
#define END_BLOCK                             1079
#define END_BLOCK_PARAMS                         1
#define IS_PLAYER_IN_GAME                     1080
#define IS_PLAYER_IN_GAME_PARAMS                 1
#define IS_BALL_IN_GAME                       1081
#define IS_BALL_IN_GAME_PARAMS                   1

#define UPDATE_CURSOR_POS                     1090
#define UPDATE_CURSOR_POS_PARAMS                 2
#define MAKE_CURSOR_STICKY                    1091
#define MAKE_CURSOR_STICKY_PARAMS                2
#define CURSOR_TRACK_MOVING_OBJECT            1092
#define CURSOR_TRACK_MOVING_OBJECT_PARAMS        2
#define GET_CURSOR_POSITION                   1093
#define GET_CURSOR_POSITION_PARAMS               0

#define AI_GET_OPEN_SPOT                      1100
#define AI_GET_OPEN_SPOT_PARAMS                 10
#define AI_GET_OPPONENTS_IN_CONE              1101
#define AI_GET_OPPONENTS_IN_CONE_PARAMS          6

#define U32_CLEAN_UP_OFF_HEAP	              1102
#define U32_CLEAN_UP_OFF_HEAP_PARAMS          0

#define DRAW_DEBUG_LINES                      1500
#define DRAW_DEBUG_LINES_PARAMS                  0

#define ADD_DEBUG_GEOM                        1501

int LogicHEBasketball::versionID() {
	return 1;
}

int32 LogicHEBasketball::dispatch(int cmdID, int paramCount, int32 *pParams) {
	bool bDidSomething = false;
	U32FltPoint3D point1_3d, point2_3d;
	U32FltPoint2D point1_2d, point2_2d;
	U32IntVector3D intVector1_3d;
	U32FltVector3D fltVector1_3d, fltVector2_3d;
	U32FltVector2D fltVector1_2d;
	U32Sphere sphere1;

	int retValue = 0;

	switch (cmdID) {

	case INIT_SCREEN_TRANSLATIONS:
		assert(paramCount == INIT_SCREEN_TRANSLATIONS_PARAMS);

		retValue = U32_User_Init_Screen_Translations();
		break;

	case WORLD_TO_SCREEN_TRANSLATION:
		assert(paramCount == WORLD_TO_SCREEN_TRANSLATION_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		retValue = U32_User_World_To_Screen_Translation(point1_3d);
		break;

	case SCREEN_TO_WORLD_TRANSLATION:
		assert(paramCount == SCREEN_TO_WORLD_TRANSLATION_PARAMS);

		point1_2d.x = (float)pParams[0];
		point1_2d.y = (float)pParams[1];

		retValue = U32_User_Screen_To_World_Translation(point1_2d);
		break;

	case GET_COURT_DIMENSIONS:
		assert(paramCount == GET_COURT_DIMENSIONS_PARAMS);

		retValue = U32_User_Get_Court_Dimensions();
		break;

	case COMPUTE_TRAJECTORY_TO_TARGET:
		assert(paramCount == COMPUTE_TRAJECTORY_TO_TARGET_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		point2_3d.x = (float)pParams[3];
		point2_3d.y = (float)pParams[4];
		point2_3d.z = (float)pParams[5];

		retValue = U32_User_Compute_Trajectory_To_Target(point1_3d, point2_3d, pParams[6]);
		break;

	case COMPUTE_LAUNCH_TRAJECTORY:
		assert(paramCount == COMPUTE_LAUNCH_TRAJECTORY_PARAMS);

		point1_2d.x = (float)pParams[0];
		point1_2d.y = (float)pParams[1];

		point2_2d.x = (float)pParams[2];
		point2_2d.y = (float)pParams[3];

		retValue = U32_User_Compute_Launch_Trajectory(point1_2d, point2_2d, pParams[4], pParams[5]);
		break;

	case COMPUTE_ANGLE_BETWEEN_VECTORS:
		assert(paramCount == COMPUTE_ANGLE_BETWEEN_VECTORS_PARAMS);

		fltVector1_3d.x = (float)pParams[0];
		fltVector1_3d.y = (float)pParams[1];
		fltVector1_3d.z = (float)pParams[2];

		fltVector2_3d.x = (float)pParams[3];
		fltVector2_3d.y = (float)pParams[4];
		fltVector2_3d.z = (float)pParams[5];

		retValue = U32_User_Compute_Angle_Between_Vectors(fltVector1_3d, fltVector2_3d);
		break;

	case COMPUTE_INITIAL_SHOT_VELOCITY:
		assert(paramCount == COMPUTE_INITIAL_SHOT_VELOCITY_PARAMS);
		retValue = U32_User_Compute_Initial_Shot_Velocity(pParams[0], pParams[1], pParams[2], pParams[3]);
		break;

	case COMPUTE_ANGLE_OF_SHOT:
		assert(paramCount == COMPUTE_ANGLE_OF_SHOT_PARAMS);
		retValue = U32_User_Compute_Angle_Of_Shot(pParams[0], pParams[1]);
		break;

	case COMPUTE_ANGLE_OF_PASS:
		assert(paramCount == COMPUTE_ANGLE_OF_PASS_PARAMS);
		retValue = U32_User_Compute_Angle_Of_Pass(pParams[0], pParams[1], pParams[2], pParams[3]);
		break;

	case COMPUTE_ANGLE_OF_BOUNCE_PASS:
		assert(paramCount == COMPUTE_ANGLE_OF_BOUNCE_PASS_PARAMS);
		retValue = U32_User_Compute_Angle_Of_Bounce_Pass(pParams[0], pParams[1], pParams[2], pParams[3], pParams[4]);
		break;

	case HIT_MOVING_TARGET:
		assert(paramCount == HIT_MOVING_TARGET_PARAMS);

		point1_2d.x = (float)pParams[0];
		point1_2d.y = (float)pParams[1];
		point2_2d.x = (float)pParams[2];
		point2_2d.y = (float)pParams[3];
		fltVector1_2d.x = (float)pParams[4];
		fltVector1_2d.y = (float)pParams[5];

		retValue = U32_User_Hit_Moving_Target(point1_2d, point2_2d, fltVector1_2d, pParams[6]);
		break;

	case GET_PASS_TARGET:
		assert(paramCount == GET_PASS_TARGET_PARAMS);

		fltVector1_3d.x = (float)pParams[1];
		fltVector1_3d.y = (float)pParams[2];

		retValue = U32_User_Get_Pass_Target(pParams[0], fltVector1_3d);
		break;

	case DETECT_PASS_BLOCKER:
		assert(paramCount == DETECT_PASS_BLOCKER_PARAMS);

		fltVector1_3d.x = (float)pParams[1];
		fltVector1_3d.y = (float)pParams[2];

		retValue = U32_User_Detect_Pass_Blocker(pParams[0], fltVector1_3d);
		break;

	case GET_BALL_INTERCEPT:
		assert(paramCount == GET_BALL_INTERCEPT_PARAMS);

		retValue = U32_User_Get_Ball_Intercept(pParams[0], pParams[1], pParams[2], pParams[3]);
		break;

	case COMPUTE_POINTS_FOR_PIXELS:
		assert(paramCount == COMPUTE_POINTS_FOR_PIXELS_PARAMS);
		retValue = U32_User_Compute_Points_For_Pixels(pParams[0], pParams[1]);
		break;

	case COMPUTE_BANK_SHOT_TARGET:
		assert(paramCount == COMPUTE_BANK_SHOT_TARGET_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		retValue = U32_User_Compute_Bank_Shot_Target(point1_3d, pParams[3]);
		break;

	case COMPUTE_SWOOSH_TARGET:
		assert(paramCount == COMPUTE_SWOOSH_TARGET_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		retValue = U32_User_Compute_Swoosh_Target(point1_3d, pParams[3]);
		break;

	case INIT_COURT:
		assert(paramCount == INIT_COURT_PARAMS);
		retValue = U32_User_Init_Court(pParams[0]);
		break;

	case DEINIT_COURT:
		assert(paramCount == DEINIT_COURT_PARAMS);
		retValue = U32_User_Deinit_Court();
		break;

	case INIT_BALL:
		assert(paramCount == INIT_BALL_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		fltVector1_3d.x = (float)pParams[3];
		fltVector1_3d.y = (float)pParams[4];
		fltVector1_3d.z = (float)pParams[5];

		retValue = U32_User_Init_Ball(point1_3d, fltVector1_3d, pParams[6], pParams[7]);
		break;

	case DEINIT_BALL:
		assert(paramCount == DEINIT_BALL_PARAMS);

		retValue = U32_User_Deinit_Ball();
		break;

	case INIT_VIRTUAL_BALL:
		assert(paramCount == INIT_VIRTUAL_BALL_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		fltVector1_3d.x = (float)pParams[3];
		fltVector1_3d.y = (float)pParams[4];
		fltVector1_3d.z = (float)pParams[5];

		retValue = U32_User_Init_Virtual_Ball(point1_3d, fltVector1_3d, pParams[6], pParams[7]);
		break;

	case DEINIT_VIRTUAL_BALL:
		assert(paramCount == DEINIT_VIRTUAL_BALL_PARAMS);

		retValue = U32_User_Deinit_Virtual_Ball();
		break;

	case INIT_PLAYER:
		assert(paramCount == INIT_PLAYER_PARAMS);

		point1_3d.x = (float)pParams[1];
		point1_3d.y = (float)pParams[2];
		point1_3d.z = (float)pParams[3];

		retValue = U32_User_Init_Player(pParams[0], point1_3d, pParams[4], pParams[5], (pParams[6] != 0));
		break;

	case DEINIT_PLAYER:
		assert(paramCount == DEINIT_PLAYER_PARAMS);
		retValue = U32_User_Deinit_Player(pParams[0]);
		break;

	case PLAYER_OFF:
		assert(paramCount == PLAYER_OFF_PARAMS);
		retValue = U32_User_Player_Off(pParams[0]);
		break;

	case PLAYER_ON:
		assert(paramCount == PLAYER_ON_PARAMS);
		retValue = U32_User_Player_On(pParams[0]);
		break;

	case SET_BALL_LOCATION:
		assert(paramCount == SET_BALL_LOCATION_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		_vm->_basketball->g_court.GetBallPtr(pParams[3])->m_center = point1_3d;

		retValue = 1;
		break;

	case GET_BALL_LOCATION:
		assert(paramCount == GET_BALL_LOCATION_PARAMS);

		point1_3d = _vm->_basketball->g_court.GetBallPtr(pParams[0])->m_center;

		writeScummVar(_vm1->VAR_U32_USER_VAR_A, _vm->_basketball->U32_Float_To_Int(point1_3d.x));
		writeScummVar(_vm1->VAR_U32_USER_VAR_B, _vm->_basketball->U32_Float_To_Int(point1_3d.y));
		writeScummVar(_vm1->VAR_U32_USER_VAR_C, _vm->_basketball->U32_Float_To_Int(point1_3d.z));

		retValue = 1;
		break;

	case SET_PLAYER_LOCATION:
		assert(paramCount == SET_PLAYER_LOCATION_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		point1_3d.z += (_vm->_basketball->g_court.GetPlayerPtr(pParams[3])->m_height / 2);
		_vm->_basketball->g_court.GetPlayerPtr(pParams[3])->m_center = point1_3d;

		retValue = 1;
		break;

	case GET_PLAYER_LOCATION:
		assert(paramCount == GET_PLAYER_LOCATION_PARAMS);

		point1_3d = _vm->_basketball->g_court.GetPlayerPtr(pParams[0])->m_center;
		point1_3d.z -= (_vm->_basketball->g_court.GetPlayerPtr(pParams[0])->m_height / 2);

		writeScummVar(_vm1->VAR_U32_USER_VAR_A, _vm->_basketball->U32_Float_To_Int(point1_3d.x));
		writeScummVar(_vm1->VAR_U32_USER_VAR_B, _vm->_basketball->U32_Float_To_Int(point1_3d.y));
		writeScummVar(_vm1->VAR_U32_USER_VAR_C, _vm->_basketball->U32_Float_To_Int(point1_3d.z));

		retValue = 1;
		break;

	case DETECT_BALL_COLLISION:
		assert(paramCount == DETECT_BALL_COLLISION_PARAMS);

		point1_3d.x = (float)pParams[0];
		point1_3d.y = (float)pParams[1];
		point1_3d.z = (float)pParams[2];

		fltVector1_3d.x = (float)pParams[3];
		fltVector1_3d.y = (float)pParams[4];
		fltVector1_3d.z = (float)pParams[5];

		retValue = U32_User_Detect_Ball_Collision(point1_3d, fltVector1_3d, pParams[6], pParams[7]);
		break;

	case DETECT_PLAYER_COLLISION:
		assert(paramCount == DETECT_PLAYER_COLLISION_PARAMS);

		point1_3d.x = (float)pParams[1];
		point1_3d.y = (float)pParams[2];
		point1_3d.z = (float)pParams[3];

		fltVector1_3d.x = (float)pParams[4];
		fltVector1_3d.y = (float)pParams[5];
		fltVector1_3d.z = (float)pParams[6];

		retValue = U32_User_Detect_Player_Collision(pParams[0], point1_3d, fltVector1_3d, (pParams[7] != 0));
		break;

	case GET_LAST_BALL_COLLISION:
		assert(paramCount == GET_LAST_BALL_COLLISION_PARAMS);

		retValue = U32_User_Get_Last_Ball_Collision(pParams[0]);
		break;

	case GET_LAST_PLAYER_COLLISION:
		assert(paramCount == GET_LAST_PLAYER_COLLISION_PARAMS);

		retValue = U32_User_Get_Last_Player_Collision(pParams[0]);
		break;

	case DETECT_SHOT_MADE:
		assert(paramCount == DETECT_SHOT_MADE_PARAMS);

		sphere1.m_center.x = (float)pParams[0];
		sphere1.m_center.y = (float)pParams[1];
		sphere1.m_center.z = (float)pParams[2];
		sphere1.m_radius = (float)pParams[6];

		intVector1_3d.x = pParams[3];
		intVector1_3d.y = pParams[4];
		intVector1_3d.z = pParams[5];

		retValue = U32_User_Detect_Shot_Made(sphere1, intVector1_3d, pParams[7], pParams[8]);
		break;

	case RASIE_SHIELDS:
		assert(paramCount == RASIE_SHIELDS_PARAMS);

		retValue = U32_User_Raise_Shields(pParams[0]);
		break;

	case LOWER_SHIELDS:
		assert(paramCount == LOWER_SHIELDS_PARAMS);

		retValue = U32_User_Lower_Shields(pParams[0]);
		break;

	case FIND_PLAYER_CLOSEST_TO_BALL:
		assert((paramCount == 0) || (paramCount == FIND_PLAYER_CLOSEST_TO_BALL_PARAMS));

		if (paramCount == 0) {
			retValue = U32_User_Get_Player_Closest_To_Ball();
		} else {
			retValue = U32_User_Get_Player_Closest_To_Ball(pParams[0]);
		}

		break;

	case IS_PLAYER_IN_BOUNDS:
		assert(paramCount == IS_PLAYER_IN_BOUNDS_PARAMS);

		retValue = U32_User_Is_Player_In_Bounds(pParams[0]);
		break;

	case IS_BALL_IN_BOUNDS:
		assert(paramCount == IS_BALL_IN_BOUNDS_PARAMS);

		retValue = U32_User_Is_Ball_In_Bounds();
		break;

	case ARE_SHIELDS_CLEAR:
		assert(paramCount == ARE_SHIELDS_CLEAR_PARAMS);

		retValue = U32_User_Are_Shields_Clear();
		break;

	case SHIELD_PLAYER:
		assert(paramCount == SHIELD_PLAYER_PARAMS);

		retValue = U32_User_Shield_Player(pParams[0], pParams[1]);
		break;

	case CLEAR_PLAYER_SHIELD:
		assert(paramCount == CLEAR_PLAYER_SHIELD_PARAMS);

		retValue = U32_User_Clear_Player_Shield(pParams[0]);
		break;

	case GET_AVOIDANCE_PATH:
		assert(paramCount == GET_AVOIDANCE_PATH_PARAMS);

		point1_2d.x = (float)pParams[1];
		point1_2d.y = (float)pParams[2];

		retValue = U32_User_Get_Avoidance_Path(pParams[0], point1_2d, (EAvoidanceType)pParams[3]);
		break;

	case START_BLOCK:
		assert(paramCount == START_BLOCK_PARAMS);
		assert((FIRST_PLAYER <= pParams[0]) && (pParams[0] <= LAST_PLAYER));

		(_vm->_basketball->g_court.GetPlayerPtr(pParams[0]))->StartBlocking(pParams[1], pParams[2]);

		retValue = 1;
		break;

	case HOLD_BLOCK:
		assert(paramCount == HOLD_BLOCK_PARAMS);
		assert((FIRST_PLAYER <= pParams[0]) && (pParams[0] <= LAST_PLAYER));

		(_vm->_basketball->g_court.GetPlayerPtr(pParams[0]))->HoldBlocking();

		retValue = 1;
		break;

	case END_BLOCK:
		assert(paramCount == END_BLOCK_PARAMS);
		assert((FIRST_PLAYER <= pParams[0]) && (pParams[0] <= LAST_PLAYER));

		(_vm->_basketball->g_court.GetPlayerPtr(pParams[0]))->EndBlocking();

		retValue = 1;
		break;

	case IS_PLAYER_IN_GAME:
		assert(paramCount == IS_PLAYER_IN_GAME_PARAMS);
		assert((FIRST_PLAYER <= pParams[0]) && (pParams[0] <= LAST_PLAYER));

		writeScummVar(_vm1->VAR_U32_USER_VAR_A, ((_vm->_basketball->g_court.GetPlayerPtr(pParams[0]))->m_playerIsInGame) ? 1 : 0);

		retValue = 1;
		break;

	case IS_BALL_IN_GAME:
		assert(paramCount == IS_BALL_IN_GAME_PARAMS);

		writeScummVar(_vm1->VAR_U32_USER_VAR_A, ((_vm->_basketball->g_court.GetBallPtr(pParams[0]))->m_ignore) ? 0 : 1);

		retValue = 1;
		break;

	case UPDATE_CURSOR_POS:
		assert(paramCount == UPDATE_CURSOR_POS_PARAMS);

		retValue = U32_User_Update_Cursor_Pos(pParams[0], pParams[1]);
		break;

	case MAKE_CURSOR_STICKY:
		assert(paramCount == MAKE_CURSOR_STICKY_PARAMS);

		retValue = U32_User_Make_Cursor_Sticky(pParams[0], pParams[1]);
		break;

	case CURSOR_TRACK_MOVING_OBJECT:
		assert(paramCount == CURSOR_TRACK_MOVING_OBJECT_PARAMS);

		retValue = U32_User_Cursor_Track_Moving_Object(pParams[0], pParams[1]);
		break;

	case GET_CURSOR_POSITION:
		assert(paramCount == GET_CURSOR_POSITION_PARAMS);

		retValue = U32_User_Get_Cursor_Pos();
		break;

	// Inputs: whichPlayer rect_upper_left_x, rect_upper_left_y, rect_lower_right_x, rect_lower_right_y,
	//         pass_point_x, pass_point_y, bUseAttracter, attract_x, attract_y
	case AI_GET_OPEN_SPOT:
		assert(paramCount == AI_GET_OPEN_SPOT_PARAMS);
		retValue = U32_User_Get_Open_Spot(
			pParams[0],                                            // whichPlayer
			U32FltVector2D((float)pParams[1], (float)pParams[2]),  // rect_upper_left
			U32FltVector2D((float)pParams[3], (float)pParams[4]),  // rect_lower_right
			U32FltVector2D((float)pParams[5], (float)pParams[6]),  // pass_point
			(pParams[7] != 0),                                     // bUseAttracter
			U32FltVector2D((float)pParams[8], (float)pParams[9])); // attract_point
		break;

	// Inputs: team width_distance_ratio*65536 end_x end_y focus_x focus_y
	case AI_GET_OPPONENTS_IN_CONE:
		assert(paramCount == AI_GET_OPPONENTS_IN_CONE_PARAMS);
		retValue = _vm->_basketball->NumOpponentsInCone(
			pParams[0],                                            // team
			(((float)pParams[1]) / 65536),                         // width_distance_ratio
			U32FltVector2D((float)pParams[2], (float)pParams[3]),  // end
			U32FltVector2D((float)pParams[4], (float)pParams[5])); // focus
		break;

	case U32_CLEAN_UP_OFF_HEAP:
		// No-op
		retValue = 1;
		break;

	default:
		break;
	}

	return retValue;
}

int LogicHEBasketball::U32_User_Get_Court_Dimensions() {
	writeScummVar(_vm1->VAR_U32_USER_VAR_A, _vm->_basketball->U32_Float_To_Int(MAX_WORLD_X));
	writeScummVar(_vm1->VAR_U32_USER_VAR_B, _vm->_basketball->U32_Float_To_Int(MAX_WORLD_Y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_C, _vm->_basketball->U32_Float_To_Int(BASKET_X));
	writeScummVar(_vm1->VAR_U32_USER_VAR_D, _vm->_basketball->U32_Float_To_Int(BASKET_Y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_E, _vm->_basketball->U32_Float_To_Int(BASKET_Z));

	return 1;
}

int LogicHEBasketball::U32_User_Init_Court(int courtID) {
	static Common::String g_CourtNames[] = {
		"",
		"Dobbaguchi", "Jocindas", "SandyFlats", "Queens",
		"Park", "Scheffler", "Polk", "McMillan",
		"CrownHill", "Memorial", "TechState", "Garden",
		"Moon", "Barn"
	};


	// Make sure nothing on the court is currently initialized.
	_vm->_basketball->g_court.m_objectTree.~CCollisionObjectTree();
	_vm->_basketball->g_court.m_objectList.clear();
	_vm->_basketball->g_court.m_homePlayerList.clear();
	_vm->_basketball->g_court.m_awayPlayerList.clear();

	// Initialize the shot spots
	_vm->_basketball->g_court.m_shotSpot[LEFT_BASKET].m_center.x = BASKET_X;
	_vm->_basketball->g_court.m_shotSpot[LEFT_BASKET].m_center.y = BASKET_Y;
	_vm->_basketball->g_court.m_shotSpot[LEFT_BASKET].m_center.z = BASKET_Z;
	_vm->_basketball->g_court.m_shotSpot[LEFT_BASKET].m_radius = SHOT_SPOT_RADIUS;

	_vm->_basketball->g_court.m_shotSpot[RIGHT_BASKET].m_center.x = MAX_WORLD_X - BASKET_X;
	_vm->_basketball->g_court.m_shotSpot[RIGHT_BASKET].m_center.y = BASKET_Y;
	_vm->_basketball->g_court.m_shotSpot[RIGHT_BASKET].m_center.z = BASKET_Z;
	_vm->_basketball->g_court.m_shotSpot[RIGHT_BASKET].m_radius = SHOT_SPOT_RADIUS;

	// Get the name and object file for this court.
	_vm->_basketball->g_court.m_name = g_CourtNames[courtID];

	// Put together to relative path and filename.
	Common::Path objectFileName = Common::Path(Common::String::format("data/courts/%s.cof", g_CourtNames[courtID - 1]));

	// Create a file stream to the collision object file
	Common::File objectFile;
	if (!objectFile.open(objectFileName))
		error("Could not open file '%s'", objectFileName.toString(Common::Path::kNativeSeparator).c_str());

	// Read in the object file version
	char fileVersion[32];
	int versionStringLength = objectFile.readUint32LE();

	if (versionStringLength <= 0 && versionStringLength > ARRAYSIZE(fileVersion) - 1)
		error("Read from stream did not read the version string length correctly.");
	
	objectFile.read(fileVersion, versionStringLength);
	fileVersion[versionStringLength] = '\0';

	if (strcmp(fileVersion, "01.05"))
		error("Invalid court version field: %s", fileVersion);

	// Read in the total number of objects
	_vm->_basketball->g_court.m_objectCount = objectFile.readUint32LE();
	_vm->_basketball->g_court.m_objectList.resize(_vm->_basketball->g_court.m_objectCount);

	// Keep a list of pointers to the court objects
	CCollisionObjectVector objectPtrList;
	objectPtrList.resize(_vm->_basketball->g_court.m_objectCount);

	// Read in each court object.
	for (int i = 0; i < _vm->_basketball->g_court.m_objectCount; i++) {
		CCollisionBox *pCurrentObject = &_vm->_basketball->g_court.m_objectList[i];

		// Read in this object's description
		int descriptionStringLength = objectFile.readUint32LE();
		char *tmp = (char *)malloc(descriptionStringLength);

		objectFile.read(tmp, descriptionStringLength);
		Common::String tmp2(tmp);

		_vm->_basketball->g_court.m_objectList[i].m_description = tmp2;

		free(tmp);

		// Read in all other object attributes
		pCurrentObject->m_objectType = (EObjectType)objectFile.readUint32LE();
		pCurrentObject->m_collisionEfficiency = objectFile.readUint32LE();
		pCurrentObject->m_friction  = objectFile.readUint32LE();
		pCurrentObject->m_soundNumber = objectFile.readUint32LE();
		pCurrentObject->m_objectID = objectFile.readUint32LE();
		pCurrentObject->m_minPoint.x = objectFile.readUint32LE();
		pCurrentObject->m_minPoint.y = objectFile.readUint32LE();
		pCurrentObject->m_minPoint.z = objectFile.readUint32LE();
		pCurrentObject->m_maxPoint.x = objectFile.readUint32LE();
		pCurrentObject->m_maxPoint.y = objectFile.readUint32LE();
		pCurrentObject->m_maxPoint.z = objectFile.readUint32LE();
		objectPtrList[i] = pCurrentObject;

		// Decide if this is a backboard, and keep track of it if it is.
		if (pCurrentObject->m_objectType == kBackboard) {
			// See which backboard it is.
			if (((pCurrentObject->m_minPoint.x + pCurrentObject->m_maxPoint.x) / 2) < (MAX_WORLD_X / 2)) {
				_vm->_basketball->g_court.m_backboardIndex[LEFT_BASKET] = i;
			} else {
				_vm->_basketball->g_court.m_backboardIndex[RIGHT_BASKET] = i;
			}
		}
	}

	_vm->_basketball->g_court.m_objectTree.Initialize(objectPtrList);
	

	// Lower all the shields
	U32_User_Lower_Shields(ALL_SHIELD_ID);

	return 1;
}

int LogicHEBasketball::U32_User_Deinit_Court() {
	// Make sure the collision object tree has been cleared
	_vm->_basketball->g_court.m_objectTree.~CCollisionObjectTree();

	// Lower all the shields
	U32_User_Lower_Shields(ALL_SHIELD_ID);

	return 1;
}

LogicHE *makeLogicHEbasketball(ScummEngine_v100he *vm) {
	return new LogicHEBasketball(vm);
}

} // End of namespace Scumm
