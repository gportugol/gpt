/***************************************************************************
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemail.net                                              *
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

#ifndef GPT_HPP
#define GPT_HPP

#include <list>
#include <string>

#include "PortugolAST.hpp"
#include "SymbolTable.hpp"


using namespace std;
using namespace antlr;


class GPT{
public:
  ~GPT();

  static GPT* self();

  void reportDicas(bool value);
  void printParseTree(bool value);
//   void usePipe(bool value);
  void setOutputFile(string str);

  void showHelp();
  void showVersion();

  bool compile(const list<string>& ifnames, bool genBinary = true);
  bool translate2C(const list<string>& ifnames);
  bool interpret(const list<string>& ifnames, const string& host, int port);
  
private:
  GPT();

  static GPT* _self;
  
  string createTmpFile();
  
  bool parse(list<pair<string,istream*> >&);

  bool prologue(const list<string>& ifname);

//   bool _usePipe;
  bool _printParseTree;
  bool _useOutputFile;
  string _outputfile;

  RefPortugolAST _astree;
  SymbolTable    _stable;
  
};

#endif
