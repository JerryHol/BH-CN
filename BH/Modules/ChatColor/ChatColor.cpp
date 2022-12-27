#include "ChatColor.h"
#include "../ScreenInfo/ScreenInfo.h"
#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../D2Stubs.h"
#include "../../D2Intercepts.h"
#include "../../D2Helpers.h"
#include "../ItemMover/ItemMover.h"
#include "../Item/Item.h"
#include "../GameSettings/GameSettings.h"


#define INVENTORY_WIDTH  p_D2CLIENT_InventoryLayout->SlotWidth
#define INVENTORY_HEIGHT p_D2CLIENT_InventoryLayout->SlotHeight
#define INVENTORY_LEFT   p_D2CLIENT_InventoryLayout->Left
#define INVENTORY_RIGHT  p_D2CLIENT_InventoryLayout->Right
#define INVENTORY_TOP    p_D2CLIENT_InventoryLayout->Top
#define INVENTORY_BOTTOM p_D2CLIENT_InventoryLayout->Bottom

//这里先不管非资料片的stash
#define STASH_WIDTH  p_D2CLIENT_StashLayout->SlotWidth
#define STASH_HEIGHT p_D2CLIENT_StashLayout->SlotHeight
#define STASH_LEFT   p_D2CLIENT_StashLayout->Left
#define STASH_RIGHT  p_D2CLIENT_StashLayout->Right
#define STASH_TOP    p_D2CLIENT_StashLayout->Top
#define STASH_BOTTOM p_D2CLIENT_StashLayout->Bottom

#define CUBE_WIDTH  p_D2CLIENT_CubeLayout->SlotWidth
#define CUBE_HEIGHT p_D2CLIENT_CubeLayout->SlotHeight
#define CUBE_LEFT   p_D2CLIENT_CubeLayout->Left
#define CUBE_RIGHT  p_D2CLIENT_CubeLayout->Right
#define CUBE_TOP    p_D2CLIENT_CubeLayout->Top
#define CUBE_BOTTOM p_D2CLIENT_CubeLayout->Bottom

#define CELL_SIZE p_D2CLIENT_InventoryLayout->SlotPixelHeight

using namespace Drawing;  //这个是Hook的namespace
 
D2EditBox* ChatColor::pD2WinEditBox;
DWORD ChatColor::dwPlayerId=0;
StatMonitor ChatColor::sMonitorStr[200] = {L'\0'};
std::map<std::string, Toggle> ChatColor::Toggles;

//尝试做中文输入的补丁
Patch* InputLine1 = new Patch(Call, D2CLIENT, { 0x70C6C, 0xB254C }, (DWORD)InputLinePatch1_ASM, 5);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FB20C6C, 0x6FB6254C), (DWORD)InputLinePatch1_ASM, 5, & fDefault},
Patch* InputLine2 = new Patch(Call, D2CLIENT, { 0x7037B, 0xB286B }, (DWORD)InputLinePatch2_ASM, 5);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT, 0x6FB2037B, 0x6FB6186B),    (DWORD)InputLinePatch2_ASM,             5 ,   &fDefault },
Patch* UnicodeSupport1 = new Patch(Jump, D2LANG, { 0x82F0, 0x82B0 }, (DWORD)D2Lang_Unicode2WinPatch, 5);
//{PatchJMP, DLLOFFSET2(D2LANG, 0x6FC082F0, 0x6FC082B0), (DWORD)D2Lang_Unicode2WinPatch, 5, & fLocalizationSupport},
Patch* UnicodeSupport2 = new Patch(Jump, D2LANG, { 0x8320, 0x82E0 }, (DWORD)D2Lang_Win2UnicodePatch, 5);
//{PatchJMP,    DLLOFFSET2(D2LANG, 0x6FC08320, 0x6FC082E0),      (DWORD)D2Lang_Win2UnicodePatch,          5 ,   &fLocalizationSupport},
Patch* UnicodeSupport3 = new Patch(Call, D2WIN, { 0x183A0, 0xE850 }, (DWORD)ChannelEnterCharPatch, 5);
//{PatchCALL, DLLOFFSET2(D2WIN, 0x6F8F83A0, 0x6F8EE850), (DWORD)ChannelEnterCharPatch, 5, & fLocalizationSupport}, //注意InputLinePatch1_ASM必须结合这个，不然会引起部分堆栈错误(虽然不影响使用)
Patch* UnicodeSupport4 = new Patch(Call, BNCLIENT, { 0xFF5C, 0x1513C }, (DWORD)MultiByteFixPatch_ASM, 6);
//{ PatchCALL,   DLLOFFSET2(BNCLIENT,0x6FF2FF5C, 0x6FF3513C),     (DWORD)MultiByteFixPatch_ASM,            6 ,   &fLocalizationSupport }, // for /w *acc msg text
//尝试做光环状态，以后再分开来吧。先都写在ChatColor.cpp里面
Patch* Monitor1 = new Patch(Call, D2CLIENT, { 0xADEB1, 0xB254C }, (DWORD)RecvCommand_A7_Patch_ASM, 9);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FB5DEB1, 0x6FB34391), (DWORD)RecvCommand_A7_Patch_ASM, 9, & fDefault},//设置状态1
Patch* Monitor2 = new Patch(Call, D2CLIENT, { 0xADD74, 0xB254C }, (DWORD)RecvCommand_A8_Patch_ASM, 9);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT , 0x6FB5DD74, 0x6FB34254),   (DWORD)RecvCommand_A8_Patch_ASM,        9 ,   &fDefault },//设置状态2
Patch* Monitor3 = new Patch(Call, D2CLIENT, { 0xADD31, 0xB254C }, (DWORD)RecvCommand_A9_Patch_ASM, 9);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT , 0x6FB5DD31, 0x6FB34211),   (DWORD)RecvCommand_A9_Patch_ASM,        9 ,   &fDefault },//结束状态
//显血显蓝
Patch* showLifeMana1 = new Patch(Call, D2CLIENT, { 0x276B5, 0xB254C }, (DWORD)ShowLifePatch_ASM, 5);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FAD76B5, 0x6FB1D765), (DWORD)ShowLifePatch_ASM, 5, & fDefault},
Patch* showLifeMana2 = new Patch(Call, D2CLIENT, { 0x27767, 0xB254C }, (DWORD)ShowManaPatch_ASM, 5);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT, 0x6FAD7767, 0x6FB1D817),    (DWORD)ShowManaPatch_ASM,           5 ,   &fDefault },
//佣兵/好友头像等级显示，招唤物个数显示
Patch* petHead = new Patch(Call, D2CLIENT, { 0x5B582, 0xB254C }, (DWORD)DrawPetHeadPath_ASM, 7);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FB0B582, 0x6FB39662), (DWORD)DrawPetHeadPath_ASM, 7, & fDefault},
Patch* partyHead = new Patch(Call, D2CLIENT, { 0x5BBE0, 0xB254C }, (DWORD)DrawPartyHeadPath_ASM, 6);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT, 0x6FB0BBE0, 0x6FB39F90),    (DWORD)DrawPartyHeadPath_ASM,          6 ,   &fDefault },
// 从HM移植防具基础值
Patch* DBase = new Patch(Call, D2CLIENT, { 0x8FDAD, 0x8FDAD }, (DWORD)StrcatDefenseStringPatch_ASM, 6);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FB3FDAD, 0x6FB422FD), (DWORD)StrcatDefenseStringPatch_ASM, 6, & fDefault},//拼防御值字符串

