/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* fCatch by hardliner66, based on zcatch by erd and Teetime                                 */

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include "fcatch.h"

CGameController_fCatch::CGameController_fCatch(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "fCatch";
	m_PlayerCount = 0;
	m_ActivePlayerCount = 0;
	m_fCatch_enabled = false;
	m_fCatch_started = false;
}

void CGameController_fCatch::Tick()
{
	IGameController::Tick();
	if (m_GameOverTick == -1)
		CalcPlayerColor();
}

void CGameController_fCatch::DoWincheck()
{
	if (m_GameOverTick == -1)
	{
		int Players = 0, Players_Spec = 0, Players_SpecExplicit = 0;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i])
			{
				Players++;
				if (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
					Players_Spec++;
				if (GameServer()->m_apPlayers[i]->m_SpecExplicit == 1)
					Players_SpecExplicit++;
			}
		}

		m_PlayerCount = Players;
		m_ActivePlayerCount = Players - Players_Spec;

		if (g_Config.m_SvAutoIdm == 0 || m_ActivePlayerCount >= g_Config.m_SvfCatchMinPlayers)
		{
			m_fCatch_enabled = true;
		}
		else if (g_Config.m_SvAutoIdm == 1 && m_fCatch_started == false)
		{
			m_fCatch_enabled = false;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (GameServer()->m_apPlayers[i])
				{
					if (GameServer()->m_apPlayers[i]->m_CaughtBy != CPlayer::FCATCH_NOT_CAUGHT)
					{
						GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
						GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));
					}
				}
			}
		}

		if (m_fCatch_enabled && m_fCatch_started) {
			if (Players == 1) {
				EndRound();
			} else if (Players == 0) {
				m_fCatch_enabled = false;
				m_fCatch_started = false;
			}
		}

		if (Players == 1)
		{
			//Do nothing
		}
		else if ((Players - Players_Spec == 1) && (Players != Players_Spec) && (Players - Players_SpecExplicit != 1))
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
					GameServer()->m_apPlayers[i]->m_Score += g_Config.m_SvBonus;
			}
			EndRound();
		}

		IGameController::DoWincheck(); //do also usual wincheck
	}
}

bool CGameController_fCatch::ShouldShowCapturesInsteadOfKills() {
	return !(g_Config.m_SvfCatchShowCapturedPlayers == 0 || (g_Config.m_SvAutoIdm == 1 && m_ActivePlayerCount < g_Config.m_SvfCatchMinPlayers));
}

int CGameController_fCatch::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponID)
{
	if (!pKiller)
		return 0;

	if (ShouldShowCapturesInsteadOfKills())
	{
		pVictim->GetPlayer()->m_Score = 0;
	}

	int VictimID = pVictim->GetPlayer()->GetCID();

	if (pKiller != pVictim->GetPlayer())
	{
		if (m_fCatch_enabled) {
			m_fCatch_started = true;
		}
		pKiller->m_Kills++;
		if (pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			pKiller->m_Kills += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;

		pVictim->GetPlayer()->m_Deaths++;
		if (pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			pVictim->GetPlayer()->m_Deaths += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;

		pKiller->m_Score++;
		if (pVictim->GetPlayer()->m_Wallshot && g_Config.m_SvWallshot == 1)
			pKiller->m_Score += g_Config.m_SvWallshotBonus; //pKiller->m_Multiplier;

		pVictim->GetPlayer()->m_Wallshot = false;

		/* Check if the killer is already killed and in spectator (victim may died through wallshot) */
		if (pKiller->GetTeam() != TEAM_SPECTATORS && m_fCatch_started == true)
		{
			pVictim->GetPlayer()->m_CaughtBy = pKiller->GetCID();
			pVictim->GetPlayer()->SetTeamDirect(TEAM_SPECTATORS);

			pVictim->GetPlayer()->m_SpectatorID = pKiller->GetCID(); // Let the victim follow his catcher

			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Caught by \"%s\". You will join the game automatically when \"%s\" dies.", Server()->ClientName(pKiller->GetCID()), Server()->ClientName(pKiller->GetCID()));
			GameServer()->SendChatTarget(VictimID, aBuf);
		}
	}
	else
	{
		//Punish selfkill/death
		if (!ShouldShowCapturesInsteadOfKills())
		{
			if (WeaponID == WEAPON_SELF || WeaponID == WEAPON_WORLD)
				pVictim->GetPlayer()->m_Score -= g_Config.m_SvKillPenalty;
		}
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			if (GameServer()->m_apPlayers[i]->m_CaughtBy == VictimID)
			{
				GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
				GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));

				//if(pKiller != pVictim->GetPlayer())
				//pKiller->m_Score++;
			}
		}
	}

	// Update colors
	OnPlayerInfoChange(pVictim->GetPlayer());

	return 0;
}

