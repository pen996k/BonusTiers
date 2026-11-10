#pragma once

#include <initializer_list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Bonus_Tiers_ScanTypes.h"

namespace BonusTiers {
  enum { SCAN_TYPE_VOID = 0, SCAN_TYPE_FLOAT = 1, SCAN_TYPE_INT = 2, SCAN_TYPE_STRING = 3, SCAN_TYPE_CLASS = 4, SCAN_TYPE_FUNC = 5, SCAN_TYPE_PROTOTYPE = 6, SCAN_TYPE_INSTANCE = 7 };
  enum { SCAN_FLAG_CONST = 1, SCAN_FLAG_RETURN = 2, SCAN_FLAG_CLASSVAR = 4, SCAN_FLAG_EXTERNAL = 8 };
  struct ScanSym { const char* name; int count, type, flags, offset, address, parent, val0; const int* values; };
  inline int ScanSymInt( const ScanSym& s, int idx ) { return ( s.values && idx >= 0 && idx < s.count ) ? s.values[idx] : s.val0; }

  template<typename T> struct ScanVec {
    T* data; int len, cap;
    void Init()  { data = 0; len = cap = 0; }
    void Free()  { free( data ); data = 0; len = cap = 0; }
    void Clear() { len = 0; }
    void Push( const T& v ) {
      if( len >= cap ) { int ncap = cap ? cap * 2 : 16; if( T* nd = (T*)realloc( data, (size_t)ncap * sizeof( T ) ) ) { data = nd; cap = ncap; } else return; }
      data[len++] = v;
    }
    T Pop() { T empty = {}; return len > 0 ? data[--len] : empty; }
    T& operator[]( int i ) { return data[i]; }
    const T& operator[]( int i ) const { return data[i]; }
    bool Has( const T& v ) const { for( int i = 0; i < len; i++ ) if( data[i] == v ) return true; return false; }
    void AddUnique( const T& v ) { if( !Has( v ) ) Push( v ); }
    template<typename Cmp> void Sort( Cmp cmp ) {
      for( int i = 1, j; i < len; i++ ) { T v = data[i]; for( j = i - 1; j >= 0 && cmp( data[j], v ) > 0; j-- ) data[j + 1] = data[j]; data[j + 1] = v; }
    }
    template<typename Cmp> void InsertUnique( const T& v, Cmp cmp ) {
      int pos = 0; for( ; pos < len; pos++ ) { const int c = cmp( data[pos], v ); if( c == 0 ) return; if( c > 0 ) break; }
      const int prev = len; Push( v ); if( len == prev ) return;
      for( int j = len - 1; j > pos; j-- ) data[j] = data[j - 1];
      data[pos] = v;
    }
  };
  template<typename... Vs> inline void InitVecs( Vs&... vs ) { int d[] = { ( vs.Init(), 0 )... }; (void)d; }
  template<typename... Vs> inline void FreeVecs( Vs&... vs ) { int d[] = { ( vs.Free(), 0 )... }; (void)d; }

  inline char ScanUpperChar( char c ) { return ( c >= 'a' && c <= 'z' ) ? (char)( c - 'a' + 'A' ) : c; }
  inline void ScanToUpper( const char* src, char* dst, int cap ) { int i = 0; for( ; src && src[i] && i < cap - 1; i++ ) dst[i] = ScanUpperChar( src[i] ); if( cap > 0 ) dst[i] = '\0'; }
  inline bool ScanStrEq( const char* a, const char* b )      { return a && b && _stricmp( a, b ) == 0; }
  inline bool ScanStartsWith( const char* s, const char* p ) { return s && p && _strnicmp( s, p, strlen( p ) ) == 0; }
  inline bool ScanEndsWith( const char* s, const char* f )   { const int ls = s ? (int)strlen( s ) : 0, lf = f ? (int)strlen( f ) : 0; return ls >= lf && _stricmp( s + ls - lf, f ) == 0; }
  inline bool ScanIsPluginSym( const char* n )               { return n && n[0] && ( ScanStartsWith( n, "BT_" ) || ScanEndsWith( n, "_OLD" ) ); }

  inline int ScanNextOp( const unsigned char* code, int end, int& pos, int& arg, int& idx ) {
    const int op = code[pos]; arg = idx = 0;
    if( op == 61 || op == 62 || op == 64 || op == 65 || op == 67 || op == 75 || op == 76 || op == 80 ) { if( pos + 5 > end ) return -1; memcpy( &arg, code + pos + 1, 4 ); pos += 5; }
    else if( op == 245 ) { if( pos + 6 > end ) return -1; memcpy( &arg, code + pos + 1, 4 ); idx = code[pos + 5]; pos += 6; }
    else pos++;
    return op;
  }
  struct ScanRawOp { int op, arg, idx; };
  inline void ScanCollectRawOps( const unsigned char* code, int start, int end, ScanVec<ScanRawOp>& ops ) {
    ops.Init(); for( int pos = start, arg = 0, idx = 0, op; pos < end && ( op = ScanNextOp( code, end, pos, arg, idx ) ) >= 0; ) ops.Push( { op, arg, idx } );
  }

  enum { INST_NONE = 0, INST_PARAM = 1, INST_OTHER = 2, INST_HERO = 3, INST_SELF = 4, INST_SYM = 5 };
  struct ScanInstRef { int kind, idx; };
  inline ScanInstRef MakeInst( int kind, int idx = 0 ) { return { kind, idx }; }
  inline bool InstEq( const ScanInstRef& a, const ScanInstRef& b ) { return a.kind == b.kind && a.idx == b.idx; }
  inline bool InstIsHeroish( const ScanInstRef& a ) { return a.kind == INST_OTHER || a.kind == INST_HERO; }

  enum { OPND_UNK = 0, OPND_CONST = 1, OPND_PARAM = 2, OPND_GLOBAL = 3, OPND_INST = 4 };
  struct ScanOperand { int kind, val; ScanInstRef inst; };
  inline ScanOperand MakeOpnd( int kind, int val = 0 )       { return { kind, val, { INST_NONE, 0 } }; }
  inline ScanOperand MakeInstOpnd( const ScanInstRef& inst ) { return { OPND_INST, 0, inst }; }
  inline bool OpndEq( const ScanOperand& a, const ScanOperand& b ) { return a.kind == b.kind && ( a.kind == OPND_UNK || ( a.kind == OPND_INST ? InstEq( a.inst, b.inst ) : a.val == b.val ) ); }

  enum { FLD_NONE = 0, FLD_NPC_LP, FLD_NPC_ATTRIBUTE, FLD_NPC_HITCHANCE, FLD_NPC_AIVAR, FLD_NPC_LEVEL, FLD_NPC_OTHER, FLD_INFO_NPC, FLD_INFO_INFORMATION, FLD_INFO_CONDITION, FLD_INFO_DESCRIPTION, FLD_INFO_TRADE, FLD_INFO_PERMANENT, FLD_INFO_IMPORTANT, FLD_INFO_OTHER };
  struct ScanFieldRef { int field, idx; ScanInstRef inst; };
  inline bool FieldRefEq( const ScanFieldRef& a, const ScanFieldRef& b ) { return a.field == b.field && a.idx == b.idx && InstEq( a.inst, b.inst ); }
  inline int  FieldRefCmp( const ScanFieldRef& a, const ScanFieldRef& b ) { return a.field != b.field ? a.field - b.field : a.idx != b.idx ? a.idx - b.idx : a.inst.kind != b.inst.kind ? a.inst.kind - b.inst.kind : a.inst.idx - b.inst.idx; }

  struct ScanGuard { int n, p[8], v[8]; };
  inline void GuardInit( ScanGuard& g ) { g.n = 0; }
  inline void GuardAdd( ScanGuard& g, int param, int val ) {
    for( int i = 0; i < g.n; i++ ) {
      if( g.p[i] == param && g.v[i] == val ) return;
      if( g.p[i] > param || ( g.p[i] == param && g.v[i] > val ) ) {
        if( g.n < 8 ) { for( int j = g.n++; j > i; j-- ) { g.p[j] = g.p[j - 1]; g.v[j] = g.v[j - 1]; } g.p[i] = param; g.v[i] = val; }
        return;
      }
    }
    if( g.n < 8 ) { g.p[g.n] = param; g.v[g.n++] = val; }
  }
  inline bool GuardEq( const ScanGuard& a, const ScanGuard& b ) {
    if( a.n != b.n ) return false;
    for( int i = 0; i < a.n; i++ ) if( a.p[i] != b.p[i] || a.v[i] != b.v[i] ) return false;
    return true;
  }

  template<typename T, typename Cmp> inline void ScanInsertUnique( T* arr, int& n, int cap, const T& val, Cmp cmp ) {
    int pos = 0; for( ; pos < n; pos++ ) { const int c = cmp( arr[pos], val ); if( c == 0 ) return; if( c > 0 ) break; }
    if( n >= cap ) return;
    for( int j = n++; j > pos; j-- ) arr[j] = arr[j - 1];
    arr[pos] = val;
  }
  template<typename T> inline void PushByEndPos( ScanVec<T>& vec, const T& item ) {
    const int prev = vec.len; vec.Push( item ); if( vec.len == prev ) return;
    int ins = prev; for( ; ins > 0 && vec[ins - 1].endPos < item.endPos; ins-- ) vec[ins] = vec[ins - 1];
    vec[ins] = item;
  }

  struct ScanThrCond { int field, idx, op, val; };
  inline int ThrCondCmp( const ScanThrCond& a, const ScanThrCond& b ) { return a.field != b.field ? a.field - b.field : a.idx != b.idx ? a.idx - b.idx : a.op != b.op ? a.op - b.op : a.val - b.val; }
  struct ScanThrSet { int n; ScanThrCond c[8]; };
  inline void ThrSetInit( ScanThrSet& ts ) { ts.n = 0; }
  inline void ThrSetAdd( ScanThrSet& ts, const ScanThrCond& cond ) { ScanInsertUnique( ts.c, ts.n, 8, cond, ThrCondCmp ); }
  inline bool ThrSetEq( const ScanThrSet& a, const ScanThrSet& b ) {
    if( a.n != b.n ) return false;
    for( int i = 0; i < a.n; i++ ) if( ThrCondCmp( a.c[i], b.c[i] ) != 0 ) return false;
    return true;
  }

  enum { ITEM_LPDEC = 1, ITEM_LEVELINC, ITEM_READ, ITEM_STATINC, ITEM_AIVARINC, ITEM_GLOBALINC, ITEM_LIMIT, ITEM_CHOICE, ITEM_PRICEFN, ITEM_TALENTSET };
  struct ScanItem { int kind, field, idx; ScanInstRef inst; ScanOperand op1, op2; ScanGuard guard; ScanThrSet thr; };
  inline ScanItem MakeEmptyItem( int kind ) { ScanItem it = {}; it.kind = kind; return it; }

  inline bool ItemEq( const ScanItem& a, const ScanItem& b ) {
    if( a.kind != b.kind ) return false;
    switch( a.kind ) {
      case ITEM_LPDEC: case ITEM_LEVELINC: return InstEq( a.inst, b.inst );
      case ITEM_READ:      return InstEq( a.inst, b.inst ) && a.field == b.field && a.idx == b.idx;
      case ITEM_STATINC:   return InstEq( a.inst, b.inst ) && a.field == b.field && OpndEq( a.op1, b.op1 ) && OpndEq( a.op2, b.op2 ) && GuardEq( a.guard, b.guard );
      case ITEM_AIVARINC:  return InstEq( a.inst, b.inst ) && a.idx == b.idx && OpndEq( a.op2, b.op2 ) && GuardEq( a.guard, b.guard );
      case ITEM_GLOBALINC: return a.idx == b.idx && OpndEq( a.op2, b.op2 ) && GuardEq( a.guard, b.guard );
      case ITEM_LIMIT:     return OpndEq( a.op1, b.op1 ) && GuardEq( a.guard, b.guard );
      case ITEM_CHOICE:    return OpndEq( a.op1, b.op1 );
      case ITEM_PRICEFN:   return a.idx == b.idx;
      case ITEM_TALENTSET: return InstEq( a.inst, b.inst ) && OpndEq( a.op1, b.op1 ) && OpndEq( a.op2, b.op2 ) && ThrSetEq( a.thr, b.thr );
      default: return false;
    }
  }
  inline void ItemSetAdd( ScanVec<ScanItem>& set, const ScanItem& it ) { for( int i = 0; i < set.len; i++ ) if( ItemEq( set[i], it ) ) return; set.Push( it ); }

  struct ScanCallSite    { int calleeSym, nArgs; ScanOperand args[16]; ScanGuard guard; };
  struct ScanCostSummary { bool isPerPoint; int nCalls, calls[8]; };
  struct ScanDirectBody  { bool evaluated, hasInfoAssign; ScanVec<ScanItem> items; ScanVec<ScanCallSite> calls; ScanVec<ScanCostSummary> costs; ScanOperand infoNpc, infoInformation, infoCondition; };

  enum { EXPR_UNK = 0, EXPR_CONST, EXPR_PARAM, EXPR_GLOBAL, EXPR_STR, EXPR_INST, EXPR_FIELD, EXPR_LOCAL, EXPR_COMPACT, EXPR_BINOP, EXPR_UNOP, EXPR_CALLRET };
  struct ScanExprNode { int kind, val, left, right, nDefs, defs[16], nFields, nCalls, ccalls[16]; ScanInstRef inst; ScanFieldRef field, cfields[16]; };