void ChatColor::Init() {
	BH::inGameOnce = false;
}

void Print(DWORD color, char* format, ...) {
	va_list vaArgs;
	va_start(vaArgs, format);
	int len = _vscprintf(format, vaArgs) + 1;
	char* str = new char[len];
	vsprintf_s(str, len, format, vaArgs);
	va_end(vaArgs);

	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CODE_PAGE, 0, str, -1, wstr, len);

	//by zyl
	for (DWORD i = 0; i < wcslen(wstr); i++)
	{
		if ((wstr[i] >= 0xFF || wstr[i] == 0x79) && wstr[i + 1] == L'c')
		{
			//if (name[i + 2] >= L'0' && name[i + 2] <= L':')
			//{
			wstr[i] = L'\377';
			//}
		};
	}

	D2CLIENT_PrintGameString(wstr, color);
	delete[] wstr;

	delete[] str;
}

DWORD gameTimer;
DWORD startExperience;
int startLevel;
unsigned int MercProtectSec;  //佣兵保护间隔

void ChatColor::OnGameJoin() {
	needOpenSkillbar = false;
	isDamageShow = true;
	inGame = true;
	gameTimer = GetTickCount();
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	startExperience = (int)D2COMMON_GetUnitStat(pUnit, STAT_EXP, 0);
	startLevel = (int)D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0);


	
}

void ChatColor::OnGameExit() {
	inGame = false;
}

void ChatColor::OnLoad() {
	InputLine1->Install();
	InputLine2->Install();
	UnicodeSupport1->Install();
	UnicodeSupport2->Install();
	UnicodeSupport3->Install();
	UnicodeSupport4->Install();
	Monitor1->Install();
	Monitor2->Install();
	Monitor3->Install();
	showLifeMana1->Install();
	showLifeMana2->Install();
	petHead->Install();
	partyHead->Install();
	DBase->Install();
	LoadConfig();
	//状态写在这里吧
	DWORD statNo[] = {   //状态id，在states.txt里面，相关的是skills.txt和skilldesc.txt
		1,2,9,11,19,21,28,55,58,60,
		62,95,61,10,16,20,26,30,31,32,
		38,51,88,94,101,117,120,128,129,130,
		131,132,133,134,135,136,137,139,140,144,
		145,153,157,158,159,177,8,14,33,34,
		37,40,41,42,45,48,49,148,149,151,
		161,162,3,188
	};
	DWORD color[] = {
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3,3,3,
		3,3,3,3
	};
	LPCSTR desc[] = {
		"冻结","中毒","伤害加深","冰减速","虚弱","眩晕","审判光环","攻击反噬","生命偷取","衰老",
		"撕裂伤口","狂战士","降低抵抗","冰封装甲","火焰强化","寒冰装甲","大叫","能量盾","毒牙","战斗体制",
		"雷云风暴","战斗指挥","碎冰甲","狂乱","神圣之盾","撞锤","野性狂暴","装甲神殿","战斗神殿","闪电神殿",
		"火焰神殿","冰冷神殿","毒素神殿","技能神殿","法力神殿","耐力神殿","经验神殿","狼人形态","熊人形态","暴风",
		"毁天灭地","魔影斗篷","速度爆发","刀刃之盾","能量消解","变身娃娃","救助","白骨装甲","力量光环","祈祷",
		"反抗光环","祝福瞄准","活力","专注光环","淨化","冥思","狂热光环","狼獾之心","橡木智者","飓风装甲",
		"橡木智者","狼獾之心","抵抗火焰","冰冷强化"
	};

	int statNoSize = sizeof(statNo) / sizeof(DWORD);  //动态数组大小

	for (DWORD n = 0; n < statNoSize; n++) {
		ChatColor::sMonitorStr[n].dwStatNo = statNo[n];
		ChatColor::sMonitorStr[n].dwColor = color[n];
		//ChatColor::sMonitorStr[0].fCountDown = TRUE;  //倒计时,这个暂时不行，持续时间的参数还不知道哪里
		MyMultiByteToWideChar(CP_ACP, 0, desc[n], -1, sMonitorStr[n].wszDesc[0], 30);
	}
	//永久显示血蓝,自定义
	*p_D2CLIENT_ShowLifeStr = TRUE;
	*p_D2CLIENT_ShowManaStr = TRUE;
	gameTimer = GetTickCount();
}

void ChatColor::OnUnload()
{
	InputLine1->Remove();
	InputLine2->Remove();
	UnicodeSupport1->Remove();
	UnicodeSupport2->Remove();
	UnicodeSupport3->Remove();
	UnicodeSupport4->Remove();
	Monitor1->Remove();
	Monitor2->Remove();
	Monitor3->Remove();
	showLifeMana1->Remove();
	showLifeMana2->Remove();
	petHead->Remove();
	partyHead->Remove();
	DBase->Remove();
}

void ChatColor::LoadConfig() {
	whisperColors.clear();

	BH::config->ReadAssoc("Whisper Color", whisperColors);

	BH::config->ReadToggle("Merc Protect", "None", true, Toggles["Merc Protect"]);  //佣兵保护
	BH::config->ReadToggle("Merc Boring", "None", true, Toggles["Merc Boring"]);  //佣兵吐槽
	BH::config->ReadToggle("Rune Number", "None", true, Toggles["Rune Number"]);  //符文数字
	BH::config->ReadToggle("Show Money", "None", false, Toggles["Show Money"]);  //贪婪模式,默认关闭
	BH::config->ReadToggle("Death Back", "None", false, Toggles["Death Back"]);  //死亡立即回城,默认关闭
	BH::config->ReadToggle("Damage Show", "None", false, Toggles["Damage Show"]);  //伤害显示开关，默认关闭吧，界面是写在ItemMover.cpp里面569行

	BH::config->ReadInt("Merc Protect Sec", MercProtectSec, 3);   //默认3秒
}

