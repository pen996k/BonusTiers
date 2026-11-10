#pragma once

namespace BonusTiers {
  struct MenuInfoItem { const char* value; const char* label; const char* option; int def; };
  static const MenuInfoItem MENU_INFO_ITEMS[] = {
    { "BT_ENABLED_VAL",  "BT_ENABLED",  "Enabled",     1 },
    { "BT_VIEW_VAL",     "BT_VIEW",     "StatusView",  0 },
    { "BT_WIDTH_VAL",    "BT_WIDTH",    "StatusWidth", 0 },
    { "BT_SPLIT_VAL",    "BT_SPLIT",    "CostSplit",   0 },
    { "BT_TEACHMAX_VAL", "BT_TEACHMAX", "TeacherMax",  1 },
    { "BT_FIGHTCAP_VAL", "BT_FIGHTCAP", "FightCap100", 1 },
    { "BT_LOG_VAL",      "BT_LOG",      "Log",         1 },
    { "BT_LANG_VAL",     "BT_LANG",     "Language",    0 },
  };
  static const int MENU_INFO_COUNT = sizeof( MENU_INFO_ITEMS ) / sizeof( MENU_INFO_ITEMS[0] );
}
