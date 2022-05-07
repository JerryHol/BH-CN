﻿#define _DEFINE_PTRS
#include "BH.h"
#include <Shlwapi.h>
#include <psapi.h>
#include "D2Ptrs.h"
#include "D2Intercepts.h"
#include "D2Handlers.h"
#include "Modules.h"
#include "MPQReader.h"
#include "MPQInit.h"
#include "TableReader.h"
#include "Task.h"

string BH::path;
HINSTANCE BH::instance;
ModuleManager* BH::moduleManager;
Config* BH::config;
Config* BH::lootFilter;
Drawing::UI* BH::settingsUI;
Drawing::StatsDisplay* BH::statsDisplay;
bool BH::initialized;
bool BH::cGuardLoaded;
WNDPROC BH::OldWNDPROC;
//map<string, Toggle>* BH::MiscToggles;
map<string, Toggle>* BH::MiscToggles2;
map<string, bool>* BH::BnetBools;
map<string, bool>* BH::GamefilterBools;

Patch* patches[] = {
	new Patch(Call, D2CLIENT, { 0x44230, 0x45280 }, (int)GameLoop_Interception, 7),
	new Patch(Call, D2CLIENT, { 0x45153, 0x461A3 }, (int)GameEnd_Interception, 5),
	//{PatchCALL,   DLLOFFSET2(D2CLIENT, 0x6FAF5153, 0x6FAF61A3),    (DWORD)GameEndPatch_ASM,                5 ,   &fDefault},

	new Patch(Jump, D2CLIENT, { 0xC3DB4, 0x1D7B4 }, (int)GameDraw_Interception, 6),
	new Patch(Jump, D2CLIENT, { 0x626C9, 0x73469 }, (int)GameAutomapDraw_Interception, 5),

	new Patch(Call, BNCLIENT, { 0xEAB6, 0xCEB6 }, (int)ChatPacketRecv_Interception, 0xEABE - 0xEAB6),
	new Patch(Call, D2MCPCLIENT, { 0x69D7, 0x6297 }, (int)RealmPacketRecv_Interception, 5),
	new Patch(Call, D2CLIENT, { 0xACE61, 0x83301 }, (int)GamePacketRecv_Interception, 5),
	new Patch(Call, D2CLIENT, { 0x70B75, 0xB24FF }, (int)GameInput_Interception, 5),
	new Patch(Call, D2MULTI, { 0xD753, 0x11D63 }, (int)ChannelInput_Interception, 5),
	new Patch(Call, D2MULTI, { 0x10781, 0x14A9A }, (int)ChannelWhisper_Interception, 5),
	new Patch(Jump, D2MULTI, { 0x108A0, 0x14BE0 }, (int)ChannelChat_Interception, 6),
	new Patch(Jump, D2MULTI, { 0x107A0, 0x14850 }, (int)ChannelEmote_Interception, 6),
};

Patch* BH::oogDraw = new Patch(Call, D2WIN, { 0x18911, 0xEC61 }, (int)OOGDraw_Interception, 5);

unsigned int index = 0;
bool BH::Startup(HINSTANCE instance, VOID* reserved) {

	BH::instance = instance;
	if (reserved != NULL) {
		cGuardModule* pModule = (cGuardModule*)reserved;
		if (!pModule)
			return FALSE;
		path.assign(pModule->szPath);
		cGuardLoaded = true;
	}
	else {
		char szPath[MAX_PATH];
		GetModuleFileName(BH::instance, szPath, MAX_PATH);
		PathRemoveFileSpec(szPath);
		path.assign(szPath);
		path += "\\";
		cGuardLoaded = false;
	}


	initialized = false;
	Initialize();
	return true;
}

DWORD WINAPI LoadMPQData(VOID* lpvoid) {
	char szFileName[1024];
	std::string patchPath;
	UINT ret = GetModuleFileName(NULL, szFileName, 1024);
	patchPath.assign(szFileName);
	size_t start_pos = patchPath.rfind("\\");
	if (start_pos != std::string::npos) {
		start_pos++;
		if (start_pos < patchPath.size()) {
			patchPath.replace(start_pos, patchPath.size() - start_pos, "Patch_D2.mpq");
		}
	}

	ReadMPQFiles(patchPath);
	InitializeMPQData();
	Tables::initTables();

	return 0;
}

