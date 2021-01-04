#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_ARDUINOCOMMON_H
#define HTCW_ARDUINOCOMMON_H
#ifndef ARDUINO
  #define PROGMEM
  #define STRNCPYP strncpy
  #define PHELPER(x) x
#else
  #define PHELPER(x) F(x)
  #define STRNCPYP strncpy_P
#endif
#endif