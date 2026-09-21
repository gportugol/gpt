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

// Include ANTLR4 headers FIRST, before any Windows headers
// Windows defines ERROR macro which conflicts with ParseTreeType::ERROR
#include "PortugolLexer.h"
#include "PortugolParser.h"
#include "antlr4-runtime.h"

#include "GPT.hpp"
#include "config.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <io.h> // unlink()
#include <windows.h>
#undef byte  // Avoid conflict with std::byte
#undef ERROR // Avoid conflict with ParseTreeType::ERROR (already parsed by
             // ANTLR4)
#else
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

#include "GPTDisplay.hpp"
#include "Interpreter.hpp"
#include "Portugol2CTranslator.hpp"
#include "PortugolErrorStrategy.hpp"
#include "SemanticAnalyzer.hpp"
#include "X86Generator.hpp"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

#ifndef WIN32
extern char **environ;
#endif

// The native backend emits 32-bit x86 assembly and NASM turns it into a flat
// binary, so -o only yields something runnable when gpt itself runs on x86.
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) ||            \
    defined(_M_X64)
#define GPT_HOST_X86 1
#endif

using namespace antlr4;

GPT *GPT::_self = 0;

GPT::GPT()
    : _printParseTree(false), _useOutputFile(false), _parseTree(nullptr) {}

GPT::~GPT() { cleanup(); }

void GPT::cleanup() {
  // Clean up in reverse order of creation
  _parseTree = nullptr;
  _parser.reset();
  _tokens.reset();
  _lexer.reset();
  _inputStream.reset();
}

GPT *GPT::self() {
  if (!GPT::_self) {
    GPT::_self = new GPT();
  }
  return GPT::_self;
}

void GPT::reportDicas(bool value) { GPTDisplay::self()->showTips(value); }

void GPT::printParseTree(bool value) { _printParseTree = value; }

void GPT::setOutputFile(string str) {
  _useOutputFile = true;
  _outputfile = str;
}

