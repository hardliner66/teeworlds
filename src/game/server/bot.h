#ifndef GAME_SERVER_BOT_H
#define GAME_SERVER_BOT_H

#include <base/vmath.h>

#include "gamecontext.h"
#include "botengine.h"

#include "ai/genetics.h"
#include "ai/strategy.h"

enum BotDifficulty {
	DIFFICULTY_PEACEFUL_STATIONARY = -3,
	DIFFICULTY_PEACEFUL_NO_HOOK,
	DIFFICULTY_PEACEFUL,
	DIFFICULTY_EASIEST,
	DIFFICULTY_VERY_EASY,
	DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,
	DIFFICULTY_VERY_HARD,
	DIFFICULTY_GODLIKE = 99
};

const BotDifficulty difficulties[10] = {
	DIFFICULTY_PEACEFUL_STATIONARY,
	DIFFICULTY_PEACEFUL_NO_HOOK,
	DIFFICULTY_PEACEFUL,
	DIFFICULTY_EASIEST,
	DIFFICULTY_VERY_EASY,
	DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,
	DIFFICULTY_VERY_HARD,
	DIFFICULTY_GODLIKE
};

inline const bool ValidDifficulty(int difficulty) {
	switch (difficulty) {
		case DIFFICULTY_PEACEFUL_STATIONARY:
		case DIFFICULTY_PEACEFUL_NO_HOOK:
		case DIFFICULTY_PEACEFUL:
		case DIFFICULTY_EASIEST:
		case DIFFICULTY_VERY_EASY:
		case DIFFICULTY_EASY:
		case DIFFICULTY_MEDIUM:
		case DIFFICULTY_HARD:
		case DIFFICULTY_VERY_HARD:
		case DIFFICULTY_GODLIKE:
			return true;
	}
	return false;
}

inline const char* GetDifficultyName(int difficulty) {
	switch (difficulty) {
		case DIFFICULTY_PEACEFUL_STATIONARY: {
			return "peaceful (stationary)";
		}break;
		case DIFFICULTY_PEACEFUL_NO_HOOK: {
			return "peaceful (no hook)";
		}break;
		case DIFFICULTY_PEACEFUL: {
			return "peaceful";
		}break;
		case DIFFICULTY_EASIEST: {
			return "easiest";
		}break;
		case DIFFICULTY_VERY_EASY: {
			return "very easy";
		}break;
		case DIFFICULTY_EASY: {
			return "easy";
		}break;
		case DIFFICULTY_MEDIUM: {
			return "medium";
		}break;
		case DIFFICULTY_HARD: {
			return "hard";
		}break;
		case DIFFICULTY_VERY_HARD: {
			return "very hard";
		}break;
		case DIFFICULTY_GODLIKE: {
			return "godlike";
		}break;
	}
	return "";
}

const char g_BotClan[12] = "Love";
const char g_aBotName[MAX_CLIENTS][16] = {
	"[B]Anna",
	"[B]Bob",
	"[B]Carlos",
	"[B]David",
	"[B]Eli",
	"[B]Florianne",
	"[B]Gaia",
	"[B]Hannibal",
	"[B]Isis",
	"[B]Juda",
	"[B]Kevin",
	"[B]Lucile",
	"[B]Marc",
	"[B]Naustradamus",
	"[B]Ondine",
	"[B]Platon"
};

const int g_aBotPriority[MAX_CLIENTS][8] = {
	{0,0,0,0,0,0,0,1},
	{0,0,0,0,0,0,0,1},
	{0,0,0,0,0,0,0,1},
	{0,0,0,0,0,0,0,1},
	{0,8,7,7,6,0,0,1},
	{0,8,7,7,6,0,0,1},
	{6,2,7,7,0,8,0,1},
	{6,2,7,7,0,8,0,1},
	{6,2,7,7,8,0,0,1},
	{6,2,7,7,8,0,0,1},
	{6,2,7,7,0,0,8,1},
	{6,2,7,7,0,0,8,1},
	{3,5,6,6,4,4,4,0},
	{3,5,6,6,4,4,4,0},
	{8,1,5,5,4,4,4,0},
	{8,1,5,5,4,4,4,0}
};

#define	BOT_HOOK_DIRS	32

#define BOT_CHECK_TIME (20*60*1000000)

class CBot
{
	class CBotEngine *m_pBotEngine;
	class CPlayer *m_pPlayer;
	class CGameContext *m_pGameServer;

protected:

	class CBotEngine *BotEngine() { return m_pBotEngine; }
	class CGameContext *GameServer() { return BotEngine()->GameServer(); }

	class CCollision *Collision() { return GameServer()->Collision(); }
	class CTuningParams *Tuning() { return GameServer()->Tuning(); }

	CBotEngine::CPath *m_pPath;

	int m_SnapID;

	enum {
		BFLAG_LOST	= 0,
		BFLAG_LEFT	= 1,
		BFLAG_RIGHT	= 2,
		BFLAG_JUMP	= 4,
		BFLAG_HOOK	= 8,
		BFLAG_FIRE	= 16
	};

	int m_Flags;

	vec2 m_Target;
	vec2 m_RealTarget;
	struct CTarget {
		vec2 m_Pos;
		enum {
			TARGET_EMPTY=-1,
			TARGET_PLAYER=0,
			TARGET_FLAG,
			TARGET_ARMOR,
			TARGET_HEALTH,
			TARGET_WEAPON_SHOTGUN,
			TARGET_WEAPON_GRENADE,
			//TARGET_POWERUP_NINJA,
			TARGET_WEAPON_LASER,
			TARGET_AIR,
			NUM_TARGETS
		};
		int m_Type;
		int m_PlayerCID;
		bool m_NeedUpdate;
		int m_StartTick;
	} m_ComputeTarget;

	class CGenetics m_Genetics;
	int m_aTargetOrder[CTarget::NUM_TARGETS];

	CStrategyPosition* m_pStrategyPosition;

	void UpdateTargetOrder();

	CNetObj_PlayerInput m_InputData;
	CNetObj_PlayerInput m_LastData;

	bool preFireLock;
	float preFireAngle;
	int preFireTimer;
	
	int m_Move;
	vec2 m_Direction;
	int m_Jump;
	int m_Attack;
	int m_Hook;

	int GetTarget();
	void UpdateTarget();
	int GetTeam(int ClientID);
	int IsFalling();
	bool IsGrounded();

	bool NeedPickup(int Type);
	bool FindPickup(int Type, vec2 *pPos, float Radius = 1000);

	void HandleWeapon(bool SeeTarget);
	void ShootWeapon();
	void HandleHook(bool SeeTarget);
	void UpdateEdge();
	void MakeChoice(bool UseTarget);

	int GetTile(int x, int y) { return BotEngine()->GetTile(x/32,y/32);}

	vec2 ClosestCharacter();

public:
	CBot(class CBotEngine *m_pBotEngine, CPlayer *pPlayer);
	virtual ~CBot();


	int m_GenomeTick;
	int m_ManualRespawnTick;
	bool m_IsDead;

	int GetID() { return m_SnapID; }
	void Snap(int SnappingClient);
	void Tick();

	virtual void OnReset();

	CNetObj_PlayerInput GetInputData() { Tick(); return m_InputData; };
	CNetObj_PlayerInput GetLastInputData() { return m_LastData; }
};

#endif