void ChatColor::OnChatPacketRecv(BYTE* packet, bool* block) {
	if (packet[1] == 0x0F && inGame) {
		unsigned int event_id = *(unsigned short int*)&packet[4];

		if (event_id == 4) {
			const char* from = (const char*)&packet[28];
			unsigned int fromLen = strlen(from);

			const char* message = (const char*)&packet[28 + fromLen + 1];
			unsigned int messageLen = strlen(message);

			bool replace = false;
			int color = 0;
			if (whisperColors.find(from) != whisperColors.end()) {
				replace = true;
				color = whisperColors[from];
			}

			if (replace) {
				*block = true;

				Print(color, "%s | %s", from, message);
			}
		}
	}
}


void ChatColor::OnDraw()
{
	DrawMonitorInfo();
}


void ChatColor::OnLoop()
{
	if (pD2WinEditBox && !D2CLIENT_GetUIState(UI_CHAT_CONSOLE)) {
		/**(DWORD*)p_D2CLIENT_LastChatMsg = 0;
		wchar_t* p = wcscpy(p_D2CLIENT_LastChatMsg, D2WIN_GetEditBoxTextHM(pD2WinEditBox));
		*p_D2CLIENT_ChatTextLength = wcslen(p);*/
		D2WIN_DestroyEditBoxHM(pD2WinEditBox);
		pD2WinEditBox = NULL;
		*p_D2WIN_FocusedControl = NULL;
	}
	CheckD2WinEditBox();

	if (BH::inGameOnce == false) {
		BH::inGameOnce = true;
		//*p_D2CLIENT_AutomapOn = TRUE;  //自动开启地图
		D2CLIENT_ShowMap();  //自动开启地图,这个才是真正的自动开启地图
		(*p_D2CLIENT_AutomapPos).x += 32;  //地图中心点偏移...
		//(*p_D2CLIENT_AutomapPos).y += 0;
		dwPlayerId = D2CLIENT_GetPlayerUnit()->dwUnitId;

	}

	//这里加一个伤害显示开关
	if (Toggles["Damage Show"].state!= isDamageShow) {
		isDamageShow = Toggles["Damage Show"].state;
		HMODULE hModule = GetModuleHandle("MDLL1.dll");  //还要判断一下本地有没有加载多页仓库
		if (hModule) {
			//这里做一下内存修改吧
			BYTE bShowDamage = 0x00;
			if (isDamageShow) bShowDamage = 0x01;
			int patchAddr9 = Patch::GetDllOffset(MDLL1, 0x1200+0x9E24);  //数据区需要计算出一个初始地址，用OD调试出来是0x1200
			//PrintText(Red, "debug:%d", patchAddr9);
			BYTE patchBytes9[1] = { bShowDamage };
			Patch::WriteBytes(patchAddr9, 1, patchBytes9);
		}
	}
}

void ResetMonitor() {

	for (int i = 0; i < 200; i++) {

		if ((int)(ChatColor::sMonitorStr[i].dwStatNo) <= 0)break;
		ChatColor::sMonitorStr[i].fEnable = 0;

	}

}

void ChatColor::OnEnd() 
{
	BH::inGameOnce = false;
	ResetMonitor();  //重置状态
}

void CheckD2WinEditBox()
{
	if (ChatColor::pD2WinEditBox) {
		if (D2CLIENT_GetUIState(UI_CHAT_CONSOLE)) {
			if (*(DWORD*)p_D2CLIENT_LastChatMsg != 0xAD5AFFFF) {
				D2WIN_DestroyEditBoxHM(ChatColor::pD2WinEditBox);
				ChatColor::pD2WinEditBox = NULL;
				*p_D2WIN_FocusedControl = NULL;
			}
			D2WIN_SetTextFont(1);
		}
	}
}

BOOL __cdecl InputLinePatch(BYTE keycode)
{
	if (keycode != VK_ESCAPE) {
		if (keycode != VK_RETURN) return TRUE;
		if (ChatColor::pD2WinEditBox) {
			wcscpy(p_D2CLIENT_LastChatMsg, D2WIN_GetEditBoxTextHM(ChatColor::pD2WinEditBox));
		}
	}
	return FALSE;
}

void __declspec(naked) InputLinePatch1_ASM()
{
	__asm {
		mov ebx, eax
		push[edi + 8]
		call InputLinePatch
		test eax, eax
		pop eax
		jz quitcode
		add dword ptr[esp], 0x427 // add to the point where function returns
		quitcode:
		ret
	}
}


BOOL __stdcall EditBoxCallBack(D2EditBox* pEditBox, DWORD dwLength, char* pChar)
{
	return TRUE;
}


BOOL __cdecl InitD2EditBox()
{
	//if (fMyChatOn) return FALSE;
	if (!D2CLIENT_GetUIState(UI_CHAT_CONSOLE)) return FALSE;
	if (!ChatColor::pD2WinEditBox) {
		static DWORD dws[] = { 0x0D, 0 };//所用字体
		ChatColor::pD2WinEditBox = D2WIN_CreateEditBoxHM(0x83,
			Hook::GetScreenHeight() - 58, Hook::GetScreenWidth() - 266,
			0x2D,
			0, 0, 0, 0, 0,
			sizeof(dws), &dws
		);
		//FOCUSECONTROL = pD2WinEditBox;
		*p_D2WIN_FocusedControlHM = ChatColor::pD2WinEditBox;  //这个比较关键，不然获取不到输入焦点
		if (ChatColor::pD2WinEditBox) {
			D2WIN_SetEditBoxProcHM(ChatColor::pD2WinEditBox, &EditBoxCallBack);
			D2WIN_SetEditBoxTextHM(ChatColor::pD2WinEditBox, p_D2CLIENT_LastChatMsg);
		}
		*(DWORD*)p_D2CLIENT_LastChatMsg = 0xAD5AFFFF;
	}
	if (ChatColor::pD2WinEditBox)
		(ChatColor::pD2WinEditBox->fnCallBack)(ChatColor::pD2WinEditBox);
	return TRUE;
}

void __declspec(naked) InputLinePatch2_ASM()
{
	__asm {
		call InitD2EditBox
		test eax, eax
		jz quitcode
		add dword ptr[esp], 0x154
	quitcode:
		mov ecx, p_D2CLIENT_LastChatMsg
		ret
	}
}

LPCSTR __fastcall D2Lang_Unicode2WinPatch(LPSTR lpWinStr, LPWSTR lpUnicodeStr, DWORD dwBufSize)
{
	WideCharToMultiByte(CP_ACP, 0, lpUnicodeStr, -1, lpWinStr, dwBufSize, NULL, NULL);
	return lpWinStr;
}


void UnicodeFix2(LPWSTR lpText)
{
	if (lpText) {
		size_t LEN = wcslen(lpText);
		for (size_t i = 0; i < LEN; i++)
		{
			if (lpText[i] == 0xf8f5) // Unicode 'y'
				lpText[i] = 0xff; // Ansi 'y'
		}
	}
}

