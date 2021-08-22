#include "bitflags.h"
#include "generic.h"
#include <igameevents.h>
#include <ISDKTools.h>
#include <smsdk_ext.h>

using namespace SourceMod;

void OnPluginMsgsHelpModeChanged(IConVar*, const char*, float);

SAFE_ENUM(EPluginMessagesHelpMode,
	Disabled,
	ConnectOnly,
	Full
)

SAFE_ENUM(EPlayerPluginMessagesFlag,
	PostChecking,  // Checking plugin messages other than per connect
	EnabledSetting // Whether client had the setting enabled at least once. Used to disable checking from that time.
)

struct SPlayerData
{
	CBitFlags<> mFlags;
	ITimer* mpConnectCheckTimer;
	QueryCvarCookie_t mCurrentQueryCookie = InvalidQueryCvarCookie;
};

class CHL2MPExtension : public SDKExtension, IConCommandBaseAccessor, IClientListener, IGameEventListener2, ITimedEvent
{
	bool RegisterConCommandBase(ConCommandBase*) override;
	bool SDK_OnMetamodLoad(ISmmAPI*, char*, size_t, bool) override;
	void SDK_OnAllLoaded();
	void SDK_OnUnload() override;
	void OnClientConnected(int) override;
	void OnClientDisconnected(int) override;
	void OnClientPostAdminCheck(int) override;
	ResultType OnTimer(ITimer*, void*) override;
	void OnTimerEnd(ITimer*, void*) override {};
	void FireGameEvent(IGameEvent*) override;
};

static CHL2MPExtension sHL2MPExtension;
SMEXT_LINK(&sHL2MPExtension);

static ISmmAPI* spMetamod;
static IPhraseCollection* spPhrases;
static IServerPluginHelpers* spPluginHelpers;
static IPluginHelpersCheck* spPluginHelpersCheck;
static IGameEventManager2* spEventManager;
static ISDKTools* spSDKTools;
static int sMessageCreateHookId, sCVarQueryFinishedHookId;
static SPlayerData sPlayersData[SM_MAXPLAYERS + 1];
static ConVar sPluginMsgsHelpModeCVar("sm_pluginmessages_help_mode", "2", 0, "Activation mode of plugin messages"
	" setting help. If 0, the feature will be completely disabled and clients won't get any help to enable the setting."
	" If 1, the help will only happen on connect. If 2, the help will happen on connect and after sending dialogs.",
	OnPluginMsgsHelpModeChanged);
static ConVar sPluginMsgsJoinDelayCVar("sm_pluginmessages_join_help_delay", "6.0", 0,
	"When 'sm_pluginmessages_help_mode' is enabled (1 or 2), this defines the delay in seconds"
	" after which the plugin messages setting will be checked to help clients upon connect", true, 0.0f, true, 10.0f);
static ConVar sPluginMsgsChatUseCVar("sm_pluginmessages_use_chat", "0", 0, "Whether to display plugin messages"
	" activation hints to chat (1) or to console (0) under 'sm_pluginmessages_help_mode 2'. Leaving it disabled"
	" to use console (0) is useful to prevent the hints from resulting in potential spam to clients, otherwise.");

SH_DECL_HOOK4(IPluginHelpersCheck, CreateMessage, SH_NOATTRIB, 0, bool, const char*, edict_t*, DIALOG_TYPE, KeyValues*);
SH_DECL_HOOK5_void(IServerPluginCallbacks, OnQueryCvarValueFinished, SH_NOATTRIB, 0,
	QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char*);