  inline bool ExprCompactEq( const ScanExprNode& a, const ScanExprNode& b ) {
    if( a.kind != b.kind ) return false;
    if( a.kind == EXPR_UNK ) return true;
    if( a.kind == EXPR_CONST || a.kind == EXPR_PARAM || a.kind == EXPR_GLOBAL || a.kind == EXPR_STR ) return a.val == b.val;
    if( a.kind != EXPR_COMPACT || a.nFields != b.nFields || a.nCalls != b.nCalls ) return false;
    for( int i = 0; i < a.nFields; i++ ) if( !FieldRefEq( a.cfields[i], b.cfields[i] ) ) return false;
    for( int i = 0; i < a.nCalls; i++ )  if( a.ccalls[i] != b.ccalls[i] ) return false;
    return true;
  }

  struct ScanTeachOpt    { int optSym, nLabels, nLimits, limits[8]; char labels[8][24]; };
  struct ScanTeacherInfo { int npcSym; char name[64]; ScanVec<int> dialogs, limits; ScanVec<ScanTeachOpt> teach; };
  struct ScanGenericFn   { int funcSym, npcParam, model, nCostFns, costFns[4]; bool directLp; };
  struct ScanNameCount   { int sym, count; };
  struct ScanCostThresh  { int funcSym, n, th[16]; };

  struct ScanExt {
    double readSeconds;
    ScanVec<ScanTeacherInfo> teachers; ScanVec<ScanGenericFn> generic;
    int realCounter[ST_COUNT], nRuleStrings;
    ScanVec<ScanNameCount> costFns, priceTextFns; ScanVec<ScanCostThresh> costThresholds;
    char ruleStrings[64][80], hpStartStr[64], hpLevelStr[64];
    void Init() { readSeconds = 0.0; InitVecs( teachers, generic, costFns, costThresholds, priceTextFns ); for( int i = 0; i < ST_COUNT; i++ ) realCounter[i] = -1; nRuleStrings = 0; hpStartStr[0] = hpLevelStr[0] = '\0'; }
    void Free() { for( int i = 0; i < teachers.len; i++ ) FreeVecs( teachers[i].dialogs, teachers[i].teach, teachers[i].limits ); FreeVecs( teachers, generic, costFns, costThresholds, priceTextFns ); }
  };
  inline void AddNameCount( ScanVec<ScanNameCount>& vec, int sym ) { for( int i = 0; i < vec.len; i++ ) if( vec[i].sym == sym ) { vec[i].count++; return; } vec.Push( { sym, 1 } ); }

  struct ScanAddrMapEntry  { int addr, symIdx; };
  struct ScanLocalDefEntry { int symIdx, nDefs, defNodes[8], rawCount, rawFirst; };
  struct ScanAliasEntry    { int symIdx; ScanInstRef inst; };
  struct ScanActiveGuard   { int endPos, param, val; };
  struct ScanActiveThr     { int endPos; ScanThrCond cond; };
  inline ScanGuard MakeGuardFromActive( const ScanVec<ScanActiveGuard>& guards ) { ScanGuard g; GuardInit( g ); for( int i = 0; i < guards.len; i++ ) GuardAdd( g, guards[i].param, guards[i].val ); return g; }

  struct ScanContext {
    const ScanSym* syms; int nSyms; const unsigned char* code; int codeSize;
    unsigned char *ignored, *memoDone, *memoBusy;
    int *fldKind, *starts, nStarts, nAddrMap;
    ScanAddrMapEntry* addrMap;
    int symSelf, symOther, symHero, symPcHero;
    int extAddChoice, extChangeAttr, extSetTalent, extGetTalent, extSetTalentVal, extGetTalentVal, extHasItems, extIsPlayer, extIsValidNpc, extGetAtrCost;
    int statConstVal[ST_COUNT], learnedSym[ST_COUNT], realAttrSym[3], nLevelCap, levelCapSym[16];
    ScanDirectBody* direct; ScanVec<ScanItem>* memoItems; ScanVec<ScanExprNode> exprPool;

    void Init( const ScanSym* s, int ns, const unsigned char* c, int cs ) {
      syms = s; nSyms = ns; code = c; codeSize = cs;
      ignored = (unsigned char*)calloc( (size_t)ns + 1, 1 );
      fldKind = (int*)calloc( (size_t)ns + 1, sizeof( int ) );
      starts = (int*)malloc( ( (size_t)ns + 1 ) * sizeof( int ) );
      addrMap = (ScanAddrMapEntry*)malloc( ( (size_t)ns + 1 ) * sizeof( ScanAddrMapEntry ) );
      nStarts = nAddrMap = nLevelCap = 0;
      symSelf = symOther = symHero = symPcHero = -1;
      extAddChoice = extChangeAttr = extSetTalent = extGetTalent = extSetTalentVal = extGetTalentVal = extHasItems = extIsPlayer = extIsValidNpc = extGetAtrCost = -1;
      for( int i = 0; i < ST_COUNT; i++ ) learnedSym[i] = -1;
      for( int i = 0; i < 3; i++ )  realAttrSym[i] = -1;
      for( int i = 0; i < 16; i++ ) levelCapSym[i] = -1;
      const int defStatConsts[ST_COUNT] = { 4, 5, 3, 1, 2, 3, 4 };
      for( int i = 0; i < ST_COUNT; i++ ) statConstVal[i] = defStatConsts[i];
      direct = (ScanDirectBody*)calloc( (size_t)ns + 1, sizeof( ScanDirectBody ) );
      memoDone = (unsigned char*)calloc( (size_t)ns + 1, 1 );
      memoBusy = (unsigned char*)calloc( (size_t)ns + 1, 1 );
      memoItems = (ScanVec<ScanItem>*)calloc( (size_t)ns + 1, sizeof( ScanVec<ScanItem> ) );
      exprPool.Init(); IndexSymbols();
    }
    void Free() {
      for( int i = 0; i < nSyms; i++ ) FreeVecs( direct[i].items, direct[i].calls, direct[i].costs, memoItems[i] );
      free( ignored ); free( fldKind ); free( starts ); free( addrMap ); free( direct ); free( memoDone ); free( memoBusy ); free( memoItems ); exprPool.Free();
    }
    static int CmpInt( const void* a, const void* b ) { return *(const int*)a - *(const int*)b; }
    static int CmpAddrMap( const void* a, const void* b ) { const ScanAddrMapEntry *ea = (const ScanAddrMapEntry*)a, *eb = (const ScanAddrMapEntry*)b; return ea->addr != eb->addr ? ea->addr - eb->addr : ea->symIdx - eb->symIdx; }

    void IndexSymbols() {
      static const char* const stConstNames[ST_COUNT] = { "ATR_STRENGTH", "ATR_DEXTERITY", "ATR_MANA_MAX", "NPC_TALENT_1H", "NPC_TALENT_2H", "NPC_TALENT_BOW", "NPC_TALENT_CROSSBOW" };
      static const char* const learnedNames[ST_COUNT] = { "LEARNED_STRENGTH", "LEARNED_DEXTERITY", "LEARNED_MANA", "LEARNED_TALENT_1H", "LEARNED_TALENT_2H", "LEARNED_TALENT_BOW", "LEARNED_TALENT_CROSSBOW" };
      const struct { const char* name; int* dst; } extTab[] = {
        { "INFO_ADDCHOICE", &extAddChoice }, { "NPC_CHANGEATTRIBUTE", &extChangeAttr }, { "NPC_SETTALENTSKILL", &extSetTalent },
        { "NPC_GETTALENTSKILL", &extGetTalent }, { "NPC_SETTALENTVALUE", &extSetTalentVal }, { "NPC_GETTALENTVALUE", &extGetTalentVal },
        { "NPC_HASITEMS", &extHasItems }, { "NPC_ISPLAYER", &extIsPlayer }, { "HLP_ISVALIDNPC", &extIsValidNpc }, { "EXT_GETATRCOST", &extGetAtrCost }
      }, symTab[] = { { "SELF", &symSelf }, { "OTHER", &symOther }, { "HERO", &symHero }, { "PC_HERO", &symPcHero } }, globalIntTab[] = {
        { "REAL_STRENGTH", &realAttrSym[0] }, { "REAL_DEXTERITY", &realAttrSym[1] }, { "REAL_MANA_MAX", &realAttrSym[2] },
        { "LEARNEDBASESTRENGTH", &learnedSym[ST_STR] }, { "STRGELERNT", &learnedSym[ST_STR] },
        { "LEARNEDBASEDEX", &learnedSym[ST_DEX] }, { "DEXGELERNT", &learnedSym[ST_DEX] },
        { "LEARNEDBASEMANA", &learnedSym[ST_MANA] }, { "LEARNED_MANA_MAX", &learnedSym[ST_MANA] }, { "MANAGELERNT", &learnedSym[ST_MANA] }
      };
      for( int i = 0; i < nSyms; i++ ) {
        const ScanSym& s = syms[i]; const char* nm = s.name ? s.name : "";
        if( !( s.flags & SCAN_FLAG_EXTERNAL ) && ( s.type == SCAN_TYPE_FUNC || s.type == SCAN_TYPE_PROTOTYPE || s.type == SCAN_TYPE_INSTANCE ) && s.address > 0 ) starts[nStarts++] = s.address;
        if( ScanIsPluginSym( nm ) ) { ignored[i] = 1; continue; }
        if( s.flags & SCAN_FLAG_CLASSVAR ) {
          if( ScanStartsWith( nm, "C_NPC." ) ) {
            const char* f = nm + 6;
            fldKind[i] = ScanStrEq( f, "LP" ) ? FLD_NPC_LP : ScanStrEq( f, "ATTRIBUTE" ) ? FLD_NPC_ATTRIBUTE : ScanStrEq( f, "HITCHANCE" ) ? FLD_NPC_HITCHANCE : ScanStrEq( f, "AIVAR" ) ? FLD_NPC_AIVAR : ScanStrEq( f, "LEVEL" ) ? FLD_NPC_LEVEL : FLD_NPC_OTHER;
          } else if( ScanStartsWith( nm, "C_INFO." ) ) {
            const char* f = nm + 7;
            fldKind[i] = ScanStrEq( f, "NPC" ) ? FLD_INFO_NPC : ScanStrEq( f, "INFORMATION" ) ? FLD_INFO_INFORMATION : ScanStrEq( f, "CONDITION" ) ? FLD_INFO_CONDITION : ScanStrEq( f, "DESCRIPTION" ) ? FLD_INFO_DESCRIPTION : ScanStrEq( f, "TRADE" ) ? FLD_INFO_TRADE : ScanStrEq( f, "PERMANENT" ) ? FLD_INFO_PERMANENT : ScanStrEq( f, "IMPORTANT" ) ? FLD_INFO_IMPORTANT : FLD_INFO_OTHER;
          }
        }
        if( s.flags & SCAN_FLAG_EXTERNAL ) {
          for( const auto& e : extTab ) if( ScanStrEq( nm, e.name ) ) { *e.dst = i; break; }
        } else {
          if( s.type == SCAN_TYPE_FUNC && s.address > 0 ) addrMap[nAddrMap++] = { s.address, i };
          for( const auto& st : symTab ) if( *st.dst < 0 && ScanStrEq( nm, st.name ) ) { *st.dst = i; break; }
          for( int st = 0; st < ST_COUNT; st++ ) {
            if( ( s.flags & SCAN_FLAG_CONST ) && s.type == SCAN_TYPE_INT && ScanStrEq( nm, stConstNames[st] ) ) statConstVal[st] = ScanSymInt( s, 0 );
            if( s.type == SCAN_TYPE_INT && !( s.flags & SCAN_FLAG_CONST ) && learnedSym[st] < 0 && ScanStrEq( nm, learnedNames[st] ) ) learnedSym[st] = i;
          }
          if( s.type == SCAN_TYPE_INT && !( s.flags & SCAN_FLAG_CONST ) )
            for( const auto& g : globalIntTab ) if( *g.dst < 0 && ScanStrEq( nm, g.name ) ) { *g.dst = i; break; }
        }
      }
      if( nStarts > 1 ) { qsort( starts, (size_t)nStarts, sizeof( int ), CmpInt ); int u = 1; for( int i = 1; i < nStarts; i++ ) if( starts[i] != starts[u - 1] ) starts[u++] = starts[i]; nStarts = u; }
      if( nAddrMap > 1 ) { qsort( addrMap, (size_t)nAddrMap, sizeof( ScanAddrMapEntry ), CmpAddrMap ); int u = 1; for( int i = 1; i < nAddrMap; i++ ) if( addrMap[i].addr != addrMap[u - 1].addr ) addrMap[u++] = addrMap[i]; nAddrMap = u; }
    }

    int BodyEnd( int startAddr ) const {
      int lo = 0, hi = nStarts;
      while( lo < hi ) { const int mid = ( lo + hi ) >> 1; if( starts[mid] <= startAddr ) lo = mid + 1; else hi = mid; }
      return ( lo < nStarts ) ? starts[lo] : codeSize;
    }
    int FuncByAddr( int addr ) const {
      for( int lo = 0, hi = nAddrMap - 1; lo <= hi; ) {
        const int mid = ( lo + hi ) >> 1;
        if( addrMap[mid].addr == addr ) return addrMap[mid].symIdx;
        if( addrMap[mid].addr < addr ) lo = mid + 1; else hi = mid - 1;
      }
      return -1;
    }
    int AllocExpr( int kind, int val = 0 ) { ScanExprNode n = {}; n.kind = kind; n.val = val; n.left = n.right = -1; exprPool.Push( n ); return exprPool.len - 1; }