void CGameController_fCatch::OnPlayerInfoChange(class CPlayer *pP)
{
	if (g_Config.m_SvColorIndicator)
	{
		int Num = 161;
		for (int i = 0; i < MAX_CLIENTS; i++)
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_CaughtBy == pP->GetCID())
				Num -= 10;
		pP->m_TeeInfos.m_ColorBody = Num * 0x010000 + 0xff00;
		pP->m_TeeInfos.m_ColorFeet = Num * 0x010000 + 0xff00;
		pP->m_TeeInfos.m_UseCustomColor = 1;
	}
}

void CGameController_fCatch::StartRound()
{
	IGameController::StartRound();
	m_fCatch_enabled = false;
	m_fCatch_started = false;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT;
			GameServer()->m_apPlayers[i]->m_Kills = 0;
			GameServer()->m_apPlayers[i]->m_Deaths = 0;
			GameServer()->m_apPlayers[i]->m_TicksSpec = 0;
			GameServer()->m_apPlayers[i]->m_TicksIngame = 0;
		}
	}
}

void CGameController_fCatch::OnCharacterSpawn(class CCharacter *pChr)
{
	// default health and armor
	pChr->IncreaseHealth(10);

	// give default weapons
	/* Instagib - Only Riffle */
	pChr->GiveWeapon(WEAPON_RIFLE, -1);

	//Update color of spawning tees
	OnPlayerInfoChange(pChr->GetPlayer());
}

void CGameController_fCatch::EndRound()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{

			if (GameServer()->m_apPlayers[i]->m_SpecExplicit == 0)
			{
				GameServer()->m_apPlayers[i]->SetTeamDirect(GameServer()->m_pController->ClampTeam(1));

				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Kills: %d | Deaths: %d", GameServer()->m_apPlayers[i]->m_Kills, GameServer()->m_apPlayers[i]->m_Deaths);
				GameServer()->SendChatTarget(i, aBuf);

				if (GameServer()->m_apPlayers[i]->m_TicksSpec != 0 || GameServer()->m_apPlayers[i]->m_TicksIngame != 0)
				{
					double TimeInSpec = (GameServer()->m_apPlayers[i]->m_TicksSpec * 100.0) / (GameServer()->m_apPlayers[i]->m_TicksIngame + GameServer()->m_apPlayers[i]->m_TicksSpec);
					str_format(aBuf, sizeof(aBuf), "Spec: %.2f%% | Ingame: %.2f%%", (double)TimeInSpec, (double)(100.0 - TimeInSpec));
					GameServer()->SendChatTarget(i, aBuf);
				}
				GameServer()->m_apPlayers[i]->m_CaughtBy = CPlayer::FCATCH_NOT_CAUGHT; //Set all players in server as non-caught
			}
		}
	}

	if (m_Warmup) // game can't end when we are running warmup
		return;

	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

bool CGameController_fCatch::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	if (pPlayer->m_CaughtBy >= 0)
		return false;
	return true;
}

bool CGameController_fCatch::OnEntity(int Index, vec2 Pos)
{
	if (Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if (Index == ENTITY_SPAWN_RED)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if (Index == ENTITY_SPAWN_BLUE)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;

	return false;
}

void CGameController_fCatch::CalcPlayerColor()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if (!pP)
			continue;
		if (pP->GetTeam() != TEAM_SPECTATORS)
			OnPlayerInfoChange(pP);
	}
}
