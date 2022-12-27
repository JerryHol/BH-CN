#include "GameSettings.h"
#include "../../Drawing.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"
#include "../../BH.h"
#include "../Item/Item.h"
#include "../ScreenInfo/ScreenInfo.h"

// This module was inspired by the RedVex plugin "Item Mover", written by kaiks.
// Thanks to kaiks for sharing his code.

map<std::string, Toggle> GameSettings::Toggles;
unsigned int GameSettings::KeyHookOffset = 300;

void GameSettings::Init() {
}

void GameSettings::LoadConfig() {
	BH::config->ReadToggle("Quick Cast", "None", false, GameSettings::Toggles["Quick Cast"]);
	BH::config->ReadToggle("Skill Bar", "None", false, GameSettings::Toggles["Skill Bar"]);
	BH::config->ReadToggle("Skill Bar Disable", "None", false, GameSettings::Toggles["Skill Bar Disable"]);
	BH::config->ReadToggle("Buff Timers", "None", false, GameSettings::Toggles["Buff Timers"]);
	BH::config->ReadToggle("Screenshake", "None", true, GameSettings::Toggles["Screenshake"]);

	BH::config->ReadToggle("Developer Aura", "None", true, GameSettings::Toggles["Developer Aura"]);
	BH::config->ReadToggle("99 Aura", "None", true, GameSettings::Toggles["99 Aura"]);
	BH::config->ReadToggle("Rathma Aura", "None", true, GameSettings::Toggles["Rathma Aura"]);
	BH::config->ReadToggle("Dclone Aura", "None", true, GameSettings::Toggles["Dclone Aura"]);

	BH::config->ReadKey("Show Players Gear", "VK_0", showPlayer);
	BH::config->ReadKey("Resync Hotkey", "VK_9", resyncKey);
	BH::config->ReadKey("Character Stats", "VK_8", advStatMenuKey);
}

void GameSettings::LoadGeneralTab() {
	Drawing::Texthook* colored_text;

	Drawing::UITab* generalTab = new Drawing::UITab("通用", BH::settingsUI);

	unsigned int x_tab = 15;
	unsigned int x = 8;
	unsigned int y = 7;

	// General
	new Drawing::Texthook(generalTab, x, (y), "通用");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "显示其它玩家鸡儿");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &showPlayer, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "同步(卡角色的时候按一下)");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &resyncKey, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "显示玩家/雇佣兵高级属性");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &advStatMenuKey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &ScreenInfo::Toggles["Experience Meter"].state, "显示经验条");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &ScreenInfo::Toggles["Experience Meter"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Screenshake"].state, "屏幕抖动");  //跟地图里面的“移除震动效果”是一样的
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Screenshake"].toggle, "");

	// Quick Cast
	y += 20;
	new Drawing::Texthook(generalTab, x, (y), "快速施法");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Quick Cast"].state, "快速施法");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Quick Cast"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Skill Bar"].state, "技能栏");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Skill Bar"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x + x_tab, y, &GameSettings::Toggles["Skill Bar Disable"].state, "只启用快速施法，不显示技能栏");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Skill Bar Disable"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Buff Timers"].state, "总是显示状态计时器");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Buff Timers"].toggle, "");

}

void GameSettings::LoadInteractionTab() {
	Drawing::Texthook* colored_text;
	Drawing::UITab* tab = new Drawing::UITab("快捷说明", BH::settingsUI);

	unsigned int x = 8;
	unsigned int y = 7;
	int offset = 150;
	int indent = 5;

	// QoL
	colored_text = new Drawing::Texthook(tab, x, y,
		"鼠标移物品上时");

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Shift+左键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"如果鉴定书在背包，就可以快速鉴定物品");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Shift+右键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"在打开的背包、箱子、盒子之间移动");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+右键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"把物品扔地上");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+右键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"移动物品到关闭着的盒子");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+左键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"切换 堆叠/非堆叠");
	colored_text->SetColor(Gold);

	y += 20;
	colored_text = new Drawing::Texthook(tab, x, y,
		"鼠标拿起物品时");

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+左键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"放置 1 个堆叠的物品");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+左键");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"放置 1 个非堆叠的物品");
	colored_text->SetColor(Gold);

	// Auras
	y += 20;
	new Drawing::Texthook(tab, x, (y), "荣誉光环 (仅限全服前3玩家)");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["99 Aura"].state, "显示 99级 荣誉光环");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["99 Aura"].toggle, "");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Dclone Aura"].state, "显示 DC击杀 荣誉光环");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Dclone Aura"].toggle, "");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Rathma Aura"].state, "显示 拉斯玛击杀 荣誉光环");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Rathma Aura"].toggle, "");

	y += 20;
	new Drawing::Texthook(tab, x, (y), "仅限开发者");
	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Developer Aura"].state, "显示 开发者 荣誉光环");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Developer Aura"].toggle, "");
}

void GameSettings::OnLoad() {
	LoadConfig();
	LoadGeneralTab();
	LoadInteractionTab();
}

void GameSettings::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	for (map<string, Toggle>::iterator it = Toggles.begin(); it != Toggles.end(); it++) {
		if (key == (*it).second.toggle) {
			*block = true;
			if (up) {
				(*it).second.state = !(*it).second.state;
				BH::config->Write();
			}
			return;
		}
	}

	if (key == showPlayer) {
		*block = true;
		if (up)
			return;
		UnitAny* selectedUnit = D2CLIENT_GetSelectedUnit();
		if (selectedUnit && selectedUnit->dwMode != 0 && selectedUnit->dwMode != 17 && ( // Alive
			selectedUnit->dwType == 0 ||					// Player
			selectedUnit->dwTxtFileNo == 291 ||		// Iron Golem
			selectedUnit->dwTxtFileNo == 357 ||		// Valkerie
			selectedUnit->dwTxtFileNo == 418)) {	// Shadow Master
			Item::viewingUnit = selectedUnit;
			if (!D2CLIENT_GetUIState(0x01))
				D2CLIENT_SetUIVar(0x01, 0, 0);
			return;
		}
	}
}
