#pragma once

#include "Bonus_Tiers_PriceCalc.h"
#include "Bonus_Tiers_ScanTypes.h"

namespace BonusTiers {
  static const int TRAIN_BIG_LIMIT = 999999, TRAIN_FIGHT_MAX = 100;

  struct TrainHost {
    virtual ~TrainHost() {}
    virtual int  GetStat(int st) = 0;
    virtual void SetStat(int st, int v) = 0;
    virtual int  GetTrained(int st) = 0;
    virtual void SetTrained(int st, int v) = 0;
    virtual void SetCounter(int st, int v) = 0;
    virtual int  GetLp() = 0;
    virtual void SetLp(int v) = 0;
    virtual int  GetParam(const ScanWrap* tf, int paramIdx) = 0;
    virtual void SetParam(const ScanWrap* tf, int paramIdx, int val) = 0;
    virtual int  GetTablets(int, int totalBonus) { return totalBonus > 0 ? totalBonus : 0; }
    virtual int  GetTalentSkill(int) { return 0; }
    virtual void SetTalentSkill(int, int) {}
    virtual void OnFightCapClipped(int, int, int) {}
    virtual void OnNetTriggered(int, int, int) {}
    virtual void OnStuckRestored() {}
  };

  struct TrainSession {
    int  statCount, fightCapMax, depth, surcharge, netAddr, netLp;
    bool teacherMaxOff, fightCapOn, swapped, netActive;
    int  shown[ST_COUNT], bonus[ST_COUNT], paid[ST_COUNT], netStat[ST_COUNT];
    const ScanTiers* paidTiers[ST_COUNT];

