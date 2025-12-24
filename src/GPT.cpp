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

#include "config.h"
#include "GPT.hpp"

#ifdef WIN32
#include <io.h> //unlink()
#endif

#include "PortugolLexer.h"
#include "PortugolParser.h"
#include "SemanticWalker.h"
#include "InterpreterWalker.h"
#include "Portugol2CWalker.h"
#include "X86Walker.h"

#include "PortugolAST.hpp"
#include "SemanticEval.hpp"
#include "SemanticAnalyzer.hpp"
#include "InterpreterEval.hpp"
#include "Interpreter.hpp"
#include "GPTDisplay.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

GPT *GPT::_self = 0;

GPT::GPT()
    : /*_usePipe(false),*/ _printParseTree(false), _useOutputFile(false) {}

GPT::~GPT() {}

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
  for (auto& p : istream_list) {
    delete p.second;
  }
  return success;
}

bool GPT::compile(const list<string> &ifnames, bool genBinary) {
  bool success = false;
  stringstream s;

  if (!prologue(ifnames)) {
    return false;
  }

  string ofname = _outputfile;
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
    // Create X86 walker and generate assembly
    pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(
        _astree.getTree(), ANTLR3_SIZE_HINT);
    
    pX86Walker x86walker = X86WalkerNew(nodes);
    
    // TODO: Implement x86 walking
    // string asmsrc = x86walker->algoritmo(x86walker);
    string asmsrc; // Placeholder
    
    x86walker->free(x86walker);
    nodes->free(nodes);

    string ftmpname = createTmpFile();
    ofstream fo;

    if (!genBinary) { // salva assembly code
      fo.open(ofname.c_str(), ios_base::out);
      if (!fo) {
        s << PACKAGE << ": não foi possível abrir o arquivo: \"" << ofname
          << "\"" << endl;
        GPTDisplay::self()->showError(s);
        goto bail;
      }
      fo << asmsrc;
      fo.close();
    } else { // compile
      fo.open(ftmpname.c_str(), ios_base::out);
      if (!fo) {
        s << PACKAGE << ": erro ao processar arquivo temporário" << endl;
        GPTDisplay::self()->showError(s);
        goto bail;
      }
      fo << asmsrc;
      fo.close();

      stringstream cmd;
      cmd << "nasm -O1 -fbin -o \"" << ofname << "\" " << ftmpname;

      if (system(cmd.str().c_str()) == -1) {
        s << PACKAGE << ": não foi possível invocar o nasm." << endl;
        GPTDisplay::self()->showError(s);
        goto bail;
      }

#ifndef WIN32
      cmd.str("");
      cmd << "chmod +x " << ofname;
      system(cmd.str().c_str());
#endif
    }

    success = true;

  bail:
    if (ftmpname.length() > 0) {
      unlink(ftmpname.c_str());
    }
    return success;
  } catch (SymbolTableException &e) {
    s << PACKAGE << ": erro interno: " << e.getMessage() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}

bool GPT::translate2C(const list<string> &ifnames) {
  bool success = false;
  stringstream s;

  if (!prologue(ifnames)) {
    return false;
  }

  string ofname = _outputfile;
  if (!_useOutputFile) {
    ofname += ".c";
  }

  try {
    // Create tree node stream for walking
    pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(
        _astree.getTree(), ANTLR3_SIZE_HINT);
    
    pPortugol2CWalker pt2cwalker = Portugol2CWalkerNew(nodes);
    
    // TODO: Implement C translation walking
    // string c_src = pt2cwalker->algoritmo(pt2cwalker);
    string c_src; // Placeholder
    
    pt2cwalker->free(pt2cwalker);
    nodes->free(nodes);

    ofstream fo;
    fo.open(ofname.c_str(), ios_base::out);
    if (!fo) {
      s << PACKAGE << ": não foi possível abrir o arquivo: \"" << ofname << "\""
        << endl;
      GPTDisplay::self()->showError(s);
      goto bail;
    }
    fo << c_src;
    fo.close();

    success = true;

  bail:
    return success;
  } catch (SymbolTableException &e) {
    s << PACKAGE << ": erro interno: " << e.getMessage() << endl;
    GPTDisplay::self()->showError(s);
    return false;
  }
}

int GPT::interpret(const list<string> &ifnames, const string &host, int port) {
  if (!prologue(ifnames)) {
    return 0;
  }

  // Use the manual interpreter that walks the AST
  // Pass source content for proper text extraction (ANTLR3 UTF-8 workaround)
  Interpreter interp(_stable, host, port, _sourceContent);
  int r = interp.run(_astree.getTree());

  return r;
}