    ScanOperand StripExpr( int nodeIdx, int depth = 0 ) const {
      if( nodeIdx < 0 || nodeIdx >= exprPool.len || depth > 12 ) return MakeOpnd( OPND_UNK );
      const ScanExprNode& n = exprPool[nodeIdx];
      if( n.kind == EXPR_CONST )  return MakeOpnd( OPND_CONST, n.val );
      if( n.kind == EXPR_PARAM )  return MakeOpnd( OPND_PARAM, n.val );
      if( n.kind == EXPR_GLOBAL ) return MakeOpnd( OPND_GLOBAL, n.val );
      if( n.kind == EXPR_LOCAL && n.nDefs > 0 ) {
        const ScanOperand first = StripExpr( n.defs[0], depth + 1 );
        for( int i = 1; i < n.nDefs; i++ ) if( !OpndEq( first, StripExpr( n.defs[i], depth + 1 ) ) ) return MakeOpnd( OPND_UNK );
        return first;
      }
      return MakeOpnd( OPND_UNK );
    }

    template<typename Fn> void WalkExpr( int idx, Fn fn, int d = 0 ) const {
      if( idx < 0 || idx >= exprPool.len || d > 12 ) return;
      const ScanExprNode& n = exprPool[idx]; fn( n );
      if( n.kind == EXPR_BINOP || n.kind == EXPR_UNOP ) { WalkExpr( n.left, fn, d + 1 ); if( n.kind == EXPR_BINOP ) WalkExpr( n.right, fn, d + 1 ); }
      else if( n.kind == EXPR_LOCAL || n.kind == EXPR_CALLRET ) for( int i = 0; i < n.nDefs; i++ ) WalkExpr( n.defs[i], fn, d + 1 );
    }
    void FieldsInExpr( int idx, ScanVec<ScanFieldRef>& out ) const {
      WalkExpr( idx, [&]( const ScanExprNode& n ) { if( n.kind == EXPR_FIELD ) out.Push( n.field ); else if( n.kind == EXPR_COMPACT ) for( int i = 0; i < n.nFields; i++ ) out.Push( n.cfields[i] ); } );
    }
    void CallsInExpr( int idx, ScanVec<int>& out ) const {
      WalkExpr( idx, [&]( const ScanExprNode& n ) { if( n.kind == EXPR_CALLRET ) out.Push( n.val ); else if( n.kind == EXPR_COMPACT ) for( int i = 0; i < n.nCalls; i++ ) out.Push( n.ccalls[i] ); } );
    }
    bool HasLearnedGlobalInExpr( int idx ) const {
      bool hit = false;
      WalkExpr( idx, [&]( const ScanExprNode& n ) {
        if( n.kind == EXPR_GLOBAL ) {
          for( int st = 0; st < ST_COUNT; st++ ) if( learnedSym[st] == n.val ) hit = true;
          for( int k = 0; k < 3; k++ )           if( realAttrSym[k] == n.val ) hit = true;
        }
      } );
      return hit;
    }

    int CompactExpr( int nodeIdx ) {
      if( nodeIdx < 0 || nodeIdx >= exprPool.len ) return AllocExpr( EXPR_UNK );
      const int k = exprPool[nodeIdx].kind;
      if( k == EXPR_CONST || k == EXPR_PARAM || k == EXPR_GLOBAL || k == EXPR_STR || k == EXPR_UNK ) return nodeIdx;
      const ScanOperand s = StripExpr( nodeIdx );
      if( s.kind == OPND_CONST || s.kind == OPND_PARAM || s.kind == OPND_GLOBAL )
        return AllocExpr( s.kind == OPND_CONST ? EXPR_CONST : s.kind == OPND_PARAM ? EXPR_PARAM : EXPR_GLOBAL, s.val );
      ScanVec<ScanFieldRef> flds; ScanVec<int> cls; InitVecs( flds, cls ); FieldsInExpr( nodeIdx, flds ); CallsInExpr( nodeIdx, cls );
      const int cIdx = AllocExpr( EXPR_COMPACT ); ScanExprNode& cn = exprPool[cIdx];
      for( int i = 0; i < flds.len; i++ ) ScanInsertUnique( cn.cfields, cn.nFields, 16, flds[i], FieldRefCmp );
      for( int i = 0; i < cls.len; i++ )  ScanInsertUnique( cn.ccalls, cn.nCalls, 16, cls[i], []( int a, int b ) { return a - b; } );
      FreeVecs( flds, cls ); return cIdx;
    }

    void EqGuardsInExpr( int nodeIdx, ScanVec<ScanActiveGuard>& out, int endPos ) const {
      if( nodeIdx < 0 || nodeIdx >= exprPool.len ) return;
      const ScanExprNode& n = exprPool[nodeIdx]; if( n.kind != EXPR_BINOP ) return;
      if( n.val == 12 ) { EqGuardsInExpr( n.left, out, endPos ); EqGuardsInExpr( n.right, out, endPos ); }
      else if( n.val == 16 ) {
        const ScanOperand a = StripExpr( n.left ), b = StripExpr( n.right );
        if( a.kind == OPND_PARAM && b.kind == OPND_CONST )      PushByEndPos( out, { endPos, a.val, b.val } );
        else if( b.kind == OPND_PARAM && a.kind == OPND_CONST ) PushByEndPos( out, { endPos, b.val, a.val } );
      }
    }
    void ThrInExpr( int nodeIdx, ScanVec<ScanActiveThr>& out, int endPos ) const {
      if( nodeIdx < 0 || nodeIdx >= exprPool.len ) return;
      const ScanExprNode& n = exprPool[nodeIdx]; if( n.kind != EXPR_BINOP ) return;
      if( n.val == 12 ) { ThrInExpr( n.left, out, endPos ); ThrInExpr( n.right, out, endPos ); }
      else if( ( n.val == 7 || n.val == 8 || n.val == 15 || n.val == 18 ) && n.left >= 0 && n.left < exprPool.len ) {
        const ScanExprNode& ln = exprPool[n.left]; const ScanOperand ro = StripExpr( n.right );
        if( ln.kind == EXPR_FIELD && ( ln.field.field == FLD_NPC_ATTRIBUTE || ln.field.field == FLD_NPC_HITCHANCE ) && ro.kind == OPND_CONST )
          PushByEndPos( out, { endPos, { ln.field.field, ln.field.idx, n.val == 7 ? RULE_LT : n.val == 8 ? RULE_GT : n.val == 15 ? RULE_LE : RULE_GE, ro.val } } );
      }
    }
    void RecordReads( int nodeIdx, ScanVec<ScanItem>& items ) const {
      ScanVec<ScanFieldRef> flds; flds.Init(); FieldsInExpr( nodeIdx, flds );
      for( int i = 0; i < flds.len; i++ ) {
        const ScanFieldRef& f = flds[i];
        if( ( f.field == FLD_NPC_ATTRIBUTE || f.field == FLD_NPC_HITCHANCE || f.field == FLD_NPC_AIVAR ) && ( f.inst.kind == INST_PARAM || f.inst.kind == INST_OTHER || f.inst.kind == INST_HERO ) ) {
          ScanItem it = MakeEmptyItem( ITEM_READ ); it.inst = f.inst; it.field = f.field; it.idx = f.idx; ItemSetAdd( items, it );
        }
      }
      flds.Free();
    }
    bool IsGlobalPlusParamExpr( int nodeIdx ) const {
      if( nodeIdx < 0 || nodeIdx >= exprPool.len ) return false;
      const ScanExprNode& n = exprPool[nodeIdx]; if( n.kind != EXPR_BINOP || n.val != 0 ) return false;
      const ScanOperand a = StripExpr( n.left ), b = StripExpr( n.right );
      return ( a.kind == OPND_GLOBAL && b.kind == OPND_PARAM ) || ( b.kind == OPND_GLOBAL && a.kind == OPND_PARAM );
    }
    int CalleeLpDecParam( int calleeIdx ) const {
      if( calleeIdx < 0 || calleeIdx >= nSyms || ignored[calleeIdx] ) return -1;
      const ScanSym& cs = syms[calleeIdx];
      if( cs.type != SCAN_TYPE_FUNC || ( cs.flags & SCAN_FLAG_EXTERNAL ) || cs.count < 1 || cs.count > 2 || cs.address <= 0 || cs.address >= codeSize ) return -1;
      const int cStart = cs.address, cEnd = BodyEnd( cStart ); if( cEnd - cStart > 256 ) return -1;
      bool hasLpField = false, hasSub = false; int paramUsed = -1;
      for( int p = cStart, arg = 0, idx = 0, op; p < cEnd && ( op = ScanNextOp( code, cEnd, p, arg, idx ) ) >= 0; ) {
        if( ( op == 65 || op == 67 ) && arg >= 0 && arg < nSyms ) {
          if( fldKind[arg] == FLD_NPC_LP ) hasLpField = true;
          else if( arg >= calleeIdx + 1 && arg < calleeIdx + 1 + cs.count ) paramUsed = arg - ( calleeIdx + 1 );
        } else if( op == 1 || op == 20 ) hasSub = true;
      }
      return ( hasLpField && hasSub && paramUsed >= 0 ) ? paramUsed : -1;
    }

