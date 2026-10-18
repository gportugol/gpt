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

#include <cerrno>
#include <cstring>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <unistd.h>

#if !defined(WIN32) && !defined(MSG_NOSIGNAL)
void sigPipeHandler(int) { InterpreterDBG::self()->closeSock(); }
#endif

InterpreterDBG *InterpreterDBG::singleton = NULL;

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
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  stringstream service;
  service << port;

  struct addrinfo *addresses = NULL;
  int rc = getaddrinfo(host.c_str(), service.str().c_str(), &hints, &addresses);
  if (rc != 0) {
    stringstream s;
    s << PACKAGE << ": não foi possível resolver o depurador em " << host << ":"
      << port << ": " << gai_strerror(rc) << endl;
    GPTDisplay::self()->showError(s);
    return;
  }

  int error = 0;
  for (struct addrinfo *ai = addresses; ai != NULL; ai = ai->ai_next) {
    clientsock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (clientsock < 0) {
      error = errno;
      continue;
    }
    if (connect(clientsock, ai->ai_addr, ai->ai_addrlen) == 0) {
      break;
    }
    error = errno;
    closeSock();
  }
  freeaddrinfo(addresses);

  if (clientsock < 0) {
    stringstream s;
    s << PACKAGE << ": não foi possível conectar ao depurador em " << host
      << ":" << port << ": " << strerror(error) << endl;
    GPTDisplay::self()->showError(s);
    return;
  }

#ifndef MSG_NOSIGNAL
  if (SIG_ERR == signal(SIGPIPE, sigPipeHandler)) {
    stringstream s;
    s << PACKAGE << ": erro interno em InterpreterDBG::init" << endl;
    GPTDisplay::self()->showError(s);
  }
#endif
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
    s << "<entry id=\"" << id++ << "\" file=\"" << (*it).first
      << "\" function=\"" << (*it).second.first << "\" line=\""
      << (*it).second.second << "\"/>";
  }
  s << "</stackinfo>";

  sendData(s);
}

void InterpreterDBG::sendVariables(map<string, Variable> globals,
                                   list<pair<string, pair<string, int>>> &stk,
                                   bool globalScope) {
  if (clientsock < 0)
    return;

  stringstream s;
  s << "<vars for=\"" << (globalScope ? "$global" : "$local") << "\" scope=\""
    << (globalScope ? stk.front().second.first : stk.back().first) << "\">";

  bool primitive;
  for (map<string, Variable>::iterator it = globals.begin();
       it != globals.end(); ++it) {
    primitive = it->second.isPrimitive;
    s << "<var name=\"" << it->first << "\" type=\""
      << Symbol::typeToString(it->second.type) << "\" primitive=\""
      << (primitive ? "true" : "false") << "\"";
    if (primitive) {
      s << " value=\"" << it->second.primitiveValue << "\"/>";
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
                                         map<string, string> &values,
                                         list<int> &dims, int type,
                                         string vindex /*= "" */) {
  list<int>::iterator lsize;
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
      string val = values[vs.str()];
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
      s << "<var name=\"" << i << "\" primitive=\"true\" value=\"" << val
        << "\"/>";
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
  string frame = s.str();
  int flags = 0;
#ifdef MSG_NOSIGNAL
  flags = MSG_NOSIGNAL;
#endif
  const char *data = frame.c_str();
  size_t left = frame.length() + 1;
  while (left > 0) {
    ssize_t n = send(clientsock, data, left, flags);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      closeSock();
      return;
    }
    data += n;
    left -= n;
  }
}

string InterpreterDBG::receiveIncomingData(bool nonBlocking) {
  char buffer[1024];
  int received = 0;

  buffer[1023] = '\0';

  string cmd;
  int msg_size;

  // retrieve msg size!
  int idx = 0;
  int rec;
  while (true) {

#ifndef WIN32
    if (nonBlocking) {
      rec = recv(clientsock, &buffer[idx], sizeof(char), MSG_DONTWAIT);
    } else {
      rec = recv(clientsock, &buffer[idx], sizeof(char), 0);
    }

    if (nonBlocking && (rec == 0)) {
      return "";
    }
#else
    // todo[win]: this code is there just to compile on windows, it should be
    // non-blocking
    rec = recv(clientsock, &buffer[idx], sizeof(char), 0);
#endif

    received += rec;

    if ((rec == 0) || (rec < 1)) {
      return cmd;
    }

    if (buffer[idx] == '\0') {
      break;
    }
    idx++;
  }

  //  cerr << "MESSAGE SIZE: " << buffer << ", received : " << received << endl;

  msg_size = atoi(buffer) * sizeof(char);

  received = recv(clientsock, buffer, msg_size, 0);
  if (received < 0) {
    cerr << PACKAGE << ": erro ao receber dados." << endl;
    return cmd;
  } else if ((received == 0) || (received < msg_size)) {
    //    cerr << "insuficient! received: " << received << ", msg_size: " <<
    //    msg_size << endl;
    closeSock();
    return cmd;
  }

  //  cmd.assign(buffer, received);
  cmd = buffer;
  //  cerr << "MESSAGE : " << cmd << ", assigned: " << received << endl;

  return cmd;
}

int InterpreterDBG::receiveCmd(bool nonBlocking) {
  if (clientsock < 0) {
    return CMDContinue;
  }

  string cmd = receiveIncomingData(nonBlocking);

  int ret;
  if (cmd.empty() && nonBlocking) {
    ret = CMDNull;
  } else if (cmd == "step_into") {
    ret = CMDStepInto;
  } else if (cmd == "step_over") {
    ret = CMDStepOver;
  } else if (cmd == "step_out") {
    ret = CMDStepOut;
  } else if (cmd == "continue") {
    ret = CMDContinue;
  } else if (cmd.find("breakpoint") != string::npos) {
    processBreakpointCMD(cmd);
    ret = receiveCmd(nonBlocking);
  } else {
    cerr << PACKAGE << ": comando desconhecido: \"" << cmd << "\"" << endl;
    ret = CMDStepInto;
  }

  return ret;
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
