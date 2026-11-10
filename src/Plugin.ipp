#pragma once

namespace GOTHIC_NAMESPACE
{
#include "Plugin/Bonus_Tiers_EquipCalc.h"
#include "Plugin/Bonus_Tiers_HpCalc.h"
#include "Plugin/Bonus_Tiers_IniParse.h"
#include "Plugin/Bonus_Tiers_Lang.h"
#include "Plugin/Bonus_Tiers_Layout.h"
#include "Plugin/Bonus_Tiers_MenuInfo.h"
#include "Plugin/Bonus_Tiers_Scan.h"
#include "Plugin/Bonus_Tiers_TrainCalc.h"

#define BT_HOOK0(cls, fn) void __fastcall Hook_##cls##_##fn(cls* self, void* vt); \
  auto Ivk_##cls##_##fn = Union::CreateHook(SIGNATURE_OF(&cls::fn), &Hook_##cls##_##fn, Union::HookType::Hook_Detours); \
  void __fastcall Hook_##cls##_##fn(cls* self, void* vt)
#define BT_HOOK(cls, fn, ...) void __fastcall Hook_##cls##_##fn(cls* self, void* vt, __VA_ARGS__); \
  auto Ivk_##cls##_##fn = Union::CreateHook(SIGNATURE_OF(&cls::fn), &Hook_##cls##_##fn, Union::HookType::Hook_Detours); \
  void __fastcall Hook_##cls##_##fn(cls* self, void* vt, __VA_ARGS__)

  namespace BonusTiers {
    static const char *PLUGIN_NAME = "Bonus Tiers", *PLUGIN_VERSION = "1.00", *OPT_SECTION = "BONUS_TIERS";
    static const int G1_TRAINED_STR = 43, G1_TRAINED_DEX = 44, G1_TRAINED_MANA = 45, G1_TRAINED_INIT = 46;
    enum { VIEW_BONUS = 0, VIEW_SLASH = 1, VIEW_VANILLA = 2 };

    static Union::StringANSI g_logPath;
    static bool g_checked = false, g_iniLoaded = false, g_iniFresh = false, g_trainSwapped = false;
    static bool g_compactAtr = false, g_compactTal = false, g_eqSuppress = false, g_pendingFightCapMsg = false, g_trainInited = false, g_layoutNotR = true;
    static int  g_crashLogged = 0, g_menuSeenTeach = 1, g_menuSeenFight = 1, g_lang = LANG_EN, g_menuInfoShown[MENU_INFO_COUNT];
    static int  s_g1FallbackTrained[ST_COUNT] = {}, s_g1FallbackInit = 0, g_migPending = 0, g_savedManaAboveShown = 0, g_savedCurHp = 0;
    static int  g_doStackDepth = 0, g_lastCostStat = -1, g_lastCostOne = 0, g_wantPermille = 1000, g_appliedPermille = 1000, g_loggedPermille = -1;
    static const ScanTiers* g_lastCostTiers = 0;
    static int  g_lastA2 = LAYOUT_NOTR_ATR_VAL_X, g_lastB2 = LAYOUT_NOTR_TAL_SKILL_X, g_lastB3 = LAYOUT_NOTR_TAL_VAL_X;
    static IniLimits      g_ini = { true, true };
    static DWORD          g_iniCheckTick = 0;
    static FILETIME       g_iniStamp = { 0, 0 };
    static EqStack        g_eqStack = {};
    static oCMenu_Status* g_statusMenu = 0;

    inline const Union::StringANSI& GameDir() {
      static Union::StringANSI dir;
      if( dir.IsEmpty() ) { char exe[MAX_PATH] = ""; GetModuleFileNameA( 0, exe, MAX_PATH ); dir = Union::StringANSI( exe ).GetDirectory().GetDirectory(); }
      return dir;
    }
    inline int  ReadOpt( const char* key, int def ) { return zoptions ? zoptions->ReadInt( OPT_SECTION, key, def ) : (int)GetPrivateProfileIntA( OPT_SECTION, key, def, GameDir() + "System\\Gothic.ini" ); }
    inline bool LogEnabled()    { return ReadOpt( "Log", 1 ) != 0; }
    inline bool PluginEnabled() { return ReadOpt( "Enabled", 1 ) != 0; }
    inline int  StatusView()    { const int v = ReadOpt( "StatusView", 0 ); return !PluginEnabled() ? VIEW_VANILLA : ( v == VIEW_SLASH || v == VIEW_VANILLA ) ? v : VIEW_BONUS; }

    inline void EnsureDefaults() {
      static bool done = false; if( done || !zoptions ) return; done = true;
      for( const auto& m : MENU_INFO_ITEMS )
        if( !zoptions->EntryExists( OPT_SECTION, m.option ) ) zoptions->WriteInt( OPT_SECTION, m.option, m.def, 0 );
    }

    inline void LogWriteRaw( const char* line, size_t len ) {
      if( g_logPath.IsEmpty() ) {
        Union::StringANSI dir = GameDir() + "Bonus_Tiers\\"; CreateDirectoryA( dir, 0 );
        g_logPath = dir + "Bonus_Tiers.log"; MoveFileExA( g_logPath, dir + "Bonus_Tiers_previous.log", MOVEFILE_REPLACE_EXISTING );
      }
      Union::FileWriter fw( g_logPath, true ); if( fw.IsOpened() ) fw.Write( (void*)line, len );
    }

    inline void LogRawFmt( const char* fmt, ... ) {
      char buf[1152]; va_list va; va_start( va, fmt ); const int n = vsnprintf( buf, sizeof( buf ), fmt, va ); va_end( va );
      if( n > 0 ) LogWriteRaw( buf, (size_t)n < sizeof( buf ) ? (size_t)n : sizeof( buf ) - 1 );
    }

    inline void Log( const char* fmt, ... ) {
      if( !LogEnabled() ) return;
      SYSTEMTIME t; GetLocalTime( &t );
      char buf[1152]; const int p = snprintf( buf, sizeof( buf ), "%04u-%02u-%02u %02u:%02u:%02u ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond );
      va_list va; va_start( va, fmt ); int m = vsnprintf( buf + p, sizeof( buf ) - (size_t)p - 2, fmt, va ); va_end( va );
      if( m < 0 ) m = 0; else if( (size_t)( p + m ) > sizeof( buf ) - 3 ) m = (int)sizeof( buf ) - p - 3;
      buf[p + m] = '\r'; buf[p + m + 1] = '\n'; buf[p + m + 2] = '\0';
      LogWriteRaw( buf, (size_t)( p + m + 2 ) );
    }

    inline zCPar_Symbol* Sym( const char* name ) { return parser ? parser->GetSymbol( name ) : 0; }
    inline int  ScriptInt( const char* name ) { int v = 0; if( zCPar_Symbol* s = Sym( name ) ) s->GetValue( v, 0 ); return v; }
    inline void SetScriptInt( const char* name, int v ) { if( zCPar_Symbol* s = Sym( name ) ) { int c = 0; s->GetValue( c, 0 ); if( c != v ) s->SetValue( v, 0 ); } }

    inline void CheckScript() {
      if( g_checked || !parser ) return; g_checked = true;
      if( !Sym( "BT_SCRIPT_VERSION" ) ) Log( "ERROR: training script not loaded - training works like the original game. Is Union (zParserExtender) installed?" );
      else Log( "training script: loaded (version %d)", ScriptInt( "BT_SCRIPT_VERSION" ) );
    }

    inline void FlushScriptLog() {
      zCPar_Symbol *cnt = Sym( "BT_LOGCOUNT" ), *buf = Sym( "BT_LOGBUF" ); int n = 0;
      if( !cnt || !buf || ( cnt->GetValue( n, 0 ), n <= 0 ) ) return;
      zSTRING s, empty( "" ); buf->GetValue( s, 0 );
      if( !s.IsEmpty() ) for( auto& line : Union::StringANSI( s.ToChar() ).Split( '|', false ) ) Log( "%s", line.ToChar() );
      cnt->SetValue( 0, 0 ); buf->SetValue( empty, 0 );
    }

    inline const char* DescribeAddress( unsigned addr, char* out, size_t cap ) {
      HMODULE mod = 0; char path[MAX_PATH] = "";
      if( GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &mod ) && mod && GetModuleFileNameA( mod, path, MAX_PATH ) ) {
        const char *sl1 = strrchr( path, '\\' ), *sl2 = strrchr( path, '/' ), *nm = ( sl1 > sl2 ? sl1 : sl2 );
        snprintf( out, cap, "%s+0x%X", nm ? nm + 1 : path, addr - (unsigned)mod );
      } else snprintf( out, cap, "0x%X", addr );
      return out;
    }