static void PrintPluginMessagesHint(int client, const char* pCVarName, const char* pToken, bool chat)
{
	smutils->SetGlobalTarget(client);
	char message[MAX_USER_MSG_DATA];
	const void* args[] = { pToken, "PluginMessages_ConsoleHint", pCVarName };

	if (spPhrases->FormatString(message, sizeof(message), "\x01[SM] %t. %t: \x04%s 1",
		(void**)args, ARRAYSIZE(args), NULL, NULL))
	{
		if (chat)
		{
			int userMsg = usermsgs->GetMessageIndex("SayText2"), clients[] = { client };

			if (userMsg > -1)
			{
				bf_write* pData = usermsgs->StartBitBufMessage(userMsg, clients,
					1, USERMSG_RELIABLE | USERMSG_BLOCKHOOKS);

				if (pData != NULL)
				{
					pData->WriteByte(0); // From world
					pData->WriteByte(true); // Show on console as well
					pData->WriteString(message);
					usermsgs->EndMessage();
					return;
				}
			}
		}

		gamehelpers->TextMsg(client, TEXTMSG_DEST_CONSOLE, message);
	}
}

static void QueryPluginMessagesValue(int client)
{
	sPlayersData[client].mCurrentQueryCookie = spPluginHelpers
		->StartQueryCvarValue(gamehelpers->EdictOfIndex(client), "cl_showpluginmessages");
}

static bool OnMessageCreate(const char*, edict_t* pPlayer, DIALOG_TYPE, KeyValues*)
{
	int client = gamehelpers->IndexOfEdict(pPlayer);

	if (!sPlayersData[client].mFlags.IsAnyBitSet(EPlayerPluginMessagesFlag::PostChecking,
		EPlayerPluginMessagesFlag::EnabledSetting))
	{
		sPlayersData[client].mFlags.SetBit(EPlayerPluginMessagesFlag::PostChecking);
		QueryPluginMessagesValue(client);
	}

	return true;
}

static void OnCVarValueQueryFinished(QueryCvarCookie_t cookie, edict_t* pPlayer,
	EQueryCvarValueStatus status, const char* pName, const char* pValue)
{
	int client = gamehelpers->IndexOfEdict(pPlayer);

	if (sPlayersData[client].mCurrentQueryCookie == cookie)
	{
		sPlayersData[client].mCurrentQueryCookie = InvalidQueryCvarCookie;
		CBitFlags<>& playerFlags = sPlayersData[client].mFlags;
		playerFlags.ClearBit(EPlayerPluginMessagesFlag::PostChecking);

		if (status == eQueryCvarValueStatus_ValueIntact)
		{
			if (Q_atoi(pValue) != 0)
			{
				return playerFlags.SetBit(EPlayerPluginMessagesFlag::EnabledSetting);
			}
			else if (playerFlags.IsBitSet(EPlayerPluginMessagesFlag::EnabledSetting)) // Are we handling connect check?
			{
				playerFlags.ClearBit(EPlayerPluginMessagesFlag::EnabledSetting);
				return PrintPluginMessagesHint(client, pName, "PluginMessagesOff_Connect", true);
			}

			PrintPluginMessagesHint(client, pName, "PluginMessageFailed", sPluginMsgsChatUseCVar.GetBool());
		}
	}
}

static void HookMessageCreate()
{
	sMessageCreateHookId = SH_ADD_HOOK_STATICFUNC(IPluginHelpersCheck,
		CreateMessage, spPluginHelpersCheck, OnMessageCreate, true);
}

static void HookCVarValueQueryFinished()
{
	sCVarQueryFinishedHookId = SH_ADD_HOOK_STATICFUNC(IServerPluginCallbacks, OnQueryCvarValueFinished,
		spMetamod->GetVSPInfo(NULL), OnCVarValueQueryFinished, true);
}

void OnPluginMsgsHelpModeChanged(IConVar*, const char*, float oldValue)
{
	if (oldValue == EPluginMessagesHelpMode::Disabled)
	{
		HookCVarValueQueryFinished();
	}
	else if (sPluginMsgsHelpModeCVar.GetInt() == EPluginMessagesHelpMode::Disabled)
	{
		SH_REMOVE_HOOK_ID(sCVarQueryFinishedHookId);
	}

	if (sPluginMsgsHelpModeCVar.GetInt() == EPluginMessagesHelpMode::Full)
	{
		HookMessageCreate();
	}
	else if (oldValue == EPluginMessagesHelpMode::Full)
	{
		SH_REMOVE_HOOK_ID(sMessageCreateHookId);
	}
}

