/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"
#include <engine/shared/config.h>

CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags;
	if (g_Config.m_SvBotsEnabled) {
		m_pGameType = (IsInstagib()) ? "iDM+|b" : "DM+|b";
	} else {
		m_pGameType = (IsInstagib()) ? "iDM+" : "DM+";
	}
}

void CGameControllerDM::Tick()
{
	IGameController::Tick();
}
