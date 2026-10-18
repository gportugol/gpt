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

#include "config.h"

#include "GPTDisplay.hpp"
#include "InterpreterDBG.hpp"
#include "InterpreterEval.hpp"

#ifdef WIN32
#include <winsock.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#endif

#define PCRE2_CODE_UNIT_WIDTH 8
#include <cerrno>
#include <cstdlib>
#include <pcre2.h>
#include <unistd.h>

#ifndef WIN32
void sigPipeHandler(int) { InterpreterDBG::self()->closeSock(); }
#endif

InterpreterDBG *InterpreterDBG::singleton = NULL;

// Length of the well-formed UTF-8 sequence that starts at str[i], or 0 when
// there is none (stray or truncated bytes, overlong forms, surrogates, values
// above U+10FFFF).
static size_t xmlUtf8Length(const string &str, string::size_type i) {
  unsigned char c = str[i];
  size_t len;
  unsigned char lo = 0x80, hi = 0xBF;
  if (c >= 0xC2 && c <= 0xDF) {
    len = 2;
  } else if (c >= 0xE0 && c <= 0xEF) {
    len = 3;
    if (c == 0xE0) {
      lo = 0xA0;
    } else if (c == 0xED) {
      hi = 0x9F;
    }
  } else if (c >= 0xF0 && c <= 0xF4) {
    len = 4;
    if (c == 0xF0) {
      lo = 0x90;
    } else if (c == 0xF4) {
      hi = 0x8F;
    }
  } else {
    return 0;
  }
  if (i + len > str.length()) {
    return 0;
  }
  for (size_t k = 1; k < len; k++) {
    unsigned char b = str[i + k];
    if (b < (k == 1 ? lo : 0x80) || b > (k == 1 ? hi : 0xBF)) {
      return 0;
    }
  }
  return len;
}

// Every text that reaches the XML sent to the debugger comes from the program
// being run: file names, identifiers and, through leia(), whatever the user
// typed. Escaped, a quote in a value cannot close the attribute holding it.
// Control characters other than tab, newline and carriage return, and bytes
// that are not valid UTF-8, or encode U+FFFE or U+FFFF, are not allowed
// anywhere in an XML document, not even as references, so each becomes U+FFFD
// and the client still sees that something was there.
static string xmlEscape(const string &str) {
  string ret;
  for (string::size_type i = 0; i < str.length(); i++) {
    unsigned char c = str[i];
    if (c >= 0x80) {
      size_t len = xmlUtf8Length(str, i);
      bool noncharacter = len == 3 && c == 0xEF &&
                          (unsigned char)str[i + 1] == 0xBF &&
                          (unsigned char)str[i + 2] >= 0xBE;
      if (len == 0 || noncharacter) {
        ret += "\xEF\xBF\xBD";
      } else {
        ret.append(str, i, len);
      }
      i += (len == 0 ? 1 : len) - 1;
      continue;
    }
    if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
      ret += "\xEF\xBF\xBD";
      continue;
    }
    switch (c) {
    case '&':
      ret += "&amp;";
      break;
    case '<':
      ret += "&lt;";
      break;
    case '>':
      ret += "&gt;";
      break;
    case '"':
      ret += "&quot;";
      break;
    case '\n':
      ret += "&#10;";
      break;
    case '\r':
      ret += "&#13;";
      break;
    case '\t':
      ret += "&#9;";
      break;
    default:
      ret += c;
    }
  }
  return ret;
}

InterpreterDBG::InterpreterDBG() {
  currentCmd = CMDNull;
#ifndef WIN32
  clientsock = -1;
#else
  clientsock = INVALID_SOCKET;
#endif
}

InterpreterDBG *InterpreterDBG::self() {
  if (!singleton) {
    singleton = new InterpreterDBG;
  }
  return singleton;
}