// Find nasm executable - check same directory as gpt first, then PATH
static string findNasm() {
#ifdef WIN32
  char exePath[MAX_PATH];
  // A result of MAX_PATH means the path did not fit and was truncated.
  DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
  if (len > 0 && len < MAX_PATH) {
    string dir(exePath);
    size_t pos = dir.find_last_of("\\/");
    if (pos != string::npos) {
      string nasmPath = dir.substr(0, pos + 1) + "nasm.exe";
      if (GetFileAttributesA(nasmPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return nasmPath;
      }
    }
  }
  return "nasm"; // Fall back to PATH
#else
  char exePath[PATH_MAX];
  // readlink truncates silently, so a full buffer may hold a partial path.
  ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
  if (len > 0 && len < (ssize_t)sizeof(exePath) - 1) {
    exePath[len] = '\0';
    string dir(exePath);
    size_t pos = dir.find_last_of('/');
    if (pos != string::npos) {
      string nasmPath = dir.substr(0, pos + 1) + "nasm";
      if (access(nasmPath.c_str(), X_OK) == 0) {
        return nasmPath;
      }
    }
  }
  return "nasm"; // Fall back to PATH
#endif
}

// The arguments reach nasm as they are, without a shell: the output name
// comes from the command line and may hold any character.
static bool runNasm(const string &ofname, const string &asmfile) {
  string nasm = findNasm();
  const char *argv[] = {nasm.c_str(),   "-O1",           "-fbin", "-o",
                        ofname.c_str(), asmfile.c_str(), NULL};
#ifdef WIN32
  // CreateProcess hands nasm a single command line, which its C runtime
  // splits back into argv: quote each argument the way that split expects.
  string cmdline;
  for (const char **arg = argv; *arg; ++arg) {
    string a(*arg);
    if (arg != argv) {
      cmdline += ' ';
    }
    if (!a.empty() && a.find_first_of(" \t\n\v\"") == string::npos) {
      cmdline += a;
      continue;
    }
    cmdline += '"';
    for (string::size_type i = 0;; ++i) {
      string::size_type backslashes = 0;
      while (i < a.size() && a[i] == '\\') {
        ++backslashes;
        ++i;
      }
      if (i == a.size()) {
        cmdline.append(backslashes * 2, '\\');
        break;
      }
      if (a[i] == '"') {
        cmdline.append(backslashes * 2 + 1, '\\');
      } else {
        cmdline.append(backslashes, '\\');
      }
      cmdline += a[i];
    }
    cmdline += '"';
  }

  vector<char> buf(cmdline.begin(), cmdline.end());
  buf.push_back('\0');
  STARTUPINFOA si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION pi;
  if (!CreateProcessA(NULL, &buf[0], NULL, NULL, TRUE, 0, NULL, NULL, &si,
                      &pi)) {
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 1;
  bool ok = GetExitCodeProcess(pi.hProcess, &code) && code == 0;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return ok;
#else
  pid_t pid;
  if (posix_spawnp(&pid, argv[0], NULL, NULL, const_cast<char **>(argv),
                   environ) != 0) {
    return false;
  }
  int status;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) {
      return false;
    }
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

#ifndef WIN32
// Same bits as "chmod +x": execute for everyone, except what the umask hides.
static bool makeExecutable(const string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  mode_t mask = umask(0);
  umask(mask);
  mode_t exec = (S_IXUSR | S_IXGRP | S_IXOTH) & ~mask;
  return chmod(path.c_str(), st.st_mode | exec) == 0;
}
#endif

// The content goes through the handle that created the file: reopening the
// name would follow whatever sits at that path by then.
bool GPT::writeTmpFile(const string &content, string &path) {
#ifdef WIN32
  char dir[MAX_PATH + 1];
  DWORD len = GetTempPathA(sizeof(dir), dir);
  if (len == 0 || len > sizeof(dir)) {
    return false;
  }
  DWORD unique = GetTickCount() ^ GetCurrentProcessId();
  for (int tries = 0; tries < 100; ++tries, ++unique) {
    stringstream name;
    name << dir << "c__" << hex << unique << ".tmp";
    // With FILE_FLAG_OPEN_REPARSE_POINT a link left at the name is itself the
    // existing file, so CREATE_NEW fails instead of resolving it.
    HANDLE h =
        CreateFileA(name.str().c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      if (GetLastError() == ERROR_FILE_EXISTS) {
        continue;
      }
      return false;
    }
    path = name.str();
    DWORD written;
    bool ok = WriteFile(h, content.data(), content.size(), &written, NULL) &&
              written == content.size();
    return CloseHandle(h) && ok;
  }
  return false;
#else
  const char *tmpdir = getenv("TMPDIR");
  string name = string(tmpdir && *tmpdir ? tmpdir : "/tmp") + "/c__XXXXXX";
  vector<char> buf(name.begin(), name.end());
  buf.push_back('\0');
  int fd = mkstemp(&buf[0]);
  if (fd == -1) {
    return false;
  }
  path = &buf[0];

  const char *data = content.data();
  string::size_type left = content.size();
  while (left > 0) {
    ssize_t n = write(fd, data, left);
    if (n == -1) {
      if (errno == EINTR) {
        continue;
      }
      close(fd);
      return false;
    }
    data += n;
    left -= n;
  }
  return close(fd) == 0;
#endif
}

void GPT::showHelp() {
  stringstream s;
  s << "Modo de uso: " << PACKAGE
    << " [opções] algoritmos...\n\n"
       "Opções:\n"
       "   -v            mostra versão do programa\n"
       "   -h            mostra esse texto\n"
       "   -o <arquivo>  compila e salva programa como <arquivo> (só em x86)\n"
       "   -t <arquivo>  salva o código em linguagem C como <arquivo>\n"
       "   -s <arquivo>  salva o código em linguagem Assembly como <arquivo>\n"
       "   -i            interpreta o algoritmo\n"
       "   -d            exibe dicas no relatório de erros\n\n"
       "   Maiores informações no manual.\n";

  GPTDisplay::self()->showMessage(s);
}

void GPT::showVersion() {
  stringstream s;
  s << "GPT - Compilador G-Portugol\n"
       "Versão  : "
    << VERSION
    << "\n"
       "Website : https://gportugol.github.io\n"
       "Copyright (C) 2003-2009 Thiago Silva <tsilva@sourcecraft.info>\n"
       "Copyright (C) 2026 G-Portugol contributors "
       "<gportugol@googlegroups.com>\n\n";
  GPTDisplay::self()->showMessage(s);
}

// UTF-8 as in RFC 3629: no overlong forms, no surrogates and nothing above
// U+10FFFF. A file in another encoding, such as ISO-8859-1, would otherwise
// reach the lexer as a run of invalid characters and unclosed quotes.
static bool isValidUtf8(const string &text) {
  string::size_type i = 0;
  while (i < text.size()) {
    unsigned char c = (unsigned char)text[i];
    if (c < 0x80) {
      i++;
      continue;
    }
    string::size_type len;
    unsigned int cp;
    if (c >= 0xC2 && c <= 0xDF) {
      len = 2;
      cp = c & 0x1F;
    } else if (c >= 0xE0 && c <= 0xEF) {
      len = 3;
      cp = c & 0x0F;
    } else if (c >= 0xF0 && c <= 0xF4) {
      len = 4;
      cp = c & 0x07;
    } else {
      return false;
    }
    if (i + len > text.size()) {
      return false;
    }
    for (string::size_type k = 1; k < len; ++k) {
      unsigned char cc = (unsigned char)text[i + k];
      if ((cc & 0xC0) != 0x80) {
        return false;
      }
      cp = (cp << 6) | (cc & 0x3F);
    }
    if ((len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000) ||
        (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
      return false;
    }
    i += len;
  }
  return true;
}

bool GPT::prologue(const list<string> &ifnames) {
  stringstream s;
  bool success = false;

  list<pair<string, istream *>> istream_list;
  for (list<string>::const_iterator it = ifnames.begin(); it != ifnames.end();
       ++it) {
    ifstream fi((*it).c_str());
    if (!fi) {
      s << PACKAGE << ": não foi possível abrir o arquivo: \"" << (*it) << "\""
        << endl;
      GPTDisplay::self()->showError(s);
      goto bail;
    }
    stringstream content;
    content << fi.rdbuf();
    string text = content.str();
    // Editors on Windows often save UTF-8 with a byte order mark, which the
    // lexer would otherwise read as an invalid character.
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
      text.erase(0, 3);
    }
    if (!isValidUtf8(text)) {
      s << PACKAGE << ": " << (*it) << ": o arquivo não está em UTF-8" << endl;
      GPTDisplay::self()->showError(s);
      goto bail;
    }
    istream_list.push_back(
        pair<string, istream *>(*it, new istringstream(text)));
  }

  if (!parse(istream_list)) {
    goto bail;
  }

  success = true;

bail:
  for (list<pair<string, istream *>>::iterator it = istream_list.begin();
       it != istream_list.end(); ++it) {
    delete it->second;
  }
  return success;
}

bool GPT::compile(const list<string> &ifnames, bool genBinary) {
  stringstream s;

#ifndef GPT_HOST_X86
  if (genBinary) {
    s << PACKAGE << ": a geração de executável só funciona em máquinas x86. "
      << "Use -t para traduzir o algoritmo para C." << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
#endif

  if (!prologue(ifnames)) {
    return false;
  }

  string ofname = _outputfile;
  string asmfile;

  if (!_useOutputFile) {
    if (!genBinary) {
      ofname += ".asm";
    }
#ifdef WIN32
    else {
      ofname += ".exe";
    }
#endif
  }

  try {
    // Generate x86 assembly using visitor
    X86Generator generator(_stable);
    string asmCode = generator.generate(
        static_cast<PortugolParser::AlgoritmoContext *>(_parseTree));

    if (genBinary) {
      if (!writeTmpFile(asmCode, asmfile)) {
        if (!asmfile.empty()) {
          unlink(asmfile.c_str());
        }
        s << PACKAGE << ": erro ao processar arquivo temporário" << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }

      bool assembled = runNasm(ofname, asmfile);
      unlink(asmfile.c_str());
      if (!assembled) {
        s << PACKAGE << ": não foi possível montar o programa com o nasm."
          << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }

#ifndef WIN32
      if (!makeExecutable(ofname)) {
        s << PACKAGE << ": não foi possível tornar o arquivo executável: \""
          << ofname << "\"" << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }
#endif
    } else {
      // Just write assembly file
      ofstream fasm(ofname.c_str());
      if (!fasm) {
        s << PACKAGE << ": não foi possível abrir o arquivo: \"" << ofname
          << "\"" << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }
      fasm << asmCode;
      fasm.close();
    }

    return true;

  } catch (SymbolTableException &e) {
    s << PACKAGE << ": erro interno: " << e.getMessage() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  } catch (std::exception &e) {
    s << PACKAGE << ": erro interno: " << e.what() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}

bool GPT::translate2C(const list<string> &ifnames) {
  stringstream s;

  if (!prologue(ifnames)) {
    return false;
  }

  string ofname = _outputfile;
  if (!_useOutputFile) {
    ofname += ".c";
  }

  try {
    // Use C translator visitor
    Portugol2CTranslator translator(_stable);
    string cCode = translator.translate(
        static_cast<PortugolParser::AlgoritmoContext *>(_parseTree));

    // Write to output file
    ofstream out(ofname);
    if (!out) {
      s << PACKAGE << ": não foi possível abrir o arquivo: \"" << ofname << "\""
        << endl;
      GPTDisplay::self()->showError(s);
      return false;
    }

    out << cCode;
    out.close();
    return true;

  } catch (SymbolTableException &e) {
    s << PACKAGE << ": erro interno: " << e.getMessage() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  } catch (std::exception &e) {
    s << PACKAGE << ": erro interno: " << e.what() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}

int GPT::interpret(const list<string> &ifnames, const string &host, int port) {
  if (!prologue(ifnames)) {
    return EXIT_FAILURE;
  }

  // Use the interpreter visitor
  Interpreter interp(_stable, host, port);
  int result =
      interp.run(static_cast<PortugolParser::AlgoritmoContext *>(_parseTree));

  return result;
}

bool GPT::parse(list<pair<string, istream *>> &istream_list) {
  stringstream s;

  try {
    // Clean up previous parse state
    cleanup();

    if (istream_list.empty()) {
      s << PACKAGE << ": nenhum arquivo de entrada" << endl;
      GPTDisplay::self()->showError(s);
      return false;
    }

    // The manual promises that extra files (command line and GPT_INCLUDE)
    // are concatenated and processed as if the algorithm were in one file.
    GPTDisplay::self()->clearFileRanges();
    string content;
    int line = 1;
    for (list<pair<string, istream *>>::iterator it = istream_list.begin();
         it != istream_list.end(); ++it) {
      std::stringstream buffer;
      buffer << it->second->rdbuf();
      string part = buffer.str();

      if (!part.empty() && part[part.size() - 1] != '\n') {
        part += '\n';
      }

      GPTDisplay::self()->addFileName(it->first);
      GPTDisplay::self()->addFileRange(line, it->first);
      for (string::size_type i = 0; i < part.size(); ++i) {
        if (part[i] == '\n') {
          line++;
        }
      }
      content += part;
    }

    string filename = istream_list.front().first;
    GPTDisplay::self()->setCurrentFile(filename);

    // Create ANTLR4 input stream (must persist for parse tree)
    _inputStream = std::make_unique<ANTLRInputStream>(content);
    _inputStream->name = filename;

    // Create lexer (must persist for parse tree)
    _lexer = std::make_unique<PortugolLexer>(_inputStream.get());
    _lexer->removeErrorListeners();
    PortugolLexerErrorListener lexerErrors;
    _lexer->addErrorListener(&lexerErrors);

    // Create token stream (must persist for parse tree)
    _tokens = std::make_unique<CommonTokenStream>(_lexer.get());
    validateTokens(*_tokens);

    // Create parser (must persist for parse tree)
    _parser = std::make_unique<PortugolParser>(_tokens.get());
    _parser->removeErrorListeners();
    _parser->setErrorHandler(std::make_shared<PortugolErrorStrategy>());

    // Parse the algorithm
    auto parseTree = _parser->algoritmo();
    _parseTree = parseTree;

    _lexer->removeErrorListeners();

    if (GPTDisplay::self()->hasError()) {
      GPTDisplay::self()->showErrors();
      return false;
    }

    // Extract algorithm name from parse tree
    if (_outputfile.empty() && parseTree) {
      auto declAlg = parseTree->declaracao_algoritmo();
      if (declAlg) {
        auto idToken = declAlg->T_IDENTIFICADOR();
        if (idToken) {
          _outputfile = idToken->getText();
        }
      }
    }

    if (_printParseTree) {
      std::cerr << parseTree->toStringTree(_parser.get()) << std::endl
                << std::endl;
    }

    // Semantic analysis
    SemanticAnalyzer analyzer(_stable);
    analyzer.analyze(parseTree);

    if (GPTDisplay::self()->hasError()) {
      GPTDisplay::self()->showErrors();
      return false;
    }

    return true;
  } catch (std::exception &e) {
    s << PACKAGE << ": erro interno: " << e.what() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}
