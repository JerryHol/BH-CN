#pragma once
#include "../../D2Structs.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"
#include "../../Task.h"

class ChatColor : public Module {
private:
	bool inGame;
	bool isDamageShow;
	bool needOpenSkillbar;
	std::map<string, unsigned int> whisperColors;
public:
	static std::map<std::string, Toggle> Toggles;
	static DWORD dwPlayerId;  //当前角色
	static D2EditBox* pD2WinEditBox;
	static StatMonitor sMonitorStr[200];
	ChatColor() : Module("Chat Color") {};

	void Init();

	void OnLoad();
	void OnUnload();
	void LoadConfig();
	void OnGameJoin();
	void OnGameExit();
	void OnDraw();
	void OnChatPacketRecv(BYTE* packet, bool *block);

	void OnLoop();
	void OnEnd();
};

static double mercLastHP=100;  //佣兵上1次的血量百分比
static long mercLastTime = 0;  //佣兵上1次喝药的时间

#define		KEY_CODE_COPY			0x03
#define		KEY_CODE_PASTE			0x16
#define		KEY_CODE_CUT			0x18

void DrawMonitorInfo();
int MyMultiByteToWideChar(
	UINT CodePage,         // code page
	DWORD dwFlags,         // character-type options
	LPCSTR lpMultiByteStr, // string to map
	int cbMultiByte,       // number of bytes in string
	LPWSTR lpWideCharStr,  // wide-character buffer
	int cchWideChar        // size of buffer
);

void ToggleIMEInput(BOOL fEnable);

void CheckD2WinEditBox();
void InputLinePatch1_ASM();
void InputLinePatch2_ASM();

LPCSTR __fastcall D2Lang_Unicode2WinPatch(LPSTR lpWinStr, LPWSTR lpUnicodeStr, DWORD dwBufSize);
LPWSTR __fastcall D2Lang_Win2UnicodePatch(LPWSTR lpUnicodeStr, LPCSTR lpWinStr, DWORD dwBufSize);

DWORD __fastcall ChannelEnterCharPatch(D2EditBox* hWnd, BYTE bKeyCode);
void MultiByteFixPatch_ASM();

void RecvCommand_A7_Patch_ASM();
void RecvCommand_A8_Patch_ASM();
void RecvCommand_A9_Patch_ASM();

void DrawDefaultFontText(wchar_t* wStr, int xpos, int ypos, DWORD dwColor, int div = 1, DWORD dwAlign = 0);
void ShowLifePatch_ASM();
void ShowManaPatch_ASM();

void DrawPetHeadPath_ASM();
void DrawPartyHeadPath_ASM();

void StrcatDefenseStringPatch_ASM();