    void Init(int nStats = ST_COUNT) {
      statCount = (nStats > 0 && nStats <= ST_COUNT) ? nStats : ST_COUNT;
      teacherMaxOff = false; fightCapOn = statCount > 3; fightCapMax = TRAIN_FIGHT_MAX; Reset();
    }
    void Reset() {
      depth = surcharge = netLp = 0; netAddr = -1; swapped = netActive = false;
      for (int i = 0; i < ST_COUNT; ++i) { shown[i] = bonus[i] = paid[i] = netStat[i] = 0; paidTiers[i] = 0; }
    }
    void SwapBegin(TrainHost& host, bool readerZero = false) {
      surcharge = 0;
      for (int i = 0; i < statCount; ++i) {
        const int full = host.GetStat(i), tr = host.GetTrained(i);
        shown[i] = readerZero ? 0 : tr; bonus[i] = readerZero ? full : (full - tr); paid[i] = 0; paidTiers[i] = 0;
        host.SetStat(i, shown[i]); host.SetCounter(i, tr);
      }
      swapped = true;
    }
    void ApplyRules(TrainHost& host) {
      const ScanResult& scan = Scan();
      for (int i = 0; i < scan.nRules; ++i) {
        const int st = scan.rule[i].stat, tal = scan.rule[i].talent;
        if (st < 0 || st >= statCount || tal <= 0) continue;
        bool seen = false;
        for (int j = 0; j < i; ++j) if (scan.rule[j].stat == st && scan.rule[j].talent == tal) { seen = true; break; }
        if (seen) continue;
        const int v = host.GetStat(st);
        bool hasGe = false, hasLt = false; int bestGeVal = 0, bestGeTv = 0, bestLtVal = 0, bestLtTv = 0;
        for (int j = i; j < scan.nRules; ++j) {
          const ScanRule& r = scan.rule[j];
          if (r.stat != st || r.talent != tal) continue;
          if ((r.op == RULE_GE && v >= r.value) || (r.op == RULE_GT && v > r.value)) {
            if (!hasGe || r.value > bestGeVal || (r.value == bestGeVal && r.talentValue > bestGeTv)) { hasGe = true; bestGeVal = r.value; bestGeTv = r.talentValue; }
          } else if ((r.op == RULE_LT && v < r.value) || (r.op == RULE_LE && v <= r.value)) {
            if (!hasLt || r.value < bestLtVal || (r.value == bestLtVal && r.talentValue < bestLtTv)) { hasLt = true; bestLtVal = r.value; bestLtTv = r.talentValue; }
          }
        }
        if (hasGe) host.SetTalentSkill(tal, bestGeTv); else if (hasLt) host.SetTalentSkill(tal, bestLtTv);
      }
    }
    void SwapEnd(TrainHost& host) {
      if (!swapped) return;
      swapped = false;
      bool anyGained = false, anyClipped = false;
      for (int i = 0; i < statCount; ++i) {
        int tr = host.GetTrained(i), after = host.GetStat(i), keep = after - shown[i];
        if (keep < 0) keep = 0;
        if (i >= ST_1H && keep > 0 && fightCapOn) {
          int k2 = FightCapClip(tr, host.GetTablets(i, bonus[i]), keep, fightCapMax), excess = keep - k2;
          if (excess > 0) {
            anyClipped = true; after -= excess;
            int refundPts = excess < paid[i] ? excess : paid[i], refundLp = 0;
            if (refundPts > 0 && paidTiers[i] && paidTiers[i]->measured) { refundLp = FairCost(*paidTiers[i], tr + k2, refundPts); host.SetLp(host.GetLp() + refundLp); }
            keep = k2; host.OnFightCapClipped(i, k2, refundLp);
          }
        }
        int newTr = tr + keep;
        int full = after + bonus[i];
        if (full < newTr) newTr = full;
        host.SetTrained(i, newTr); host.SetStat(i, full); host.SetCounter(i, newTr);
        if (keep > 0) anyGained = true;
      }
      if (!anyGained && !anyClipped && surcharge != 0)
        host.SetLp(host.GetLp() + surcharge);
      surcharge = 0;
      ApplyRules(host);
    }
    void OnEnter(TrainHost& host, int addr, int callDepth) {
      if (callDepth == 0 && !swapped && !ScanIsWrap(addr)) {
        netActive = true; netAddr = addr; netLp = host.GetLp();
        for (int i = 0; i < statCount; ++i) netStat[i] = host.GetStat(i);
      }
      if (ScanIsWrap(addr)) {
        if (depth == 0) { const ScanWrap* w = ScanFindWrap(addr); SwapBegin(host, teacherMaxOff && w && w->kind == WRAP_READER && !ScanFindTrainFn(addr)); }
        ++depth;
      }
      const ScanWrap* tf = ScanFindTrainFn(addr);
      if (!tf || !swapped || tf->statParam < 0 || tf->amountParam < 0) return;
      const int st = ScanStatOf(tf, host.GetParam(tf, tf->statParam)), amount = host.GetParam(tf, tf->amountParam);
      if (st < 0 || st >= statCount) return;
      const int origCur = shown[st]; bool statZeroed = false;
      if (teacherMaxOff && tf->limitParam >= 0) host.SetParam(tf, tf->limitParam, TRAIN_BIG_LIMIT);
      else if ((teacherMaxOff && st < ST_1H) || (!fightCapOn && tf->limitParam < 0 && st >= ST_1H && shown[st] + amount > fightCapMax)) {
        bonus[st] += shown[st]; shown[st] = 0; host.SetStat(st, 0); statZeroed = true;
      }
      if (amount <= 0) return;
      paid[st] += amount;
      const ScanTiers& tiers = ScanTiersOf(tf, st);
      if (tiers.measured) {
        paidTiers[st] = &tiers;
        const bool zeroedNoCounter = statZeroed && Scan().counter[st].kind == CNT_NONE;
        if (tiers.model == COST_PER_POINT_TIMES_N || zeroedNoCounter) {
          const int diff = FairCost(tiers, origCur, amount) - ModCost(tiers, zeroedNoCounter ? 0 : origCur, amount);
          host.SetLp(host.GetLp() - diff); surcharge += diff;
        }
      }
    }
    void OnLeave(TrainHost& host, int addr, int callDepth) {
      if (ScanIsWrap(addr)) { if (depth > 0) --depth; if (depth == 0 && swapped) SwapEnd(host); }
      if (callDepth == 0 && netActive && netAddr == addr) {
        netActive = false;
        if (host.GetLp() < netLp) {
          for (int i = 0; i < statCount; ++i) {
            const int up = host.GetStat(i) - netStat[i];
            if (up > 0) { const int newTr = host.GetTrained(i) + up; host.SetTrained(i, newTr); host.SetCounter(i, newTr); host.OnNetTriggered(addr, i, up); }
          }
        }
      }
    }
    void FrameCheck(TrainHost& host) {
      if (swapped && depth == 0) { host.OnStuckRestored(); SwapEnd(host); }
      if (swapped) return;
      for (int i = 0; i < statCount; ++i) { const int full = host.GetStat(i); if (full < host.GetTrained(i)) { host.SetTrained(i, full); host.SetCounter(i, full); } }
    }
  };
}
