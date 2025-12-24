/*
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
 */

#ifndef ANTLR3_ADAPTER_HPP
#define ANTLR3_ADAPTER_HPP

#include <antlr3.h>
#include <memory>
#include <stdexcept>
#include <string>

// Forward declarations
class PortugolAST;

/**
 * Exception class for ANTLR3 parsing errors
 */
class Antlr3Exception : public std::runtime_error {
public:
  Antlr3Exception(const std::string &msg, int line = 0)
      : std::runtime_error(msg), _line(line) {}

  int getLine() const { return _line; }

private:
  int _line;
};

/**
 * Wrapper class for ANTLR3 AST nodes to provide C++ interface
 */
class Antlr3AST {
public:
  Antlr3AST() : _tree(nullptr) {}
  Antlr3AST(pANTLR3_BASE_TREE tree) : _tree(tree) {}

  bool isNull() const { return _tree == nullptr; }

  std::string getText() const {
    if (!_tree)
      return "";
    pANTLR3_STRING str = _tree->getText(_tree);
    if (!str)
      return "";
    return std::string((const char *)str->chars);
  }

  int getType() const {
    if (!_tree)
      return 0;
    return _tree->getType(_tree);
  }

  int getLine() const {
    if (!_tree)
      return 0;
    pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
    if (!token)
      return 0;
    return token->getLine(token);
  }

  std::string getFilename() const {
    // ANTLR3 doesn't directly store filename in token
    return "";
  }

  Antlr3AST getFirstChild() const {
    if (!_tree)
      return Antlr3AST(nullptr);
    return Antlr3AST((pANTLR3_BASE_TREE)_tree->getFirstChildWithType(_tree, 0));
  }

  Antlr3AST getNextSibling() const {
    if (!_tree)
      return Antlr3AST(nullptr);
    pANTLR3_BASE_TREE parent = (pANTLR3_BASE_TREE)_tree->getParent(_tree);
    if (!parent)
      return Antlr3AST(nullptr);

    ANTLR3_UINT32 idx = _tree->getChildIndex(_tree);
    return Antlr3AST((pANTLR3_BASE_TREE)parent->getChild(parent, idx + 1));
  }

  ANTLR3_UINT32 getChildCount() const {
    if (!_tree)
      return 0;
    return _tree->getChildCount(_tree);
  }

  Antlr3AST getChild(ANTLR3_UINT32 i) const {
    if (!_tree)
      return Antlr3AST(nullptr);
    return Antlr3AST((pANTLR3_BASE_TREE)_tree->getChild(_tree, i));
  }

  pANTLR3_BASE_TREE getTree() const { return _tree; }

private:
  pANTLR3_BASE_TREE _tree;
};

/**
 * Smart pointer type for managing ANTLR3 AST references
 */
typedef Antlr3AST RefPortugolAST;

/**
 * Wrapper class for ANTLR3 lexer/parser integration
 */
class Antlr3ParserAdapter {
public:
  Antlr3ParserAdapter()
      : _input(nullptr), _lexer(nullptr), _tokens(nullptr), _parser(nullptr) {}

  ~Antlr3ParserAdapter() { cleanup(); }

  void cleanup() {
    if (_parser) {
      _parser->free(_parser);
      _parser = nullptr;
    }
    if (_tokens) {
      _tokens->free(_tokens);
      _tokens = nullptr;
    }
    if (_lexer) {
      _lexer->free(_lexer);
      _lexer = nullptr;
    }
    if (_input) {
      _input->close(_input);
      _input = nullptr;
    }
  }

  bool hasErrors() const { return _hasErrors; }

protected:
  pANTLR3_INPUT_STREAM _input;
  void *_lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  void *_parser;
  bool _hasErrors = false;
};

/**
 * Null AST constant for comparison
 */
namespace antlr3 {
static const Antlr3AST nullAST;
}

#endif // ANTLR3_ADAPTER_HPP