    inline LONG WINAPI CrashObserver( EXCEPTION_POINTERS* ep ) {
      if( !ep || !ep->ExceptionRecord || !ep->ContextRecord || g_crashLogged >= 4 || !LogEnabled() ) return EXCEPTION_CONTINUE_SEARCH;
      const EXCEPTION_RECORD* er = ep->ExceptionRecord;
      if( er->ExceptionCode != EXCEPTION_ACCESS_VIOLATION && er->ExceptionCode != EXCEPTION_STACK_OVERFLOW && er->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION ) return EXCEPTION_CONTINUE_SEARCH;
      g_crashLogged++;
      const CONTEXT* c = ep->ContextRecord;
      const ULONG_PTR kind = er->NumberParameters > 0 ? er->ExceptionInformation[0] : 0;
      const unsigned fault = er->NumberParameters > 1 ? (unsigned)er->ExceptionInformation[1] : 0;
      char desc[96];
      LogRawFmt( "CRASH: code 0x%08lX at %s (EIP 0x%08lX), %s address 0x%08X\r\n", er->ExceptionCode, DescribeAddress( c->Eip, desc, sizeof( desc ) ), c->Eip, kind == 0 ? "reading" : ( kind == 1 ? "writing" : "executing" ), fault );
      LogRawFmt( "CRASH: EAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX ESI=%08lX EDI=%08lX EBP=%08lX ESP=%08lX\r\n", c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp );
      const unsigned* sp = (const unsigned*)c->Esp;
      for( int i = 0, shown = 0; i < 512 && shown < 28; i++ ) {
        MEMORY_BASIC_INFORMATION mbi, cmbi;
        if( !VirtualQuery( (LPCVOID)( sp + i ), &mbi, sizeof( mbi ) ) || mbi.State != MEM_COMMIT ) break;
        if( sp[i] >= 0x10000 && VirtualQuery( (LPCVOID)sp[i], &cmbi, sizeof( cmbi ) ) && cmbi.State == MEM_COMMIT && ( cmbi.Protect & ( PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY ) ) ) {
          LogRawFmt( "CRASH: stack[+0x%X] = 0x%08X  %s\r\n", i * 4, sp[i], DescribeAddress( sp[i], desc, sizeof( desc ) ) ); shown++;
        }
      }
      return EXCEPTION_CONTINUE_SEARCH;
    }

    inline void IniRefresh() {
      const DWORD now = GetTickCount();
      if( g_iniLoaded && now - g_iniCheckTick < 1000 ) return;
      g_iniCheckTick = now;
      Union::StringANSI path = GameDir() + "System\\Bonus_Tiers.ini", text;
      WIN32_FILE_ATTRIBUTE_DATA fa;
      if( !GetFileAttributesExA( path, GetFileExInfoStandard, &fa ) ) {
        if( Union::StringANSI( INI_DEFAULT ).WriteToFile( path, Union::StringBase::ANSI ) ) Log( "Bonus_Tiers.ini: created (MaxTeachingThreshold = True, MaxFightingSkillThreshold = True)" );
        if( !GetFileAttributesExA( path, GetFileExInfoStandard, &fa ) ) { g_iniLoaded = true; return; }
      }
      if( ( g_iniLoaded && CompareFileTime( &fa.ftLastWriteTime, &g_iniStamp ) == 0 ) || !text.ReadFromFile( path ) ) { g_iniStamp = fa.ftLastWriteTime; return; }
      g_iniStamp = fa.ftLastWriteTime;
      int bad = 0; const IniLimits r = IniParse( text, &bad );
      const bool changed = !g_iniLoaded || r.teacherMax != g_ini.teacherMax || r.fightCap != g_ini.fightCap;
      g_ini = r; g_iniLoaded = g_iniFresh = true;
      if( changed || bad )
        Log( "Bonus_Tiers.ini: MaxTeachingThreshold = %s (%s), MaxFightingSkillThreshold = %s (%s)%s",
             r.teacherMax ? "True" : "False", r.teacherMax ? "teacher limit" : "no teacher limit",
             r.fightCap ? "True" : "False", r.fightCap ? "weapons up to 100%" : "weapons past 100%", bad ? " - WARNING: unknown lines skipped" : "" );
    }

    inline void ApplyIni() {
      IniRefresh();
      if( zoptions && g_iniLoaded ) {
        if( g_iniFresh ) {
          g_iniFresh = false; g_menuSeenTeach = g_ini.teacherMax ? 1 : 0; g_menuSeenFight = g_ini.fightCap ? 1 : 0;
          if( ReadOpt( "TeacherMax", -1 ) != g_menuSeenTeach ) zoptions->WriteInt( OPT_SECTION, "TeacherMax", g_menuSeenTeach, 0 );
          if( ReadOpt( "FightCap100", -1 ) != g_menuSeenFight ) zoptions->WriteInt( OPT_SECTION, "FightCap100", g_menuSeenFight, 0 );
        } else {
          const int t = ReadOpt( "TeacherMax", g_menuSeenTeach ) ? 1 : 0, f = ReadOpt( "FightCap100", g_menuSeenFight ) ? 1 : 0;
          if( t != g_menuSeenTeach || f != g_menuSeenFight ) {
            g_menuSeenTeach = t; g_menuSeenFight = f; g_ini = { t != 0, f != 0 };
            Union::StringANSI path = GameDir() + "System\\Bonus_Tiers.ini";
            if( !Union::StringANSI::Format( "MaxTeachingThreshold = {0}\r\nMaxFightingSkillThreshold = {1}\r\n", g_ini.teacherMax ? "True" : "False", g_ini.fightCap ? "True" : "False" ).WriteToFile( path, Union::StringBase::ANSI ) )
              Log( "Bonus_Tiers.ini: cannot save the menu change (file locked?)" );
            else if( WIN32_FILE_ATTRIBUTE_DATA fa; GetFileAttributesExA( path, GetFileExInfoStandard, &fa ) ) g_iniStamp = fa.ftLastWriteTime;
            Log( "Union menu -> Bonus_Tiers.ini: MaxTeachingThreshold = %s, MaxFightingSkillThreshold = %s", t ? "True" : "False", f ? "True" : "False" );
          }
        }
      }
      SetScriptInt( "BT_TEACHERMAXOFF", g_ini.teacherMax ? 0 : 1 );
      SetScriptInt( "BT_FIGHTCAPOFF",   g_ini.fightCap ? 0 : 1 );
      SetScriptInt( "BT_SHOWCOSTSPLIT", ReadOpt( "CostSplit", 0 ) ? 1 : 0 );
    }

    inline void ReleaseItem( zCMenuItem* item ) {
#if ENGINE <= Engine_G1A
      if( item->m_iRefCtr > 1 ) item->m_iRefCtr--;
#else
      item->Release();
#endif
    }

    inline void MenuInfoReset() { for( int& s : g_menuInfoShown ) s = -2; }

    inline void LangUpdate() {
      static int s_gameLang = -1;
      const int opt = ReadOpt( "Language", 0 ); int want = LANG_EN;
      if( opt >= 1 && opt <= LANG_COUNT ) want = opt - 1;
      else {
        if( s_gameLang < 0 ) {
          if( zCPar_Symbol* s = Sym( "PRINT_NOTENOUGHLP" ) )         { zSTRING v; s->GetValue( v, 0 ); s_gameLang = LangFromNotEnoughLP( (const unsigned char*)v.ToChar() ); }
          else if( zCPar_Symbol* b = Sym( "NAME_BUYATTRIBUTESTR" ) ) { zSTRING v; b->GetValue( v, 0 ); s_gameLang = LangFromBuyStrength( (const unsigned char*)v.ToChar() ); }
        }
        if( s_gameLang >= 0 ) want = s_gameLang;
      }
      if( want != g_lang ) { g_lang = want; Log( "language: %s (option %d)", LANG_NAME[g_lang], opt ); }
      SetScriptInt( "BT_LANG", g_lang );
    }

    inline void MenuTextWrite( zCMenuItem* it, const MenuText& m ) {
      if( m.t0[g_lang] )  it->SetText( m.t0[g_lang], 0, 1 );
      if( m.t1[g_lang] )  { it->SetText( m.t1[g_lang], 1, 1 ); it->m_parText[1] = m.t1[g_lang]; }
      if( m.us1[g_lang] ) it->m_parUserString[1] = m.us1[g_lang];
    }