void InterpreterDBG::init(string host, int port) {
  if (host.length() == 0)
    return;

#ifndef WIN32
  struct sockaddr_in name;

  struct hostent *hostinfo = NULL;

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  hostinfo = gethostbyname(host.c_str());

  if (hostinfo == NULL) {
    // cerr << "client: unknow host : "  << hostname << endl;
    return;
  }

  name.sin_addr = *(struct in_addr *)hostinfo->h_addr;

  clientsock = socket(PF_INET, SOCK_STREAM, 0);
  if (clientsock < 0) {
    stringstream s;
    s << PACKAGE << ": não foi possível criar socket\n";
    GPTDisplay::self()->showError(s);
    return;
  }

  if (connect(clientsock, (struct sockaddr *)&name, sizeof(name)) != 0) {
    // cerr << PACKAGE << ": unable to connect" << endl;
    // unable to connect
    closeSock();
  } else {
    if (SIG_ERR == signal(SIGPIPE, sigPipeHandler)) {
      stringstream s;
      s << PACKAGE << ": erro interno em InterpreterDBG::init" << endl;
      GPTDisplay::self()->showError(s);
    }
  }
#else
  WORD wVersionRequested = MAKEWORD(1, 1);
  WSADATA wsaData;

  //
  // Initialize WinSock and check the version
  //
  WSAStartup(wVersionRequested, &wsaData);

  if (wsaData.wVersion != wVersionRequested) {
    // cerr << PACKAGE << ": weong winsock version" << endl;
    return;
  }

  LPHOSTENT lpHostEntry;

  lpHostEntry = gethostbyname(host.c_str());
  if (lpHostEntry == NULL) {
    return;
  }

  //
  // Create a TCP/IP stream socket
  //

  clientsock = socket(AF_INET,      // Address family
                      SOCK_STREAM,  // Socket type
                      IPPROTO_TCP); // Protocol
  if (clientsock == INVALID_SOCKET) {
    return;
  }

  //
  // Fill in the address structure
  //
  SOCKADDR_IN saServer;

  saServer.sin_family = AF_INET;
  saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
  // ^ Server's address
  saServer.sin_port = htons(port); // Port number from command line

  //
  // connect to the server
  //
  if (connect(clientsock, (LPSOCKADDR)&saServer, sizeof(struct sockaddr)) ==
      SOCKET_ERROR) {
    closesocket(clientsock);
    clientsock = INVALID_SOCKET;
    return;
  }
#endif
}

void InterpreterDBG::checkData() {
  // processar comandos recebidos pelo cliente (ie. add/remove breakpoints)
  // NO blocking mode!! read if data exists only
  int cmd = receiveCmd(true);
  if (cmd != CMDNull) {
    currentCmd = cmd;
  }
}

void InterpreterDBG::sendInfo(Variables &v,
                              list<pair<string, pair<string, int>>> &stk) {

  sendStackInfo(stk);

  sendVariables(v.getGlobals(), stk, true);
  sendVariables(v.getLocals(), stk, false);
}

int InterpreterDBG::getCmd() {
  int ret;
  if (currentCmd == CMDNull) {
    ret = receiveCmd();
  } else {
    ret = currentCmd;
  }

  currentCmd = CMDNull;
  return ret;
}

void InterpreterDBG::sendStackInfo(list<pair<string, pair<string, int>>> &stk) {
  if (clientsock < 0)
    return;

  stringstream s;
  s << "<stackinfo>";
  int id = 0;
  for (list<pair<string, pair<string, int>>>::reverse_iterator it =
           stk.rbegin();
       it != stk.rend(); ++it) {
    s << "<entry id=\"" << id++ << "\" file=\"" << xmlEscape((*it).first)
      << "\" function=\"" << xmlEscape((*it).second.first) << "\" line=\""
      << (*it).second.second << "\"/>";
  }
  s << "</stackinfo>";

  sendData(s);
}

void InterpreterDBG::sendVariables(const map<string, Variable> &globals,
                                   list<pair<string, pair<string, int>>> &stk,
                                   bool globalScope) {
  if (clientsock < 0)
    return;

  stringstream s;
  s << "<vars for=\"" << (globalScope ? "$global" : "$local") << "\" scope=\""
    << xmlEscape(globalScope ? stk.front().second.first : stk.back().first)
    << "\">";

  bool primitive;
  for (map<string, Variable>::const_iterator it = globals.begin();
       it != globals.end(); ++it) {
    primitive = it->second.isPrimitive;
    s << "<var name=\"" << xmlEscape(it->first) << "\" type=\""
      << Symbol::typeToString(it->second.type) << "\" primitive=\""
      << (primitive ? "true" : "false") << "\"";
    if (primitive) {
      s << " value=\"" << xmlEscape(it->second.primitiveValue) << "\"/>";
    } else {
      stringstream vv;
      s << "><values>";

      //       for(list<int>::iterator dit = it->second.dimensions.begin(); dit
      //       != it->second.dimensions.end(); ++dit) {
      s << matrixValuesNodes(0, it->second.dimensions.size(), it->second.values,
                             it->second.dimensions, it->second.type);
      //         level++;
      //       }
      s << "</values></var>";

      /*    s << ">"
            << "<dimensions>";
            int level = 0;
            for(list<int>::iterator dit = it->second.dimensions.begin(); dit !=
         it->second.dimensions.end(); ++dit) { s << "<dimension level=\"" <<
         level << "\" size=\"" << (*dit) << "\"/>"; level++;
            }
            s << "</dimensions><values>";

            for(map<string, string>::iterator vit = it->second.values.begin();
         vit != it->second.values.end(); vit++) { s << "<value index=\"" <<
         vit->first << "\" value=\"" << vit->second << "\"/>";
            }
            s << "</values></var>";*/
    }
  }
  s << "</vars>";

  sendData(s);
}

