/***************************************************************************
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "GPTDisplay.hpp"

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>
#include <vector>

using namespace std;

GPTDisplay *GPTDisplay::_self = 0L;

GPTDisplay::GPTDisplay()
    : MAX_ERRORS(10), _totalErrors(0), _stopOnError(false), _showTips(false) {}

GPTDisplay::~GPTDisplay() {}

int GPTDisplay::totalErrors() { return _totalErrors; }

string GPTDisplay::toLatin1(const string &str) {
  char c = 0;
  string ret;
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == 0xffffffc3) {
      c = 1;
      continue;
    }
    if (c) {
      ret += str[i] | 0x40;
    } else {
      ret += str[i];
    }
    c = 0;
  }
  return ret;
}

string GPTDisplay::toOEM(const string &str) {
#ifdef WIN32
  string latin1 = toLatin1(str);
  vector<char> buffer(latin1.length() + 1);
  CharToOem(latin1.c_str(), buffer.data());
  return string(buffer.data());
#else
  return str;
#endif
}

void GPTDisplay::showError(stringstream &s) { cerr << toOEM(s.str()); }

void GPTDisplay::showError(const string &str) { cerr << toOEM(str) << endl; }

void GPTDisplay::showMessage(stringstream &s) {
  cout << toOEM(s.str());
  cout.flush();
}

void GPTDisplay::stopOnError(bool val) { _stopOnError = val; }

bool GPTDisplay::hasError() { return _totalErrors > 0; }

void GPTDisplay::showErrors() {
  errors_map_t::reverse_iterator it;
  for (it = _errors.rbegin(); it != _errors.rend(); ++it) {
    for (map<int, list<ErrorMsg>>::iterator ll = it->second.begin();
         ll != it->second.end(); ++ll) {
      for (list<ErrorMsg>::iterator lit = ll->second.begin();
           lit != ll->second.end(); ++lit) {
        showError((*lit));
        if (_showTips && (*lit).hasTip) {
          showTip((*lit));
        }
      }
    }
  }
}

GPTDisplay *GPTDisplay::self() {
  if (!GPTDisplay::_self) {
    GPTDisplay::_self = new GPTDisplay();
  }
  return GPTDisplay::_self;
}

void GPTDisplay::addFileName(const string &str) {
  static int c = 0;
  _file_map[str] = c++;
}

int GPTDisplay::add(const string &msg, int line) {
  _totalErrors++;
  if (_stopOnError)
    throw UniqueErrorException(msg, line);

  if (totalErrors() > MAX_ERRORS) {
    showErrors();
    exit(1);
  }

  ErrorMsg err;
  string file = _currentFile;
  mapLine(line, file, line);
  err.line = line;
  err.msg = msg;
  err.file = file;

  _errors[_file_map[file]][line].push_back(err);

  return _errors[_file_map[file]][line].size();
}

void GPTDisplay::showError(ErrorMsg &err) {
  stringstream s;
  s << err.file << ":" << err.line << " - " << err.msg << "." << endl;
  showError(s);
}

void GPTDisplay::showTip(ErrorMsg &err) {
  stringstream s;
  s << "\tDica: " << err.tip << "." << endl;
  showError(s);
}

void GPTDisplay::addTip(const string &msg, int line, int cd) {
  string file = _currentFile;
  mapLine(line, file, line);
  list<ErrorMsg>::iterator it = _errors[_file_map[file]][line].begin();

  for (int i = 0; i < cd - 1; ++i, ++it)
    ;
  (*it).hasTip = true;
  (*it).tip = msg;
}

void GPTDisplay::setCurrentFile(const string &file) { _currentFile = file; }

void GPTDisplay::addFileRange(int firstLine, const string &file) {
  _file_ranges.push_back(pair<int, string>(firstLine, file));
}

void GPTDisplay::clearFileRanges() { _file_ranges.clear(); }

void GPTDisplay::mapLine(int line, string &file, int &localLine) {
  localLine = line;
  for (file_ranges_t::reverse_iterator it = _file_ranges.rbegin();
       it != _file_ranges.rend(); ++it) {
    if (line >= it->first) {
      file = it->second;
      localLine = line - it->first + 1;
      return;
    }
  }
}

string GPTDisplay::getCurrentFile() { return _currentFile; }

GPTDisplay::ErrorMsg GPTDisplay::getFirstError() {
  return *(_errors.begin()->second.begin()->second.begin());
}

void GPTDisplay::showTips(bool value) { _showTips = value; }

void GPTDisplay::clear() {
  _errors.clear();
  _totalErrors = 0;
}