    inline void ApplyMenuLanguage() {
      static bool done[MENU_TEXT_COUNT]; static int doneLang = -1, doneCount = 0;
      if( doneLang != g_lang ) { doneLang = g_lang; doneCount = 0; for( bool& d : done ) d = false; }
      if( doneCount == MENU_TEXT_COUNT ) return;
      bool touched = false;
      for( int i = 0; i < MENU_TEXT_COUNT; i++ ) {
        if( done[i] ) continue;
        zCMenuItem* it = zCMenuItem::GetByName( MENU_TEXT[i].item );
        if( !it ) continue;
        MenuTextWrite( it, MENU_TEXT[i] );
#if ENGINE <= Engine_G1A
        for( const auto& m : MENU_TEXT_G1 ) if( strcmp( m.item, MENU_TEXT[i].item ) == 0 ) MenuTextWrite( it, m );
#endif
        ReleaseItem( it ); done[i] = touched = true; doneCount++;
      }
      if( touched ) MenuInfoReset();
    }

    inline void MenuInfoSet( zCMenuItem* it, const zSTRING& info, zCMenu* menu ) {
      if( !it || it->m_parText[1] == info.ToChar() ) return;
      it->m_parText[1] = info;
      if( menu && menu->GetActiveItem() == it ) menu->PrintInformation( info );
    }

    inline void UpdateMenuInfo() {
      zCMenu* menu = zCMenu::GetActive();
      if( !menu || !Union::StringANSI( menu->name.ToChar() ).IsSame( "MENU_OPT_BONUS_TIERS", Union::StringANSI::Flags::IgnoreCase ) ) { MenuInfoReset(); return; }
      for( int i = 0; i < MENU_INFO_COUNT; i++ ) {
        const MenuInfoItem& mi = MENU_INFO_ITEMS[i]; const int v = ReadOpt( mi.option, mi.def );
        if( v == g_menuInfoShown[i] ) continue;
        zCMenuItem* val = zCMenuItem::GetByName( mi.value );
        if( !val ) continue;
        auto parts = Union::StringANSI( val->m_parUserString[1].ToChar() ).Split( '|', false );
        if( (unsigned)v < parts.GetCount() && !parts[v].IsEmpty() ) {
          const zSTRING info( parts[v].ToChar() ); MenuInfoSet( val, info, menu );
          if( zCMenuItem* lab = zCMenuItem::GetByName( mi.label ) ) { MenuInfoSet( lab, info, menu ); ReleaseItem( lab ); }
        }
        ReleaseItem( val ); g_menuInfoShown[i] = v;
      }
    }

    inline bool IsMroczneTajemnice() {
#if ENGINE <= Engine_G1A
      static int s_isMT = -1; if( s_isMT < 0 ) s_isMT = ( Sym( "B_GIVEPROCENTSKILL" ) || Sym( "MIECZ1H" ) ) ? 1 : 0; return s_isMT != 0;
#else
      return false;
#endif
    }
    inline bool G1UsesVanillaAivar() {
#if ENGINE <= Engine_G1A
      return Scan().symbols <= 37000 && !IsMroczneTajemnice();
#else
      return false;
#endif
    }

    inline int HeroStatAccess( oCNpc* hero, int st, const int* set = 0 ) {
      if( !hero ) return 0;
      if( st >= ST_1H && st <= ST_XBOW ) {
#if ENGINE <= Engine_G1A
        const int idx = ( IsMroczneTajemnice() ? 8 : 1 ) + ( st - ST_1H );
        if( set ) hero->SetTalentValue( idx, *set );
        return hero->GetTalentValue( idx );
#else
        int& hc = hero->hitChance[1 + ( st - ST_1H )]; if( set ) hc = *set; return hc;
#endif
      }
      int* atr = st == ST_STR ? &hero->attribute[NPC_ATR_STRENGTH] : st == ST_DEX ? &hero->attribute[NPC_ATR_DEXTERITY] : st == ST_MANA ? &hero->attribute[NPC_ATR_MANAMAX] : 0;
      if( !atr ) return 0;
      if( set ) *atr = *set;
      return *atr;
    }
    inline int  HeroFullStat( oCNpc* hero, int st )           { return HeroStatAccess( hero, st ); }
    inline void HeroSetFullStat( oCNpc* hero, int st, int v ) { HeroStatAccess( hero, st, &v ); }

    inline int ModCounterAccess( int st, const int* set = 0 ) {
      if( st < 0 || st >= ST_COUNT ) return 0;
      const ScanResult& r = Scan(); const ScanCounter& c = r.counter[st];
      if( c.kind == CNT_GLOBAL && parser && c.index >= 0 ) {
        int v = 0; if( zCPar_Symbol* s = parser->GetSymbol( c.index ) ) { if( set ) s->SetValue( *set, 0 ); else s->GetValue( v, 0 ); }
        return v;
      }
      int* slot = ( c.kind == CNT_AIVAR && player && c.index >= 0 && c.index < 100 ) ? &player->aiscriptvars[c.index] : 0;
#if ENGINE >= Engine_G2
      if( !slot && r.symbols <= 0 && player ) slot = &player->aiscriptvars[81 + st];
#endif
      if( !slot ) return 0;
      if( set ) *slot = *set;
      return *slot;
    }
    inline int  ModCounterGet( int st )        { return ModCounterAccess( st ); }
    inline void ModCounterSet( int st, int v ) { ModCounterAccess( st, &v ); }

    inline void G1SyncTrained( oCNpc* hero, int st, int v ) {
#if ENGINE <= Engine_G1A
      s_g1FallbackTrained[st] = v;
      if( G1UsesVanillaAivar() && st < ST_1H ) hero->aiscriptvars[G1_TRAINED_STR + st] = v;
#else
      (void)hero; (void)st; (void)v;
#endif
    }

    inline void EnsureTrained() {
      oCNpc* hero = player; if( !hero ) return;
      zCPar_Symbol* bt = Sym( "BT_TRAINED" );
#if ENGINE >= Engine_G2
      if( !bt ) return;
#endif
      if( bt ? ( ScriptInt( "BT_COUNTERSINIT" ) != 0 ) : ( s_g1FallbackInit == 1 ) ) return;
      for( int st = 0; st < ST_COUNT; st++ ) {
        const int full = HeroFullStat( hero, st ); int v = bt ? ModCounterGet( st ) : full;
#if ENGINE <= Engine_G1A
        if( G1UsesVanillaAivar() && st < ST_1H && hero->aiscriptvars[G1_TRAINED_INIT] == 1 ) v = hero->aiscriptvars[G1_TRAINED_STR + st];
#endif
        if( v <= 0 || v > full ) v = full;
        if( bt ) { bt->SetValue( v, st ); ModCounterSet( st, v ); }
        G1SyncTrained( hero, st, v );
      }
#if ENGINE <= Engine_G1A
      if( G1UsesVanillaAivar() ) hero->aiscriptvars[G1_TRAINED_INIT] = 1;
#endif
      if( bt ) SetScriptInt( "BT_COUNTERSINIT", 1 ); else s_g1FallbackInit = 1;
    }

    inline int TrainedGet( int st ) {
      oCNpc* hero = player; if( !hero || st < 0 || st >= ST_COUNT ) return 0;
      EnsureTrained();
      zCPar_Symbol* bt = Sym( "BT_TRAINED" ); int v = 0;
      if( bt ) bt->GetValue( v, st );
#if ENGINE <= Engine_G1A
      else if( G1UsesVanillaAivar() && st < ST_1H && hero->aiscriptvars[G1_TRAINED_INIT] == 1 ) v = hero->aiscriptvars[G1_TRAINED_STR + st];
      else v = s_g1FallbackTrained[st];
#else
      else v = ModCounterGet( st );
#endif
      if( !g_trainSwapped ) {
        const int full = HeroFullStat( hero, st );
        if( ( v <= 0 && full > 0 ) || v > full ) {
          const int mc = ModCounterGet( st ); v = ( v <= 0 && mc > 0 && mc <= full ) ? mc : full;
          if( bt ) bt->SetValue( v, st );
          G1SyncTrained( hero, st, v ); ModCounterSet( st, v );
        }
      }
      return v;
    }

    inline void TrainedSet( int st, int v ) {
      oCNpc* hero = player; if( !hero || st < 0 || st >= ST_COUNT ) return;
      EnsureTrained();
      if( zCPar_Symbol* bt = Sym( "BT_TRAINED" ) ) bt->SetValue( v, st );
      G1SyncTrained( hero, st, v );
      if( !g_trainSwapped ) ModCounterSet( st, v );
    }

    inline int HpBase( oCNpc* hero ) {
      if( !hero ) return 0;
      const ScanResult& r = Scan();
      if( r.symbols <= 0 ) { const int x = ScriptInt( "HP_PER_LEVEL" ); return HpFromLevels( 40, ( x > 0 && x < 1000 ) ? x : 12, false, 0, hero->level ); }
      int live = 0; bool hasLive = false;
      if( r.hpPerLevelSym >= 0 ) { zCPar_Symbol* s = parser ? parser->GetSymbol( r.hpPerLevelSym ) : 0; if( !s ) return 0; s->GetValue( live, 0 ); hasLive = true; }
      return HpFromLevels( r.hpStart, r.hpPerLevel, hasLive, live, hero->level );
    }