string InterpreterDBG::matrixValuesNodes(unsigned int level, int dimsize,
                                         const map<string, string> &values,
                                         const list<int> &dims, int type,
                                         string vindex /*= "" */) {
  list<int>::const_iterator lsize;
  unsigned int idx = 0;
  for (lsize = dims.begin(); lsize != dims.end(); lsize++, idx++) {
    if (idx == level) {
      break;
    }
  }

  stringstream s;
  stringstream vs;
  for (int i = 0; i < (*lsize); i++) {
    vs.str("");
    vs << vindex;
    if (vindex.length() == 0) {
      vs << i;
    } else {
      vs << ":" << i;
    }

    if (level == (dims.size() - 1)) {
      map<string, string>::const_iterator found = values.find(vs.str());
      string val = found == values.end() ? "" : found->second;
      if (val.length() == 0) {
        switch (type) {
        case TIPO_INTEIRO:
        case TIPO_REAL:
        case TIPO_CARACTERE:
        case TIPO_LOGICO:
          val = "0";
          break;
        }
      }
      s << "<var name=\"" << i << "\" primitive=\"true\" value=\""
        << xmlEscape(val) << "\"/>";
    } else {

      s << "<var name=\"" << i << "\" primitive=\"false\"><values>";
      s << matrixValuesNodes(level + 1, dimsize, values, dims, type, vs.str());
      s << "</values></var>";
    }
  }
  return s.str();
}

void InterpreterDBG::sendData(stringstream &s) {
  if (clientsock < 0)
    return;
  string str = s.str();
  s.rdbuf()->str("");
  s << str.length() << '\0' << str;
  if (send(clientsock, s.str().c_str(), sizeof(char) * (s.str().length() + 1),
           0) < 0) { // error
    if (clientsock > 0) {
      // cerr << PACKAGE << ": erro ao enviar dados." << endl;
    } // else, server closed con
  }
}

// A message is "<length in ASCII>\0<body>". The header has to fit the stack
// buffer and the body is capped well above the longest command, so the
// peer never decides how much memory is used. Anything malformed, a receive
// error or a message cut short drops the socket after one diagnostic and the
// program keeps running without the debugger, as it does when the connection
// fails; a peer that closes between messages is dropped silently.
string InterpreterDBG::receiveIncomingData(bool nonBlocking) {
  const size_t maxBody = 64 * 1024;
  char buffer[1024];

  string cmd;

  size_t idx = 0;
  int rec;
  while (true) {
    if (idx >= sizeof(buffer) - 1) {
      dropConnection("cabeçalho de mensagem maior que o buffer");
      return cmd;
    }

#ifndef WIN32
    // Only the first byte is polled: once a message has started, the rest of
    // it is waited for, or a header split across packets would be lost.
    int flags = (nonBlocking && idx == 0) ? MSG_DONTWAIT : 0;
    rec = recv(clientsock, &buffer[idx], sizeof(char), flags);
    if (rec < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return cmd;
    }
#else
    // todo[win]: this code is there just to compile on windows, it should be
    // non-blocking
    rec = recv(clientsock, &buffer[idx], sizeof(char), 0);
#endif

    if (rec == 0 && idx == 0) {
      closeSock();
      return cmd;
    }
    if (rec <= 0) {
      dropConnection(rec == 0 ? "mensagem incompleta"
                              : "erro ao receber dados");
      return cmd;
    }

    if (buffer[idx] == '\0') {
      break;
    }
    idx++;
  }

  char *end;
  long msg_size = strtol(buffer, &end, 10);
  if (end == buffer || *end != '\0' || msg_size <= 0 ||
      msg_size > (long)maxBody) {
    dropConnection("tamanho de mensagem inválido");
    return cmd;
  }

  string body((size_t)msg_size, '\0');
  size_t received = 0;
  while (received < body.size()) {
    rec = recv(clientsock, &body[received], body.size() - received, 0);
    if (rec <= 0) {
      dropConnection(rec == 0 ? "mensagem incompleta"
                              : "erro ao receber dados");
      return cmd;
    }
    received += rec;
  }

  return body.substr(0, body.find('\0'));
}

