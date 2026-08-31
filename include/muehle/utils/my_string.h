#ifndef MUEHLE_UTILS_MY_STRING_H_
#define MUEHLE_UTILS_MY_STRING_H_

#ifdef _WIN32
#include <windows.h>
#else  // _WIN32
#include <muehle/win_32_compat.h>
#endif  // _WIN32
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include <iostream>
#include <string>

namespace muehle {

/* General functions */
bool ReadAsciiData(HANDLE h_file, double *p_data, unsigned int num_values,
                   unsigned char decimal_separator,
                   unsigned char column_separator);

/* Class to mutually convert between ascii and wide char strings */
class MyString {
 private:
  /* Variables */
  WCHAR *str_w = nullptr;
  char *str_a = nullptr;
  size_t length = 0;
  size_t reserved = 0;

 public:
  /* Functions */
  MyString();
  MyString(const char *c_str);
  MyString(const WCHAR *c_str);
  ~MyString();

  const char *CStrA();
  const WCHAR *CStrW();
  MyString &Assign(const char *c_str);
  MyString &Assign(const WCHAR *c_str);
};

}  // namespace muehle

#endif  // MUEHLE_UTILS_MY_STRING_H_