    inline void ScanGame() {
      static int tries = 0;
      if( tries >= 3 || Scan().symbols > 0 || !parser || parser->symtab.table.GetNum() <= 0 ) return;
      tries++;
      const int n = parser->symtab.table.GetNum();
      ScanSym* syms = ( n > 0 && parser->stack.stack && parser->stack.stacksize > 0 ) ? (ScanSym*)calloc( (size_t)n, sizeof( ScanSym ) ) : 0;
      LARGE_INTEGER f, t0, t1; QueryPerformanceFrequency( &f ); QueryPerformanceCounter( &t0 );
      bool ran = false;
      __try {
        if( syms ) {
          for( int i = 0; i < n; i++ ) {
            zCPar_Symbol* s = parser->symtab.table[i]; ScanSym& d = syms[i]; d.parent = -1;
            if( !s ) { d.name = ""; continue; }
            d.name = s->name.ToChar(); d.count = (int)s->ele; d.type = (int)s->type; d.flags = (int)s->flags; d.offset = s->offset;
            if( ( d.type == zPAR_TYPE_FUNC || d.type == zPAR_TYPE_PROTOTYPE || d.type == zPAR_TYPE_INSTANCE ) && !( d.flags & ( zPAR_FLAG_EXTERNAL | zPAR_FLAG_CLASSVAR ) ) ) d.address = s->single_intdata;
            else if( d.type == zPAR_TYPE_INT && !( d.flags & zPAR_FLAG_CLASSVAR ) && d.count > 0 ) {
              if( d.count == 1 ) d.val0 = s->single_intdata; else if( s->intdata ) { d.val0 = s->intdata[0]; d.values = s->intdata; }
            }
          }
          ScanRun( syms, n, parser->stack.stack, parser->stack.stacksize, ( ENGINE <= Engine_G1A ) ? 1 : 2, 0 ); ran = true;
        }
      } __except( EXCEPTION_EXECUTE_HANDLER ) { ran = false; }
      QueryPerformanceCounter( &t1 );
      const int ms = f.QuadPart ? (int)( ( t1.QuadPart - t0.QuadPart ) * 1000 / f.QuadPart ) : 0;
      free( syms );
      if( !ran ) { ScanReset(); Scan().symbols = 0; Log( "scanner: failed (script tables not available or scan crashed); training is not recognised" ); return; }
      ScanResult& r = Scan(); r.seconds = ms / 1000.0;
      Log( "scanner: %d symbols, %d dialogs, %d teachers (%d teach stats), %d wrapped functions, %d training functions, %d talent rules, ok=%d, %d ms",
           r.symbols, r.cInfo, r.teachers, r.statTeachers, r.nWrap, r.nTrainFn, r.nRules, r.ok, ms );
      for( int st = 0; st < ST_COUNT; st++ )
        Log( "scanner: stat %d counter kind %d index %d, tiers measured %d, %d thresholds", st, r.counter[st].kind, r.counter[st].index, r.statTiers[st].measured, r.statTiers[st].n );
      Log( "scanner: hp start %d, per level %d (live symbol %d)", r.hpStart, r.hpPerLevel, r.hpPerLevelSym );
    }

    inline bool EqCounted( oCItem* item ) { return item && ( item->mainflag & ( ITM_CAT_NF | ITM_CAT_FF | ITM_CAT_ARMOR | ITM_CAT_MAGIC ) ) != 0; }

    inline void EqRead( oCNpc* npc, int* v ) {
      v[0] = npc->attribute[NPC_ATR_STRENGTH]; v[1] = npc->attribute[NPC_ATR_DEXTERITY];
      v[2] = npc->attribute[NPC_ATR_MANAMAX];  v[3] = npc->attribute[NPC_ATR_HITPOINTSMAX];
      for( int t = 1; t <= 4; t++ )
#if ENGINE <= Engine_G1A
        v[3 + t] = npc->GetTalentValue( t );
#else
        v[3 + t] = npc->hitChance[t];
#endif
    }

    inline bool EqSync( EqLedger& L, bool write ) {
      zCPar_Symbol *s = Sym( "BT_EQ" ), *k = Sym( "BT_EQ_KNOWN" );
      if( !s || !k ) return false;
      for( int i = 0; i < EQ_N; i++ )         if( write ) s->SetValue( L.sum[i], i );   else { L.sum[i] = 0;   s->GetValue( L.sum[i], i ); }
      for( int i = 0; i < EQ_KNOWN_MAX; i++ ) if( write ) k->SetValue( L.known[i], i ); else { L.known[i] = 0; k->GetValue( L.known[i], i ); }
      if( !write && EqKnownEmpty( L ) ) for( int i = 0; i < EQ_N; i++ ) if( L.sum[i] != 0 ) { L.sum[i] = 0; s->SetValue( 0, i ); }
      return true;
    }

    inline void EqCurrent( int* eq ) { EqLedger L; const bool ok = EqSync( L, false ); for( int i = 0; i < EQ_N; i++ ) eq[i] = ok ? L.sum[i] : 0; }
    inline bool EqBefore( oCNpc* npc ) { if( g_eqSuppress || !npc || npc != player ) return false; int now[EQ_N]; EqRead( npc, now ); return EqPush( g_eqStack, now ); }

    inline void EqAfter( oCNpc* npc, oCItem* item, bool add ) {
      int now[EQ_N], own[EQ_N]; EqRead( npc, now ); EqLedger L = {};
      if( !EqSync( L, false ) ) { EqPop( g_eqStack, L, now, 0, add, false, own ); return; }
      const int r = EqPop( g_eqStack, L, now, item ? item->GetInstance() : 0, add, EqCounted( item ), own );
      if( r == EQ_ADDED || r == EQ_REMOVED ) EqSync( L, true );
      if( r == EQ_NONE ) return;
      bool any = false; for( int x : own ) any = any || x != 0;
      if( !any && ( r == EQ_ADDED || r == EQ_REMOVED ) ) return;
      static const char* const names[EQ_N] = { "Strength", "Dexterity", "Mana", "HP", "1H", "2H", "Bow", "Crossbow" };
      zSTRING nm = item->GetInstanceName();
      Union::StringANSI msg = Union::StringANSI::Format( "equipment: {0} {1}:", add ? "equipped" : "unequipped", nm.ToChar() ? nm.ToChar() : "?" );
      auto list = [&]( const int* v ) { for( int i = 0; i < EQ_N; i++ ) if( v[i] ) msg += Union::StringANSI::Format( " {0} {1}{2}", names[i], v[i] > 0 ? "+" : "", v[i] ); };
      list( own );
      if( r == EQ_OLD )       msg += " - equipped before the plugin, stays in the potion bonus";
      else if( r == EQ_FULL ) msg += " - too many equipped items, skipped";
      else { msg += " | equipment total:"; list( L.sum ); }
      Log( "%s", msg.ToChar() );
    }
  }

  BT_HOOK( oCNpc, AddItemEffects, oCItem* item ) { const bool m = BonusTiers::EqBefore( self ); Ivk_oCNpc_AddItemEffects( self, vt, item ); if( m ) BonusTiers::EqAfter( self, item, true ); }
  BT_HOOK( oCNpc, RemoveItemEffects, oCItem* item ) { const bool m = BonusTiers::EqBefore( self ); Ivk_oCNpc_RemoveItemEffects( self, vt, item ); if( m ) BonusTiers::EqAfter( self, item, false ); }

  namespace BonusTiers {
    struct EqSavedState { oCItem* items[EQ_KNOWN_MAX]; int condVal[EQ_KNOWN_MAX][ITM_COND_MAX], count, curHp, curMana; };

    inline void EqCollectFromList( const zCListSort<oCItem>& list, EqSavedState& st ) {
      for( zCListSort<oCItem>* node = list.GetNextInList(); node; node = node->GetNextInList() ) {
        oCItem* it = node->GetData(); if( !it || !( it->flags & ITM_FLAG_ACTIVE ) || !EqCounted( it ) ) continue;
        bool dup = false; for( int i = 0; i < st.count; i++ ) if( st.items[i] == it ) { dup = true; break; }
        if( !dup && st.count < EQ_KNOWN_MAX ) st.items[st.count++] = it;
      }
    }

