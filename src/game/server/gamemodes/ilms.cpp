/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* iLMS by erd and Teetime                                                                 */

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include "iLMS.h"

CGameController_iLMS::CGameController_iLMS(class CGameContext *pGameServer) :
		IGameController(pGameServer)
{
	m_pGameType = "iLMS";
	m_PlayerCount = 0;
	m_ActivePlayerCount = 0;
	m_iLMS_enabled = true;
  m_points = 0;
}

void CGameController_iLMS::Tick()
{
	IGameController::Tick();
	if(m_GameOverTick == -1)
		CalcPlayerColor();
}

void CGameController_iLMS::DoWincheck()
{
	if(m_GameOverTick == -1)
	{
		int Players = 0, Players_Spec = 0, Players_SpecExplicit = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				Players++;
				if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
					Players_Spec++;
				if(GameServer()->m_apPlayers[i]->m_SpecExplicit == 1)
					Players_SpecExplicit++;
			}
		}

		m_PlayerCount = Players;
		m_ActivePlayerCount = Players - Players_Spec;

		/*if(m_ActivePlayerCount >= g_Config.m_SvfCatchMinPlayers)
		{
			m_iLMS_enabled = true;
		}
		else if (g_Config.m_SvAutoIdm == 1)
		{
			m_iLMS_enabled = true;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i])
				{
					if(GameServer()->m_apPlayers[i]->m_CaughtBy != CPlayer::FCATCH_NOT_CAUGHT)
					{
						GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
						GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));
					}
				}
			}
		}*/

    m_iLMS_enabled = true;
		if(Players == 1)
		{
			//Do nothing
		}
		else if((Players - Players_Spec == 1) && (Players != Players_Spec) && (Players - Players_SpecExplicit != 1))
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				{
          //GameServer()->m_apPlayers[i]->m_Score += g_Config.m_SvBonus;
          GameServer()->m_apPlayers[i]->m_Score = m_points;
        }
			}
			EndRound();
		}

		IGameController::DoWincheck(); //do also usual wincheck
	}
}

int CGameController_iLMS::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponID)
{
	if(!pKiller)
		return 0;

	int VictimID = pVictim->GetPlayer()->GetCID();

  pVictim->GetPlayer()->m_Score = m_points;

  m_points++;

	if(pKiller != pVictim->GetPlayer())
	{
		pKiller->m_Kills++;
		if(pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			pKiller->m_Kills += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;


		pVictim->GetPlayer()->m_Deaths++;
		if(pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			pVictim->GetPlayer()->m_Deaths += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;

		//pKiller->m_Score++;
		//if(pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			//pKiller->m_Score += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;

		pVictim->GetPlayer()->m_Wallshot=false;

		/* Check if the killer is already killed and in spectator (victim may died through wallshot) */
		if(pKiller->GetTeam() != TEAM_SPECTATORS)
		{
			pVictim->GetPlayer()->m_CaughtBy = pKiller->GetCID();
      //pVictim->GetPlayer()->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
			pVictim->GetPlayer()->SetTeamDirect(TEAM_SPECTATORS);

			pVictim->GetPlayer()->m_SpectatorID = pKiller->GetCID(); // Let the victim follow his catcher

			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Killed by \"%s\".", Server()->ClientName(pKiller->GetCID()));
			GameServer()->SendChatTarget(VictimID, aBuf);
		}
	}
	else
	{
		//Punish selfkill/death
		if(WeaponID == WEAPON_SELF || WeaponID == WEAPON_WORLD)
		{
      //pVictim->GetPlayer()->m_Score -= g_Config.m_SvKillPenalty;
    }
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(GameServer()->m_apPlayers[i]->m_CaughtBy == VictimID)
			{
				//GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
				//GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));
        GameServer()->m_apPlayers[i]->m_CaughtBy = pKiller->GetCID();
        GameServer()->m_apPlayers[i]->m_SpectatorID = pKiller->GetCID(); // Let the victim follow his catcher

				if(pKiller != pVictim->GetPlayer())
				{
          pVictim->GetPlayer()->m_Score = pVictim->GetPlayer()->m_Score;
          //m_points++;
        }
			}
		}
	}

	// Update colors
	OnPlayerInfoChange(pVictim->GetPlayer());

	return 0;
}

void CGameController_iLMS::OnPlayerInfoChange(class CPlayer *pP)
{
	if(g_Config.m_SvColorIndicator)
	{
		int Num = 161;
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_CaughtBy == pP->GetCID())
				Num -= 10;
		pP->m_TeeInfos.m_ColorBody = Num * 0x010000 + 0xff00;
		pP->m_TeeInfos.m_ColorFeet = Num * 0x010000 + 0xff00;
		pP->m_TeeInfos.m_UseCustomColor = 1;
	}
}

void CGameController_iLMS::StartRound()
{
	IGameController::StartRound();
  m_points = 0;
	m_iLMS_enabled = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
			GameServer()->m_apPlayers[i]->m_Kills = 0;
			GameServer()->m_apPlayers[i]->m_Deaths = 0;
			GameServer()->m_apPlayers[i]->m_TicksSpec = 0;
			GameServer()->m_apPlayers[i]->m_TicksIngame = 0;
		}
	}
}

void CGameController_iLMS::OnCharacterSpawn(class CCharacter *pChr)
{
	// default health and armor
	pChr->IncreaseHealth(10);

	// give default weapons
	/* Instagib - Only Riffle */
	pChr->GiveWeapon(WEAPON_RIFLE, -1);

	//Update color of spawning tees
	OnPlayerInfoChange(pChr->GetPlayer());
}

void CGameController_iLMS::EndRound()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{

			if(GameServer()->m_apPlayers[i]->m_SpecExplicit == 0)
			{
				GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));

				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Kills: %d | Deaths: %d", GameServer()->m_apPlayers[i]->m_Kills, GameServer()->m_apPlayers[i]->m_Deaths);
				GameServer()->SendChatTarget(i, aBuf);

				if(GameServer()->m_apPlayers[i]->m_TicksSpec != 0 || GameServer()->m_apPlayers[i]->m_TicksIngame != 0)
				{
					double TimeInSpec = (GameServer()->m_apPlayers[i]->m_TicksSpec * 100.0) / (GameServer()->m_apPlayers[i]->m_TicksIngame + GameServer()->m_apPlayers[i]->m_TicksSpec);
					str_format(aBuf, sizeof(aBuf), "Spec: %.2f%% | Ingame: %.2f%%", (double) TimeInSpec, (double) (100.0 - TimeInSpec));
					GameServer()->SendChatTarget(i, aBuf);
				}
				GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT; //Set all players in server as non-caught
			}
		}
	}

	if(m_Warmup) // game can't end when we are running warmup
		return;

	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

bool CGameController_iLMS::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	if(pPlayer->m_CaughtBy >= 0)
		return false;
	return true;
}

bool CGameController_iLMS::OnEntity(int Index, vec2 Pos)
{
	if(Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if(Index == ENTITY_SPAWN_RED)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if(Index == ENTITY_SPAWN_BLUE)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;

	return false;
}

void CGameController_iLMS::CalcPlayerColor()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(!pP)
			continue;
		if(pP->GetTeam() != TEAM_SPECTATORS)
			OnPlayerInfoChange(pP);
	}
}