void InterpreterDBG::dropConnection(const string &why) {
  stringstream s;
  s << PACKAGE << ": depurador: " << why << ". Continuando sem depurador."
    << endl;
  GPTDisplay::self()->showError(s);
  closeSock();
}

int InterpreterDBG::receiveCmd(bool nonBlocking) {
  while (true) {
    if (clientsock < 0) {
      return CMDContinue;
    }

    string cmd = receiveIncomingData(nonBlocking);

    if (clientsock < 0) {
      return CMDContinue;
    } else if (cmd.empty() && nonBlocking) {
      return CMDNull;
    } else if (cmd == "step_into") {
      return CMDStepInto;
    } else if (cmd == "step_over") {
      return CMDStepOver;
    } else if (cmd == "step_out") {
      return CMDStepOut;
    } else if (cmd == "continue") {
      return CMDContinue;
    } else if (cmd.find("breakpoint") != string::npos) {
      processBreakpointCMD(cmd);
    } else {
      cerr << PACKAGE << ": comando desconhecido: \"" << cmd << "\"" << endl;
      return CMDStepInto;
    }
  }
}

void InterpreterDBG::processBreakpointCMD(string &bpcommand) {
  // cerr << "process breakpoint " << bpcommand << endl;

  pcre2_code *re;
  pcre2_match_data *md;
  PCRE2_UCHAR *str;
  PCRE2_SPTR pat, subj;
  PCRE2_SIZE offset;
  int line, rc;

  pat = reinterpret_cast<PCRE2_SPTR>(
      "breakpoint cmd=(add|remove).*file=\"([^\"]*)\".*line=(\\d+)");
  subj = reinterpret_cast<PCRE2_SPTR>(bpcommand.c_str());
  re = pcre2_compile(pat, PCRE2_ZERO_TERMINATED, 0, &rc, &offset, nullptr);
  if (offset != 0) {
    return;
  }

  md = pcre2_match_data_create_from_pattern(re, nullptr);
  rc = pcre2_match(re, subj, bpcommand.length(), 0, 0, md, nullptr);
  pcre2_code_free(re);
  if (rc != 4) {
    // cerr << PACKAGE << ": comando invalido (2): \"" << cmd << "\"" << endl;
    pcre2_match_data_free(md);
    return;
  }

  pcre2_substring_get_bynumber(md, 1, &str, &offset);
  string cmd(reinterpret_cast<char *>(str));
  pcre2_substring_free(str);
  pcre2_substring_get_bynumber(md, 2, &str, &offset);
  string file(reinterpret_cast<char *>(str));
  pcre2_substring_free(str);
  pcre2_substring_get_bynumber(md, 3, &str, &offset);
  line = atoi((const char *)str);
  pcre2_substring_free(str);
  pcre2_match_data_free(md);

  // cerr << PACKAGE << ": capturado:" << cmd << ":" << file << ":" << line <<
  // endl;

  if (cmd == "add") {
    breakpoints[file].push_back(line);
    // cerr << PACKAGE << ": adding \"" << file << ":" << line << " -- " << cmd
    // << "\"" << endl;
  } else if (cmd == "remove") {
    breakpoints[file].remove(line);
    // cerr << PACKAGE << ": removing \"" << file << ":" << line << " -- " <<
    // cmd << "\"" << endl;
  } else {
    cerr << PACKAGE << ": breakpoint cmd invalido \"" << bpcommand << "\""
         << endl;
    return;
  }
}

bool InterpreterDBG::breakOn(const string &file, int line) {
  // cerr << "Breakon file:" << file << " * line:" << line << endl;
  for (list<int>::iterator it = breakpoints[file].begin();
       it != breakpoints[file].end(); ++it) {
    // cerr << "---breakon checkin on " << file << " * line:" << (*it) << endl;
    if ((*it) == line) {
      // cerr << "found! breakon BREAK on " << *it << endl;
      return true;
    }
  }

  return false;
}

void InterpreterDBG::closeSock() {
#ifndef WIN32
  shutdown(clientsock, SHUT_RDWR);
  close(clientsock);
  clientsock = -1;
#else
  closesocket(clientsock);
  clientsock = INVALID_SOCKET;
#endif
}
