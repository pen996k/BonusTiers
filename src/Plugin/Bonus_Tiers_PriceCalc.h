#pragma once

#include <stdio.h>
#include "Bonus_Tiers_ScanTypes.h"

namespace BonusTiers {
  inline int PriceAt(const ScanTiers& t, int v) {
    for (int i = 0; i < t.n; ++i)
      if (v < t.threshold[i]) return t.price[i];
    return (t.price[t.n] > 0) ? t.price[t.n] : 1;
  }
  inline int FairCost(const ScanTiers& t, int trained, int points) {
    if (points <= 0) return 0;
    int total = 0;
    for (int k = 0; k < points; ++k)
      total += PriceAt(t, trained + k);
    return total;
  }
  inline int ModCost(const ScanTiers& t, int trained, int points) {
    if (points <= 0) return 0;
    return PriceAt(t, trained) * points;
  }
  inline int PriceAppendChar(char* out, int pos, int cap, char c) { if (out && pos + 1 < cap) { out[pos] = c; out[pos + 1] = '\0'; } return pos + 1; }
  inline int PriceAppendStr(char* out, int pos, int cap, const char* s) { while (*s) pos = PriceAppendChar(out, pos, cap, *s++); return pos; }
  inline int PriceAppendInt(char* out, int pos, int cap, int v) { char buf[16]; snprintf(buf, sizeof(buf), "%d", v); return PriceAppendStr(out, pos, cap, buf); }

  inline int CostSplit(const ScanTiers& t, int trained, int points, char* out, int cap) {
    if (out && cap > 0) out[0] = '\0';
    if (!out || cap <= 1 || points <= 1) return 0;
    const int firstPrice = PriceAt(t, trained);
    bool mixed = false;
    for (int k = 1; k < points; ++k) if (PriceAt(t, trained + k) != firstPrice) { mixed = true; break; }
    if (!mixed) return 0;
    int pos = 0, runPrice = firstPrice, runCount = 1;
    for (int k = 1; k <= points; ++k) {
      const int p = (k < points) ? PriceAt(t, trained + k) : -1;
      if (p == runPrice) ++runCount;
      else {
        if (pos > 0) pos = PriceAppendStr(out, pos, cap, " + ");
        pos = PriceAppendInt(out, pos, cap, runCount);
        pos = PriceAppendChar(out, pos, cap, 'x');
        pos = PriceAppendInt(out, pos, cap, runPrice);
        runPrice = p; runCount = 1;
      }
    }
    return (pos < cap) ? pos : (cap - 1);
  }

  inline int FightCapClip(int trained, int tablets, int gain, int cap) {
    if (gain <= 0) return 0;
    if (cap <= 0) return gain;
    int tabs = (tablets > 0) ? tablets : 0, tr = (trained > 0) ? trained : 0;
    int room = cap - tr - tabs;
    if (room < 0) room = 0;
    return (gain < room) ? gain : room;
  }
}