void BH::Initialize()
{
	moduleManager = new ModuleManager();
	config = new Config("ProjectDiablo.cfg");
	if (!config->Parse()) {
		config->SetConfigName("ProjectDiablo_Default.cfg");
		if (!config->Parse()) {
			string msg = "Could not find ProjectDiablo config.\nAttempted to load " +
				path + "ProjectDiablo.cfg (failed).\nAttempted to load " +
				path + "ProjectDiablo_Default.cfg (failed).\n\nDefaults loaded.";
			MessageBox(NULL, msg.c_str(), "Failed to load ProjectDiablo config", MB_OK);
		}
	}

	lootFilter = new Config("loot.filter");
	if (!lootFilter->Parse()) {
		lootFilter->SetConfigName("default.filter");
		if (!lootFilter->Parse()) {
			string msg = "Could not find default loot filter.\nAttempted to load " +
				path + "loot.filter (failed).\nAttempted to load " +
				path + "default.filter (failed).\n\nDefaults loaded.";
			MessageBox(NULL, msg.c_str(), "Failed to load ProjectDiablo lootFilter", MB_OK);
		}
	}


	// Do this asynchronously because D2GFX_GetHwnd() will be null if
	// we inject on process start
	Task::Enqueue([]() -> void {
		std::chrono::milliseconds duration(200);
		while (!D2GFX_GetHwnd()) {
			std::this_thread::sleep_for(duration);
		}
		BH::OldWNDPROC = (WNDPROC)GetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC);
		SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)GameWindowEvent);
		});

	settingsUI = new Drawing::UI(SETTINGS_TEXT, 400, 311);

	Task::InitializeThreadPool(2);

	// Read the MPQ Data asynchronously
	//CreateThread(0, 0, LoadMPQData, 0, 0, 0);
	Task::Enqueue([]() -> void {
		LoadMPQData(NULL);
		moduleManager->MpqLoaded();
		});


	new ScreenInfo();
	new Gamefilter();
	new Bnet();
	new Item();
	new Party();
	new ItemMover();
	//new StashExport();   //这个先不要吧
	new MapNotify();   //这个就是MapHack修改而来
	new ChatColor();

	BnetBools = ((Bnet*)moduleManager->Get("bnet"))->GetBools();
	GamefilterBools = ((Gamefilter*)moduleManager->Get("gamefilter"))->GetBools();

	moduleManager->LoadModules();

	statsDisplay = new Drawing::StatsDisplay("Stats");

	MiscToggles2 = ((Item*)moduleManager->Get("item"))->GetToggles();

	// Injection would occasionally deadlock (I only ever saw it when using Tabbed Diablo
	// but theoretically it could happen during regular injection):
	// installation until after all startup initialization is done.
	for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++) {
		patches[n]->Install();
	}

	if (!D2CLIENT_GetPlayerUnit())
		oogDraw->Install();

	// GameThread can potentially run oogDraw->Install, so create the thread after all
	// loading/installation finishes.
	//CreateThread(0, 0, GameThread, 0, 0, 0);  //这里去掉，参考HM的onJoinGame和ExitGame

	initialized = true;
}

bool BH::Shutdown() {
	if (initialized) {
		moduleManager->UnloadModules();

		delete moduleManager;
		delete settingsUI;
		delete statsDisplay;

		SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)BH::OldWNDPROC);
		for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++) {
			delete patches[n];
		}

		oogDraw->Remove();
		delete config;
	}

	return true;
}

bool BH::ReloadConfig() {
	if (initialized) {
		if (D2CLIENT_GetPlayerUnit()) {
			PrintText(0, "Reloading config: %s", config->GetConfigName().c_str());
			PrintText(0, "Reloading filter: %s", lootFilter->GetConfigName().c_str());
		}
		config->Parse();
		lootFilter->Parse();
		moduleManager->ReloadConfig();
		statsDisplay->LoadConfig();
	}
	return true;
}
