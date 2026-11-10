#pragma once

#include <stdio.h>

namespace BonusTiers {
  static const int EQ_N = 8, EQ_KNOWN_MAX = 16, EQ_DEPTH_MAX = 8;
  struct EqLedger { int sum[EQ_N], known[EQ_KNOWN_MAX]; };
  struct EqFrame  { int before[EQ_N], child[EQ_N]; };
  struct EqStack  { EqFrame f[EQ_DEPTH_MAX]; int depth; };
  enum { EQ_NONE = 0, EQ_ADDED = 1, EQ_REMOVED = 2, EQ_OLD = 3, EQ_FULL = 4 };

  inline bool EqPush( EqStack& st, const int* now ) {
    if( st.depth >= EQ_DEPTH_MAX ) return false;
    EqFrame& f = st.f[st.depth++];
    for( int i = 0; i < EQ_N; i++ ) { f.before[i] = now[i]; f.child[i] = 0; }
    return true;
  }
  inline int  EqKnownFind( const EqLedger& L, int inst ) { for( int i = 0; i < EQ_KNOWN_MAX; i++ ) if( L.known[i] == inst ) return i; return -1; }
  inline bool EqKnownEmpty( const EqLedger& L )          { for( int i = 0; i < EQ_KNOWN_MAX; i++ ) if( L.known[i] != 0 ) return false; return true; }

  inline int EqPop( EqStack& st, EqLedger& L, const int* now, int inst, bool add, bool counted, int* own ) {
    EqFrame& f = st.f[--st.depth]; int total[EQ_N]; bool any = false;
    for( int i = 0; i < EQ_N; i++ ) { total[i] = now[i] - f.before[i]; own[i] = total[i] - f.child[i]; any = any || own[i] != 0; }
    if( st.depth > 0 ) for( int i = 0; i < EQ_N; i++ ) st.f[st.depth - 1].child[i] += total[i];
    if( !counted || inst <= 0 ) return EQ_NONE;
    if( !any ) {
      if( add ) return EQ_NONE;
      const int k = EqKnownFind( L, inst ); if( k < 0 ) return EQ_NONE;
      L.known[k] = 0; if( EqKnownEmpty( L ) ) for( int i = 0; i < EQ_N; i++ ) L.sum[i] = 0;
      return EQ_REMOVED;
    }
    if( add ) {
      const int slot = EqKnownFind( L, 0 ); if( slot < 0 ) return EQ_FULL;
      L.known[slot] = inst; for( int i = 0; i < EQ_N; i++ ) L.sum[i] += own[i];
      return EQ_ADDED;
    }
    const int k = EqKnownFind( L, inst ); if( k < 0 ) return EQ_OLD;
    L.known[k] = 0;
    if( EqKnownEmpty( L ) ) for( int i = 0; i < EQ_N; i++ ) L.sum[i] = 0;
    else                    for( int i = 0; i < EQ_N; i++ ) L.sum[i] += own[i];
    return EQ_REMOVED;
  }

  inline void EqFormat( char* out, int size, const char* shown, int bonus, int eq ) {
    const int pot = bonus - eq;
    if( eq != 0 && pot >= 0 ) snprintf( out, (size_t)size, "%s +%d %s%d", shown, pot, eq > 0 ? "+" : "", eq );
    else if( bonus > 0 )      snprintf( out, (size_t)size, "%s +%d", shown, bonus );
    else                      snprintf( out, (size_t)size, "%s", shown );
  }
}
