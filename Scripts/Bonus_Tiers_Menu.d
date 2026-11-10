META
{
  Parser = Menu;
  After  = Bonus_Tiers_Union.d;
  Engine = G2A;
};

const int BT_TEXT_X  = 1200;
const int BT_TEXT_DX = 3600;
const int BT_TEXT_DY = 320;
const int BT_VAL_X   = 4900;
const int BT_VAL_DX  = 2300;
const int BT_VAL_DY  = 320;
const int BT_TITLE_Y = 800;
const int BT_START_Y = 1350;
const int BT_DY      = 380;

instance C_BT_LABEL(C_MENU_ITEM_DEF)
{
  backpic        = "";
  posx           = BT_TEXT_X;
  posy           = BT_START_Y;
  dimx           = BT_TEXT_DX;
  dimy           = BT_TEXT_DY;
  flags          = flags | IT_EFFECTS_NEXT;
  fontname       = MENU_FONT_SMALL;
  onselaction[0] = SEL_ACTION_UNDEF;
};

instance C_BT_CHOICE(C_MENU_ITEM_DEF)
{
  backpic               = "MENU_CHOICE_BACK.TGA";
  type                  = MENU_ITEM_CHOICEBOX;
  fontname              = MENU_FONT_SMALL;
  posx                  = BT_VAL_X;
  posy                  = BT_START_Y;
  dimx                  = BT_VAL_DX;
  dimy                  = BT_VAL_DY;
  flags                 = flags & ~IT_SELECTABLE;
  flags                 = flags | IT_TXT_CENTER;
  onchgsetoptionsection = "BONUS_TIERS";
};

instance MENU_OPT_BONUS_TIERS(C_MENU_DEF)
{
  backpic        = "MENU_INGAME.TGA";
  items[0]       = "BT_TITLE";
  items[1]       = "BT_ENABLED";
  items[2]       = "BT_ENABLED_VAL";
  items[3]       = "BT_VIEW";
  items[4]       = "BT_VIEW_VAL";
  items[5]       = "BT_WIDTH";
  items[6]       = "BT_WIDTH_VAL";
  items[7]       = "BT_SPLIT";
  items[8]       = "BT_SPLIT_VAL";
  items[9]       = "BT_TEACHMAX";
  items[10]      = "BT_TEACHMAX_VAL";
  items[11]      = "BT_FIGHTCAP";
  items[12]      = "BT_FIGHTCAP_VAL";
  items[13]      = "BT_LOG";
  items[14]      = "BT_LOG_VAL";
  items[15]      = "BT_LANG";
  items[16]      = "BT_LANG_VAL";
  items[17]      = "BT_AUTHOR";
  items[18]      = "BT_VERSION";
  items[19]      = "UNION_MENUITEM_BACK";
  defaultoutgame = 0;
  defaultingame  = 0;
  flags          = flags | MENU_SHOW_INFO;
};

instance BT_TITLE(C_MENU_ITEM_DEF)
{
  type    = MENU_ITEM_TEXT;
  posx    = 0;
  posy    = BT_TITLE_Y;
  dimx    = 8100;
  dimy    = BT_TEXT_DY;
  flags   = flags & ~IT_SELECTABLE;
  flags   = flags | IT_TXT_CENTER;
  text[0] = "BONUS TIERS - SETTINGS";
};

instance BT_ENABLED(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 0;
  text[0] = "Plugin enabled";
  text[1] = "Yes: bonuses do not raise LP costs or block teachers.";
};

instance BT_ENABLED_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 0;
  onchgsetoption = "Enabled";
  text[0]        = "No|Yes";
  text[1]        = "Yes: bonuses do not raise LP costs or block teachers.";
  userString[1]  = "No: plugin off, training and screen work like the game.|Yes: bonuses do not raise LP costs or block teachers.";
};

instance BT_VIEW(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 1;
  text[0] = "Character screen";
  text[1] = "58 +3 +5: total, +3 potions, +5 jewelry and weapons.";
};

instance BT_VIEW_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 1;
  onchgsetoption = "StatusView";
  text[0]        = "58 +3 +5|25/45|Vanilla";
  text[1]        = "58 +3 +5: total, +3 potions, +5 jewelry and weapons.";
  userString[1]  = "58 +3 +5: total, +3 potions, +5 jewelry and weapons.|25/45: 25 trained by teachers, 45 with all bonuses.|Vanilla: character screen unchanged, no bonuses shown.";
};

