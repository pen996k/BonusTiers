#pragma once

#include <string.h>

namespace BonusTiers {
  enum ELang { LANG_EN = 0, LANG_PL = 1, LANG_DE = 2, LANG_CS = 3, LANG_COUNT = 4 };

  inline bool LangStartsWith( const unsigned char* s, const char* p ) {
    return s && strncmp( (const char*)s, p, strlen( p ) ) == 0;
  }

  inline int LangFromNotEnoughLP( const unsigned char* s ) {
    if( LangStartsWith( s, "Za ma\xB3o Punkt" ) ) return LANG_PL;
    if( LangStartsWith( s, "Nicht genug Lernpunkte" ) ) return LANG_DE;
    if( LangStartsWith( s, "Nem" ) && strstr( (const char*)s, " dostatek " ) != 0 ) return LANG_CS;
    return LANG_EN;
  }

  inline int LangFromBuyStrength( const unsigned char* s ) {
    if( LangStartsWith( s, "Si\xB3" "a" ) ) return LANG_PL;
    if( LangStartsWith( s, "St\xE4rke" ) || LangStartsWith( s, "St\xC3\xA4rke" ) || LangStartsWith( s, "Staerke" ) ) return LANG_DE;
    if( LangStartsWith( s, "S\xEDla" ) || LangStartsWith( s, "S\xC3\xADla" ) ) return LANG_CS;
    return LANG_EN;
  }
}