int MyMultiByteToWideChar(
	UINT CodePage,         // code page
	DWORD dwFlags,         // character-type options
	LPCSTR lpMultiByteStr, // string to map
	int cbMultiByte,       // number of bytes in string
	LPWSTR lpWideCharStr,  // wide-character buffer
	int cchWideChar        // size of buffer
)
{
	int r = MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
	UnicodeFix2(lpWideCharStr);
	return r;
}

LPWSTR __fastcall D2Lang_Win2UnicodePatch(LPWSTR lpUnicodeStr, LPCSTR lpWinStr, DWORD dwBufSize)
{
	MyMultiByteToWideChar(CP_ACP, 0, lpWinStr, -1, lpUnicodeStr, dwBufSize);
	return lpUnicodeStr;
}

void UnicodeFix(LPWSTR lpText, DWORD* dwPos)
{
	DWORD len, i, j;
	WORD  char1, char2;
	WORD  uchar;
	BYTE  mbchar[2];
	DWORD	dwOldPos;
	DWORD	dwNewPos;

	if (!lpText) return;
	len = wcslen(lpText);
	if (dwPos) dwOldPos = *dwPos;
	else dwOldPos = 0;
	if (len >= 2) {
		dwNewPos = -1;
		for (i = 0, j = 0; i < len; i++) {
			char1 = lpText[i];
			char2 = lpText[i + 1];
			if (i == dwOldPos) {
				dwNewPos = j;
			}
			//a non-english character
			if (!HIBYTE(char1) && (char1) & 0x80 && char1 != 0xff && LOBYTE(char2)) {
				mbchar[0] = LOBYTE(char1);
				mbchar[1] = LOBYTE(char2);
				//if (fLocalizationSupport == 3) GB2GBK((char*)mbchar);
				if (MultiByteToWideChar(CP_ACP, 0, (char*)mbchar, sizeof(mbchar), (LPWSTR)&uchar, 1)) {
					//GbToBig5(&uchar);
					lpText[j++] = uchar;
					i++;
					if (i == dwOldPos) {
						dwNewPos = j - 1;
					}
				}
				else {
					lpText[j++] = char1;
				}
			}
			else {
				lpText[j++] = char1;
			}
		}
		if (dwNewPos == (DWORD)-1) {
			dwNewPos = j;
		}
		lpText[j] = 0;
	}
	else {
		dwNewPos = dwOldPos;
	}
	if (dwPos) *dwPos = dwNewPos;
	return;
}

void EditDeleteSelect(D2EditBox* hWnd)
{
	DWORD dwStart;
	DWORD len, i, j;

	if (hWnd->dwSelectStart == (DWORD)-1) return;
	if (hWnd->dwSelectStart == hWnd->dwSelectEnd) return;
	if (hWnd->dwSelectStart > hWnd->dwSelectEnd) {
		dwStart = hWnd->dwSelectEnd;
		len = hWnd->dwSelectStart - dwStart;
	}
	else {
		dwStart = hWnd->dwSelectStart;
		len = hWnd->dwSelectEnd - dwStart;
	}
	hWnd->dwSelectStart = (DWORD)-1;
	hWnd->dwSelectEnd = 0;
	i = dwStart;
	j = dwStart + len;
	while (1) {
		if (!hWnd->wszText[j]) break;
		hWnd->wszText[i++] = hWnd->wszText[j++];
	}
	hWnd->wszText[i] = 0;
	if (hWnd->dwCursorPos > dwStart && hWnd->dwCursorPos < dwStart + len) {
		hWnd->dwCursorPos = dwStart;
	}
	else if (hWnd->dwCursorPos >= dwStart + len) {
		hWnd->dwCursorPos -= len;
	}
	return;
}

DWORD __fastcall ChannelEnterCharPatch(D2EditBox *hWnd, BYTE bKeyCode)
{
	DWORD	ret;
	LPWSTR	lpText;
	DWORD len;
	WORD  char1;
	WORD  uchar;
	BYTE	mbchar[2];
	DWORD	dwPos;
	HGLOBAL hGlobal;
	LPSTR  lpStr;
	static BYTE lastcode = 0;
	
	lpText = (LPWSTR)(hWnd->wszText);
	len = wcslen(lpText);
	if (!len) memset( hWnd->wszText, 0, sizeof(hWnd->wszText) );
	if (hWnd->dwCursorPos > len) hWnd->dwCursorPos = len;
	if (hWnd->dwCursorPos > 0) {
		char1 = lastcode;
		if (!HIBYTE(char1) && (char1) & 0x80 && char1 != 0xff) {
			mbchar[0] = LOBYTE(char1);
			mbchar[1] = bKeyCode;
			lastcode = 0;
			//if( fLocalizationSupport==3 ) GB2GBK((char *)mbchar);
			if (MultiByteToWideChar(CP_ACP, 0, (char *)mbchar, sizeof(mbchar), (LPWSTR)&uchar, 1)) {
				lpText[hWnd->dwCursorPos-1] = uchar;
				return TRUE;
			}
		}
		
	}
	if (bKeyCode == KEY_CODE_PASTE) {
		ret = D2WIN_AddEditBoxChar(hWnd, bKeyCode);
		dwPos = hWnd->dwCursorPos;
		UnicodeFix(lpText, &dwPos);
		hWnd->dwCursorPos = dwPos;
		return ret;
	} else if (bKeyCode == KEY_CODE_COPY || bKeyCode == KEY_CODE_CUT) {
		DWORD dwStart;
		DWORD n;

		if (hWnd->dwSelectStart == (DWORD)-1) return FALSE;
		if (hWnd->dwSelectStart == hWnd->dwSelectEnd) return FALSE;
		if (hWnd->dwSelectStart > hWnd->dwSelectEnd) {
			dwStart = hWnd->dwSelectEnd;
			len = hWnd->dwSelectStart - dwStart;
		} else {
			dwStart = hWnd->dwSelectStart;
			len = hWnd->dwSelectEnd - dwStart;
		}
		if (!OpenClipboard(NULL)) return FALSE;
		EmptyClipboard();
		hGlobal = GlobalAlloc(GMEM_MOVEABLE, (len+1) * 2); 
		if (hGlobal && (lpStr = (LPSTR)GlobalLock(hGlobal))) {
			n = WideCharToMultiByte(CP_ACP, 0, lpText + dwStart, len, lpStr, (len+1)*2, NULL, NULL);
			lpStr[n] = 0;
			GlobalUnlock(hGlobal);
			SetClipboardData(CF_TEXT, hGlobal);
		}
		CloseClipboard();
		if (hGlobal) GlobalFree(hGlobal);
		if (bKeyCode == KEY_CODE_CUT) EditDeleteSelect(hWnd);
		return TRUE;
	}
	if (bKeyCode >= ' ') {

		if ((bKeyCode & 0x80) && (bKeyCode != 0xff)) {
			lastcode = bKeyCode;
			ret = D2WIN_AddEditBoxChar(hWnd, 33);
		}else{
			ret = D2WIN_AddEditBoxChar(hWnd, bKeyCode);
		}
		UnicodeFix(hWnd->wszText, &hWnd->dwCursorPos);

		return ret;
	}
	ret = D2WIN_AddEditBoxChar(hWnd, bKeyCode);
	return ret;
}

