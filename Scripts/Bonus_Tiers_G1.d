META
{
  Parser    = Game;
  MergeMode = 1;
  Engine    = G1;
};

const int BT_SCRIPT_VERSION = 100;
const int BT_G1_STR   = 43;
const int BT_G1_DEX   = 44;
const int BT_G1_MANA  = 45;
const int BT_G1_INIT  = 46;
const int BT_G1_LIMIT = 100;

var int BT_TeacherMaxOff;
var int BT_FightCapOff;
var int BT_ShowCostSplit;
var int BT_Lang;

var int BT_TRAINED[7];
var int BT_CountersInit;

var int BT_EQ[8];
var int BT_EQ_KNOWN[16];

var int BT_Init;

var string BT_LogBuf;
var int    BT_LogCount;
var string BT_LogLine;

func void BT_LogAdd(var string s)
{
	BT_LogLine = ConcatStrings(BT_LogLine, s);
};

func void BT_LogAddI(var int v)
{
	BT_LogLine = ConcatStrings(BT_LogLine, IntToString(v));
};

func void BT_LogEnd()
{
	if (BT_LogCount >= 60)
	{
		BT_LogBuf = "";
		BT_LogCount = 0;
	};
	if (BT_LogCount == 0)
	{
		BT_LogBuf = BT_LogLine;
	}
	else
	{
		BT_LogBuf = ConcatStrings(BT_LogBuf, "|");
		BT_LogBuf = ConcatStrings(BT_LogBuf, BT_LogLine);
	};
	BT_LogCount = BT_LogCount + 1;
	BT_LogLine = "";
};

func string BT_AttrName(var int attrib)
{
	if (attrib == ATR_STRENGTH)      { return "Strength"; };
	if (attrib == ATR_DEXTERITY)     { return "Dexterity"; };
	if (attrib == ATR_MANA_MAX)      { return "Mana"; };
	if (attrib == ATR_HITPOINTS_MAX) { return "HP"; };
	return "attribute";
};

func void BT_G1_SetCounters(var C_NPC slf)
{
	if (slf.aivar[BT_G1_INIT] == 1)
	{
		return;
	};
	slf.aivar[BT_G1_STR]  = slf.attribute[ATR_STRENGTH];
	slf.aivar[BT_G1_DEX]  = slf.attribute[ATR_DEXTERITY];
	slf.aivar[BT_G1_MANA] = slf.attribute[ATR_MANA_MAX];
	slf.aivar[BT_G1_INIT] = 1;
	BT_LogAdd("trained counters set: Strength ");
	BT_LogAddI(slf.aivar[BT_G1_STR]);
	BT_LogAdd(", Dexterity ");
	BT_LogAddI(slf.aivar[BT_G1_DEX]);
	BT_LogAdd(", Mana ");
	BT_LogAddI(slf.aivar[BT_G1_MANA]);
	BT_LogEnd();
};

func int BT_GetTrainedAttribute(var C_NPC slf, var int atr)
{
	if (atr == ATR_STRENGTH)  { return slf.aivar[BT_G1_STR]; };
	if (atr == ATR_DEXTERITY) { return slf.aivar[BT_G1_DEX]; };
	if (atr == ATR_MANA_MAX)  { return slf.aivar[BT_G1_MANA]; };
	return -1;
};

func void BT_SetTrainedAttribute(var C_NPC slf, var int atr, var int value)
{
	if      (atr == ATR_STRENGTH)  { slf.aivar[BT_G1_STR]  = value; }
	else if (atr == ATR_DEXTERITY) { slf.aivar[BT_G1_DEX]  = value; }
	else if (atr == ATR_MANA_MAX)  { slf.aivar[BT_G1_MANA] = value; };
};

func int BT_GetFullAttribute(var C_NPC slf, var int atr)
{
	var int v; v = -1;
	if      (atr == ATR_STRENGTH)  { v = slf.attribute[ATR_STRENGTH]; }
	else if (atr == ATR_DEXTERITY) { v = slf.attribute[ATR_DEXTERITY]; }
	else if (atr == ATR_MANA_MAX)  { v = slf.attribute[ATR_MANA_MAX]; };
	return v;
};

func void BT_SetFullAttribute(var C_NPC slf, var int atr, var int value)
{
	if      (atr == ATR_STRENGTH)  { slf.attribute[ATR_STRENGTH]  = value; }
	else if (atr == ATR_DEXTERITY) { slf.attribute[ATR_DEXTERITY] = value; }
	else if (atr == ATR_MANA_MAX)  { slf.attribute[ATR_MANA_MAX]  = value; };
};

var int BT_MigHoratio;

func int BT_MigMove(var int trained, var int n)
{
	var int r; r = trained - n;
	if (r < 10) { r = 10; };
	if (r > trained) { r = trained; };
	return r;
};

func void BT_MigrateOld()
{
	if (BT_Init != 0) { return; };
	BT_Init = 1;

	var int fresh; fresh = (hero.aivar[BT_G1_INIT] != 1);
	BT_G1_SetCounters(hero);
	if (!fresh)
	{
		return;
	};
	var int s; s = 0;
	if (BT_MigHoratio) { s = s + 5; };

	var int oS; oS = hero.aivar[BT_G1_STR];
	var int n;  n  = BT_MigMove(oS, s);
	hero.aivar[BT_G1_STR] = n;

	BT_LogAdd("old save: moved to bonus Strength "); BT_LogAddI(oS - hero.aivar[BT_G1_STR]);
	BT_LogAdd(" (Horatio; elixirs: BT BONUS in the console)");
	BT_LogEnd();
};

const int BT_FIGHT_MAX = 100;

func void BT_PrintFightCap()
{
	var string t;
	if (BT_Lang == 1)
	{
		t = ConcatStrings("Maksimum to ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (razem z bonusami)");
	}
	else if (BT_Lang == 2)
	{
		t = ConcatStrings("Maximum ist ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (mit Boni)");
	}
	else if (BT_Lang == 3)
	{
		t = ConcatStrings("Maximum je ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (vcetne bonusu)");
	}
	else
	{
		t = ConcatStrings("Maximum is ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (bonuses included)");
	};
	PrintScreen(t, -1, -1, _STR_FONT_ONSCREEN, 2);
};