instance BT_WIDTH(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 2;
  text[0] = "Character screen width";
  text[1] = "Auto: only as wide as needed (at most 120%).";
};

instance BT_WIDTH_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 2;
  onchgsetoption = "StatusWidth";
  text[0]        = "Auto|100%|105%|110%|115%|120%";
  text[1]        = "Auto: only as wide as needed (at most 120%).";
  userString[1]  = "Auto: only as wide as needed (at most 120%).|100%: like the game. Long numbers may leave the frame.|Fixed character screen width. Auto picks it itself.|Fixed character screen width. Auto picks it itself.|Fixed character screen width. Auto picks it itself.|120%: the widest window, everything fits.";
};

instance BT_SPLIT(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 3;
  text[0] = "LP split in dialogs";
  text[1] = "No: teacher dialogs show only the LP cost.";
};

instance BT_SPLIT_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 3;
  onchgsetoption = "CostSplit";
  text[0]        = "No|Yes";
  text[1]        = "No: teacher dialogs show only the LP cost.";
  userString[1]  = "No: teacher dialogs show only the LP cost.|Yes: LP cost by tiers, e.g. 6 LP (4x1 + 1x2).";
};

instance BT_TEACHMAX(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 4;
  text[0] = "Teacher limit";
  text[1] = "Yes: teachers train up to their own limit (like NotR).";
};

instance BT_TEACHMAX_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 4;
  onchgsetoption = "TeacherMax";
  text[0]        = "No|Yes";
  text[1]        = "Yes: teachers train up to their own limit (like NotR).";
  userString[1]  = "No: teachers train past their own limit.|Yes: teachers train up to their own limit (like NotR).";
};

instance BT_FIGHTCAP(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 5;
  text[0] = "Weapon skills up to 100%";
  text[1] = "Yes: weapon skills at most 100% (with tablets), like NotR.";
};

instance BT_FIGHTCAP_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 5;
  onchgsetoption = "FightCap100";
  text[0]        = "No|Yes";
  text[1]        = "Yes: weapon skills at most 100% (with tablets), like NotR.";
  userString[1]  = "No: weapon skills can go past 100%.|Yes: weapon skills at most 100% (with tablets), like NotR.";
};

instance BT_LOG(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 6;
  text[0] = "Log file";
  text[1] = "Yes: training and bonuses go to Bonus_Tiers.log.";
};

instance BT_LOG_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 6;
  onchgsetoption = "Log";
  text[0]        = "No|Yes";
  text[1]        = "Yes: training and bonuses go to Bonus_Tiers.log.";
  userString[1]  = "No: the plugin writes nothing.|Yes: training and bonuses go to Bonus_Tiers.log.";
};

instance BT_LANG(C_MENU_ITEM_DEF)
{
  C_BT_LABEL();
  posy   += BT_DY * 7;
  text[0] = "Language";
  text[1] = "Auto: the language of the game (else English).";
};

instance BT_LANG_VAL(C_MENU_ITEM_DEF)
{
  C_BT_CHOICE();
  posy          += BT_DY * 7;
  onchgsetoption = "Language";
  text[0]        = "Auto|English|Polski|Deutsch|Cestina";
  text[1]        = "Auto: the language of the game (else English).";
  userString[1]  = "Auto: the language of the game (else English).|English texts.|Polish texts (Polski).|German texts (Deutsch).|Czech texts (Cestina).";
};

instance BT_AUTHOR(C_MENU_ITEM_DEF)
{
  type     = MENU_ITEM_TEXT;
  posx     = BT_TEXT_X;
  posy     = BT_START_Y + BT_DY * 12;
  dimx     = 5800;
  dimy     = BT_TEXT_DY;
  fontname = MENU_FONT_SMALL;
  flags    = flags & ~IT_SELECTABLE;
  text[0]  = "Author: PEN996";
};

instance BT_VERSION(C_MENU_ITEM_DEF)
{
  type     = MENU_ITEM_TEXT;
  posx     = BT_TEXT_X;
  posy     = BT_START_Y + BT_DY * 13;
  dimx     = 5800;
  dimy     = BT_TEXT_DY;
  fontname = MENU_FONT_SMALL;
  flags    = flags & ~IT_SELECTABLE;
  text[0]  = "Version: 1.00";
};