    inline void EqUnapplyEquipped( oCNpc* npc, EqSavedState& st ) {
      st.count = 0; st.curHp = npc ? npc->attribute[NPC_ATR_HITPOINTS] : 0; st.curMana = npc ? npc->attribute[NPC_ATR_MANA] : 0;
      EqLedger empty = {}; EqSync( empty, true ); if( !npc ) return;
#if ENGINE <= Engine_G1A
      for( int cat = 0; cat < INV_MAX; cat++ ) EqCollectFromList( npc->inventory2.inventory[cat], st );
#else
      EqCollectFromList( npc->inventory2.inventory, st );
      if( npc->inventory2.contents && npc->inventory2.contents != &npc->inventory2.inventory ) EqCollectFromList( *npc->inventory2.contents, st );
#endif
      if( st.count <= 0 ) return;
      for( int i = 0; i < st.count; i++ ) for( int k = 0; k < ITM_COND_MAX; k++ ) { st.condVal[i][k] = st.items[i]->cond_value[k]; st.items[i]->cond_value[k] = 0; }
      g_eqSuppress = true; for( int i = st.count - 1; i >= 0; i-- ) Ivk_oCNpc_RemoveItemEffects( npc, 0, st.items[i] ); g_eqSuppress = false;
    }

    inline void EqReapplyEquipped( oCNpc* npc, const EqSavedState& st ) {
      if( !npc || st.count <= 0 ) return;
      for( int i = 0; i < st.count; i++ ) Hook_oCNpc_AddItemEffects( npc, 0, st.items[i] );
      for( int i = 0; i < st.count; i++ ) for( int k = 0; k < ITM_COND_MAX; k++ ) st.items[i]->cond_value[k] = st.condVal[i][k];
      const int maxHp = npc->attribute[NPC_ATR_HITPOINTSMAX], maxMana = npc->attribute[NPC_ATR_MANAMAX];
      if( st.curHp > 0 )   npc->attribute[NPC_ATR_HITPOINTS] = st.curHp <= maxHp ? st.curHp : maxHp;
      if( st.curMana > 0 ) npc->attribute[NPC_ATR_MANA]      = st.curMana <= maxMana ? st.curMana : maxMana;
    }

    inline const char* AddrSymName( int addr ) {
      if( !parser || addr <= 0 ) return "?";
      for( int i = 0, n = parser->symtab.table.GetNumInList(); i < n; i++ ) {
        zCPar_Symbol* s = parser->symtab.table[i];
        if( s && s->type == zPAR_TYPE_FUNC && !s->HasFlag( zPAR_FLAG_EXTERNAL ) && s->single_intdata == addr ) return s->name.ToChar() ? s->name.ToChar() : "?";
      }
      return "?";
    }

    inline bool IsInstParam( int funcSym, int p ) { zCPar_Symbol* ps = parser->GetSymbol( funcSym + 1 + p ); return ps && ps->type == zPAR_TYPE_INSTANCE; }

    inline int ParamSlot( int funcSym, int paramIdx ) {
      zCPar_Symbol* fn = ( parser && funcSym >= 0 && paramIdx >= 0 ) ? parser->GetSymbol( funcSym ) : 0;
      if( !fn || paramIdx >= (int)fn->ele ) return -1;
      int above = 0; for( int p = paramIdx + 1; p < (int)fn->ele; p++ ) above += IsInstParam( funcSym, p ) ? 1 : 2;
      const int slot = parser->datastack.sptr - above - 1;
      return ( slot >= 0 && slot < (int)( sizeof( parser->datastack.stack ) / sizeof( int ) ) ) ? slot : -1;
    }

    inline int ReadStackInt( int funcSym, int paramIdx ) {
      const int tok = ParamSlot( funcSym, paramIdx ); if( tok < 1 ) return 0;
      const int t = parser->datastack.stack[tok], raw = parser->datastack.stack[tok - 1];
      return t == zPAR_TOK_PUSHINT ? raw : ( ( t == zPAR_TOK_PUSHVAR && raw ) ? *(const int*)raw : 0 );
    }

    inline void WriteStackInt( int funcSym, int paramIdx, int value ) {
      if( const int tok = ParamSlot( funcSym, paramIdx ); tok >= 1 ) { parser->datastack.stack[tok - 1] = value; parser->datastack.stack[tok] = zPAR_TOK_PUSHINT; }
    }

    struct EngineTrainHost : public TrainHost {
      virtual int  GetStat( int st )                            { return HeroFullStat( player, st ); }
      virtual void SetStat( int st, int v )                     { HeroSetFullStat( player, st, v ); }
      virtual int  GetTrained( int st )                         { return TrainedGet( st ); }
      virtual void SetTrained( int st, int v )                  { TrainedSet( st, v ); }
      virtual void SetCounter( int st, int v )                  { ModCounterSet( st, v ); }
      virtual int  GetLp()                                      { return player ? (int)player->learn_points : 0; }
      virtual void SetLp( int v )                               { if( player ) player->learn_points = v; }
      virtual int  GetParam( const ScanWrap* tf, int p )        { return ( tf && p >= 0 ) ? ReadStackInt( tf->funcSym, p ) : 0; }
      virtual void SetParam( const ScanWrap* tf, int p, int v ) { if( tf && p >= 0 ) WriteStackInt( tf->funcSym, p, v ); }
      virtual int  GetTablets( int st, int totalBonus ) {
        if( st < ST_1H || st >= ST_COUNT ) return 0;
        int eq[EQ_N]; EqCurrent( eq ); const int perm = totalBonus - eq[4 + ( st - ST_1H )]; return perm > 0 ? perm : 0;
      }
      virtual int  GetTalentSkill( int t )        { return player ? player->GetTalentSkill( t ) : 0; }
      virtual void SetTalentSkill( int t, int v ) { if( player ) player->SetTalentSkill( t, v ); }
      virtual void OnFightCapClipped( int st, int keptGain, int refundedLp ) { g_pendingFightCapMsg = true; Log( "training: fight cap 100%% on stat %d (kept +%d, refunded %d LP)", st, keptGain, refundedLp ); }
      virtual void OnNetTriggered( int addr, int st, int gain ) { Log( "training safety net: %s (0x%X) raised stat %d by +%d", AddrSymName( addr ), (unsigned)addr, st, gain ); }
      virtual void OnStuckRestored() { Log( "training: restored stuck swap outside script execution" ); }
    };

    static EngineTrainHost g_trainHost;
    static TrainSession    g_trainSession;
    inline void EnsureTrainInit() { if( !g_trainInited ) { g_trainSession.Init( ST_COUNT ); g_trainInited = true; } }

    struct SavedNpcLimits {
      oCNpc* npc; int attr[6], talVal[NPC_TAL_FIGHT_MAX];
#if ENGINE >= Engine_G2
      int hitChance[NPC_HITCHANCE_MAX];
#endif
    };
    static SavedNpcLimits g_savedNpcLimits[4];
    static int            g_numSavedNpcLimits = 0;

    inline void RaiseNpcTeacherLimits( oCNpc* npc ) {
      if( !npc || npc == player || g_numSavedNpcLimits >= 4 ) return;
      for( int i = 0; i < g_numSavedNpcLimits; i++ ) if( g_savedNpcLimits[i].npc == npc ) return;
      SavedNpcLimits& s = g_savedNpcLimits[g_numSavedNpcLimits++]; s.npc = npc;
      for( int a = 0; a < 6; a++ )                 s.attr[a]   = npc->attribute[a];
      for( int t = 0; t < NPC_TAL_FIGHT_MAX; t++ ) s.talVal[t] = npc->GetTalentValue( t );
      npc->attribute[NPC_ATR_STRENGTH] = npc->attribute[NPC_ATR_DEXTERITY] = npc->attribute[NPC_ATR_MANAMAX] = TRAIN_BIG_LIMIT;
      for( int t = 1; t < NPC_TAL_FIGHT_MAX; t++ ) npc->SetTalentValue( t, TRAIN_BIG_LIMIT );
#if ENGINE >= Engine_G2
      for( int h = 1; h < NPC_HITCHANCE_MAX; h++ ) { s.hitChance[h] = npc->hitChance[h]; npc->hitChance[h] = TRAIN_BIG_LIMIT; }
#endif
    }

