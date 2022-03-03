/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include "dm.h"
#include <engine/shared/config.h>
#include <string.h>

vec2 mid;
CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags;
	if (g_Config.m_SvBotsEnabled) {
		m_pGameType = (IsInstagib()) ? "iDM+|b" : "DM+|b";
	} else {
		m_pGameType = (IsInstagib()) ? "iDM+" : "DM+";
	}
	mid = vec2(rand() % 2000 + 500, rand() % 2000 + 500);
}
int rotation = 0;
int radius = 500;
int highestAmountPlayers = 0;

int EndScreen = 0;
int winner = 0;

void CGameControllerDM::Tick()
{
	IGameController::Tick();

	if(m_Warmup)
		return;

	EndScreen--;
	if(EndScreen > 0)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GameServer()->m_apPlayers[i] && i != winner)
				GameServer()->m_apPlayers[i]->m_SpectatorID = winner;
		GameServer()->m_World.m_Paused = true;
		return;
	}else if(EndScreen == 0)
	{
		StartRound();
		DoWarmup(5);
		GameServer()->m_World.m_Paused = false;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED);
			//GameServer()->m_apPlayers[i]->TryRespawn();
		}
		rotation = 0;
	}
	
	
	
	if(highestAmountPlayers > 1)
		rotation++;
	
	vec2 offset = vec2(sin(rotation), cos(rotation))*radius;
	if(radius > 2000)
		GameServer()->CreateEmptyExplosion(mid -offset);

	if(radius > 2000)
		GameServer()->CreateEmptyExplosion(mid +vec2(sin(rotation+90), cos(rotation+90))*radius);

	GameServer()->CreateEmptyExplosion(mid + offset);
	
	int time = GetRoundTick();
	if(time % 2000 < 1000)
		radius -= min(max((int)(radius * 0.001), 1), 2);

	if(rotation < 50)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i] || !GameServer()->m_apPlayers[i]->GetCharacter())
				continue;
			int dist = distance(mid,GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos);
			//printf("%i   %i", dist, radius);
			if(dist > radius)
				radius = dist + 1000;
		}
	}
		//-= 1;
	//printf("%i, %f\n", radius, max(0.0, sin(time/400)+0.5));
	if(radius < 200)
	{
		radius = 200;
	}
	
	if(rotation % 5 == 0 && radius > 700)
		GameServer()->CreateSound(mid + offset, SOUND_GRENADE_EXPLODE);
	if(rotation % 13 == 0 && radius < 700)
		GameServer()->CreateSound(mid + offset, SOUND_GRENADE_EXPLODE);

	int playersAlive = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;
		if(GameServer()->m_apPlayers[i]->GetCharacter())
			playersAlive++;
		//printf("%i  %i   %i\n",i, 	 GameServer()->m_apPlayers[i], GameServer()->m_apPlayers[i]->GetCharacter());
		
		if(rotation % 25 == 0)
		{
			if(GameServer()->m_apPlayers[i]->GetCharacter() && distance(mid, GameServer()->m_apPlayers[i]->GetCharacter()->GetPos())> radius-50)
				GameServer()->m_apPlayers[i]->GetCharacter()->TakeDamage(vec2(0,0), 1, -1, 0);
		}
	}
	if(highestAmountPlayers < playersAlive)
		highestAmountPlayers = playersAlive;
	
	if(!highestAmountPlayers)
	{
		StartRound();
		DoWarmup(5);
		return;
	}
	// if(playersAlive == 0 && GetRoundTick() > 66)
	// {
	// 	StartRound();
	// 	DoWarmup(5);
	// }
	if(playersAlive == 1 && highestAmountPlayers > 1 && GetRoundTick() > 66)
	{

		int type = rand()%3;
		while(m_aNumSpawnPoints[type])
		{
			mid = m_aaSpawnPoints[type][rand() % m_aNumSpawnPoints[type]];
			type = rand()%3;
		}
		// mid = vec2(rand() % 2000 + 500, rand() % 2000 + 500);
		EndRound();
		GameServer()->m_World.m_Paused = true;
		//StartRound();
		//DoWarmup(5);
		EndScreen = 500;
		highestAmountPlayers = 0;
		printf("%i\n", playersAlive);
		winner = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;
			if(GameServer()->m_apPlayers[i]->GetCharacter())
				winner = i;
			//GameServer()->m_apPlayers[i]->TryRespawn();
		}
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s won!!?!", Server()->ClientName(winner));
		GameServer()->SendBroadcast(aBuf, -1);
	}
}