    const ScanDirectBody& GetBody( int symIdx ) {
      ScanDirectBody& db = direct[symIdx];
      if( db.evaluated ) return db;
      db.evaluated = true;
      if( symIdx < 0 || symIdx >= nSyms || ignored[symIdx] ) return db;
      const ScanSym& fsym = syms[symIdx];
      if( ( fsym.type != SCAN_TYPE_FUNC && fsym.type != SCAN_TYPE_PROTOTYPE && fsym.type != SCAN_TYPE_INSTANCE ) || ( fsym.flags & SCAN_FLAG_EXTERNAL ) || fsym.address <= 0 || fsym.address >= codeSize ) return db;

      const int start = fsym.address, end = BodyEnd( start ), nparam = ( fsym.type == SCAN_TYPE_FUNC ) ? fsym.count : 0, pfirst = symIdx + 1;
      char prefix[128]; ScanToUpper( fsym.name, prefix, (int)sizeof( prefix ) - 2 );
      const int plen = (int)strlen( prefix ); prefix[plen] = '.'; prefix[plen + 1] = '\0';

      exprPool.Clear();
      const int unkNode = AllocExpr( EXPR_UNK );
      ScanVec<int> stack, lpdecDirect, relCmp; ScanVec<ScanLocalDefEntry> localDefs; ScanVec<ScanAliasEntry> aliases; ScanVec<ScanActiveGuard> guards; ScanVec<ScanActiveThr> thr;
      InitVecs( stack, localDefs, aliases, guards, thr, lpdecDirect, relCmp );

      auto popNode = [&]() { return stack.len > 0 ? stack.Pop() : unkNode; };
      auto popArgs = [&]( int cnt, int* out ) {
        const int na = cnt < 0 ? 0 : ( cnt > 16 ? 16 : cnt );
        for( int i = na - 1; i >= 0; i-- ) out[i] = popNode();
        for( int i = 0; i < na; i++ )      RecordReads( out[i], db.items );
        return na;
      };
      auto pushCallRet = [&]( int val, int na, const int* args ) {
        const int rNode = AllocExpr( EXPR_CALLRET, val ); exprPool[rNode].nDefs = na;
        for( int i = 0; i < na; i++ ) exprPool[rNode].defs[i] = args[i];
        stack.Push( rNode );
      };

      ScanInstRef cur = MakeInst( INST_NONE );
      for( int pos = start, arg = 0, idx = 0, op; pos < end; ) {
        while( guards.len > 0 && guards[guards.len - 1].endPos <= pos ) guards.Pop();
        while( thr.len > 0 && thr[thr.len - 1].endPos <= pos )          thr.Pop();
        if( ( op = ScanNextOp( code, end, pos, arg, idx ) ) < 0 ) break;

        if( op == 64 ) stack.Push( AllocExpr( EXPR_CONST, arg ) );
        else if( op == 65 || op == 245 ) {
          const int vIdx = ( op == 245 ) ? idx : 0;
          if( arg < 0 || arg >= nSyms ) stack.Push( unkNode );
          else if( fldKind[arg] != FLD_NONE ) {
            const int fk = fldKind[arg], n = AllocExpr( EXPR_FIELD );
            exprPool[n].field = { fk, vIdx, ( fk >= FLD_NPC_LP && fk <= FLD_NPC_OTHER ) ? cur : MakeInst( INST_NONE ) }; stack.Push( n );
          } else if( arg >= pfirst && arg < pfirst + nparam ) stack.Push( AllocExpr( EXPR_PARAM, arg - pfirst ) );
          else if( ( syms[arg].flags & SCAN_FLAG_CONST ) && syms[arg].type == SCAN_TYPE_INT && syms[arg].count > 0 )
            stack.Push( AllocExpr( EXPR_CONST, ScanSymInt( syms[arg], vIdx < syms[arg].count ? vIdx : 0 ) ) );
          else if( syms[arg].type == SCAN_TYPE_STRING ) stack.Push( AllocExpr( EXPR_STR, arg ) );
          else if( ScanStartsWith( syms[arg].name, prefix ) ) {
            const int n = AllocExpr( EXPR_LOCAL, arg );
            for( int li = 0; li < localDefs.len; li++ ) if( localDefs[li].symIdx == arg ) {
              exprPool[n].nDefs = localDefs[li].nDefs; for( int d = 0; d < localDefs[li].nDefs; d++ ) exprPool[n].defs[d] = localDefs[li].defNodes[d]; break;
            }
            stack.Push( n );
          } else stack.Push( AllocExpr( EXPR_GLOBAL, arg ) );
        } else if( op == 67 || op == 80 ) {
          ScanInstRef ir = MakeInst( INST_SYM, arg );
          if( arg >= pfirst && arg < pfirst + nparam ) ir = MakeInst( INST_PARAM, arg - pfirst );
          else {
            bool foundAlias = false;
            for( int ai = 0; ai < aliases.len; ai++ ) if( aliases[ai].symIdx == arg ) { ir = aliases[ai].inst; foundAlias = true; break; }
            if( !foundAlias && arg >= 0 && arg < nSyms ) {
              const char* nm = syms[arg].name;
              if( ScanStrEq( nm, "OTHER" ) ) ir = MakeInst( INST_OTHER ); else if( ScanStrEq( nm, "HERO" ) ) ir = MakeInst( INST_HERO ); else if( ScanStrEq( nm, "SELF" ) ) ir = MakeInst( INST_SELF );
            }
          }
          if( op == 67 ) { const int n = AllocExpr( EXPR_INST ); exprPool[n].inst = ir; stack.Push( n ); } else cur = ir;
        } else if( op == 9 || ( op >= 19 && op <= 22 ) || ( op >= 70 && op <= 73 ) ) {
          const int tNode = popNode(), vNode = popNode();
          const ScanExprNode tCopy = exprPool[tNode];
          if( tCopy.kind == EXPR_FIELD && tCopy.field.inst.kind == INST_NONE && tCopy.field.field >= FLD_INFO_NPC && tCopy.field.field <= FLD_INFO_IMPORTANT ) {
            db.hasInfoAssign = true; const ScanOperand vo = StripExpr( vNode );
            if( tCopy.field.field == FLD_INFO_NPC ) db.infoNpc = vo; else if( tCopy.field.field == FLD_INFO_INFORMATION ) db.infoInformation = vo; else if( tCopy.field.field == FLD_INFO_CONDITION ) db.infoCondition = vo;
          } else {
            RecordReads( vNode, db.items );
            if( tCopy.kind == EXPR_FIELD && tCopy.field.inst.kind != INST_NONE ) {
              const ScanInstRef inst = tCopy.field.inst;
              if( inst.kind != INST_SYM ) {
                int effVal = vNode;
                if( op == 19 || op == 20 ) { effVal = AllocExpr( EXPR_BINOP, op == 20 ? 1 : 0 ); exprPool[effVal].left = tNode; exprPool[effVal].right = vNode; }
                const ScanExprNode& vn = exprPool[effVal];
                if( vn.kind == EXPR_BINOP ) {
                  const bool sameL = vn.left >= 0 && exprPool[vn.left].kind == EXPR_FIELD && FieldRefEq( exprPool[vn.left].field, tCopy.field );
                  const bool sameR = vn.right >= 0 && exprPool[vn.right].kind == EXPR_FIELD && FieldRefEq( exprPool[vn.right].field, tCopy.field );
                  if( tCopy.field.field == FLD_NPC_LP && vn.val == 1 && sameL ) {
                    ScanItem it = MakeEmptyItem( ITEM_LPDEC ); it.inst = inst; ItemSetAdd( db.items, it ); lpdecDirect.Push( vn.right );
                  } else if( vn.val == 0 && ( sameL || sameR ) ) {
                    const int amtNode = sameL ? vn.right : vn.left; const ScanGuard gNow = MakeGuardFromActive( guards );
                    if( ( tCopy.field.field == FLD_NPC_ATTRIBUTE && ( tCopy.field.idx == 1 || tCopy.field.idx == 3 || tCopy.field.idx == 4 || tCopy.field.idx == 5 ) ) ||
                        ( tCopy.field.field == FLD_NPC_HITCHANCE && tCopy.field.idx >= 1 && tCopy.field.idx <= 4 ) ) {
                      ScanItem it = MakeEmptyItem( ITEM_STATINC ); it.inst = inst; it.field = tCopy.field.field; it.op1 = MakeOpnd( OPND_CONST, tCopy.field.idx ); it.op2 = StripExpr( amtNode ); it.guard = gNow; ItemSetAdd( db.items, it );
                    } else if( tCopy.field.field == FLD_NPC_AIVAR ) {
                      ScanItem it = MakeEmptyItem( ITEM_AIVARINC ); it.inst = inst; it.idx = tCopy.field.idx; it.op2 = StripExpr( amtNode ); it.guard = gNow; ItemSetAdd( db.items, it );
                    } else if( tCopy.field.field == FLD_NPC_LEVEL ) {
                      ScanItem it = MakeEmptyItem( ITEM_LEVELINC ); it.inst = inst; ItemSetAdd( db.items, it );
                    }
                  }
                }
              }
            } else if( tCopy.kind == EXPR_LOCAL ) {
              const int cNode = CompactExpr( vNode ); int lIdx = -1;
              for( int li = 0; li < localDefs.len; li++ ) if( localDefs[li].symIdx == tCopy.val ) { lIdx = li; break; }
              if( lIdx < 0 ) { ScanLocalDefEntry ne = {}; ne.symIdx = tCopy.val; localDefs.Push( ne ); lIdx = localDefs.len - 1; }
              ScanLocalDefEntry& le = localDefs[lIdx]; bool dup = false;
              for( int d = 0; d < le.nDefs; d++ ) if( ExprCompactEq( exprPool[le.defNodes[d]], exprPool[cNode] ) ) { dup = true; break; }
              if( !dup && le.nDefs < 8 ) le.defNodes[le.nDefs++] = cNode;
              if( le.rawCount++ == 0 ) le.rawFirst = vNode;
            } else if( tCopy.kind == EXPR_GLOBAL ) {
              int effVal = vNode;
              if( op == 19 ) { effVal = AllocExpr( EXPR_BINOP, 0 ); exprPool[effVal].left = tNode; exprPool[effVal].right = vNode; }
              const ScanExprNode& vn = exprPool[effVal];
              if( vn.kind == EXPR_BINOP && vn.val == 0 ) {
                const bool sameL = vn.left >= 0 && exprPool[vn.left].kind == EXPR_GLOBAL && exprPool[vn.left].val == tCopy.val;
                const bool sameR = vn.right >= 0 && exprPool[vn.right].kind == EXPR_GLOBAL && exprPool[vn.right].val == tCopy.val;
                if( sameL || sameR ) {
                  const ScanOperand amtOp = StripExpr( sameL ? vn.right : vn.left );
                  if( amtOp.kind == OPND_PARAM ) { ScanItem it = MakeEmptyItem( ITEM_GLOBALINC ); it.idx = tCopy.val; it.op2 = amtOp; it.guard = MakeGuardFromActive( guards ); ItemSetAdd( db.items, it ); }
                }
              }
            }
          }
        } else if( op == 74 ) {
          const int tNode = popNode(), vNode = popNode(); const ScanExprNode &tn = exprPool[tNode], &vn = exprPool[vNode];
          if( tn.kind == EXPR_INST && tn.inst.kind == INST_SYM && vn.kind == EXPR_INST ) {
            bool found = false;
            for( int ai = 0; ai < aliases.len; ai++ ) if( aliases[ai].symIdx == tn.inst.idx ) { aliases[ai].inst = vn.inst; found = true; break; }
            if( !found ) aliases.Push( { tn.inst.idx, vn.inst } );
          }
        } else if( op <= 8 || ( op >= 11 && op <= 18 ) ) {
          const int aNode = popNode(), bNode = popNode(), n = AllocExpr( EXPR_BINOP, op ); exprPool[n].left = aNode; exprPool[n].right = bNode;
          if( op == 7 || op == 8 || op == 15 || op == 18 ) relCmp.Push( n );
          stack.Push( n );
        } else if( op >= 30 && op <= 33 ) {
          const int aNode = popNode(), n = AllocExpr( EXPR_UNOP, op ); exprPool[n].left = aNode; stack.Push( n );
        } else if( op == 76 ) {
          const int cNode = popNode(); RecordReads( cNode, db.items ); EqGuardsInExpr( cNode, guards, arg ); ThrInExpr( cNode, thr, arg );
        } else if( op == 60 ) {
          if( stack.len > 0 ) RecordReads( stack[stack.len - 1], db.items );
          stack.Clear();
        } else if( op == 61 ) {
          const int calleeIdx = FuncByAddr( arg ); if( calleeIdx < 0 ) continue;
          const ScanSym& callee = syms[calleeIdx]; int argNodes[16]; const int na = popArgs( callee.count, argNodes );
          ScanCallSite cs = {}; cs.calleeSym = calleeIdx; cs.nArgs = na;
          for( int i = 0; i < na; i++ ) { const ScanExprNode& an = exprPool[argNodes[i]]; cs.args[i] = ( an.kind == EXPR_INST ) ? MakeInstOpnd( an.inst ) : StripExpr( argNodes[i] ); }
          cs.guard = MakeGuardFromActive( guards ); db.calls.Push( cs );
          const int lpPar = CalleeLpDecParam( calleeIdx );
          if( lpPar >= 0 && lpPar < na ) {
            lpdecDirect.Push( argNodes[lpPar] ); ScanItem it = MakeEmptyItem( ITEM_LPDEC ); it.inst = MakeInst( INST_HERO ); ItemSetAdd( db.items, it );
          }
          if( callee.flags & SCAN_FLAG_RETURN ) pushCallRet( calleeIdx, na, argNodes );
        } else if( op == 62 ) {
          if( arg < 0 || arg >= nSyms ) continue;
          const ScanSym& ext = syms[arg]; int argNodes[16]; const int na = popArgs( ext.count, argNodes );
          if( arg == extAddChoice && na == 3 ) {
            ScanItem it = MakeEmptyItem( ITEM_CHOICE ); it.op1 = StripExpr( argNodes[2] ); ItemSetAdd( db.items, it );
            ScanVec<int> pcalls; pcalls.Init(); CallsInExpr( argNodes[1], pcalls );
            for( int ci = 0; ci < pcalls.len; ci++ ) { ScanItem pit = MakeEmptyItem( ITEM_PRICEFN ); pit.idx = pcalls[ci]; ItemSetAdd( db.items, pit ); }
            pcalls.Free();
          } else if( arg == extChangeAttr && na == 3 && exprPool[argNodes[0]].kind == EXPR_INST ) {
            ScanItem it = MakeEmptyItem( ITEM_STATINC ); it.inst = exprPool[argNodes[0]].inst; it.field = FLD_NPC_ATTRIBUTE;
            it.op1 = StripExpr( argNodes[1] ); it.op2 = StripExpr( argNodes[2] ); it.guard = MakeGuardFromActive( guards ); ItemSetAdd( db.items, it );
          } else if( arg == extSetTalent && na == 3 && exprPool[argNodes[0]].kind == EXPR_INST ) {
            ScanItem it = MakeEmptyItem( ITEM_TALENTSET ); it.inst = exprPool[argNodes[0]].inst;
            it.op1 = StripExpr( argNodes[1] ); it.op2 = StripExpr( argNodes[2] ); ThrSetInit( it.thr );
            for( int ti = 0; ti < thr.len; ti++ ) ThrSetAdd( it.thr, thr[ti].cond );
            ItemSetAdd( db.items, it );
          } else if( arg == extSetTalentVal && na == 3 && exprPool[argNodes[0]].kind == EXPR_INST ) {
            int amtNode = argNodes[2];
            if( amtNode >= 0 && amtNode < exprPool.len && exprPool[amtNode].kind == EXPR_BINOP && exprPool[amtNode].val == 0 ) {
              const int left = exprPool[amtNode].left, right = exprPool[amtNode].right;
              const ScanOperand soRight = StripExpr( right ), soLeft = StripExpr( left );
              if( soRight.kind == OPND_PARAM || soRight.kind == OPND_CONST ) amtNode = right;
              else if( soLeft.kind == OPND_PARAM || soLeft.kind == OPND_CONST ) amtNode = left;
            }
            ScanItem it = MakeEmptyItem( ITEM_STATINC ); it.inst = exprPool[argNodes[0]].inst; it.field = FLD_NPC_HITCHANCE;
            it.op1 = StripExpr( argNodes[1] ); if( it.op1.kind == OPND_UNK && nparam >= 3 ) it.op1 = MakeOpnd( OPND_PARAM, 1 );
            it.op2 = StripExpr( amtNode ); it.guard = MakeGuardFromActive( guards );
            const bool crossStat = it.guard.n == 1 && it.op1.kind == OPND_CONST && it.op1.val >= 1 && it.op1.val <= 4 && it.guard.v[0] >= 1 && it.guard.v[0] <= 4 && it.op1.val != it.guard.v[0];
            if( !crossStat ) ItemSetAdd( db.items, it );
          }
          if( arg == extGetTalentVal && na == 2 && exprPool[argNodes[0]].kind == EXPR_INST ) {
            const ScanOperand talOp = StripExpr( argNodes[1] ); const int n = AllocExpr( EXPR_FIELD );
            exprPool[n].field = { FLD_NPC_HITCHANCE, ( talOp.kind == OPND_CONST && talOp.val > 0 ) ? talOp.val : 0, exprPool[argNodes[0]].inst }; stack.Push( n );
          } else if( ext.flags & SCAN_FLAG_RETURN ) pushCallRet( -arg - 1, na, argNodes );
        }
      }

      if( lpdecDirect.len > 0 ) {
        const ScanGuard gEnd = MakeGuardFromActive( guards );
        ScanVec<ScanItem> parLim, constLim; InitVecs( parLim, constLim );
        for( int ri = 0; ri < relCmp.len; ri++ ) {
          const ScanExprNode& rn = exprPool[relCmp[ri]];
          const int sides[2][2] = { { rn.left, rn.right }, { rn.right, rn.left } };
          for( int si = 0; si < 2; si++ ) {
            const int xNode = sides[si][0], yNode = sides[si][1]; const ScanOperand sx = StripExpr( xNode );
            if( sx.kind != OPND_PARAM && sx.kind != OPND_CONST && sx.kind != OPND_GLOBAL ) continue;
            ScanVec<ScanFieldRef> yf; yf.Init(); FieldsInExpr( yNode, yf );
            bool hasStatField = false;
            for( int fi = 0; fi < yf.len; fi++ ) { const int fk = yf[fi].field; if( fk == FLD_NPC_ATTRIBUTE || fk == FLD_NPC_HITCHANCE || fk == FLD_NPC_AIVAR ) { hasStatField = true; break; } }
            yf.Free();
            if( !hasStatField && ( HasLearnedGlobalInExpr( yNode ) || ( sx.kind != OPND_CONST && IsGlobalPlusParamExpr( yNode ) ) ) ) hasStatField = true;
            if( hasStatField ) {
              if( sx.kind == OPND_GLOBAL ) {
                if( sx.val >= 0 && sx.val < nSyms && syms[sx.val].type == SCAN_TYPE_INT && !( syms[sx.val].flags & SCAN_FLAG_CONST ) && ScanSymInt( syms[sx.val], 0 ) <= 0 && IsGlobalPlusParamExpr( yNode ) ) {
                  bool dup = false; for( int ci = 0; ci < nLevelCap; ci++ ) if( levelCapSym[ci] == sx.val ) { dup = true; break; }
                  if( !dup && nLevelCap < 16 ) levelCapSym[nLevelCap++] = sx.val;
                }
                continue;
              }
              ScanItem lit = MakeEmptyItem( ITEM_LIMIT ); lit.op1 = sx; lit.guard = gEnd;
              ( sx.kind == OPND_PARAM ? parLim : constLim ).Push( lit );
            }
          }
        }
        ScanVec<ScanItem>& chosen = ( parLim.len > 0 ) ? parLim : constLim;
        for( int i = 0; i < chosen.len; i++ ) ItemSetAdd( db.items, chosen[i] );
        FreeVecs( parLim, constLim );
      }

      for( int ci = 0; ci < lpdecDirect.len; ci++ ) {
        int vNode = lpdecDirect[ci];
        if( vNode >= 0 && vNode < exprPool.len && exprPool[vNode].kind == EXPR_LOCAL ) {
          const int lSym = exprPool[vNode].val;
          for( int li = 0; li < localDefs.len; li++ ) if( localDefs[li].symIdx == lSym && localDefs[li].rawCount == 1 ) { vNode = localDefs[li].rawFirst; break; }
        }
        ScanCostSummary cs = {};
        ScanVec<int> allCalls; allCalls.Init(); CallsInExpr( vNode, allCalls );
        for( int k = 0; k < allCalls.len; k++ ) if( allCalls[k] >= 0 && cs.nCalls < 8 ) cs.calls[cs.nCalls++] = allCalls[k];
        allCalls.Free();
        if( vNode >= 0 && vNode < exprPool.len ) {
          const ScanExprNode& vn = exprPool[vNode];
          if( vn.kind == EXPR_BINOP && vn.val == 2 ) {
            const bool hasCallRet = ( vn.left >= 0 && exprPool[vn.left].kind == EXPR_CALLRET ) || ( vn.right >= 0 && exprPool[vn.right].kind == EXPR_CALLRET );
            const bool hasParam = ( StripExpr( vn.left ).kind == OPND_PARAM ) || ( StripExpr( vn.right ).kind == OPND_PARAM );
            if( hasCallRet && hasParam ) cs.isPerPoint = true;
          }
        }
        db.costs.Push( cs );
      }
      FreeVecs( stack, localDefs, aliases, guards, thr, lpdecDirect, relCmp );
      return db;
    }