    inline void RaiseTeacherLimitsForCall( int addr ) {
      if( !g_trainSession.teacherMaxOff || !g_trainSession.swapped || !parser ) return;
      if( zCPar_Symbol* selfSym = Sym( "SELF" ) ) RaiseNpcTeacherLimits( (oCNpc*)selfSym->GetInstanceAdr() );
      const ScanWrap* tf = ScanFindTrainFn( addr );
      zCPar_Symbol* fn = ( tf && tf->funcSym >= 0 ) ? parser->GetSymbol( tf->funcSym ) : 0;
      for( int p = 0; fn && p < (int)fn->ele; p++ ) {
        const int slot = IsInstParam( tf->funcSym, p ) ? ParamSlot( tf->funcSym, p ) : -1;
        zCPar_Symbol* inst = slot >= 0 ? parser->GetSymbol( parser->datastack.stack[slot] ) : 0;
        if( inst && inst->type == zPAR_TYPE_INSTANCE ) RaiseNpcTeacherLimits( (oCNpc*)inst->GetInstanceAdr() );
      }
    }

    inline void SavePoolsBeforeSwap() {
      g_savedManaAboveShown = g_savedCurHp = 0; if( !player ) return;
      g_savedCurHp = player->attribute[NPC_ATR_HITPOINTS];
      const int curMana = player->attribute[NPC_ATR_MANA], shownMax = player->attribute[NPC_ATR_MANAMAX];
      if( curMana > shownMax ) { g_savedManaAboveShown = curMana - shownMax; player->attribute[NPC_ATR_MANA] = shownMax; }
    }

    inline void RestorePoolsAfterSwap() {
      for( int i = g_numSavedNpcLimits - 1; i >= 0; i-- ) {
        SavedNpcLimits& s = g_savedNpcLimits[i]; if( !s.npc ) continue;
        for( int a = 0; a < 6; a++ )                 s.npc->attribute[a] = s.attr[a];
        for( int t = 0; t < NPC_TAL_FIGHT_MAX; t++ ) s.npc->SetTalentValue( t, s.talVal[t] );
#if ENGINE >= Engine_G2
        for( int h = 1; h < NPC_HITCHANCE_MAX; h++ ) s.npc->hitChance[h] = s.hitChance[h];
#endif
      }
      g_numSavedNpcLimits = 0; if( !player ) return;
      if( g_savedManaAboveShown > 0 ) {
        const int curMana = player->attribute[NPC_ATR_MANA] + g_savedManaAboveShown, maxMana = player->attribute[NPC_ATR_MANAMAX];
        player->attribute[NPC_ATR_MANA] = curMana > maxMana ? maxMana : curMana; g_savedManaAboveShown = 0;
      }
      if( g_savedCurHp > 0 && player->attribute[NPC_ATR_HITPOINTS] < g_savedCurHp && g_savedCurHp <= player->attribute[NPC_ATR_HITPOINTSMAX] )
        player->attribute[NPC_ATR_HITPOINTS] = g_savedCurHp;
      g_savedCurHp = 0;
    }

    inline void CheckCostQuery( int addr ) {
      if( !g_trainSession.swapped || !parser ) return;
      const ScanResult& r = Scan();
      for( int i = 0; i < r.nTrainFn; i++ ) {
        const ScanWrap& tf = r.trainFn[i];
        for( int s = 0; s < ST_COUNT; s++ ) {
          if( tf.statValue[s] < 0 ) continue;
          const ScanTiers& tiers = ScanTiersOf( &tf, s );
          if( tiers.model != COST_PER_POINT_TIMES_N || tiers.costFuncSym < 0 ) continue;
          zCPar_Symbol* cf = parser->GetSymbol( tiers.costFuncSym );
          if( !cf || cf->single_intdata != addr || cf->ele < 2 || ReadStackInt( tiers.costFuncSym, 1 ) != tf.statValue[s] ) continue;
          g_lastCostStat = s; g_lastCostOne = PriceAt( tiers, TrainedGet( s ) ); g_lastCostTiers = &tiers; return;
        }
      }
    }

    inline bool AdjustBuildLearnStringBefore( int addr, char* splitOut, int splitCap ) {
      splitOut[0] = '\0';
      if( !g_trainSession.swapped || g_lastCostStat < 0 || g_lastCostOne <= 0 || !parser ) return false;
      const int blsIdx = parser->GetIndex( "B_BUILDLEARNSTRING" );
      zCPar_Symbol* bls = blsIdx >= 0 ? parser->GetSymbol( blsIdx ) : 0;
      if( !bls || bls->type != zPAR_TYPE_FUNC || bls->ele < 2 || bls->single_intdata != addr ) return false;
      const int kosten = ReadStackInt( blsIdx, 1 ), st = g_lastCostStat, c1 = g_lastCostOne;
      const ScanTiers& tiers = g_lastCostTiers ? *g_lastCostTiers : ScanTiersOf( 0, st );
      g_lastCostStat = -1; g_lastCostOne = 0; g_lastCostTiers = 0;
      if( kosten <= c1 || kosten % c1 != 0 ) return false;
      const int pts = kosten / c1, tr = TrainedGet( st ), fair = FairCost( tiers, tr, pts );
      if( fair > 0 && fair != kosten ) WriteStackInt( blsIdx, 1, fair );
      if( ReadOpt( "CostSplit", 0 ) != 0 && CostSplit( tiers, tr, pts, splitOut, splitCap ) > 1 ) return true;
      splitOut[0] = '\0'; return false;
    }

    inline void TrainFlushFrame() {
      const bool wasSwapped = g_trainSession.swapped; g_trainSession.FrameCheck( g_trainHost );
      if( wasSwapped && !g_trainSession.swapped ) RestorePoolsAfterSwap();
      g_trainSwapped = g_trainSession.swapped;
    }

    inline void TrainRestoreAll() {
      EnsureTrainInit(); g_doStackDepth = 0; g_eqStack.depth = 0;
      if( !player ) { g_trainSession.Reset(); g_trainSwapped = false; return; }
      if( g_trainSession.swapped || g_trainSession.depth > 0 || g_trainSession.netActive ) { g_trainSession.depth = 0; TrainFlushFrame(); }
      g_trainSwapped = false;
    }

    inline void TrainCheckStuck() {
      EnsureTrainInit(); g_doStackDepth = 0;
      if( !PluginEnabled() ) { TrainRestoreAll(); return; }
      g_trainSession.teacherMaxOff = !g_ini.teacherMax; g_trainSession.fightCapOn = g_ini.fightCap;
      if( !player || Scan().symbols <= 0 ) return;
      EnsureTrained(); TrainFlushFrame();
      if( g_pendingFightCapMsg ) { g_pendingFightCapMsg = false; if( parser && Sym( "BT_PRINTFIGHTCAP" ) ) parser->CallFunc( zSTRING( "BT_PRINTFIGHTCAP" ) ); }
    }
  }

  BT_HOOK( zCParser, DoStack, int addr ) {
    using namespace BonusTiers;
    if( self != parser || !player || !PluginEnabled() || Scan().symbols <= 0 ) { Ivk_zCParser_DoStack( self, vt, addr ); return; }
    EnsureTrainInit(); g_trainSession.teacherMaxOff = !g_ini.teacherMax; g_trainSession.fightCapOn = g_ini.fightCap;
    const int callDepth = g_doStackDepth++; const bool wasSwapped = g_trainSession.swapped;
    g_trainSession.OnEnter( g_trainHost, addr, callDepth );
    if( !wasSwapped && g_trainSession.swapped ) { g_trainSwapped = true; g_lastCostStat = -1; g_lastCostOne = 0; g_lastCostTiers = 0; SavePoolsBeforeSwap(); }
    char splitBuf[64] = ""; bool appendSplit = false;
    if( g_trainSession.swapped ) { RaiseTeacherLimitsForCall( addr ); CheckCostQuery( addr ); appendSplit = AdjustBuildLearnStringBefore( addr, splitBuf, sizeof( splitBuf ) ); }
    Ivk_zCParser_DoStack( self, vt, addr );
    if( appendSplit && splitBuf[0] && parser && parser->datastack.sptr >= 2 ) {
      const int tok = parser->datastack.stack[parser->datastack.sptr - 1], raw = parser->datastack.stack[parser->datastack.sptr - 2];
      if( tok == zPAR_TOK_PUSHVAR && raw ) *(zSTRING*)raw += Union::StringANSI::Format( " ({0})", splitBuf ).ToChar();
    }
    if( g_doStackDepth > 0 ) --g_doStackDepth;
    const bool stillSwapped = g_trainSession.swapped; g_trainSession.OnLeave( g_trainHost, addr, callDepth );
    if( stillSwapped && !g_trainSession.swapped ) { RestorePoolsAfterSwap(); g_trainSwapped = false; g_lastCostStat = -1; g_lastCostOne = 0; g_lastCostTiers = 0; }
  }

  namespace BonusTiers {
    inline int MigTold( const char* info ) { const int idx = ( ogame && ogame->infoman && parser ) ? parser->GetIndex( info ) : -1; return ( idx >= 0 && ogame->infoman->InformationTold( idx ) ) ? 1 : 0; }