void UnicodeToUTF_8(char* pOut, wchar_t* pText) {

	// 注意 WCHAR高低字的顺序,低字节在前，高字节在后   
	char* pchar = (char*)pText;
	pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
	pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
	pOut[2] = (0x80 | (pchar[0] & 0x3F));
	return;

}

void __stdcall MultiByteFix(LPSTR str)
{
	struct tagChat {
		DWORD flag0;
		DWORD flag;
		DWORD _1[5];
		char txt[100];
	};
	tagChat* pc = (tagChat*)str;
	if (InGame == FALSE && pc->flag != 4 && pc->flag != 10 && pc->flag != 6)return;
	//if (pc->flag != 4 && pc->flag != 10)return;

	char szTemp[300];
	char* pStr = pc->txt + strlen(pc->txt) + 1;
	char* ptemp = szTemp;
	char* p = pStr;

	//先还原成gb2312
	while (*p) {
		if (((*p) & 0xFC) == 0xC0) {//汉字
			*ptemp = ((p[0] & 0x03) << 6) + (p[1] & 0x3F);
			p++;
		}
		else {
			*ptemp = *p;
		}
		p++;
		ptemp++;
	}
	*ptemp = '\0';

	//重新转换utf8
	ptemp = szTemp;
	char buf[4];
	wchar_t pbuffer = L'\0';
	while (*ptemp) {
		if (*ptemp > 0) {
			*pStr++ = *ptemp++;
		}
		else {
			memset(buf, 0, 4);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptemp, 2, &pbuffer, 1);
			UnicodeToUTF_8(buf, &pbuffer);
			*pStr++ = buf[0];
			*pStr++ = buf[1];
			*pStr++ = buf[2];
			ptemp++;
			ptemp++;
		}
	}
	*pStr = '\0';
}

void __declspec(naked) MultiByteFixPatch_ASM()
{
	__asm {
		mov eax, [p_BNCLIENT_BnChatMessage];
		mov[eax], ebp;
		push ebp;
		call MultiByteFix;
		ret;
	}
}


//以下是状态监视器

void DrawMonitorInfo() {

	//if (tStateMonitorToggle.isOn == 0) return;  //默认开启
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (!pUnit) return;

	wchar_t wszTemp[512];
	memset(wszTemp, 0, sizeof(wszTemp));

	int xpos = Hook::GetScreenWidth() - 10;
	int ypos = Hook::GetScreenHeight() - 130;

	DWORD dwTimer = GetTickCount();
	DWORD dwOldFone = D2WIN_SetTextFont(8);
	for (int i = 0; i < 200; i++) {

		if ((int)(ChatColor::sMonitorStr[i].dwStatNo) <= 0)break;

		if (ChatColor::sMonitorStr[i].fEnable) {

			//SkillsTxt* test1 = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[146];
			//SkillsTxt* test2 = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[147];
			//SkillsTxt* test3 = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[155];
			//SkillsTxt* test4 = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[149];
			//SkillDescTxt* test1 =  &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[155];

			DWORD secs = (dwTimer - ChatColor::sMonitorStr[i].dwTimer) / 1000;
			if (ChatColor::sMonitorStr[i].fCountDown) {
				secs = ChatColor::sMonitorStr[i].dwTimer - secs;  //倒计时
			}
			//wsprintfW(wszTemp, L"%s: %.2d:%.2d:%.2d", ChatColor::sMonitorStr[i].wszDesc[0], secs / 3600, (secs / 60) % 60, secs % 60);
			wsprintfW(wszTemp, L"%s: %.2d:%.2d", ChatColor::sMonitorStr[i].wszDesc[0], (secs / 60) % 60, secs % 60);
			DWORD width, fileNo;
			D2WIN_GetTextWidthFileNo(wszTemp, &width, &fileNo);  //这个是BH的取字体宽的方法,跟HM有点不一样
			D2WIN_DrawText(wszTemp, xpos - width, ypos, ChatColor::sMonitorStr[i].dwColor, 0);
			ypos = ypos - 15;

		}

	}
	D2WIN_SetTextFont(dwOldFone);
}



void __stdcall SetState(DWORD dwStateNo, BOOL fSet) {

	//if (tStateMonitorToggle.isOn == 0) return;
	for (int i = 0; i < 200; i++) {
		if ((int)(ChatColor::sMonitorStr[i].dwStatNo) <= 0)break;
		if (ChatColor::sMonitorStr[i].dwStatNo == dwStateNo) {
			ChatColor::sMonitorStr[i].fEnable = fSet;
			ChatColor::sMonitorStr[i].dwTimer = GetTickCount();
			break;
		}
	}

}

void __declspec(naked) RecvCommand_A7_Patch_ASM()
{
	__asm {

		mov esi, ecx
		movzx edx, byte ptr[esi + 1]
		mov ecx, dword ptr[esi + 2]

		cmp edx, 0
		jne  org
		cmp ecx, [ChatColor::dwPlayerId]
		jne org
		movzx eax, byte ptr[esi + 06]

		push ecx
		push edx
		push esi

		push 1
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
		org :
		ret

	}

}

void __declspec(naked) RecvCommand_A8_Patch_ASM()
{
	__asm {

		mov esi, ecx
		movzx edx, byte ptr[esi + 1]
		mov ecx, dword ptr[esi + 2]

		cmp edx, 0
		jne  org
		cmp ecx, [ChatColor::dwPlayerId]
		jne org
		movzx eax, byte ptr[esi + 07]

		push ecx
		push edx
		push esi

		push 1
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
		org :
		ret

	}

}



void __declspec(naked) RecvCommand_A9_Patch_ASM()
{
	__asm {

		mov esi, ecx
		movzx edx, byte ptr[esi + 1]
		mov ecx, dword ptr[esi + 2]

		cmp edx, 0
		jne  org
		cmp ecx, [ChatColor::dwPlayerId]
		jne org
		movzx eax, byte ptr[esi + 06]

		push ecx
		push edx
		push esi

		push 0
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
		org :
		ret

	}

}

wchar_t* __cdecl wsprintfW2(wchar_t* dest, char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int len = wvsprintf((char*)dest, fmt, va);
	for (int i = len; i >= 0; i--) {
		dest[i] = ((char*)dest)[i];
	}
	return dest;
}

