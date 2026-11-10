META
{
  Parser    = Game;
  MergeMode = 1;
  Engine    = G2A;
};

const int BT_SCRIPT_VERSION = 100;

const int BT_ST_STR  = 0;
const int BT_ST_DEX  = 1;
const int BT_ST_MANA = 2;
const int BT_ST_1H   = 3;
const int BT_ST_2H   = 4;
const int BT_ST_BOW  = 5;
const int BT_ST_CB   = 6;

var int BT_TeacherMaxOff;
var int BT_FightCapOff;
var int BT_ShowCostSplit;
var int BT_Lang;
const int BT_FIGHT_MAX = 100;

var int BT_TRAINED[7];
var int BT_CountersInit;

var int BT_TAB_1H;
var int BT_TAB_2H;
var int BT_TAB_BOW;
var int BT_TAB_CB;

var int BT_EQ[8];
var int BT_EQ_KNOWN[16];

var int BT_Init;
var int BT_MigStr;
var int BT_MigDex;
var int BT_MigMana;
var int BT_Mig1H;
var int BT_Mig2H;
var int BT_MigDaron;
var int BT_MigWulfgar;
var int BT_MigVatras;
var int BT_MigRengaru;

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
	if (attrib == ATR_STRENGTH)  { return "Strength"; };
	if (attrib == ATR_DEXTERITY) { return "Dexterity"; };
	if (attrib == ATR_MANA_MAX)  { return "Mana"; };
	return "attribute";
};

func string BT_TalentName(var int talent)
{
	if (talent == NPC_TALENT_1H)       { return "1H"; };
	if (talent == NPC_TALENT_2H)       { return "2H"; };
	if (talent == NPC_TALENT_BOW)      { return "Bow"; };
	if (talent == NPC_TALENT_CROSSBOW) { return "Crossbow"; };
	return "talent";
};

func int BT_CostTotal(var int x)
{
	var int r;
	if      (x <= 30)  { r = x; }
	else if (x <= 60)  { r = 30 + ((x - 30) * 2); }
	else if (x <= 90)  { r = 90 + ((x - 60) * 3); }
	else if (x <= 120) { r = 180 + ((x - 90) * 4); }
	else               { r = 300 + ((x - 120) * 5); };
	return r;
};

func int BT_Cost(var int trained, var int points)
{
	var int hi; hi = BT_CostTotal(trained + points);
	var int lo; lo = BT_CostTotal(trained);
	return hi - lo;
};

func int BT_CostStep(var int x)
{
	if (x < 30)  { return 1; };
	if (x < 60)  { return 2; };
	if (x < 90)  { return 3; };
	if (x < 120) { return 4; };
	return 5;
};

func string BT_CostSplit(var int trained, var int points)
{
	var int c1; c1 = BT_CostStep(trained);
	var int c2; c2 = BT_CostStep(trained + points - 1);
	if (c1 == c2) { return ""; };
	var int edge; edge = 30;
	if (c1 == 2) { edge = 60; };
	if (c1 == 3) { edge = 90; };
	if (c1 == 4) { edge = 120; };
	var int n1; n1 = edge - trained;
	var int n2; n2 = points - n1;
	var string s; s = " (";
	s = ConcatStrings(s, IntToString(n1));
	s = ConcatStrings(s, "x");
	s = ConcatStrings(s, IntToString(c1));
	s = ConcatStrings(s, " + ");
	s = ConcatStrings(s, IntToString(n2));
	s = ConcatStrings(s, "x");
	s = ConcatStrings(s, IntToString(c2));
	s = ConcatStrings(s, ")");
	return s;
};

func void BT_SetCounters(var C_NPC slf)
{
	if (BT_CountersInit == 1)
	{
		return;
	};
	BT_TRAINED[BT_ST_STR]  = slf.attribute[ATR_STRENGTH];
	BT_TRAINED[BT_ST_DEX]  = slf.attribute[ATR_DEXTERITY];
	BT_TRAINED[BT_ST_MANA] = slf.attribute[ATR_MANA_MAX];
	BT_TRAINED[BT_ST_1H]   = slf.HitChance[NPC_TALENT_1H];
	BT_TRAINED[BT_ST_2H]   = slf.HitChance[NPC_TALENT_2H];
	BT_TRAINED[BT_ST_BOW]  = slf.HitChance[NPC_TALENT_BOW];
	BT_TRAINED[BT_ST_CB]   = slf.HitChance[NPC_TALENT_CROSSBOW];
	BT_CountersInit = 1;
	BT_LogAdd("trained counters set: Strength ");
	BT_LogAddI(BT_TRAINED[BT_ST_STR]);
	BT_LogAdd(", Dexterity ");
	BT_LogAddI(BT_TRAINED[BT_ST_DEX]);
	BT_LogAdd(", Mana ");
	BT_LogAddI(BT_TRAINED[BT_ST_MANA]);
	BT_LogAdd(", 1H ");
	BT_LogAddI(BT_TRAINED[BT_ST_1H]);
	BT_LogAdd(", 2H ");
	BT_LogAddI(BT_TRAINED[BT_ST_2H]);
	BT_LogAdd(", Bow ");
	BT_LogAddI(BT_TRAINED[BT_ST_BOW]);
	BT_LogAdd(", Crossbow ");
	BT_LogAddI(BT_TRAINED[BT_ST_CB]);
	BT_LogEnd();
};

func int BT_GetTrainedAttribute(var int attrib)
{
	if (attrib == ATR_STRENGTH)  { return BT_TRAINED[BT_ST_STR]; };
	if (attrib == ATR_DEXTERITY) { return BT_TRAINED[BT_ST_DEX]; };
	if (attrib == ATR_MANA_MAX)  { return BT_TRAINED[BT_ST_MANA]; };
	return -1;
};