    static bool MapItem( const ScanItem& it, const ScanCallSite& site, ScanItem& out ) {
      out = it;
      if( ( it.kind == ITEM_LPDEC || it.kind == ITEM_LEVELINC || it.kind == ITEM_READ || it.kind == ITEM_STATINC || it.kind == ITEM_AIVARINC || it.kind == ITEM_TALENTSET ) && it.inst.kind == INST_PARAM ) {
        const int p = it.inst.idx; if( p < 0 || p >= site.nArgs || site.args[p].kind != OPND_INST ) return false;
        const ScanInstRef& mapped = site.args[p].inst;
        if( mapped.kind != INST_PARAM && mapped.kind != INST_OTHER && mapped.kind != INST_HERO && mapped.kind != INST_SELF ) return false;
        out.inst = mapped;
      }
      if( it.kind == ITEM_STATINC || it.kind == ITEM_AIVARINC || it.kind == ITEM_GLOBALINC || it.kind == ITEM_LIMIT ) {
        ScanGuard mg = site.guard;
        for( int i = 0; i < it.guard.n; i++ ) {
          const int p = it.guard.p[i], v = it.guard.v[i]; const ScanOperand a = ( p >= 0 && p < site.nArgs ) ? site.args[p] : MakeOpnd( OPND_UNK );
          if( a.kind == OPND_CONST && a.val != v ) return false;
          if( a.kind == OPND_PARAM ) GuardAdd( mg, a.val, v );
        }
        out.guard = mg;
      }
      auto mapOpnd = [&]( ScanOperand& op ) {
        if( op.kind == OPND_PARAM ) { const ScanOperand a = ( op.val >= 0 && op.val < site.nArgs ) ? site.args[op.val] : MakeOpnd( OPND_UNK ); op = ( a.kind == OPND_INST ) ? MakeOpnd( OPND_UNK ) : a; }
      };
      if( it.kind == ITEM_STATINC || it.kind == ITEM_LIMIT || it.kind == ITEM_CHOICE || it.kind == ITEM_TALENTSET ) mapOpnd( out.op1 );
      if( it.kind == ITEM_STATINC || it.kind == ITEM_AIVARINC || it.kind == ITEM_GLOBALINC || it.kind == ITEM_TALENTSET ) mapOpnd( out.op2 );
      return true;
    }

    const ScanVec<ScanItem>& GetSummary( int symIdx ) {
      if( memoDone[symIdx] || memoBusy[symIdx] ) return memoItems[symIdx];
      memoBusy[symIdx] = 1;
      const ScanDirectBody& db = GetBody( symIdx );
      ScanVec<ScanItem> localOut; localOut.Init();
      for( int i = 0; i < db.items.len; i++ ) ItemSetAdd( localOut, db.items[i] );

      const int nCalls = db.calls.len;
      ScanCallSite* callCopy = nCalls > 0 ? (ScanCallSite*)malloc( (size_t)nCalls * sizeof( ScanCallSite ) ) : 0;
      if( callCopy ) memcpy( callCopy, db.calls.data, (size_t)nCalls * sizeof( ScanCallSite ) );
      for( int ci = 0; ci < nCalls; ci++ ) {
        const ScanCallSite& cs = callCopy[ci]; const ScanVec<ScanItem>& sub = GetSummary( cs.calleeSym );
        for( int si = 0; si < sub.len; si++ ) { ScanItem mapped; if( MapItem( sub[si], cs, mapped ) ) ItemSetAdd( localOut, mapped ); }
      }
      free( callCopy );

      int lpParam = -1;
      for( int i = 0; i < localOut.len; i++ ) if( localOut[i].kind == ITEM_LPDEC && localOut[i].inst.kind == INST_PARAM ) { lpParam = localOut[i].inst.idx; break; }
      if( lpParam >= 0 ) {
        for( int i = 0; i < localOut.len; i++ )
          if( localOut[i].kind == ITEM_STATINC && localOut[i].inst.kind == INST_HERO && localOut[i].op2.kind == OPND_PARAM ) localOut[i].inst = MakeInst( INST_PARAM, lpParam );
      } else {
        int statNpcParam = -1;
        for( int i = 0; i < localOut.len; i++ )
          if( localOut[i].kind == ITEM_STATINC && localOut[i].inst.kind == INST_PARAM && localOut[i].op2.kind == OPND_PARAM ) { statNpcParam = localOut[i].inst.idx; break; }
        if( statNpcParam >= 0 )
          for( int i = 0; i < localOut.len; i++ ) if( localOut[i].kind == ITEM_LPDEC && localOut[i].inst.kind == INST_HERO ) localOut[i].inst = MakeInst( INST_PARAM, statNpcParam );
      }
      memoBusy[symIdx] = 0; memoDone[symIdx] = 1; memoItems[symIdx] = localOut;
      return memoItems[symIdx];
    }

    struct VmNpc         { int lp, attribute[8], hitChance[5], aivar[300], level, talent[64], talentSkill[64]; };
    struct VmVal         { int kind, val, elem; VmNpc* npc; };
    struct VmDirtyGlobal { int symIdx, elem, val; };
    struct VmFrame       { int retPc; VmNpc* curNpc; };

    bool IsStubFunc( int symIdx ) const {
      if( symIdx < 0 || symIdx >= nSyms || !syms[symIdx].name ) return true;
      const char* n = syms[symIdx].name;
      for( const char* s : { "B_SAY", "B_GIVEINVITEMS", "PRINTS_EXT", "PRINT_EXT", "PRINTS", "EXT_LOGDEBUG", "PRINTDEBUGNPC", "PRINTDEBUGINST", "PRINTDEBUGINSTSTR", "PRINTHEROATTRIBUTESTRING", "UPDATEMANAPERCENTRINGEFFECT", "_@S", "_^", "_@", "_@F" } )
        if( ScanStrEq( n, s ) ) return true;
      for( const char* p : { "AI_FUNCTION", "ANIM8", "MEM_", "MEMINT_", "SB_", "SPINE_", "TELEMETRY_" } ) if( ScanStartsWith( n, p ) ) return true;
      return false;
    }

    int EvalVmVal( const VmVal& v, const VmDirtyGlobal* dirty, int nDirty ) const {
      if( v.kind == 0 ) return v.val;
      if( v.kind == 1 ) {
        for( int i = nDirty - 1; i >= 0; i-- ) if( dirty[i].symIdx == v.val && dirty[i].elem == v.elem ) return dirty[i].val;
        return ( v.val >= 0 && v.val < nSyms && syms[v.val].type == SCAN_TYPE_INT && !( syms[v.val].flags & SCAN_FLAG_CLASSVAR ) && syms[v.val].count > 0 )
               ? ScanSymInt( syms[v.val], v.elem < syms[v.val].count ? v.elem : 0 ) : 0;
      }
      if( v.kind == 2 && v.npc ) {
        switch( v.val ) {
          case FLD_NPC_LP:        return v.npc->lp;
          case FLD_NPC_ATTRIBUTE: return ( v.elem >= 0 && v.elem < 8 ) ? v.npc->attribute[v.elem] : 0;
          case FLD_NPC_HITCHANCE: return ( v.elem >= 0 && v.elem < 5 ) ? v.npc->hitChance[v.elem] : ( ( v.elem >= 0 && v.elem < 64 ) ? v.npc->talent[v.elem] : 0 );
          case FLD_NPC_AIVAR:     return ( v.elem >= 0 && v.elem < 300 ) ? v.npc->aivar[v.elem] : 0;
          case FLD_NPC_LEVEL:     return v.npc->level;
        }
      }
      return 0;
    }

    static void StoreVmVal( const VmVal& t, int val, VmDirtyGlobal* dirty, int& nDirty ) {
      if( t.kind == 1 ) {
        for( int i = 0; i < nDirty; i++ ) if( dirty[i].symIdx == t.val && dirty[i].elem == t.elem ) { dirty[i].val = val; return; }
        if( nDirty < 64 ) dirty[nDirty++] = { t.val, t.elem, val };
      } else if( t.kind == 2 && t.npc ) {
        switch( t.val ) {
          case FLD_NPC_LP:        t.npc->lp = val; break;
          case FLD_NPC_ATTRIBUTE: if( t.elem >= 0 && t.elem < 8 ) t.npc->attribute[t.elem] = val; break;
          case FLD_NPC_HITCHANCE: if( t.elem >= 0 && t.elem < 5 ) t.npc->hitChance[t.elem] = val; if( t.elem >= 0 && t.elem < 64 ) t.npc->talent[t.elem] = val; break;
          case FLD_NPC_AIVAR:     if( t.elem >= 0 && t.elem < 300 ) t.npc->aivar[t.elem] = val; break;
          case FLD_NPC_LEVEL:     t.npc->level = val; break;
        }
      }
    }