bool CHL2MPExtension::RegisterConCommandBase(ConCommandBase* pCommandBase)
{
	return META_REGCVAR(pCommandBase);
}

bool CHL2MPExtension::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool)
{
	spMetamod = ismm;
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, spPluginHelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetServerFactory, spPluginHelpersCheck, IPluginHelpersCheck,
		INTERFACEVERSION_PLUGINHELPERSCHECK);
	GET_V_IFACE_CURRENT(GetEngineFactory, spEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	return true;
}

void CHL2MPExtension::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, spSDKTools);
	ConVar_Register(0, this);
	OnPluginMsgsHelpModeChanged(&sPluginMsgsJoinDelayCVar, "", EPluginMessagesHelpMode::Disabled);
	spPhrases = translator->CreatePhraseCollection();
	spPhrases->AddPhraseFile("hl2dm.ext.phrases");
	playerhelpers->AddClientListener(this);
	spEventManager->AddListener(this, "round_start", true);

	// Late load clients
	for (int i = 1; i <= playerhelpers->GetMaxClients(); ++i)
	{
		OnClientConnected(i);
		IGamePlayer* pPlayer = playerhelpers->GetGamePlayer(i);

		if (pPlayer->IsAuthorized() && pPlayer->IsInGame())
		{
			OnClientPostAdminCheck(i);
		}
	}
}

void CHL2MPExtension::SDK_OnUnload()
{
	SH_REMOVE_HOOK_ID(sMessageCreateHookId);
	SH_REMOVE_HOOK_ID(sCVarQueryFinishedHookId);
	ConVar_Unregister();
	spPhrases->Destroy();
	playerhelpers->RemoveClientListener(this);
	spEventManager->RemoveListener(this);

	for (int i = 1; i <= playerhelpers->GetMaxClients(); ++i)
	{
		OnClientDisconnected(i);
	}
}

void CHL2MPExtension::OnClientConnected(int client)
{
	// Assume messages are on, which simplifies correct logic (ensures no queries happen until connect check)
	sPlayersData[client].mFlags.SetBit(EPlayerPluginMessagesFlag::EnabledSetting);
}

void CHL2MPExtension::OnClientDisconnected(int client)
{
	if (sPlayersData[client].mpConnectCheckTimer != NULL)
	{
		timersys->KillTimer(sPlayersData[client].mpConnectCheckTimer);
	}

	sPlayersData[client] = {};
	sPlayersData[client].mCurrentQueryCookie = InvalidQueryCvarCookie;
}

void CHL2MPExtension::OnClientPostAdminCheck(int client)
{
	if (sPluginMsgsHelpModeCVar.GetInt() > EPluginMessagesHelpMode::Disabled)
	{
		sPlayersData[client].mpConnectCheckTimer = timersys->CreateTimer(this, sPluginMsgsJoinDelayCVar.GetFloat(),
			(void*)playerhelpers->GetGamePlayer(client)->GetUserId(), TIMER_FLAG_NO_MAPCHANGE);
	}
}

ResultType CHL2MPExtension::OnTimer(ITimer*, void* userId)
{
	int client = playerhelpers->GetClientOfUserId((int)userId);

	if (client > 0)
	{
		sPlayersData[client].mpConnectCheckTimer = NULL;
		QueryPluginMessagesValue(client);
	}

	return ResultType::Pl_Handled;
}

void CHL2MPExtension::FireGameEvent(IGameEvent*)
{
	// Fix up unsynced SM cached timeleft upon 'round_start' events
	float gameStartTime = spMetamod->GetCGlobals()->curtime;

	if (spSDKTools != NULL && spSDKTools->GetGameRules() != NULL)
	{
		sm_sendprop_info_t* pGameStartTimeInfo = NULL;

		if (gamehelpers->FindSendPropInfo("CHL2MPGameRulesProxy", "m_flGameStartTime", pGameStartTimeInfo))
		{
			gameStartTime = *(float*)((uint)spSDKTools->GetGameRules() + pGameStartTimeInfo->actual_offset);
		}
	}

	timersys->NotifyOfGameStart();
	timersys->MapTimeLeftChanged();
}