//long long ExpByLevel[] = {
//	0,
//	500,
//	1500,
//	3750,
//	7875,
//	14175,
//	22680,
//	32886,
//	44396,
//	57715,
//	72144,
//	90180,
//	112725,
//	140906,
//	176132,
//	220165,
//	275207,
//	344008,
//	430010,
//	537513,
//	671891,
//	839864,
//	1049830,
//	1312287,
//	1640359,
//	2050449,
//	2563061,
//	3203826,
//	3902260,
//	4663553,
//	5493363,
//	6397855,
//	7383752,
//	8458379,
//	9629723,
//	10906488,
//	12298162,
//	13815086,
//	15468534,
//	17270791,
//	19235252,
//	21376515,
//	23710491,
//	26254525,
//	29027522,
//	32050088,
//	35344686,
//	38935798,
//	42850109,
//	47116709,
//	51767302,
//	56836449,
//	62361819,
//	68384473,
//	74949165,
//	82104680,
//	89904191,
//	98405658,
//	107672256,
//	117772849,
//	128782495,
//	140783010,
//	153863570,
//	168121381,
//	183662396,
//	200602101,
//	219066380,
//	239192444,
//	261129853,
//	285041630,
//	311105466,
//	339515048,
//	370481492,
//	404234916,
//	441026148,
//	481128591,
//	524840254,
//	572485967,
//	624419793,
//	681027665,
//	742730244,
//	809986056,
//	883294891,
//	963201521,
//	1050299747,
//	1145236814,
//	1248718217,
//	1361512946,
//	1484459201,
//	1618470619,
//	1764543065,
//	1923762030,
//	2097310703,
//	2286478756,
//	2492671933,
//	2717422497,
//	2962400612,
//	3229426756,
//	3520485254,
//	3837739017,
//	9999999999
//};

void drawExperienceInfo() {
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (!pUnit) return;

	int nTime = ((GetTickCount() - gameTimer) / 1000);
	DWORD cExp = (DWORD)D2COMMON_GetUnitStat(pUnit, STAT_EXP, 0);
	if (startExperience == 0) { startExperience = cExp; }

	int cLevel = (int)D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0);
	if (startLevel == 0) { startLevel = cLevel; }

	char sExp[255] = { 0 };
	double oldPctExp = ((double)startExperience - ExpByLevel[startLevel - 1]) / (ExpByLevel[startLevel] - ExpByLevel[startLevel - 1]) * 100.0;
	double pExp = ((double)cExp - ExpByLevel[cLevel - 1]) / (ExpByLevel[cLevel] - ExpByLevel[cLevel - 1]) * 100.0;
	double expGainPct = pExp - oldPctExp;
	if (cLevel > startLevel) {
		expGainPct = (100 - oldPctExp) + pExp + ((cLevel - startLevel) - 1) * 100;
	}
	double expPerSecond = nTime > 0 ? (cExp - startExperience) / (double)nTime : 0;
	char* unit = "";
	if (expPerSecond > 1E9) {
		expPerSecond /= 1E9;
		unit = "B";
	}
	else if (expPerSecond > 1E6) {
		expPerSecond /= 1E6;
		unit = "M";
	}
	else if (expPerSecond > 1E3) {
		expPerSecond /= 1E3;
		unit = "K";
	}

	sprintf_s(sExp, "等级：%00d，经验：%00.2f%% (%s%00.2f%%) [%s%.2f%s/s]", cLevel, pExp, expGainPct >= 0 ? "+" : "", expGainPct, expPerSecond >= 0 ? "+" : "", expPerSecond, unit);

	Texthook::Draw((*p_D2CLIENT_ScreenSizeX / 2) - 200, *p_D2CLIENT_ScreenSizeY - 60, Center, 6, White, "%s", sExp);
}

void drawRuneNum() {
	//尝试给符文画一个数字
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (!pUnit) return;
	
	if (Item::viewingUnit) return;   //如果是查看其它玩家鸡儿的时候就不显示符文数字 

	if (!(D2CLIENT_GetUIState(UI_INVENTORY) || D2CLIENT_GetUIState(UI_STASH) || D2CLIENT_GetUIState(UI_CUBE) || D2CLIENT_GetUIState(UI_NPCSHOP)))
		return;

	for (UnitAny* pItem = pUnit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
		int left = 0;
		int top = 0;
		if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY) {
			left = INVENTORY_LEFT;
			top = INVENTORY_TOP;
			//角色背包都显示
			//if (!D2CLIENT_GetUIState(UI_INVENTORY)&& !D2CLIENT_GetUIState(UI_STASH)) continue;
		}
		else if (pItem->pItemData->ItemLocation == STORAGE_STASH) {
			left = STASH_LEFT;
			top = STASH_TOP;
			if (!D2CLIENT_GetUIState(UI_STASH)) continue;
		}
		else if (pItem->pItemData->ItemLocation == STORAGE_CUBE) {
			left = CUBE_LEFT;
			top = CUBE_TOP;
			if (!D2CLIENT_GetUIState(UI_CUBE)) continue;
		}

		if (code[0] == 'r' && code[1] >= '0' && code[1] <= '3' && code[2] >= '0' && code[2] <= '9') {
			char numCode[2];
			numCode[0] = code[1];
			numCode[1] = code[2];
			Texthook::Draw(left + 1 + CELL_SIZE * pItem->pObjectPath->dwPosX, top + 20 + CELL_SIZE * pItem->pObjectPath->dwPosY, None, 6, White, "%d", stoi(numCode));
		}
	}
}


void DrawDefaultFontText(wchar_t* wStr, int xpos, int ypos, DWORD dwColor, int div, DWORD dwAlign)
{
	D2WIN_DrawText(wStr, xpos - (D2WIN_GetTextPixelLen(wStr) >> div), ypos, dwColor, dwAlign);//dwAlign:多行时对齐有用 1居中 0 靠左
}

char* szOrbPattern = "%d/%d (%d%%)";
DWORD __stdcall ShowLifeWithMyPattern(DWORD callBack, int min, int max) {

	if (ChatColor::Toggles["Death Back"].state&&min<=0)   //这里写个死亡后立即回城,主要是解决去尸体后又会复活的bug。
	{
		PrintText(Red, "回城之歌：我送你离开~千里之外~~");
		SendMessage(D2GFX_GetHwnd(), WM_KEYDOWN, VK_ESCAPE, 0);
		Sleep(100);  //这个可以防止多次发送
	}
	wchar_t wszTemp[64];
	int iPercent = 100 * min / max;
	wsprintfW2(wszTemp, szOrbPattern, min, max, iPercent);
	DrawDefaultFontText(wszTemp, 75, Hook::GetScreenHeight() - 95, 0);
	return callBack;

}