func void BT_SetTrainedAttribute(var int attrib, var int value)
{
	if      (attrib == ATR_STRENGTH)  { BT_TRAINED[BT_ST_STR]  = value; }
	else if (attrib == ATR_DEXTERITY) { BT_TRAINED[BT_ST_DEX]  = value; }
	else if (attrib == ATR_MANA_MAX)  { BT_TRAINED[BT_ST_MANA] = value; };
};

func int BT_GetTrainedTalent(var int talent)
{
	if (talent == NPC_TALENT_1H)       { return BT_TRAINED[BT_ST_1H]; };
	if (talent == NPC_TALENT_2H)       { return BT_TRAINED[BT_ST_2H]; };
	if (talent == NPC_TALENT_BOW)      { return BT_TRAINED[BT_ST_BOW]; };
	if (talent == NPC_TALENT_CROSSBOW) { return BT_TRAINED[BT_ST_CB]; };
	return -1;
};

func void BT_SetTrainedTalent(var int talent, var int value)
{
	if      (talent == NPC_TALENT_1H)       { BT_TRAINED[BT_ST_1H]  = value; }
	else if (talent == NPC_TALENT_2H)       { BT_TRAINED[BT_ST_2H]  = value; }
	else if (talent == NPC_TALENT_BOW)      { BT_TRAINED[BT_ST_BOW] = value; }
	else if (talent == NPC_TALENT_CROSSBOW) { BT_TRAINED[BT_ST_CB]  = value; };
};

func int BT_GetFightBonus(var int talent)
{
	if (talent == NPC_TALENT_1H)       { return BT_TAB_1H; };
	if (talent == NPC_TALENT_2H)       { return BT_TAB_2H; };
	if (talent == NPC_TALENT_BOW)      { return BT_TAB_BOW; };
	if (talent == NPC_TALENT_CROSSBOW) { return BT_TAB_CB; };
	return 0;
};

func void BT_AddFightBonus(var int talent, var int percent)
{
	if      (talent == NPC_TALENT_1H)       { BT_TAB_1H  = BT_TAB_1H  + percent; }
	else if (talent == NPC_TALENT_2H)       { BT_TAB_2H  = BT_TAB_2H  + percent; }
	else if (talent == NPC_TALENT_BOW)      { BT_TAB_BOW = BT_TAB_BOW + percent; }
	else if (talent == NPC_TALENT_CROSSBOW) { BT_TAB_CB  = BT_TAB_CB  + percent; };
};

func void BT_PrintFightCap()
{
	var string t;
	if (BT_Lang == 1)
	{
		t = ConcatStrings("Maksimum to ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (razem z tabliczkami)");
	}
	else if (BT_Lang == 2)
	{
		t = ConcatStrings("Maximum ist ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (mit Steintafeln)");
	}
	else if (BT_Lang == 3)
	{
		t = ConcatStrings("Maximum je ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (vcetne kamennych tabulek)");
	}
	else
	{
		t = ConcatStrings("Maximum is ", IntToString(BT_FIGHT_MAX));
		t = ConcatStrings(t, "% (stone tablets included)");
	};
	PrintScreen(t, -1, -1, FONT_Screen, 2);
};

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

	BT_SetCounters(hero);

	var int s;  s  = BT_MigStr;
	var int d;  d  = BT_MigDex  + BT_MigRengaru;
	var int m;  m  = BT_MigMana + (BT_MigDaron * 2) + (BT_MigVatras * 3);
	var int h1; h1 = BT_Mig1H   + (BT_MigWulfgar * 2);
	var int h2; h2 = BT_Mig2H;

	var int oS; oS = BT_TRAINED[BT_ST_STR];
	var int oD; oD = BT_TRAINED[BT_ST_DEX];
	var int oM; oM = BT_TRAINED[BT_ST_MANA];
	var int o1; o1 = BT_TRAINED[BT_ST_1H];
	var int o2; o2 = BT_TRAINED[BT_ST_2H];
	var int n;
	n = BT_MigMove(oS, s);  BT_TRAINED[BT_ST_STR]  = n;
	n = BT_MigMove(oD, d);  BT_TRAINED[BT_ST_DEX]  = n;
	n = BT_MigMove(oM, m);  BT_TRAINED[BT_ST_MANA] = n;
	n = BT_MigMove(o1, h1); BT_TRAINED[BT_ST_1H]   = n;
	n = BT_MigMove(o2, h2); BT_TRAINED[BT_ST_2H]   = n;
	BT_TAB_1H = BT_TAB_1H + (o1 - BT_TRAINED[BT_ST_1H]);
	BT_TAB_2H = BT_TAB_2H + (o2 - BT_TRAINED[BT_ST_2H]);

	BT_LogAdd("old save: moved to bonus Strength "); BT_LogAddI(oS - BT_TRAINED[BT_ST_STR]);
	BT_LogAdd(", Dexterity "); BT_LogAddI(oD - BT_TRAINED[BT_ST_DEX]);
	BT_LogAdd(", Mana ");      BT_LogAddI(oM - BT_TRAINED[BT_ST_MANA]);
	BT_LogAdd(", 1H ");        BT_LogAddI(o1 - BT_TRAINED[BT_ST_1H]);
	BT_LogAdd(", 2H ");        BT_LogAddI(o2 - BT_TRAINED[BT_ST_2H]);
	BT_LogAdd(" (shrines, books, rewards; elixirs: BT BONUS in the console)");
	BT_LogEnd();
};
