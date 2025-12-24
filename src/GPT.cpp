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
#include <libgen.h> // dirname
#include <limits.h> // PATH_MAX
#endif

#include "GPTDisplay.hpp"
#include "Interpreter.hpp"
#include "Portugol2CTranslator.hpp"
#include "SemanticAnalyzer.hpp"
#include "X86Generator.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

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

// Find nasm executable - check same directory as gpt first, then PATH
static string findNasm() {
#ifdef WIN32
  // Get path to current executable
  char exePath[MAX_PATH];
  if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0) {
    string dir(exePath);
    size_t pos = dir.find_last_of("\\/");
    if (pos != string::npos) {
      string nasmPath = dir.substr(0, pos + 1) + "nasm.exe";
      // Check if file exists
      if (GetFileAttributesA(nasmPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return "\"" + nasmPath + "\"";
      }
    }
  }
  return "nasm"; // Fall back to PATH
#else
  // On Linux, check same directory as executable
  char exePath[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
  if (len > 0) {
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

string GPT::createTmpFile() {
#ifdef WIN32
  string cf = getenv("TEMP");
  cf += "gpt_tmp"; // TODO! criar real tmp file
  return cf;
#else
  char cfilepath[] = "/tmp/c__XXXXXX";
  int fd = mkstemp(cfilepath);
  close(fd);
  return cfilepath;
#endif
}

void GPT::showHelp() {
  stringstream s;
  s << "Modo de uso: " << PACKAGE
    << " [opções] algoritmos...\n\n"
       "Opções:\n"
       "   -v            mostra versão do programa\n"
       "   -h            mostra esse texto\n"
       "   -o <arquivo>  compila e salva programa como <arquivo>\n"
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
       "Copyright (C) 2003-2009 Thiago Silva <tsilva@sourcecraft.info>\n\n";
  GPTDisplay::self()->showMessage(s);
}

bool GPT::prologue(const list<string> &ifnames) {
  stringstream s;
  bool success = false;

  list<pair<string, istream *>> istream_list;
  for (list<string>::const_iterator it = ifnames.begin(); it != ifnames.end();
       ++it) {
    ifstream *fi = new ifstream((*it).c_str());
    if (!*fi) {
      s << PACKAGE << ": não foi possível abrir o arquivo: \"" << (*it) << "\""
        << endl;
      GPTDisplay::self()->showError(s);
      goto bail;
    }
    istream_list.push_back(pair<string, istream *>(*it, fi));
  }

  if (!parse(istream_list)) {
    goto bail;
  }

  success = true;

bail:
  // Clean up streams
  for (auto &p : istream_list) {
    delete p.second;
  }
  return success;
}

bool GPT::compile(const list<string> &ifnames, bool genBinary) {
  stringstream s;

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
      // Find nasm executable (check same dir as gpt, then PATH)
      string nasmCmd = findNasm();

      // Check if nasm is available
      string checkCmd = nasmCmd + " -v";
#ifdef WIN32
      checkCmd += " >NUL 2>&1";
#else
      checkCmd += " >/dev/null 2>&1";
#endif
      int nasmCheck = system(checkCmd.c_str());
      if (nasmCheck != 0) {
        s << PACKAGE << ": nasm não encontrado. Instale com: "
#ifdef WIN32
          << "pacman -S mingw-w64-x86_64-nasm"
#else
          << "sudo apt install nasm"
#endif
          << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }

      // Write to temp file, then assemble
      asmfile = createTmpFile();
      ofstream fasm(asmfile.c_str());
      if (!fasm) {
        s << PACKAGE << ": não foi possível criar arquivo temporário" << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }
      fasm << asmCode;
      fasm.close();

#ifdef WIN32
      // Windows: use -f win32 to generate .obj, then link with gcc
      string objfile = asmfile + ".obj";
      string cmd =
          nasmCmd + " -f win32 -o \"" + objfile + "\" \"" + asmfile + "\" 2>&1";

      int ret = system(cmd.c_str());
      if (ret != 0) {
        s << PACKAGE << ": erro ao executar nasm (exit code: " << ret << ")"
          << endl;
        s << "Arquivo assembly: " << asmfile << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }

      // Link with 32-bit gcc (requires /mingw32/bin in PATH)
      cmd = "gcc -o \"" + ofname + "\" \"" + objfile +
            "\" -lkernel32 -nostartfiles -Wl,-e,_start 2>&1";
      ret = system(cmd.c_str());
      if (ret != 0) {
        s << PACKAGE << ": erro ao linkar (exit code: " << ret << ")" << endl;
        GPTDisplay::self()->showError(s);
        unlink(objfile.c_str());
        return false;
      }

      // Cleanup temp files
      unlink(asmfile.c_str());
      unlink(objfile.c_str());
#else
      // Linux: produces raw binary (ELF header is embedded in the asm)
      string cmd =
          nasmCmd + " -O1 -fbin -o \"" + ofname + "\" \"" + asmfile + "\" 2>&1";

      int ret = system(cmd.c_str());
      if (ret != 0) {
        s << PACKAGE << ": erro ao executar nasm (exit code: " << ret << ")"
          << endl;
        s << "Arquivo assembly: " << asmfile << endl;
        GPTDisplay::self()->showError(s);
        // Don't delete temp file so user can debug
        return false;
      }

      // Make executable on Linux
      cmd = "chmod +x \"" + ofname + "\"";
      system(cmd.c_str());

      // Cleanup temp file
      unlink(asmfile.c_str());
#endif
    } else {
      // Just write assembly file
      ofstream fasm(ofname.c_str());
      if (!fasm) {
        s << PACKAGE << ": não foi possível criar arquivo: " << ofname << endl;
        GPTDisplay::self()->showError(s);
        return false;
      }
      fasm << asmCode;
      fasm.close();
    }

    return true;

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
      s << PACKAGE << ": não foi possível criar o arquivo: " << ofname << endl;
      GPTDisplay::self()->showError(s);
      return false;
    }

    out << cCode;
    out.close();
    return true;

  } catch (std::exception &e) {
    s << PACKAGE << ": erro interno: " << e.what() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}

int GPT::interpret(const list<string> &ifnames, const string &host, int port) {
  if (!prologue(ifnames)) {
    return 0;
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

    // Get the first (and primary) file
    string filename = istream_list.front().first;
    istream *input = istream_list.front().second;

    GPTDisplay::self()->addFileName(filename);
    GPTDisplay::self()->setCurrentFile(filename);

    // Read entire file into memory
    std::stringstream buffer;
    buffer << input->rdbuf();
    std::string content = buffer.str();

    // Store original content
    _sourceContent = content;

    // Create ANTLR4 input stream (must persist for parse tree)
    _inputStream = std::make_unique<ANTLRInputStream>(content);
    _inputStream->name = filename;

    // Create lexer (must persist for parse tree)
    _lexer = std::make_unique<PortugolLexer>(_inputStream.get());

    // Create token stream (must persist for parse tree)
    _tokens = std::make_unique<CommonTokenStream>(_lexer.get());

    // Create parser (must persist for parse tree)
    _parser = std::make_unique<PortugolParser>(_tokens.get());

    // Parse the algorithm
    auto parseTree = _parser->algoritmo();
    _parseTree = parseTree;

    // Check for errors
    if (_parser->getNumberOfSyntaxErrors() > 0) {
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
    SemanticAnalyzer analyzer(_stable, _sourceContent);
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

  s << "GPT::parse: bug, nao deveria executar essa linha!" << endl;
  GPTDisplay::self()->showError(s);
  return false;
}