DWORD __stdcall ShowManaWithMyPattern(DWORD callBack, int min, int max) {

	wchar_t wszTemp[64];
	int iPercent = 100 * min / max;
	wsprintfW2(wszTemp, szOrbPattern, min, max, iPercent);
	DrawDefaultFontText(wszTemp, Hook::GetScreenWidth() - 65, Hook::GetScreenHeight() - 95, 0);

	//这段代码写在这里，主要是不会把物品的属性给挡住（目前还不知道D2的Draw的优先级问题）
	//这里开始计算腰带剩余数量
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (!pUnit) return callBack;

	DWORD col1 = 0;
	DWORD col2 = 0;
	DWORD col3 = 0;
	DWORD col4 = 0;
	//腰带物品位置,16进制,pObjectPath.x
	//12,13,14,15
	//8,9,10,11
	//4,5,6,7
	//0,1,2,3
	for (UnitAny* pItem = pUnit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->ItemLocation == STORAGE_NULL && pItem->pItemData->NodePage == NODEPAGE_BELTSLOTS) {
			DWORD beltPos = pItem->pObjectPath->dwPosX;
			if (beltPos == 0 || beltPos == 4 || beltPos == 8 || beltPos == 12) {
				col1 += 1;
			}
			else if (beltPos == 1 || beltPos == 5 || beltPos == 9 || beltPos == 13) {
				col2 += 1;
			}
			else if (beltPos == 2 || beltPos == 6 || beltPos == 10 || beltPos == 14) {
				col3 += 1;
			}
			else if (beltPos == 3 || beltPos == 7 || beltPos == 11 || beltPos == 15) {
				col4 += 1;
			}
		}
	}
	//开始画腰带剩余数量
	DWORD beltOffsetX = 31;
	Texthook::Draw(*p_D2CLIENT_ScreenSizeX / 2 + 40, *p_D2CLIENT_ScreenSizeY - 33, None, 0, White, "%d", col1);
	Texthook::Draw(*p_D2CLIENT_ScreenSizeX / 2 + 40 + beltOffsetX, *p_D2CLIENT_ScreenSizeY - 33, None, 0, White, "%d", col2);
	Texthook::Draw(*p_D2CLIENT_ScreenSizeX / 2 + 40 + beltOffsetX * 2, *p_D2CLIENT_ScreenSizeY - 33, None, 0, White, "%d", col3);
	Texthook::Draw(*p_D2CLIENT_ScreenSizeX / 2 + 40 + beltOffsetX * 3, *p_D2CLIENT_ScreenSizeY - 33, None, 0, White, "%d", col4);

	//等级经验显示移到这里
	if (ScreenInfo::Toggles["Experience Meter"].state) {
		drawExperienceInfo();
	}

	//显示符文数字，再加个开关
	if(ChatColor::Toggles["Rune Number"].state)
		drawRuneNum();


	return callBack;

}
BYTE showOrbs = 2;  //显示自己的格式 
void __declspec(naked) ShowLifePatch_ASM()
{
	__asm {
		cmp [showOrbs], 2
		je showme
		mov ecx, 0x00001045
		ret
		showme :
		call ShowLifeWithMyPattern
		push eax
		add dword ptr[esp], 0x4E
		ret
	}
}

void __declspec(naked) ShowManaPatch_ASM()
{
	__asm {
		cmp [showOrbs], 2
		je showme
		mov ecx, 0x00001046
		ret
		showme :
		call ShowManaWithMyPattern
		push eax
		add dword ptr[esp], 0x5B
		ret
	}
}

//这里暂时先不复用代码，vc++不太熟^^
void AutoToBelt()
{
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return;

	//"hp", "mp", "rv"
		//循环查找背包里面的药
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY || pItem->pItemData->ItemLocation == STORAGE_CUBE) {   //只取背包和盒子里面的
			char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
			if (code[0] == 'h' && code[1] == 'p') {
				DWORD itemId = pItem->dwUnitId;  //红药
				//试一下这个不知道是不是填充的数据包
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
			if (code[0] == 'm' && code[1] == 'p') {
				DWORD itemId = pItem->dwUnitId;  //蓝药
				//试一下这个不知道是不是填充的数据包
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
			if (code[0] == 'r' && code[1] == 'v') {
				DWORD itemId = pItem->dwUnitId;  //紫药
				//试一下这个不知道是不是填充的数据包
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
		}
	}
}

//佣兵自动喝药,只支持腰带上的药
void AutoMercDrink(double perHP) {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return;
	if (perHP > 65) return;  //大于65的阈值也直接跳过算了
	//"hp", "mp", "rv"
		//循环查找背包里面的药
	DWORD itemId = 0;
	DWORD itemId_hp = 0;   //红药
	DWORD itemId_rv = 0;   //紫药
	char* code = "NULL";
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->ItemLocation == STORAGE_NULL && pItem->pItemData->NodePage == NODEPAGE_BELTSLOTS) {   //只能用腰带里的
			code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
			if (code[0] == 'h' && code[1] == 'p') {
				itemId_hp = pItem->dwUnitId; //红药
			}
			if (code[0] == 'r' && code[1] == 'v') {
				itemId_rv = pItem->dwUnitId; //紫药
			}
			if(itemId_hp !=0 && itemId_rv !=0) break;  //都找到了就直接中断循环
		}
	}
	if (itemId_hp == 0 && itemId_rv==0 && perHP <= 35) {
		if (ChatColor::Toggles["Merc Boring"].state)
			PrintText(Yellow, "你都没药啦，还不带我回家？：%.0f%%", perHP);
	}
	else {
		if (perHP > 35 && perHP <= 65) {
			if (itemId_hp == 0 && itemId_rv!=0) {
				itemId = itemId_rv;
				if (ChatColor::Toggles["Merc Boring"].state)
					PrintText(Purple, "佣兵少血啦,紫喝起来,干杯！：%.0f%%", perHP);
			}
			else if(itemId_hp != 0){
				itemId = itemId_hp;
				if (ChatColor::Toggles["Merc Boring"].state)
					PrintText(Red, "佣兵少血啦,红喝起来,干杯！：%.0f%%", perHP);
			}
		}
		else if (perHP <= 35) {
			if (itemId_rv != 0) {
				itemId = itemId_rv;
				if (ChatColor::Toggles["Merc Boring"].state)
					PrintText(Purple, "佣兵少血啦,紫喝起来,干杯！：%.0f%%", perHP);
			}
		}
	}
	if (itemId == 0) return;  //没药了或不用喝直接跳过
	BYTE PacketData[13] = { 0x26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	*reinterpret_cast<int*>(PacketData + 1) = itemId;
	*reinterpret_cast<int*>(PacketData + 5) = 1;  //是否给佣兵吃药,1是给，0是不给
	D2NET_SendPacket(13, 0, PacketData);
	//喝完还要填充回去
	Task::Enqueue([=]()->void {
		Sleep(1000);  //停1秒试试看
		AutoToBelt();
	});
}

