/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "laser.h"

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_CanKill = true;
	GameWorld()->InsertEntity(this);
	DoBounce();
}



bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
	{
		//pOwnerChar->GetPlayer()->m_Multiplier=0;
		return false;
	}

	m_From = From;
	m_Pos = At;
	m_Energy = -1;

	auto game_type = GameServer()->m_pController->m_pGameType;

  if (str_find_nocase(game_type, "iLMS") == game_type)
  {
    m_CanKill = true;
    TryKill(pHit);
  }
  else
  {
	  if(m_Bounces > 0)
	  {
		  pHit->GetPlayer()->m_Wallshot = true;
  		//pOwnerChar->GetPlayer()->RaiseMultiplier();
	  }
	  if (g_Config.m_SvfCatchMode == 2)
		  m_CanKill = checkPosition(pOwnerChar,pHit) && pHit->m_FreezeTicks > 0;

    // 	if(m_Bounces > 0)
  	if(g_Config.m_SvWallshot != 2)
  	{
  		TryKill(pHit);
	  }
	  else
	  {
		  if(m_Bounces > 0)
		  {
			  TryKill(pHit);
  		}
	  }
  }
	return true;
}

void CLaser::TryKill(CCharacter *pHit)
{
	if(m_CanKill)
	{
		pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE);
	}
	else
	{
		if (g_Config.m_SvfCatchMode > 0 && pHit->m_FreezeTicks <= 0)
			pHit->Freeze(Server()->TickSpeed()*g_Config.m_SvfCatchFreezeTime);
	}
}

bool CLaser::checkPosition(CCharacter *pOwnerChar,CCharacter *pHit)
{
int diff = pOwnerChar->m_Pos.y - pHit->m_Pos.y;
return (diff > 0 && diff > g_Config.m_SvfCatchMinDiff && !pHit->IsGrounded());
}

void CLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir * m_Energy;

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);

			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
			m_Bounces++;

			if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
				m_Energy = -1;

			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);

			if(m_Bounces == 1 && g_Config.m_SvLaserjumps)
				GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GAME, false);
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}
}

void CLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CLaser::TickPaused()
{
	++m_EvalTick;
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
