#pragma once

#include <initializer_list>
#include <string.h>

namespace BonusTiers {
  struct IniLimits { bool teacherMax, fightCap; };
  inline char IniLower( char c ) { return ( c >= 'A' && c <= 'Z' ) ? (char)( c - 'A' + 'a' ) : c; }

  inline bool IniKeyIs( const char* a, const char* b ) {
    for( ;; ) {
      while( *a == ' ' || *a == '_' || *a == '\t' ) a++;
      while( *b == ' ' || *b == '_' ) b++;
      if( !*a || !*b ) return !*a && !*b;
      if( IniLower( *a++ ) != IniLower( *b++ ) ) return false;
    }
  }
  inline int IniBool( const char* v ) {
    char w[16]; int k = 0;
    for( ; *v && k < 15; v++ ) if( *v != ' ' && *v != '\t' && *v != '"' ) w[k++] = *v;
    w[k] = 0;
    for( const char* t : { "true", "tak", "1", "on", "yes" } ) if( _stricmp( w, t ) == 0 ) return 1;
    for( const char* f : { "false", "nie", "0", "off", "no" } ) if( _stricmp( w, f ) == 0 ) return 0;
    return -1;
  }
  inline IniLimits IniParse( const char* text, int* unknown ) {
    IniLimits r = { true, true }; int bad = 0;
    for( const char* p = text; p && *p; ) {
      char line[256]; int k = 0;
      while( *p && *p != '\n' && *p != '\r' ) { if( k < 255 ) line[k++] = *p; p++; }
      while( *p == '\n' || *p == '\r' ) p++;
      line[k] = 0; char* s = line;
      if( (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF ) s += 3;
      while( *s == ' ' || *s == '\t' ) s++;
      if( !*s || *s == ';' || *s == '#' || *s == '[' || ( s[0] == '/' && s[1] == '/' ) ) continue;
      char* eq = strchr( s, '=' ); if( !eq ) { bad++; continue; }
      *eq = 0; char* val = eq + 1;
      for( char* c = val; *c; c++ ) if( *c == ';' || *c == '#' ) { *c = 0; break; }
      for( char* e = eq - 1; e >= s && ( *e == ' ' || *e == '\t' ); e-- ) *e = 0;
      const int b = IniBool( val );
      if( IniKeyIs( s, "MaxTeachingThreshold" ) )           { if( b < 0 ) bad++; else r.teacherMax = b == 1; }
      else if( IniKeyIs( s, "MaxFightingSkillThreshold" ) ) { if( b < 0 ) bad++; else r.fightCap = b == 1; }
      else bad++;
    }
    if( unknown ) *unknown = bad;
    return r;
  }
  static const char INI_DEFAULT[] = "MaxTeachingThreshold = True\r\nMaxFightingSkillThreshold = True\r\n";
}
