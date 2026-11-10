#pragma once

namespace BonusTiers {
  static const int LAYOUT_NOTR_DIMX        = 8192;
  static const int LAYOUT_NOTR_ATR_TITLE_X = 500;
  static const int LAYOUT_NOTR_ATR_VAL_X   = 2400;
  static const int LAYOUT_NOTR_TAL_TITLE_X = 3800;
  static const int LAYOUT_NOTR_TAL_SKILL_X = 6000;
  static const int LAYOUT_NOTR_TAL_VAL_X   = 7200;
  static const int LAYOUT_WIDTH_STEP       = 5;
  static const int LAYOUT_WIDTH_COUNT      = 5;
  static const int LAYOUT_TITLE_A_PX       = 139;
  static const int LAYOUT_TITLE_B_PX       = 171;
  static const int LAYOUT_SKILL_PX         = 83;
  static const int LAYOUT_GAP_PX           = 12;
  static const int LAYOUT_FRAME_ATR_X      = 3456;
  static const int LAYOUT_FRAME_TAL_X      = 7648;
  static const int LAYOUT_MARGIN_PX        = 6;
  static const int LAYOUT_AUTO_MAX_PCT     = 120;
  static const int LAYOUT_AUTO_STEP        = 1;
  static const int LAYOUT_TAL_MAX_PCT      = 120;
  static const int LAYOUT_PLAN_MAX         = 8;

  inline int LayoutPx( int px, int permille ) { return px * 12800 / permille; }
  inline int LayoutAtrVal( int pm )   { const int x = LAYOUT_NOTR_ATR_TITLE_X + LayoutPx( LAYOUT_TITLE_A_PX + LAYOUT_GAP_PX, pm ); return ( pm <= 1000 || x >= LAYOUT_NOTR_ATR_VAL_X ) ? LAYOUT_NOTR_ATR_VAL_X : x; }
  inline int LayoutTalSkill( int pm ) { const int x = LAYOUT_NOTR_TAL_TITLE_X + LayoutPx( LAYOUT_TITLE_B_PX + LAYOUT_GAP_PX, pm ); return ( pm <= 1000 || x >= LAYOUT_NOTR_TAL_SKILL_X ) ? LAYOUT_NOTR_TAL_SKILL_X : x; }
  inline int LayoutTalVal( int pm )   { const int x = LayoutTalSkill( pm ) + LayoutPx( LAYOUT_SKILL_PX + LAYOUT_GAP_PX, pm ); return ( pm <= 1000 || x >= LAYOUT_NOTR_TAL_VAL_X ) ? LAYOUT_NOTR_TAL_VAL_X : x; }

  inline int LayoutWidthPermille( int widthIdx, int view, int autoPermille ) {
    if( view == 2 ) return 1000;
    if( widthIdx < 1 ) return autoPermille;
    return ( 100 + ( ( widthIdx > LAYOUT_WIDTH_COUNT ? LAYOUT_WIDTH_COUNT : widthIdx ) - 1 ) * LAYOUT_WIDTH_STEP ) * 10;
  }

  inline int LayoutCharPx( char c ) {
    switch( c ) {
      case '0': return 9;  case '1': return 6;  case '2': return 9;  case '3': return 9;
      case '4': return 11; case '5': return 8;  case '6': return 8;  case '7': return 9;
      case '8': return 8;  case '9': return 8;  case '/': return 8;  case '+': return 9;
      case ' ': return 11; case '%': return 11; case '-': return 7;
    }
    return 15;
  }
  inline int  LayoutTextPx( const char* s ) { int w = 0; for( ; s && *s; s++ ) w += LayoutCharPx( *s ); return w; }
  inline bool LayoutFits( int pm, int colX, int frameX, const char* text ) { return (__int64)( frameX - colX ) * pm * 640 >= (__int64)( LayoutTextPx( text ) + LAYOUT_MARGIN_PX ) * 8192 * 1000; }

  inline bool LayoutColumnFits( bool atrCol, const char* const* t, int n, int pm ) {
    const int colX = atrCol ? LayoutAtrVal( pm ) : LayoutTalVal( pm ), frameX = atrCol ? LAYOUT_FRAME_ATR_X : LAYOUT_FRAME_TAL_X;
    for( int i = 0; i < n; i++ ) if( !LayoutFits( pm, colX, frameX, t[i] ) ) return false;
    return true;
  }
  inline int LayoutFitColumn( bool atrCol, const char* const* t, int n, int maxPct ) {
    for( int pm = 1000; pm <= maxPct * 10; pm += LAYOUT_AUTO_STEP * 10 ) if( LayoutColumnFits( atrCol, t, n, pm ) ) return pm;
    return maxPct * 10 + 1;
  }
  inline void LayoutCompact( const char* s, char* out, int size ) {
    int k = 0; for( ; s && *s && k < size - 1; s++ ) if( !( s[0] == ' ' && ( s[1] == '+' || s[1] == '-' ) ) ) out[k++] = *s;
    out[k] = 0;
  }

  struct LayoutPlan { int permille; bool compactAtr, compactTal; };

  inline int LayoutFitOrCompact( bool atrCol, const char* const* src, int n, int room, bool& compact ) {
    if( n > LAYOUT_PLAN_MAX ) n = LAYOUT_PLAN_MAX;
    const char* ptrs[LAYOUT_PLAN_MAX]; for( int i = 0; i < n; i++ ) ptrs[i] = src[i];
    int pm = LayoutFitColumn( atrCol, ptrs, n, LAYOUT_AUTO_MAX_PCT );
    if( pm > room ) {
      compact = true; char buf[LAYOUT_PLAN_MAX][64];
      for( int i = 0; i < n; i++ ) { LayoutCompact( src[i], buf[i], 64 ); ptrs[i] = buf[i]; }
      pm = LayoutFitColumn( atrCol, ptrs, n, LAYOUT_AUTO_MAX_PCT );
      if( pm > room ) pm = room;
    }
    return pm;
  }
  inline LayoutPlan LayoutPlanAuto( const char* const* atr, int nAtr, const char* const* tal, int nTal ) {
    LayoutPlan p = { 1000, false, false };
    const int a = LayoutFitOrCompact( true, atr, nAtr, LAYOUT_AUTO_MAX_PCT * 10, p.compactAtr );
    const int t = LayoutFitOrCompact( false, tal, nTal, a > LAYOUT_TAL_MAX_PCT * 10 ? a : LAYOUT_TAL_MAX_PCT * 10, p.compactTal );
    p.permille = a > t ? a : t; return p;
  }
}
