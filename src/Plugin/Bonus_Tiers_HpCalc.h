#pragma once

namespace BonusTiers {
  static const int HP_SANE_MAX = 100000;

  inline int HpFromLevels( int start, int perLevel, bool hasLive, int perLevelLive, int level ) {
    const int per = hasLive ? perLevelLive : perLevel;
    if( start < 0 || start > HP_SANE_MAX || level < 0 || per < 0 || per > HP_SANE_MAX ) return 0;
    const long long v = (long long)start + (long long)level * per;
    return ( v > 0 && v <= 2000000000LL ) ? (int)v : 0;
  }
}