    inline void MigrateCheck() {
      if( !g_migPending || !player || !Sym( "BT_INIT" ) ) return;
      const int kind = g_migPending, initVer = ScriptInt( "BT_INIT" ); g_migPending = 0;
      if( initVer >= 2 ) return;
      EqSavedState eqState; EqUnapplyEquipped( player, eqState );
      auto initG2 = [&]( bool fromMod ) {
        if( zCPar_Symbol* tr = Sym( "BT_TRAINED" ) ) {
          for( int st = 0; st < ST_COUNT; st++ ) { int v = fromMod ? ModCounterGet( st ) : 0; tr->SetValue( v > 0 ? v : HeroFullStat( player, st ), st ); }
          SetScriptInt( "BT_COUNTERSINIT", 1 );
        }
      };
      if( kind == 1 ) {
#if ENGINE <= Engine_G1A
        EnsureTrained();
#else
        if( ScriptInt( "BT_COUNTERSINIT" ) == 0 ) initG2( false );
#endif
        EqReapplyEquipped( player, eqState ); SetScriptInt( "BT_INIT", 2 ); return;
      }
      if( initVer == 0 ) {
#if ENGINE <= Engine_G1A
        SetScriptInt( "BT_MIGHORATIO", MigTold( "DIA_HORATIO_HELPSTR" ) );
#else
        if( ScriptInt( "BT_COUNTERSINIT" ) == 0 ) initG2( true );
        const int hm = ScriptInt( "MANA_LOUSHAMMER" );
        const int migStr  = ScriptInt( "SHRINE_STR_BONUS" ) + ( ScriptInt( "EROL_BONUS" ) ? 1 : 0 ) + ( ScriptInt( "NEORAS_SCUSEDDRAGONEGGDRINK" ) ? 3 : 0 );
        const int migMana = ScriptInt( "SHRINE_MANA_BONUS" ) + ( ScriptInt( "HAMMER_ONCE" ) ? ( hm > 0 ? hm : 1 ) : 0 ) + ( ScriptInt( "ASTRONOMY_ONCE" ) ? 2 : 0 );
        for( auto [sym, val] : {
          std::pair{ "BT_MIGSTR", migStr }, { "BT_MIGDEX", ScriptInt( "SHRINE_DEX_BONUS" ) }, { "BT_MIGMANA", migMana },
          { "BT_MIG1H", ScriptInt( "LERNE_EINHAND" ) ? 5 : 0 }, { "BT_MIG2H", ScriptInt( "LERNE_ZWEIHAND" ) ? 5 : 0 },
          { "BT_MIGDARON", ScriptInt( "B_DARONSEGEN.BONUS_1" ) ? 1 : 0 }, { "BT_MIGWULFGAR", MigTold( "DIA_WULFGAR_BONUS" ) },
          { "BT_MIGVATRAS", MigTold( "DIA_ADDON_VATRAS_FREE" ) }, { "BT_MIGRENGARU", MigTold( "DIA_RENGARU_ZEICHEN" ) }
        } ) SetScriptInt( sym, val );
#endif
        Log( "old save without the plugin: moving to the bonus what the game remembers" );
#if ENGINE <= Engine_G1A
        const bool keepAiv = !G1UsesVanillaAivar(); int savedAiv[4] = {};
        if( keepAiv ) { for( int k = 0; k < 4; k++ ) savedAiv[k] = player->aiscriptvars[G1_TRAINED_STR + k]; player->aiscriptvars[G1_TRAINED_INIT] = 0; }
#endif
        parser->CallFunc( zSTRING( "BT_MIGRATEOLD" ) );
#if ENGINE <= Engine_G1A
        if( zCPar_Symbol* bt = Sym( "BT_TRAINED" ) ) {
          for( int st = 0; st < ST_COUNT; st++ ) {
            int v = ( st < ST_1H && player->aiscriptvars[G1_TRAINED_INIT] == 1 ) ? player->aiscriptvars[G1_TRAINED_STR + st] : HeroFullStat( player, st );
            bt->SetValue( v, st ); ModCounterSet( st, v );
          }
          SetScriptInt( "BT_COUNTERSINIT", 1 );
        }
        if( keepAiv ) for( int k = 0; k < 4; k++ ) player->aiscriptvars[G1_TRAINED_STR + k] = savedAiv[k];
#endif
      }
      if( zCPar_Symbol* btSym = Sym( "BT_TRAINED" ) )
        for( int st = 0; st < ST_COUNT; st++ ) {
          const int unequippedFull = HeroFullStat( player, st ); int trVal = 0; btSym->GetValue( trVal, st );
          if( trVal > unequippedFull && unequippedFull > 0 ) { btSym->SetValue( unequippedFull, st ); ModCounterSet( st, unequippedFull ); }
        }
      EqReapplyEquipped( player, eqState ); SetScriptInt( "BT_INIT", 2 );
    }

    struct ShownTexts { char atr[4][64], tal[4][64]; int nAtr, nTal; };
    static ShownTexts g_shown = {};
    inline bool IsTalentItem( const char* name ) { return Union::StringANSI( name ).StartsWith( "MENU_ITEM_TALENT_" ); }

    inline void SetItemText( const char* name, const char* text ) {
      if( zCMenuItem* item = zCMenuItem::GetByName( name ) ) {
        item->SetText( text, 0, 1 ); ReleaseItem( item );
        char* slot = ( Union::StringANSI( name ).StartsWith( "MENU_ITEM_ATTRIBUTE_" ) && g_shown.nAtr < 4 ) ? g_shown.atr[g_shown.nAtr++]
                   : ( IsTalentItem( name ) && g_shown.nTal < 4 ) ? g_shown.tal[g_shown.nTal++] : 0;
        if( slot ) lstrcpynA( slot, text, 64 );
      }
    }

    inline LayoutPlan ShownPlan() {
      const char *atr[4], *tal[4];
      for( int i = 0; i < g_shown.nAtr; i++ ) atr[i] = g_shown.atr[i];
      for( int i = 0; i < g_shown.nTal; i++ ) tal[i] = g_shown.tal[i];
      return LayoutPlanAuto( atr, g_shown.nAtr, tal, g_shown.nTal );
    }

    inline bool CanShowStat( const char* name, int total, int base ) {
      const bool tal = IsTalentItem( name );
      if( base <= 0 && !( IsMroczneTajemnice() && tal && total > 0 ) ) return false;
      if( !tal ) return true;
      zCMenuItem* item = zCMenuItem::GetByName( name ); if( !item ) return false;
      const char* t = item->m_parText[0].ToChar();
      const bool ok = ( ENGINE <= Engine_G1A && IsMroczneTajemnice() && ( !t || !*t ) ) || ( t && strpbrk( t, "0123456789" ) != 0 );
      ReleaseItem( item ); return ok;
    }

    inline void SetSlash( const char* name, int total, int trained, const char* suffix ) {
      if( CanShowStat( name, total, trained ) ) SetItemText( name, Union::StringANSI::Format( "{0}/{1}{2}", trained, total, suffix ) );
    }

    inline void SetPlus( const char* name, const char* shown, int bonus, int eq = 0 ) {
      char out[64], c[64]; EqFormat( out, sizeof( out ), shown, bonus, eq );
      if( IsTalentItem( name ) ? g_compactTal : g_compactAtr ) { LayoutCompact( out, c, sizeof( c ) ); lstrcpynA( out, c, sizeof( out ) ); }
      SetItemText( name, out );
    }

    inline void ShowValue( const char* name, int total, int learned, const char* suffix, int eq = 0 ) {
      if( CanShowStat( name, total, learned ) ) SetPlus( name, Union::StringANSI::Format( "{0}{1}", total, suffix ), total - learned, eq );
    }
    inline void ShowPool( const char* name, int cur, int max, int base, int eq = 0 ) {
      if( base > 0 ) SetPlus( name, Union::StringANSI::Format( "{0}/{1}", cur, max ), max - base, eq );
    }

