META
{
  Parser = Menu;
  After  = zUnionMenu.d;
};

instance MENUITEM_UNION_AUTO_BONUS_TIERS(C_MENU_ITEM_UNION_DEF)
{
  onselaction[0]   = SEL_ACTION_STARTMENU;
  onselaction_s[0] = "MENU_OPT_BONUS_TIERS";
  flags            = flags | IT_TXT_CENTER;
  text[0]          = "Bonus Tiers";
  text[1]          = "Potion and tablet bonuses do not raise training costs.";
};