//头像相关
void __fastcall DrawPetHeadPath(int xpos, UnitAny* pUnit) {
	
	wchar_t wszTemp[512];
	wsprintfW(wszTemp, L"%d", D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0));
	//swprintf(wszTemp, L"%d，%f/%f", D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0), hp,maxhp);
		
	//D2WIN_DrawText(1, wszTemp, xpos + 5, 57, 0);
	DWORD dwOldFone = D2WIN_SetTextFont(1);   //设置字体
	D2WIN_DrawText(wszTemp,xpos+5,57,White,0);
	//D2WIN_DrawText(wszTemp, xpos + 700, 570, White, 0);
	D2WIN_SetTextFont(dwOldFone);
	bool test1 = ChatColor::Toggles["Merc Protect"].state;
	if (test1) {
		//下面是佣兵自动喝药
		DWORD mHP = D2COMMON_GetUnitStat(pUnit, STAT_HP, 0);
		DWORD mMAXHP = D2COMMON_GetUnitStat(pUnit, STAT_MAXHP, 0);
		//if (mHP > 0x8000) {  //这个说明merc的血发生了变化,暂时先不用这个去判断 by zyl 20220422
			double maxhp = (double)(mMAXHP >> 8);  //这个佣兵取过来是300，不知道为什么
			if (maxhp > 128) maxhp = 128;   //这里先限制为128试试看了。
			double hp = (double)(mHP >> 8);
			double perHP = (hp / maxhp) * 100.0;
			if (perHP > 100) perHP = 100;  //这里喝药的时候有可能会有超出100的情况
			if ((GetTickCount() - mercLastTime) < (MercProtectSec*1000)) {  //这里先固定3秒吧
				//PrintText(White, "上次刚喝过药，先不喝");
				return;  //上次喝药时间少于3秒先不喝
			}
			if (perHP < mercLastHP)
			{
				//PrintText(White, "佣兵血变少：%.0f%%，%.0f%%", perHP, mercLastHP);
				//开始喝药
				AutoMercDrink(perHP);
				mercLastTime = GetTickCount();  //记录一下喝药的时间
			}
			mercLastHP = perHP;
		//}
	}
}

void __declspec(naked) DrawPetHeadPath_ASM()
{
	//ecx  xpos
	//eax  ypos
	//ebx  pPet
	__asm {
		push esi

		mov edx, ebx
		call DrawPetHeadPath

		pop esi
		//org
		mov[esp + 0x56], 0
		ret
	}
}


void __fastcall DrawPartyHeadPath(int xpos, RosterUnit* pRosterUnit) {

	wchar_t wszTemp[512];

	//if (tShowPartyLevel.isOn) {
		wsprintfW(wszTemp, L"%d", pRosterUnit->wLevel);
		//DrawD2Text(1, wszTemp, xpos + 5, 57, 0);
		DWORD dwOldFone = D2WIN_SetTextFont(1);   //设置字体
		D2WIN_DrawText(wszTemp, xpos + 5, 57, White, 0);
		D2WIN_SetTextFont(dwOldFone);
	//}

	//if (tShowPartyPosition.isOn) {  //场景号看不懂，先不开
	//	wsprintfW(wszTemp, L"%d", pRosterUnit->dwLevelNo);
	//	DrawCenterText(1, wszTemp, xpos + 20, 15, 4, 1, 1);
	//}
}

void __declspec(naked) DrawPartyHeadPath_ASM()
{
	//[ebx]  xpos
	//eax  ypos
	//[esp+0C]  pRosterUnit
	__asm {
		mov ecx, dword ptr[ebx]
		mov edx, dword ptr[esp + 0xC]

		push ebx
		push edi
		push eax

		call DrawPartyHeadPath

		pop eax
		pop edi
		pop ebx

		mov ecx, dword ptr[ebx]
		mov edx, dword ptr[esp + 0xC]
		ret
	}
}

wchar_t* wcsrcat(wchar_t* dest, wchar_t* src)
{
	memmove(dest + wcslen(src), dest, (wcslen(dest) + 1) * sizeof(wchar_t));
	memcpy(dest, src, wcslen(src) * sizeof(wchar_t));
	return dest;
}

wchar_t* __fastcall StrcatDefenseStringPatch(wchar_t* wszStr) {

	//拼防御字符串的地方,下一步代码拼接回车
	//对于ETH物品 Bug打孔，如果bug出来比正常的非bug最大值还小，则认为是非bug
	//if (tShowItemVariableProp.isOn) {

		UnitAny* pUnit = *p_D2CLIENT_SelectedInvItem;
		if (pUnit) {

			ItemTxt* pItemTxt = (ItemTxt*)D2COMMON_GetItemText(pUnit->dwTxtFileNo);
			if (pItemTxt && pItemTxt->dwMinDef > 0) {

				int mDef = D2COMMON_GetBaseStatSigned(pUnit, 31, 0);
				int mMinDef = pItemTxt->dwMinDef;
				int mMaxDef = pItemTxt->dwMaxDef;
				wchar_t wszPrep[256] = { L"\0" };
				wchar_t wszTemp[512] = { L"\0" };

				if (D2COMMON_CheckItemFlag(pUnit, ITEM_ETHEREAL, 0, "?")) {
					mMinDef = (int)(mMinDef * 1.5);
					mMaxDef = (int)(mMaxDef * 1.5);

					if (mDef > mMaxDef && pUnit->pItemData->dwQuality == 2 && D2COMMON_CheckItemFlag(pUnit, ITEM_HASSOCKETS, 0, "?")) {
						//普通物品 ,带孔，防御比最大防御还大，则为ETH BUG
						mMinDef = (int)(mMinDef * 1.5);
						mMaxDef = (int)(mMaxDef * 1.5);
						wcscpy(wszPrep, L"(Bug)");
					}

				}

				if (mDef > mMaxDef) {//超过最大防御，说明天生带ED，防御自动max+1
					wsprintfW(wszTemp, L"%s(最小: %d, 最大: %d, 现在: %d+%d)\n", wszPrep, mMinDef, mMaxDef, mMaxDef, mDef - mMaxDef);
				}
				else {
					wsprintfW(wszTemp, L"%s(最小: %d, 最大: %d, 现在: %d)\n", wszPrep, mMinDef, mMaxDef, mDef);
				}
				wcsrcat(wszStr, wszTemp);
			}

		}

	//}
	return wszStr;
}

void __declspec(naked) StrcatDefenseStringPatch_ASM()
{
	__asm {
		push ebx

		mov ecx, edi
		call StrcatDefenseStringPatch

		pop ebx

		mov edx, dword ptr[esp + 0x1C]
		mov ecx, eax
		ret
	}
}