    int MeasureOnePointPrice( const ScanWrap& tf, int st, int trainedVal, const ScanCounter counter[ST_COUNT] ) const {
      if( tf.funcSym < 0 || tf.funcSym >= nSyms || tf.statValue[st] < 0 ) return -1;
      const ScanSym& fnSym = syms[tf.funcSym];
      if( fnSym.address <= 0 || fnSym.address >= codeSize ) return -1;

      VmNpc hero = {}, teacher = {};
      hero.attribute[0] = hero.attribute[1] = 40; hero.attribute[2] = hero.attribute[3] = hero.attribute[4] = hero.attribute[5] = 10;
      for( int i = 1; i <= 4; i++ ) hero.hitChance[i] = hero.talent[i] = hero.talent[7 + i] = 10;
      const int startLp = 100000;
      for( int i = 0; i < 8; i++ )  teacher.attribute[i] = startLp;
      for( int i = 0; i < 64; i++ ) teacher.talent[i] = startLp;
      hero.lp = startLp;
      if( st <= ST_MANA ) hero.attribute[st == ST_STR ? 4 : st == ST_DEX ? 5 : 3] = trainedVal;
      else if( st >= ST_1H && st <= ST_XBOW ) {
        for( int i = 1; i <= 4; i++ ) hero.hitChance[i] = trainedVal;
        hero.talent[1 + ( st - ST_1H )] = hero.talent[8 + ( st - ST_1H )] = trainedVal;
      }

      VmDirtyGlobal dirty[64]; int nDirty = 0;
      auto setDirty = [&]( int sym, int val ) { if( sym >= 0 && nDirty < 64 ) dirty[nDirty++] = { sym, 0, val }; };
      for( int i = 0; i < ST_COUNT; i++ ) {
        const int v = ( i == st || ( st >= ST_1H && i >= ST_1H ) ) ? trainedVal : 10;
        if( counter[i].kind == CNT_AIVAR && counter[i].index >= 0 && counter[i].index < 300 ) hero.aivar[counter[i].index] = v;
        else if( counter[i].kind == CNT_GLOBAL ) setDirty( counter[i].index, v );
      }
      for( int i = 0; i < nLevelCap; i++ ) setDirty( levelCapSym[i], startLp );
      if( st <= ST_MANA && counter[st].kind == CNT_NONE ) setDirty( realAttrSym[st], ( trainedVal >= 5 ) ? ( trainedVal - 5 ) : 0 );

      VmVal stack[256]; int sp = 0;
      auto popVm  = [&]() { return sp > 0 ? stack[--sp] : VmVal{}; };
      auto pushVm = [&]( const VmVal& v ) { if( sp < 255 ) stack[sp++] = v; };

      for( int p = 0; p < fnSym.count && sp < 250; p++ ) {
        if( p == tf.npcParam ) stack[sp++] = { 3, 0, 0, &hero };
        else if( p < tf.npcParam || ( tf.funcSym + 1 + p < nSyms && syms[tf.funcSym + 1 + p].type == SCAN_TYPE_INSTANCE ) ) stack[sp++] = { 3, 0, 0, &teacher };
        else if( p == tf.statParam )   stack[sp++] = { 0, tf.statValue[st], 0, 0 };
        else if( p == tf.amountParam ) stack[sp++] = { 0, 1, 0, 0 };
        else if( p == tf.limitParam )  stack[sp++] = { 0, startLp, 0, 0 };
        else                           stack[sp++] = {};
      }

      VmFrame frames[32]; int fp = 0, pc = fnSym.address, steps = 0, nInst = 0, instSym[32]; VmNpc *curNpc = 0, *instPtr[32];
      while( pc >= 0 && pc < codeSize && steps++ < 20000 ) {
        int arg = 0, idx = 0, nxt = pc;
        const int op = ScanNextOp( code, codeSize, nxt, arg, idx );
        if( op < 0 ) break;
        if( op == 60 ) { if( fp == 0 ) break; --fp; pc = frames[fp].retPc; curNpc = frames[fp].curNpc; continue; }
        if( op == 64 ) pushVm( { 0, arg, 0, 0 } );
        else if( op == 65 || op == 245 ) {
          const int el = ( op == 245 ) ? idx : 0; VmVal v = { 0, 0, el, 0 };
          if( arg >= 0 && arg < nSyms ) v = ( fldKind[arg] >= FLD_NPC_LP && fldKind[arg] <= FLD_NPC_OTHER ) ? VmVal{ 2, fldKind[arg], el, curNpc } : VmVal{ 1, arg, el, 0 };
          pushVm( v );
        } else if( op == 67 || op == 80 ) {
          VmNpc* resolved = ( arg == symHero || arg == symOther ) ? &hero : &teacher;
          if( arg != symHero && arg != symOther && arg != symSelf ) for( int ii = nInst - 1; ii >= 0; ii-- ) if( instSym[ii] == arg ) { resolved = instPtr[ii]; break; }
          if( op == 67 ) pushVm( { 3, arg, 0, resolved } ); else curNpc = resolved;
        } else if( op == 74 ) {
          const VmVal t = popVm(), v = popVm(); VmNpc* npc = v.npc ? v.npc : &teacher; bool updated = false;
          for( int ii = 0; ii < nInst; ii++ ) if( instSym[ii] == t.val ) { instPtr[ii] = npc; updated = true; break; }
          if( !updated && nInst < 32 ) { instSym[nInst] = t.val; instPtr[nInst++] = npc; }
        } else if( op == 9 || ( op >= 19 && op <= 22 ) || ( op >= 70 && op <= 73 ) ) {
          const VmVal t = popVm(), rv = popVm(); int rval = EvalVmVal( rv, dirty, nDirty );
          if( op >= 19 && op <= 22 ) { const int oldv = EvalVmVal( t, dirty, nDirty ); rval = op == 19 ? oldv + rval : op == 20 ? oldv - rval : op == 21 ? oldv * rval : ( rval ? oldv / rval : 0 ); }
          StoreVmVal( t, rval, dirty, nDirty );
        } else if( op <= 8 || ( op >= 11 && op <= 18 ) ) {
          const int a = EvalVmVal( popVm(), dirty, nDirty ), b = EvalVmVal( popVm(), dirty, nDirty ); int r = 0;
          switch( op ) {
            case 0: r = a + b; break; case 1: r = a - b; break; case 2: r = a * b; break; case 3: r = b ? a / b : 0; break; case 4: r = b ? a % b : 0; break;
            case 5: r = a | b; break; case 6: r = a & b; break; case 7: r = a < b; break; case 8: r = a > b; break;
            case 11: r = ( a != 0 || b != 0 ); break; case 12: r = ( a != 0 && b != 0 ); break; case 13: r = a << b; break; case 14: r = a >> b; break;
            case 15: r = a <= b; break; case 16: r = a == b; break; case 17: r = a != b; break; case 18: r = a >= b; break;
          }
          pushVm( { 0, r, 0, 0 } );
        } else if( op >= 30 && op <= 33 ) {
          int a = EvalVmVal( popVm(), dirty, nDirty );
          if( op == 31 ) a = -a; else if( op == 32 ) a = ( a == 0 ); else if( op == 33 ) a = ~a;
          pushVm( { 0, a, 0, 0 } );
        } else if( op == 75 ) { pc = arg; continue; }
        else if( op == 76 )   { if( EvalVmVal( popVm(), dirty, nDirty ) == 0 ) { pc = arg; continue; } }
        else if( op == 61 ) {
          const int calleeIdx = FuncByAddr( arg );
          if( calleeIdx < 0 || IsStubFunc( calleeIdx ) ) {
            const int na = ( calleeIdx >= 0 ) ? syms[calleeIdx].count : 0;
            for( int i = 0; i < na && sp > 0; i++ ) sp--;
            if( calleeIdx >= 0 && ( syms[calleeIdx].flags & SCAN_FLAG_RETURN ) ) pushVm( { 0, ScanStrEq( syms[calleeIdx].name, "B_GIVEINVITEMS" ) ? 1 : 0, 0, 0 } );
          } else if( fp < 32 ) { frames[fp++] = { nxt, curNpc }; pc = arg; continue; }
        } else if( op == 62 && arg >= 0 && arg < nSyms ) {
          const ScanSym& ext = syms[arg]; const int na = ext.count > 16 ? 16 : ext.count;
          VmVal args[16]; int ev[16];
          for( int i = na - 1; i >= 0; i-- ) { args[i] = popVm(); ev[i] = EvalVmVal( args[i], dirty, nDirty ); }
          int rv = 0;
          if( arg == extChangeAttr && na == 3 && args[0].npc && ev[1] >= 0 && ev[1] < 8 )       args[0].npc->attribute[ev[1]] += ev[2];
          else if( arg == extSetTalent && na == 3 && args[0].npc && ev[1] >= 0 && ev[1] < 64 )   args[0].npc->talentSkill[ev[1]] = ev[2];
          else if( arg == extSetTalentVal && na == 3 && args[0].npc && ev[1] >= 0 && ev[1] < 64 ) args[0].npc->talent[ev[1]] = ev[2];
          else if( arg == extGetTalent && na == 2 && args[0].npc && ev[1] >= 0 && ev[1] < 64 )    rv = args[0].npc->talentSkill[ev[1]];
          else if( arg == extGetTalentVal && na == 2 && args[0].npc && ev[1] >= 0 && ev[1] < 64 ) rv = args[0].npc->talent[ev[1]];
          else if( arg == extHasItems )              rv = startLp;
          else if( arg == extIsPlayer && na >= 1 )   rv = ( args[0].npc == &hero ) ? 1 : 0;
          else if( arg == extIsValidNpc )            rv = 1;
          else if( arg == extGetAtrCost && na >= 2 ) rv = ( ev[1] > ev[0] ) ? ( ev[1] - ev[0] ) : 1;
          if( ext.flags & SCAN_FLAG_RETURN ) pushVm( { 0, rv, 0, 0 } );
        }
        pc = nxt;
      }

      int afterStat = trainedVal;
      if( st <= ST_MANA ) afterStat = hero.attribute[st == ST_STR ? 4 : st == ST_DEX ? 5 : 3];
      else if( st >= ST_1H && st <= ST_XBOW ) {
        afterStat = hero.hitChance[st - ST_1H + 1];
        if( afterStat == trainedVal && hero.talent[8 + ( st - ST_1H )] != trainedVal ) afterStat = hero.talent[8 + ( st - ST_1H )];
        if( afterStat == trainedVal && hero.talent[1 + ( st - ST_1H )] != trainedVal ) afterStat = hero.talent[1 + ( st - ST_1H )];
      }
      return ( afterStat - trainedVal == 1 ) ? ( startLp - hero.lp ) : -1;
    }
  };

  inline int StatIndexFromField( int field, int idx ) {
    if( field == FLD_NPC_ATTRIBUTE ) return idx == 4 ? ST_STR : idx == 5 ? ST_DEX : idx == 3 ? ST_MANA : -1;
    if( field == FLD_NPC_HITCHANCE ) return ( idx >= 1 && idx <= 4 ) ? ST_1H + ( idx - 1 ) : ( idx >= 8 && idx <= 11 ) ? ST_1H + ( idx - 8 ) : -1;
    return -1;
  }
  inline bool TeachesWho( const ScanVec<ScanItem>& items, const ScanInstRef& who, bool* hasStatOut = 0, bool* hasTalOut = 0 ) {
    bool hasLp = false, hasSt = false, hasTs = false;
    for( int i = 0; i < items.len; i++ ) {
      const ScanItem& it = items[i]; if( !InstEq( it.inst, who ) ) continue;
      if( it.kind == ITEM_LPDEC ) hasLp = true; else if( it.kind == ITEM_STATINC ) hasSt = true; else if( it.kind == ITEM_TALENTSET ) hasTs = true;
    }
    if( hasStatOut ) *hasStatOut = hasSt;
    if( hasTalOut )  *hasTalOut = hasTs;
    return hasLp && ( hasSt || hasTs );
  }
  inline bool FormatStatLabel( const ScanItem& it, char* out, int cap ) {
    if( it.kind != ITEM_STATINC || it.op1.kind != OPND_CONST ) return false;
    static const char* const stNames[ST_COUNT] = { "STR", "DEX", "MANA_MAX", "1H", "2H", "BOW", "CROSSBOW" };
    const int st = StatIndexFromField( it.field, it.op1.val ); if( st < 0 ) return false;
    if( it.op2.kind == OPND_CONST ) snprintf( out, (size_t)cap, "%s+%d", stNames[st], it.op2.val );
    else                            snprintf( out, (size_t)cap, "%s+?", stNames[st] );
    return true;
  }
  inline bool ReadsTraining( const ScanVec<ScanItem>& items, const bool aivarInReal[300] ) {
    for( int i = 0; i < items.len; i++ ) {
      const ScanItem& it = items[i]; if( it.kind != ITEM_READ || !InstIsHeroish( it.inst ) ) continue;
      if( ( it.field == FLD_NPC_ATTRIBUTE && ( it.idx == 3 || it.idx == 4 || it.idx == 5 ) ) || ( it.field == FLD_NPC_HITCHANCE && it.idx >= 1 && it.idx <= 4 ) || ( it.field == FLD_NPC_AIVAR && it.idx >= 0 && it.idx < 300 && aivarInReal[it.idx] ) ) return true;
    }
    return false;
  }

