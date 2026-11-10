#pragma once

#include <stdlib.h>
#include <string.h>

namespace BonusTiers {
  enum { ST_STR, ST_DEX, ST_MANA, ST_1H, ST_2H, ST_BOW, ST_XBOW, ST_COUNT };
  enum { COST_UNKNOWN, COST_PER_POINT_TIMES_N, COST_FUNCTION, COST_INLINE };
  enum { CNT_NONE, CNT_AIVAR, CNT_GLOBAL };
  enum { RULE_GE, RULE_GT, RULE_LE, RULE_LT };
  enum { WRAP_TRAIN, WRAP_OPTION, WRAP_READER };
  static const int SCAN_MAX_TIERS = 8, SCAN_MAX_RULES = 32;

  struct ScanCounter { int kind, index; };
  struct ScanTiers   { int model, costFuncSym, measured, n, threshold[SCAN_MAX_TIERS], price[SCAN_MAX_TIERS + 1]; };
  struct ScanRule    { int stat, op, value, talent, talentValue; };
  struct ScanWrap    { int funcSym, addr, kind, statMask, limitParam, npcParam, statParam, amountParam, statGroup, statValue[ST_COUNT], tierBase; };
  struct ScanResult {
    int ok, engine, symbols, cInfo, teachers, statTeachers; double seconds;
    ScanCounter counter[ST_COUNT]; ScanTiers statTiers[ST_COUNT];
    int nRules; ScanRule rule[SCAN_MAX_RULES];
    int nWrap; ScanWrap* wrap; int nTrainFn; ScanWrap* trainFn; int nTierPool; ScanTiers* tierPool;
    int hpStart, hpPerLevel, hpPerLevelSym;
  };
  struct ScanStore { ScanResult res; unsigned char* bits; int codeSize; };

  inline ScanStore& ScanStorage() { static ScanStore s; return s; }
  inline ScanResult& Scan() { return ScanStorage().res; }
  inline bool ScanIsWrap( int addr ) { ScanStore& s = ScanStorage(); return s.bits && (unsigned)addr < (unsigned)s.codeSize && ( ( s.bits[addr >> 3] >> ( addr & 7 ) ) & 1 ); }

  inline const ScanWrap* ScanFindWrap( int addr ) {
    ScanResult& r = Scan();
    for( int lo = 0, hi = r.nWrap - 1; lo <= hi; ) {
      const int mid = ( lo + hi ) >> 1;
      if( r.wrap[mid].addr == addr ) return &r.wrap[mid];
      if( r.wrap[mid].addr < addr ) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
  }
  inline const ScanWrap* ScanFindTrainFn( int addr ) {
    ScanResult& r = Scan(); for( int i = 0; i < r.nTrainFn; i++ ) if( r.trainFn[i].addr == addr ) return &r.trainFn[i];
    return 0;
  }
  inline int ScanStatOf( const ScanWrap* fn, int value ) {
    if( fn ) for( int s = 0; s < ST_COUNT; s++ ) if( fn->statValue[s] == value ) return s;
    return -1;
  }
  inline const ScanTiers& ScanTiersOf( const ScanWrap* fn, int stat ) {
    static ScanTiers none; ScanResult& r = Scan();
    if( stat < 0 || stat >= ST_COUNT ) return none;
    return ( fn && fn->tierBase >= 0 && r.tierPool && fn->tierBase + stat < r.nTierPool ) ? r.tierPool[fn->tierBase + stat] : r.statTiers[stat];
  }

  inline void ScanIndexWraps( int codeSize ) {
    ScanStore& s = ScanStorage(); ScanResult& r = s.res;
    for( int i = 1, j; i < r.nWrap; i++ ) {
      ScanWrap w = r.wrap[i];
      for( j = i - 1; j >= 0 && r.wrap[j].addr > w.addr; j-- ) r.wrap[j + 1] = r.wrap[j];
      r.wrap[j + 1] = w;
    }
    free( s.bits ); s.bits = 0; s.codeSize = codeSize > 0 ? codeSize : 0;
    if( !s.codeSize || !( s.bits = (unsigned char*)calloc( ( s.codeSize >> 3 ) + 1, 1 ) ) ) { s.codeSize = 0; return; }
    for( int i = 0; i < r.nWrap; i++ ) { const int a = r.wrap[i].addr; if( a >= 0 && a < s.codeSize ) s.bits[a >> 3] |= (unsigned char)( 1 << ( a & 7 ) ); }
  }

  inline void ScanReset() {
    ScanStore& s = ScanStorage();
    free( s.res.wrap ); free( s.res.trainFn ); free( s.res.tierPool ); free( s.bits ); memset( &s, 0, sizeof( s ) );
    for( int i = 0; i < ST_COUNT; i++ ) s.res.statTiers[i].costFuncSym = -1;
    s.res.hpStart = s.res.hpPerLevel = s.res.hpPerLevelSym = -1;
  }
}
