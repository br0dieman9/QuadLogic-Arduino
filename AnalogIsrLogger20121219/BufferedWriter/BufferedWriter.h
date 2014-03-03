/* Arduino BufferedWriter Library
 * Copyright (C) 2011 by William Greiman
 *
 * This file is part of the Arduino BufferedWriter Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino BufferedWriter Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
 /**
 * \file
 * \brief Fast text formatter
 */
#ifndef BufferedWriter_h
#define BufferedWriter_h
#include <limits.h>
#include <SdFat.h>
//------------------------------------------------------------------------------
/** BufferedWriter version YYYYMMDD */
#define BUFFERED_WRITER_VERSION 20121116
//------------------------------------------------------------------------------
/**
 * \class BufferedWriter
 * \brief Class for fast formatted output to a file.
 */
class BufferedWriter {
 public:
 /** initialize object
  * \param[in] file file for data
  */
  void init(SdBaseFile* file) {
    file_ = file;
    in_ = 0;
  }
  /** write a char
   * \param[in] c character to be written
   */
  void putChar(char c) {
    if (in_ == sizeof(buf_)) writeBuf();
    buf_[in_++] = c;
  }
  /** write CR/LF */
  void putCRLF() {
    putChar('\r');
    putChar('\n');
  }
  //----------------------------------------------------------------------------
  /** Write a char as a number
   * \param[in] n number to be written
   */
  void putNum(char n) {
    if (CHAR_MIN == 0) {
      putNum((unsigned char)n);
    } else {
      putNum((signed char)n);
   }
  }
  //----------------------------------------------------------------------------
  /** Write a signed int
   * \param[in] n number to be written
   */
  void putNum(signed int n) {
    if (INT_MAX == SHRT_MAX) {
      putNum((signed short)n);
    } else {
      putNum((signed long)n);
    }
  }
  //----------------------------------------------------------------------------
  /** Write a unsigned int
   * \param[in] n number to be written
   */
  void putNum(unsigned int n) {
    if (UINT_MAX == USHRT_MAX) {
      putNum((unsigned short)n);
    } else {
      putNum((unsigned long)n);
    }
  }
  //----------------------------------------------------------------------------
  void putHex(unsigned long n);
  void putNum(signed char n);
  void putNum(unsigned char n);
  void putNum(signed short n);
  void putNum(unsigned short n);
  void putNum(signed long n);
  void putNum(unsigned long n);
  void putStr(char* str);
  void putStr_P(PGM_P str);
  void writeBuf();
 private:
  static const size_t BUF_DIM = 64;
  char buf_[BUF_DIM];
  size_t in_;
  SdBaseFile* file_;
};
#endif  // BufferedWriter_h