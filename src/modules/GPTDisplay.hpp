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

#ifndef GPTDISPLAY_HPP
#define GPTDISPLAY_HPP

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

class UniqueErrorException {
public:
  UniqueErrorException(const string& msg, int line)
    : _msg(msg), _line(line) {  }

  ~UniqueErrorException() {}

   string message() {return _msg;}
   int line()       {return _line;}
private:
  string _msg;
  int _line;
};



class GPTDisplay {
public:
  const int MAX_ERRORS;

  class ErrorMsg {
    public:
    ErrorMsg() : line(0), hasTip(false), cd(0) {}
    int line;
    bool hasTip;
    int cd;
    string file;
    string msg;
    string tip;
  };

  ~GPTDisplay();

  static GPTDisplay* self();


  void showError(stringstream& s);
  void showError(const string&);
  void showMessage(stringstream& s);

  void stopOnError(bool);

  void addFileName(const string&);

  int add(const string& msg, int line);
  void addTip(const string& msg, int line, int cd);

  void setCurrentFile(const string& file);
  string getCurrentFile();
//   void addInternalError(const string&);
//   void addInternalError(const stringstream&);

  ErrorMsg getFirstError();

  void showErrors();
  bool hasError();

  int totalErrors();

  void showTips(bool);

  void clear();
private:
  GPTDisplay();

  static GPTDisplay* _self;

  string toOEM(const string& str);
  string toLatin1(const string&);
  
  void showError(ErrorMsg&);
  void showTip(ErrorMsg&);

  void processAndAdd(const string&, int);
  void processTipAndAdd(const string& msg, int line, int cd);

  int  _totalErrors;
  bool _stopOnError;
  bool _showTips;

  typedef map<string, int> file_map_t;
  file_map_t _file_map;
//  typedef map<int, list<ErrorMsg> > errors_map_t;
  typedef map<int, map<int, list<ErrorMsg> > > errors_map_t;
  errors_map_t _errors;

  string _currentFile;
};

#endif