  inline void ScanRun( const ScanSym* syms, int nSyms, const unsigned char* code, int codeSize, int engineVer = 2, ScanExt* ext = 0 ) {
    ScanReset(); ScanResult& res = Scan(); res.engine = engineVer; res.symbols = nSyms;
    if( !syms || nSyms <= 0 || !code || codeSize <= 0 ) return;
    ScanContext ctx; ctx.Init( syms, nSyms, code, codeSize );

    struct InfoInst { int symIdx, npcSym, infSym, condSym; };
    ScanVec<InfoInst> infos; infos.Init();
    for( int i = 0; i < nSyms; i++ ) {
      if( ctx.ignored[i] || syms[i].type != SCAN_TYPE_INSTANCE || syms[i].address <= 0 ) continue;
      const ScanDirectBody& db = ctx.GetBody( i );
      if( db.hasInfoAssign && db.infoInformation.kind != OPND_UNK )
        infos.Push( { i, db.infoNpc.kind == OPND_CONST ? db.infoNpc.val : -1, db.infoInformation.kind == OPND_CONST ? db.infoInformation.val : -1, db.infoCondition.kind == OPND_CONST ? db.infoCondition.val : -1 } );
    }
    res.cInfo = infos.len;

    int votes[ST_COUNT][300] = {};
    for( int i = 0; i < nSyms; i++ ) {
      if( ctx.ignored[i] || syms[i].type != SCAN_TYPE_FUNC || ( syms[i].flags & SCAN_FLAG_EXTERNAL ) || syms[i].address <= 0 ) continue;
      const ScanVec<ScanItem>& items = ctx.GetSummary( i ); bool anyStat = false;
      for( int j = 0; j < items.len; j++ ) if( items[j].kind == ITEM_STATINC && items[j].guard.n > 0 && items[j].op1.kind == OPND_CONST ) { anyStat = true; break; }
      if( !anyStat ) continue;
      for( int a = 0; a < items.len; a++ ) {
        const ScanItem& av = items[a];
        const bool isGlob = av.kind == ITEM_GLOBALINC && av.guard.n > 0 && av.idx >= 0 && av.idx < nSyms;
        const bool isAiv  = av.kind == ITEM_AIVARINC  && av.guard.n > 0 && av.idx >= 0 && av.idx < 300;
        if( !isGlob && !isAiv ) continue;
        for( int s = 0; s < items.len; s++ ) {
          const ScanItem& st = items[s];
          if( st.kind == ITEM_STATINC && st.guard.n > 0 && st.op1.kind == OPND_CONST && ( isGlob || InstEq( st.inst, av.inst ) ) && GuardEq( st.guard, av.guard ) && OpndEq( st.op2, av.op2 ) ) {
            const int stIdx = StatIndexFromField( st.field, st.op1.val ); if( stIdx < 0 || stIdx >= ST_COUNT ) continue;
            if( isGlob && ctx.learnedSym[stIdx] < 0 ) ctx.learnedSym[stIdx] = av.idx; else if( isAiv ) votes[stIdx][av.idx]++;
          }
        }
      }
    }

    bool aivarInReal[300] = {};
    for( int st = 0; st < ST_COUNT; st++ ) {
      int bestAv = -1, bestCnt = 0;
      for( int av = 0; av < 300; av++ ) if( votes[st][av] > bestCnt ) { bestCnt = votes[st][av]; bestAv = av; }
      if( bestAv >= 0 ) { res.counter[st] = { CNT_AIVAR, bestAv }; aivarInReal[bestAv] = true; if( ext ) ext->realCounter[st] = bestAv; }
      else if( ctx.learnedSym[st] >= 0 ) res.counter[st] = { CNT_GLOBAL, ctx.learnedSym[st] };
    }

    unsigned char *inWrap = (unsigned char*)calloc( (size_t)nSyms + 1, 1 ), *inTeachOpt = (unsigned char*)calloc( (size_t)nSyms + 1, 1 );
    int *optStatMask = (int*)calloc( (size_t)nSyms + 1, sizeof( int ) ), *optLimitParam = (int*)malloc( ( (size_t)nSyms + 1 ) * sizeof( int ) );
    for( int i = 0; i <= nSyms; i++ ) optLimitParam[i] = -1;

    ScanVec<ScanTeacherInfo> teacherList; teacherList.Init();
    const auto cmpInt = []( int a, int b ) { return a - b; };

    for( int ii = 0; ii < infos.len; ii++ ) {
      const int npcSym = infos[ii].npcSym, infSym = infos[ii].infSym, condSym = infos[ii].condSym;
      if( infSym < 0 || infSym >= nSyms ) continue;

      ScanVec<int> graph, todo, teachingFns, infoLimits; ScanVec<ScanTeachOpt> taught;
      InitVecs( graph, todo, teachingFns, infoLimits, taught ); todo.Push( infSym );
      while( todo.len > 0 && graph.len < 400 ) {
        const int f = todo.Pop();
        if( f < 0 || f >= nSyms || ctx.ignored[f] || syms[f].type != SCAN_TYPE_FUNC || graph.Has( f ) ) continue;
        graph.Push( f );
        const ScanVec<ScanItem>& sum = ctx.GetSummary( f );
        for( int k = 0; k < sum.len; k++ ) if( sum[k].kind == ITEM_CHOICE && sum[k].op1.kind == OPND_CONST ) todo.Push( sum[k].op1.val );
      }

      bool statTeach = false;
      for( int gi = 0; gi < graph.len; gi++ ) {
        const int f = graph[gi]; const ScanVec<ScanItem>& items = ctx.GetSummary( f );
        const ScanInstRef heroes[2] = { MakeInst( INST_OTHER ), MakeInst( INST_HERO ) };
        for( int hi = 0; hi < 2; hi++ ) {
          bool hasSt = false, hasTs = false;
          if( !TeachesWho( items, heroes[hi], &hasSt, &hasTs ) ) continue;
          teachingFns.AddUnique( f ); inTeachOpt[f] = 1;
          ScanTeachOpt topt = {}; topt.optSym = f;
          for( int k = 0; k < items.len; k++ ) {
            const ScanItem& it = items[k];
            if( it.kind == ITEM_STATINC && InstEq( it.inst, heroes[hi] ) ) {
              if( it.op1.kind == OPND_CONST ) { const int stIdx = StatIndexFromField( it.field, it.op1.val ); if( stIdx >= 0 ) optStatMask[f] |= ( 1 << stIdx ); }
              char lbl[24];
              if( FormatStatLabel( it, lbl, sizeof( lbl ) ) ) {
                int pos = 0; for( ; pos < topt.nLabels && strcmp( topt.labels[pos], lbl ) < 0; pos++ );
                if( ( pos >= topt.nLabels || strcmp( topt.labels[pos], lbl ) != 0 ) && topt.nLabels < 8 ) {
                  for( int m = topt.nLabels++; m > pos; m-- ) memcpy( topt.labels[m], topt.labels[m - 1], 24 );
                  memcpy( topt.labels[pos], lbl, 24 );
                }
              }
            } else if( it.kind == ITEM_LIMIT && it.op1.kind == OPND_CONST ) {
              infoLimits.AddUnique( it.op1.val ); ScanInsertUnique( topt.limits, topt.nLimits, 8, it.op1.val, cmpInt );
            }
          }
          if( topt.nLabels > 0 ) { statTeach = true; taught.Push( topt ); }
          else if( hasTs ) { strcpy_s( topt.labels[0], sizeof( topt.labels[0] ), "skill" ); topt.nLabels = 1; taught.Push( topt ); }
          break;
        }
        if( ext ) for( int k = 0; k < items.len; k++ ) if( items[k].kind == ITEM_PRICEFN && items[k].idx >= 0 ) AddNameCount( ext->priceTextFns, items[k].idx );
      }

      if( teachingFns.len > 0 ) {
        ScanVec<int> menus, cluster; InitVecs( menus, cluster );
        for( int i = 0; i < teachingFns.len; i++ ) cluster.AddUnique( teachingFns[i] );
        for( int gi = 0; gi < graph.len; gi++ ) {
          const int f = graph[gi]; const ScanVec<ScanItem>& sum = ctx.GetSummary( f );
          for( int k = 0; k < sum.len; k++ ) if( sum[k].kind == ITEM_CHOICE && sum[k].op1.kind == OPND_CONST && teachingFns.Has( sum[k].op1.val ) ) { menus.AddUnique( f ); cluster.AddUnique( f ); break; }
        }
        for( int mi = 0; mi < menus.len; mi++ ) {
          const ScanVec<ScanItem>& sum = ctx.GetSummary( menus[mi] );
          for( int k = 0; k < sum.len; k++ ) if( sum[k].kind == ITEM_CHOICE && sum[k].op1.kind == OPND_CONST ) cluster.AddUnique( sum[k].op1.val );
        }
        if( cluster.Has( infSym ) && condSym >= 0 && condSym < nSyms && !ctx.ignored[condSym] && syms[condSym].type == SCAN_TYPE_FUNC ) cluster.AddUnique( condSym );
        if( statTeach )
          for( int ci = 0; ci < cluster.len; ci++ ) {
            const int f = cluster[ci];
            if( f >= 0 && f < nSyms && !ctx.ignored[f] && syms[f].type == SCAN_TYPE_FUNC && ( teachingFns.Has( f ) || ReadsTraining( ctx.GetSummary( f ), aivarInReal ) ) ) inWrap[f] = 1;
          }
        FreeVecs( menus, cluster );

        char tName[64];
        if( npcSym >= 0 && npcSym < nSyms && syms[npcSym].name ) ScanToUpper( syms[npcSym].name, tName, sizeof( tName ) ); else strcpy_s( tName, sizeof( tName ), "?" );
        int tIdx = -1;
        for( int ti = 0; ti < teacherList.len; ti++ ) if( strcmp( teacherList[ti].name, tName ) == 0 ) { tIdx = ti; break; }
        if( tIdx < 0 ) {
          ScanTeacherInfo nt = {}; nt.npcSym = npcSym; strcpy_s( nt.name, sizeof( nt.name ), tName );
          InitVecs( nt.dialogs, nt.teach, nt.limits ); teacherList.Push( nt ); tIdx = teacherList.len - 1;
        }
        ScanTeacherInfo& ti = teacherList[tIdx]; ti.dialogs.Push( infos[ii].symIdx );
        for( int k = 0; k < taught.len; k++ )     ti.teach.Push( taught[k] );
        for( int k = 0; k < infoLimits.len; k++ ) ti.limits.AddUnique( infoLimits[k] );
      }
      FreeVecs( graph, todo, teachingFns, taught, infoLimits );
    }

    res.teachers = teacherList.len;
    int statTeachCount = 0;
    for( int ti = 0; ti < teacherList.len; ti++ ) {
      teacherList[ti].limits.Sort( cmpInt );
      for( int k = 0; k < teacherList[ti].teach.len; k++ ) {
        const ScanTeachOpt& o = teacherList[ti].teach[k];
        if( o.nLabels > 0 && !( o.nLabels == 1 && strcmp( o.labels[0], "skill" ) == 0 ) ) { statTeachCount++; break; }
      }
    }
    res.statTeachers = statTeachCount;

    ScanVec<ScanGenericFn> genList; genList.Init();
    for( int i = 0; i < nSyms; i++ ) {
      if( ctx.ignored[i] || syms[i].type != SCAN_TYPE_FUNC || ( syms[i].flags & SCAN_FLAG_EXTERNAL ) || syms[i].address <= 0 ) continue;
      const ScanVec<ScanItem>& items = ctx.GetSummary( i );
      for( int k = 0; k < syms[i].count; k++ ) {
        if( !TeachesWho( items, MakeInst( INST_PARAM, k ) ) ) continue;
        const ScanDirectBody& db = ctx.GetBody( i );
        ScanGenericFn gf = {}; gf.funcSym = i; gf.npcParam = k; gf.directLp = ( db.costs.len > 0 ); gf.model = COST_INLINE;
        if( gf.directLp )
          for( int ci = 0; ci < db.costs.len; ci++ ) {
            const ScanCostSummary& cs = db.costs[ci];
            for( int c = 0; c < cs.nCalls; c++ ) { const int cSym = cs.calls[c]; if( gf.nCostFns < 4 ) gf.costFns[gf.nCostFns++] = cSym; if( ext ) AddNameCount( ext->costFns, cSym ); }
            gf.model = cs.isPerPoint ? COST_PER_POINT_TIMES_N : ( cs.nCalls > 0 ? COST_FUNCTION : COST_INLINE );
          }
        genList.Push( gf ); break;
      }
    }

    ScanVec<ScanWrap> trainFnVec; trainFnVec.Init();
    for( int gi = 0; gi < genList.len; gi++ ) {
      const ScanGenericFn& gf = genList[gi]; const int fSym = gf.funcSym; const ScanVec<ScanItem>& items = ctx.GetSummary( fSym );
      ScanWrap tw = {}; tw.funcSym = fSym; tw.addr = syms[fSym].address; tw.kind = WRAP_TRAIN;
      tw.limitParam = tw.statParam = tw.amountParam = tw.statGroup = tw.tierBase = -1; tw.npcParam = gf.npcParam;
      for( int s = 0; s < ST_COUNT; s++ ) tw.statValue[s] = -1;

      const ScanInstRef targetInst = MakeInst( INST_PARAM, gf.npcParam );
      for( int k = 0; k < items.len; k++ ) {
        const ScanItem& it = items[k];
        if( it.kind == ITEM_STATINC && InstEq( it.inst, targetInst ) && it.op2.kind == OPND_PARAM ) {
          if( it.op1.kind == OPND_CONST && it.guard.n > 0 ) {
            const int stIdx = StatIndexFromField( it.field, it.op1.val );
            if( stIdx >= 0 && stIdx < ST_COUNT ) {
              tw.statMask |= ( 1 << stIdx ); tw.statGroup = ( stIdx < ST_1H ) ? 0 : 1; tw.amountParam = it.op2.val; tw.statParam = it.guard.p[0];
              if( tw.statValue[stIdx] < 0 || it.guard.v[0] == ctx.statConstVal[stIdx] ) tw.statValue[stIdx] = it.guard.v[0];
            }
          } else if( it.field == FLD_NPC_ATTRIBUTE && it.op1.kind == OPND_PARAM && syms[fSym].count >= 3 ) {
            tw.statMask |= ( 1 << ST_STR ) | ( 1 << ST_DEX ) | ( 1 << ST_MANA ); tw.statGroup = 0; tw.amountParam = it.op2.val; tw.statParam = it.op1.val;
            for( int s = ST_STR; s <= ST_MANA; s++ ) tw.statValue[s] = ctx.statConstVal[s];
          } else if( it.field == FLD_NPC_HITCHANCE && ( it.op1.kind == OPND_UNK || it.op1.kind == OPND_PARAM ) && syms[fSym].count >= 3 ) {
            tw.statMask |= ( 1 << ST_1H ) | ( 1 << ST_2H ) | ( 1 << ST_BOW ) | ( 1 << ST_XBOW ); tw.statGroup = 1; tw.amountParam = it.op2.val; tw.statParam = ( it.op1.kind == OPND_PARAM ) ? it.op1.val : 1;
            for( int s = 0; s < 4; s++ ) tw.statValue[ST_1H + s] = 1 + s;
          }
        }
      }
      for( int k = 0; k < items.len; k++ ) {
        const ScanItem& it = items[k];
        if( it.kind == ITEM_LIMIT && it.op1.kind == OPND_PARAM && it.op1.val != tw.npcParam && it.op1.val != tw.statParam && it.op1.val != tw.amountParam ) { tw.limitParam = it.op1.val; break; }
      }
      if( tw.statMask != 0 && tw.statParam >= 0 && tw.amountParam >= 0 ) trainFnVec.Push( tw );
    }

    trainFnVec.Sort( []( const ScanWrap& a, const ScanWrap& b ) { return a.addr - b.addr; } );
    res.nTrainFn = trainFnVec.len;
    if( res.nTrainFn > 0 ) {
      res.trainFn = (ScanWrap*)calloc( (size_t)res.nTrainFn, sizeof( ScanWrap ) );
      res.nTierPool = res.nTrainFn * ST_COUNT; res.tierPool = (ScanTiers*)calloc( (size_t)res.nTierPool, sizeof( ScanTiers ) );
      for( int i = 0; i < res.nTrainFn; i++ ) {
        trainFnVec[i].tierBase = i * ST_COUNT; res.trainFn[i] = trainFnVec[i];
        for( int s = 0; s < ST_COUNT; s++ ) res.tierPool[i * ST_COUNT + s].costFuncSym = -1;
      }
    }
    trainFnVec.Free();

    int* trainCallCount = (int*)calloc( (size_t)res.nTrainFn + 1, sizeof( int ) );
    for( int i = 0; i < nSyms; i++ ) {
      if( ctx.ignored[i] || syms[i].type != SCAN_TYPE_FUNC || ( syms[i].flags & SCAN_FLAG_EXTERNAL ) || syms[i].address <= 0 ) continue;
      const ScanDirectBody& db = ctx.GetBody( i );
      for( int c = 0; c < db.calls.len; c++ ) for( int ti = 0; ti < res.nTrainFn; ti++ ) if( db.calls[c].calleeSym == res.trainFn[ti].funcSym ) trainCallCount[ti]++;
    }

    int bestCallsForStat[ST_COUNT]; for( int s = 0; s < ST_COUNT; s++ ) bestCallsForStat[s] = -1;
    for( int ti = 0; ti < res.nTrainFn; ti++ ) {
      const ScanWrap& tf = res.trainFn[ti]; int model = COST_INLINE, costFnSym = -1;
      for( int gi = 0; gi < genList.len; gi++ ) if( genList[gi].funcSym == tf.funcSym ) { model = genList[gi].model; if( genList[gi].nCostFns > 0 ) costFnSym = genList[gi].costFns[0]; break; }
      for( int s = 0; s < ST_COUNT; s++ ) {
        ScanTiers& dst = res.tierPool[tf.tierBase + s];
        dst.model = model; dst.costFuncSym = costFnSym; dst.measured = dst.n = 0;
        if( tf.statValue[s] < 0 ) continue;
        const int top = ( s < ST_1H ) ? 400 : 101; int prices[400], accepted = 0;
        for( int v = 0, p; v < top && ( p = ctx.MeasureOnePointPrice( tf, s, v, res.counter ) ) > 0; v++ ) prices[accepted++] = p;
        if( accepted >= 30 ) {
          for( int v = accepted; v < top; v++ ) prices[v] = prices[accepted - 1];
          dst.measured = 1; dst.price[0] = prices[0];
          for( int v = 1; v < top; v++ ) if( prices[v] != prices[v - 1] && dst.n < SCAN_MAX_TIERS ) { dst.threshold[dst.n] = v; dst.price[dst.n + 1] = prices[v]; dst.n++; }
          if( trainCallCount[ti] > bestCallsForStat[s] ) { bestCallsForStat[s] = trainCallCount[ti]; res.statTiers[s] = dst; }
        }
      }
    }
    free( trainCallCount );

    int symMnozenie = -1;
    for( int i = 0; i < nSyms; i++ ) if( ScanStrEq( syms[i].name, "MNOZENIE" ) || ScanStrEq( syms[i].name, "MIECZ1H" ) ) { symMnozenie = i; break; }
    if( symMnozenie >= 0 ) {
      for( int s = ST_1H; s <= ST_XBOW; s++ ) {
        ScanTiers& t = res.statTiers[s]; t.model = COST_INLINE; t.costFuncSym = -1; t.measured = 1; t.n = 4;
        const int th[4] = { 25, 50, 75, 90 };
        for( int k = 0; k < 4; k++ ) t.threshold[k] = th[k];
        for( int k = 0; k < 5; k++ ) t.price[k] = k + 1;
        if( res.tierPool )
          for( int ti = 0; ti < res.nTrainFn; ti++ )
            if( res.trainFn[ti].statGroup == 1 && res.trainFn[ti].tierBase >= 0 && res.trainFn[ti].tierBase + s < res.nTierPool ) res.tierPool[res.trainFn[ti].tierBase + s] = t;
      }
    }

    struct RuleStrPair { char text[80]; ScanRule rule; bool validForEngine; };
    ScanVec<RuleStrPair> rulePairs; rulePairs.Init();
    for( int gi = 0; gi < genList.len; gi++ ) {
      const ScanVec<ScanItem>& items = ctx.GetSummary( genList[gi].funcSym );
      for( int k = 0; k < items.len; k++ ) {
        const ScanItem& it = items[k]; if( it.kind != ITEM_TALENTSET || it.thr.n == 0 ) continue;
        for( int ti = 0; ti < it.thr.n; ti++ ) {
          const ScanThrCond& tc = it.thr.c[ti];
          const char* fName = ( tc.field == FLD_NPC_ATTRIBUTE ) ? "ATTRIBUTE" : "HITCHANCE";
          const char* opName = tc.op == RULE_GT ? "GT" : tc.op == RULE_LE ? "LE" : tc.op == RULE_LT ? "LT" : "GE";
          char talBuf[16] = "?", valBuf[16] = "?";
          if( it.op1.kind == OPND_CONST ) snprintf( talBuf, sizeof( talBuf ), "%d", it.op1.val );
          if( it.op2.kind == OPND_CONST ) snprintf( valBuf, sizeof( valBuf ), "%d", it.op2.val );
          RuleStrPair rp = {};
          snprintf( rp.text, sizeof( rp.text ), "%s[%d] %s %d -> talent %s = %s", fName, tc.idx, opName, tc.val, talBuf, valBuf );
          const int stIdx = StatIndexFromField( tc.field, tc.idx );
          if( stIdx >= 0 && it.op1.kind == OPND_CONST && it.op1.val > 0 && it.op2.kind == OPND_CONST ) {
            rp.validForEngine = true; rp.rule = { stIdx, tc.op, tc.val, it.op1.val, it.op2.val };
          }
          rulePairs.InsertUnique( rp, []( const RuleStrPair& a, const RuleStrPair& b ) { return strcmp( a.text, b.text ); } );
        }
      }
    }
    for( int i = 0; i < rulePairs.len; i++ ) {
      if( rulePairs[i].validForEngine && res.nRules < SCAN_MAX_RULES ) res.rule[res.nRules++] = rulePairs[i].rule;
      if( ext && ext->nRuleStrings < 64 ) strcpy_s( ext->ruleStrings[ext->nRuleStrings++], sizeof( ext->ruleStrings[0] ), rulePairs[i].text );
    }
    rulePairs.Free();

    int wrapCount = 0; for( int i = 0; i < nSyms; i++ ) if( inWrap[i] ) wrapCount++;
    res.nWrap = wrapCount;
    if( wrapCount > 0 ) {
      res.wrap = (ScanWrap*)calloc( (size_t)wrapCount, sizeof( ScanWrap ) );
      for( int i = 0, wi = 0; i < nSyms; i++ ) {
        if( !inWrap[i] ) continue;
        ScanWrap w = {}; w.funcSym = i; w.addr = syms[i].address; w.kind = inTeachOpt[i] ? WRAP_OPTION : WRAP_READER;
        w.statMask = optStatMask[i]; w.limitParam = optLimitParam[i]; w.npcParam = w.statParam = w.amountParam = w.statGroup = w.tierBase = -1;
        for( int s = 0; s < ST_COUNT; s++ ) w.statValue[s] = -1;
        res.wrap[wi++] = w;
      }
    }
    ScanIndexWraps( codeSize );

    if( ctx.symPcHero >= 0 && syms[ctx.symPcHero].address > 0 ) {
      ScanVec<ScanRawOp> ops; ScanCollectRawOps( code, syms[ctx.symPcHero].address, ctx.BodyEnd( syms[ctx.symPcHero].address ), ops );
      for( int j = 1; j + 1 < ops.len; j++ ) {
        if( ops[j].op == 245 && ops[j].arg >= 0 && ops[j].arg < nSyms && ctx.fldKind[ops[j].arg] == FLD_NPC_ATTRIBUTE && ops[j].idx == 1 && ops[j + 1].op == 9 ) {
          const int pop = ops[j - 1].op, parg = ops[j - 1].arg;
          if( pop == 64 ) { res.hpStart = parg; if( ext ) snprintf( ext->hpStartStr, sizeof( ext->hpStartStr ), "%d", parg ); }
          else if( pop == 65 && parg >= 0 && parg < nSyms ) {
            const ScanSym& ps = syms[parg];
            if( ( ps.flags & SCAN_FLAG_CONST ) && ps.count > 0 ) { res.hpStart = ScanSymInt( ps, 0 ); if( ext ) snprintf( ext->hpStartStr, sizeof( ext->hpStartStr ), "%d", res.hpStart ); }
            else { res.hpStart = -1; if( ext ) ScanToUpper( ps.name, ext->hpStartStr, sizeof( ext->hpStartStr ) ); }
          } else if( pop == 61 ) {
            res.hpStart = -1;
            if( ext ) { const int fIdx = ctx.FuncByAddr( parg ); char fnm[48]; ScanToUpper( fIdx >= 0 ? syms[fIdx].name : "?", fnm, sizeof( fnm ) ); snprintf( ext->hpStartStr, sizeof( ext->hpStartStr ), "func %s", fnm ); }
          }
        }
      }
      ops.Free();
    }

    for( int i = 0; i < nSyms; i++ ) {
      if( ctx.ignored[i] || syms[i].type != SCAN_TYPE_FUNC || ( syms[i].flags & SCAN_FLAG_EXTERNAL ) || syms[i].address <= 0 ) continue;
      const ScanDirectBody& db = ctx.GetBody( i ); bool raisesLevel = false;
      for( int k = 0; k < db.items.len; k++ ) if( db.items[k].kind == ITEM_LEVELINC && InstIsHeroish( db.items[k].inst ) ) { raisesLevel = true; break; }
      if( !raisesLevel ) continue;
      const ScanVec<ScanItem>& sum = ctx.GetSummary( i ); bool foundHp = false;
      for( int k = 0; k < sum.len; k++ ) {
        const ScanItem& it = sum[k];
        if( it.kind == ITEM_STATINC && it.field == FLD_NPC_ATTRIBUTE && it.op1.kind == OPND_CONST && it.op1.val == 1 && InstIsHeroish( it.inst ) ) {
          if( it.op2.kind == OPND_CONST ) { res.hpPerLevel = it.op2.val; res.hpPerLevelSym = -1; if( ext ) snprintf( ext->hpLevelStr, sizeof( ext->hpLevelStr ), "%d", it.op2.val ); foundHp = true; break; }
          else if( it.op2.kind == OPND_GLOBAL && it.op2.val >= 0 && it.op2.val < nSyms ) { res.hpPerLevel = -1; res.hpPerLevelSym = it.op2.val; if( ext ) ScanToUpper( syms[it.op2.val].name, ext->hpLevelStr, sizeof( ext->hpLevelStr ) ); foundHp = true; break; }
        }
      }
      if( foundHp ) break;
    }

    if( ext ) {
      ext->teachers = teacherList; ext->generic = genList;
      for( int ci = 0; ci < ext->costFns.len; ci++ ) {
        const int cSym = ext->costFns[ci].sym; ScanCostThresh ct = {}; ct.funcSym = cSym;
        if( cSym >= 0 && cSym < nSyms && syms[cSym].address > 0 ) {
          ScanVec<ScanRawOp> ops; ScanVec<int> ths; ths.Init(); ScanCollectRawOps( code, syms[cSym].address, ctx.BodyEnd( syms[cSym].address ), ops );
          for( int j = 2; j < ops.len; j++ ) {
            const int op = ops[j].op;
            if( op == 7 || op == 8 || op == 15 || op == 18 )
              for( int back = 1; back <= 4 && j - back >= 0; back++ ) if( ops[j - back].op == 64 ) { ths.AddUnique( ops[j - back].arg ); break; }
          }
          ths.Sort( cmpInt );
          for( int k = 0; k < ths.len && ct.n < 16; k++ ) ct.th[ct.n++] = ths[k];
          FreeVecs( ths, ops );
        }
        ext->costThresholds.Push( ct );
      }
    } else {
      for( int i = 0; i < teacherList.len; i++ ) FreeVecs( teacherList[i].dialogs, teacherList[i].teach, teacherList[i].limits );
      FreeVecs( teacherList, genList );
    }

    res.ok = ( res.nWrap > 0 && res.nTrainFn > 0 ) ? 1 : 0;
    free( inWrap ); free( inTeachOpt ); free( optStatMask ); free( optLimitParam );
    FreeVecs( infos, ctx );
  }
}