    inline void ApplyFormat( oCNpc* hero, const int* trained, const int* full, const int* eq, int hpBase ) {
      const int view = StatusView(); if( view == VIEW_VANILLA ) return;
      if( view == VIEW_SLASH ) {
        for( int i = ST_STR; i <= ST_MANA; i++ ) SetSlash( Union::StringANSI::Format( "MENU_ITEM_ATTRIBUTE_{0}", i + 1 ), full[i], trained[i], "" );
        SetSlash( "MENU_ITEM_ATTRIBUTE_4", hero->attribute[NPC_ATR_HITPOINTSMAX], hpBase, "" );
        for( int i = 0; i < 4; i++ ) SetSlash( Union::StringANSI::Format( "MENU_ITEM_TALENT_{0}", i + 1 ), full[ST_1H + i], trained[ST_1H + i], "%" );
        return;
      }
      ShowValue( "MENU_ITEM_ATTRIBUTE_1", full[ST_STR], trained[ST_STR], "", eq[0] );
      ShowValue( "MENU_ITEM_ATTRIBUTE_2", full[ST_DEX], trained[ST_DEX], "", eq[1] );
      ShowPool( "MENU_ITEM_ATTRIBUTE_3", hero->attribute[NPC_ATR_MANA], hero->attribute[NPC_ATR_MANAMAX], trained[ST_MANA], eq[2] );
      ShowPool( "MENU_ITEM_ATTRIBUTE_4", hero->attribute[NPC_ATR_HITPOINTS], hero->attribute[NPC_ATR_HITPOINTSMAX], hpBase, eq[3] );
      for( int i = 0; i < 4; i++ ) ShowValue( Union::StringANSI::Format( "MENU_ITEM_TALENT_{0}", i + 1 ), full[ST_1H + i], trained[ST_1H + i], "%", eq[4 + i] );
    }

    inline void FillAndWidenStatus( oCMenu_Status* menu ) {
      if( oCNpc* hero = player ) {
        EnsureTrained(); int eq[EQ_N], trained[ST_COUNT], full[ST_COUNT]; EqCurrent( eq ); const bool isMT = IsMroczneTajemnice();
        for( int i = 0; i < ST_COUNT; i++ ) { trained[i] = TrainedGet( i ); full[i] = HeroFullStat( hero, i ) + ( ( isMT && i >= ST_1H ) ? eq[4 + ( i - ST_1H )] : 0 ); }
        const int hpBase = HpBase( hero );
        Log( "character screen: Strength %d (trained %d), Dexterity %d (%d), Mana %d (%d), HP %d (from levels %d), 1H %d (%d), 2H %d (%d), Bow %d (%d), Crossbow %d (%d), LP %d",
             full[ST_STR], trained[ST_STR], full[ST_DEX], trained[ST_DEX], full[ST_MANA], trained[ST_MANA], hero->attribute[NPC_ATR_HITPOINTSMAX], hpBase, full[ST_1H], trained[ST_1H], full[ST_2H], trained[ST_2H], full[ST_BOW], trained[ST_BOW], full[ST_XBOW], trained[ST_XBOW], (int)hero->learn_points );
        g_shown.nAtr = g_shown.nTal = 0; g_compactAtr = g_compactTal = false;
        ApplyFormat( hero, trained, full, eq, hpBase );
        if( ReadOpt( "StatusWidth", 0 ) < 1 ) {
          const LayoutPlan plan = ShownPlan();
          if( plan.compactAtr || plan.compactTal ) { g_shown.nAtr = g_shown.nTal = 0; g_compactAtr = plan.compactAtr; g_compactTal = plan.compactTal; ApplyFormat( hero, trained, full, eq, hpBase ); }
        }
      }
      g_statusMenu = menu; g_layoutNotR = false; if( !menu ) return;
#if ENGINE >= Engine_G2
      bool notr = menu->dimx == LAYOUT_NOTR_DIMX;
      if( notr ) if( zCMenuItem* a1 = zCMenuItem::GetByName( "MENU_ITEM_ATTRIBUTE_1" ) ) { notr = a1->m_parPosX == LAYOUT_NOTR_ATR_VAL_X || a1->m_parPosX == g_lastA2; ReleaseItem( a1 ); }
      if( !notr ) { static bool otherLogged = false; if( !otherLogged ) { otherLogged = true; Log( "character screen: other layout (a mod?) - window not widened" ); } return; }
#endif
      g_layoutNotR = true; g_appliedPermille = -1; const int view = StatusView();
      g_wantPermille = LayoutWidthPermille( ReadOpt( "StatusWidth", 0 ), view, view == VIEW_VANILLA ? 1000 : ShownPlan().permille );
    }
  }

  BT_HOOK0( zCMenu, UpdateWindowGeometry ) {
    using namespace BonusTiers; Ivk_zCMenu_UpdateWindowGeometry( self, vt );
    if( !g_layoutNotR || !g_statusMenu || self != (zCMenu*)g_statusMenu ) return;
    const int base = self->m_use_dimx; int pm = g_wantPermille;
    if( base <= 0 || base > 8192 ) pm = 1000;
    else if( pm > 1000 ) {
      int target = base * pm / 1000; const int maxUse = 8192 * 96 / 100;
      if( target > maxUse ) { target = maxUse > base ? maxUse : base; pm = target * 1000 / base; }
      self->m_use_dimx = target;
      if( self->flags & MN_ALIGN_CENTER ) self->m_use_posx = 0x1000 - target / 2;
      else if( self->m_use_posx + target > 8192 ) self->m_use_posx = 8192 - target;
    }
    if( pm != g_appliedPermille ) {
      const int a2 = LayoutAtrVal( pm ), b2 = LayoutTalSkill( pm ), b3 = LayoutTalVal( pm );
      for( const auto& id : g_statusMenu->itemID ) {
        if( id.IsEmpty() ) continue;
        zCMenuItem* it = zCMenuItem::GetByName( id ); if( !it ) continue;
        int& x = it->m_parPosX;
        if( x == LAYOUT_NOTR_ATR_VAL_X || x == g_lastA2 )        x = a2;
        else if( x == LAYOUT_NOTR_TAL_SKILL_X || x == g_lastB2 ) x = b2;
        else if( x == LAYOUT_NOTR_TAL_VAL_X || x == g_lastB3 )   x = b3;
        ReleaseItem( it );
      }
      g_lastA2 = a2; g_lastB2 = b2; g_lastB3 = b3; g_appliedPermille = pm;
    }
    if( pm != g_loggedPermille ) { g_loggedPermille = pm; Log( "character screen: window width %d%%", pm / 10 ); }
  }

  BT_HOOK0( oCMenu_Status, InitForDisplay ) { Ivk_oCMenu_Status_InitForDisplay( self, vt ); BonusTiers::FillAndWidenStatus( self ); }
  inline void CommonTick() { BonusTiers::EnsureDefaults(); BonusTiers::ScanGame(); BonusTiers::LangUpdate(); BonusTiers::ApplyIni(); }
  inline void OnLoadEnd( bool newGame ) { BonusTiers::s_g1FallbackInit = 0; BonusTiers::FlushScriptLog(); BonusTiers::g_migPending = newGame ? 1 : 2; BonusTiers::MigrateCheck(); BonusTiers::FlushScriptLog(); }

  BT_HOOK0( oCGame, Init ) {
    Ivk_oCGame_Init( self, vt ); BonusTiers::Log( "%s %s started", BonusTiers::PLUGIN_NAME, BonusTiers::PLUGIN_VERSION );
    AddVectoredExceptionHandler( 1, BonusTiers::CrashObserver ); BonusTiers::EnsureDefaults(); BonusTiers::ScanGame();
  }
  void __fastcall oCGame_MainWorld_Render();
  auto Partial_zCView_Render = Union::CreatePartialHook( (void*)( zSwitch( 0x0063DC76, 0x0066498B, 0x0066BA76, 0x006C87EB ) ), &oCGame_MainWorld_Render );
  void __fastcall oCGame_MainWorld_Render() { CommonTick(); BonusTiers::TrainCheckStuck(); BonusTiers::CheckScript(); BonusTiers::MigrateCheck(); BonusTiers::FlushScriptLog(); }
  BT_HOOK0( zCMenu, Render ) { Ivk_zCMenu_Render( self, vt ); CommonTick(); BonusTiers::ApplyMenuLanguage(); BonusTiers::UpdateMenuInfo(); }
  BT_HOOK( oCGame, LoadGame, int slot, const zSTRING& levelPath ) { BonusTiers::TrainRestoreAll(); Ivk_oCGame_LoadGame( self, vt, slot, levelPath ); OnLoadEnd( true ); }
  BT_HOOK( oCGame, LoadSavegame, int slot, int loadGlobals ) { BonusTiers::TrainRestoreAll(); Ivk_oCGame_LoadSavegame( self, vt, slot, loadGlobals ); OnLoadEnd( false ); }
  BT_HOOK( oCGame, WriteSavegame, int slot, int saveGlobals ) { BonusTiers::TrainRestoreAll(); Ivk_oCGame_WriteSavegame( self, vt, slot, saveGlobals ); }
  BT_HOOK( oCGame, ChangeLevel, const zSTRING& levelpath, const zSTRING& startpoint ) { BonusTiers::TrainRestoreAll(); Ivk_oCGame_ChangeLevel( self, vt, levelpath, startpoint ); BonusTiers::FlushScriptLog(); }
}