bool GPT::parse(list<pair<string, istream *>> &istream_list) {
  stringstream s;

  try {
    // For now, we only support single file parsing with ANTLR3
    // Multiple file support would require concatenating or chaining inputs
    if (istream_list.empty()) {
      s << PACKAGE << ": nenhum arquivo de entrada" << endl;
      GPTDisplay::self()->showError(s);
      return false;
    }

    // Get the first (and primary) file
    string filename = istream_list.front().first;
    istream* input = istream_list.front().second;
    
    GPTDisplay::self()->addFileName(filename);
    GPTDisplay::self()->setCurrentFile(filename);

    // Read entire file into memory for ANTLR3
    std::stringstream buffer;
    buffer << input->rdbuf();
    std::string content = buffer.str();
    
    // Store original content for text extraction (workaround for ANTLR3 UTF-8 bug)
    _sourceContent = content;

    // Create ANTLR3 input stream from string
    // Use UTF8 encoding to convert bytes to Unicode code points
    pANTLR3_INPUT_STREAM inputStream = antlr3StringStreamNew(
        (pANTLR3_UINT8)content.c_str(),
        ANTLR3_ENC_UTF8,
        content.size(),
        (pANTLR3_UINT8)filename.c_str()
    );

    if (!inputStream) {
      s << PACKAGE << ": erro ao criar stream de entrada" << endl;
      GPTDisplay::self()->showError(s);
      return false;
    }

    // Create lexer
    pPortugolLexer lexer = PortugolLexerNew(inputStream);
    if (!lexer) {
      s << PACKAGE << ": erro ao criar lexer" << endl;
      GPTDisplay::self()->showError(s);
      inputStream->close(inputStream);
      return false;
    }

    // Create token stream
    pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(
        ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
    if (!tokens) {
      s << PACKAGE << ": erro ao criar token stream" << endl;
      GPTDisplay::self()->showError(s);
      lexer->free(lexer);
      inputStream->close(inputStream);
      return false;
    }

    // Create parser
    pPortugolParser parser = PortugolParserNew(tokens);
    if (!parser) {
      s << PACKAGE << ": erro ao criar parser" << endl;
      GPTDisplay::self()->showError(s);
      tokens->free(tokens);
      lexer->free(lexer);
      inputStream->close(inputStream);
      return false;
    }

    // Parse the algorithm
    PortugolParser_algoritmo_return parseResult = parser->algoritmo(parser);

    // Check for errors
    if (parser->pParser->rec->state->errorCount > 0) {
      GPTDisplay::self()->showErrors();
      parser->free(parser);
      tokens->free(tokens);
      lexer->free(lexer);
      inputStream->close(inputStream);
      return false;
    }

    // Get AST
    _astree = PortugolAST(parseResult.tree);
    
    if (_astree.isNull()) {
      s << PACKAGE << ": erro interno: no parse tree" << endl;
      GPTDisplay::self()->showError(s);
      parser->free(parser);
      tokens->free(tokens);
      lexer->free(lexer);
      inputStream->close(inputStream);
      return false;
    }

    // Extract algorithm name from AST (first child after T_KW_ALGORITMO)
    if (_outputfile.empty() && _astree.getChildCount() > 0) {
      PortugolAST* algDecl = _astree.getChild(0);
      if (algDecl && algDecl->getChildCount() > 0) {
        PortugolAST* nameNode = algDecl->getChild(0);
        if (nameNode) {
          _outputfile = nameNode->getText();
        }
      }
    }

    if (_printParseTree) {
      pANTLR3_STRING treeStr = parseResult.tree->toStringTree(parseResult.tree);
      std::cerr << (const char*)treeStr->chars << std::endl << std::endl;
    }

    // Semantic analysis using manual analyzer
    // Pass source content for proper text extraction (ANTLR3 UTF-8 workaround)
    SemanticAnalyzer analyzer(_stable, _sourceContent);
    analyzer.analyze(parseResult.tree);

    if (GPTDisplay::self()->hasError()) {
      GPTDisplay::self()->showErrors();
      parser->free(parser);
      tokens->free(tokens);
      lexer->free(lexer);
      inputStream->close(inputStream);
      return false;
    }
    // Note: Don't free parser, tokens, lexer, inputStream yet
    // as they may be needed for the AST
    // In a production system, you'd need to manage this memory more carefully

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
