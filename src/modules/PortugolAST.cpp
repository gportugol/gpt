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

#include "PortugolAST.hpp"
#include <map>

// Null AST singleton
namespace antlr {
    PortugolAST* nullAST = nullptr;
}

// Cache for AST nodes to avoid memory leaks
static std::map<pANTLR3_BASE_TREE, PortugolAST*> astCache;

PortugolAST::PortugolAST() 
    : _tree(nullptr), _evalType(0), _endLine(0) {
}

PortugolAST::PortugolAST(pANTLR3_BASE_TREE tree)
    : _tree(tree), _evalType(0), _endLine(0) {
}

PortugolAST::PortugolAST(const PortugolAST& other)
    : _tree(other._tree), _evalType(other._evalType), 
      _endLine(other._endLine), _filename(other._filename) {
}

PortugolAST::~PortugolAST() {
    // Don't free _tree - it's managed by ANTLR3
}

PortugolAST& PortugolAST::operator=(const PortugolAST& other) {
    if (this != &other) {
        _tree = other._tree;
        _evalType = other._evalType;
        _endLine = other._endLine;
        _filename = other._filename;
    }
    return *this;
}

bool PortugolAST::isNull() const {
    return _tree == nullptr;
}

std::string PortugolAST::getText() const {
    if (!_tree) return "";
    pANTLR3_STRING str = _tree->getText(_tree);
    if (!str) return "";
    return std::string((const char*)str->chars);
}

int PortugolAST::getType() const {
    if (!_tree) return 0;
    return _tree->getType(_tree);
}

int PortugolAST::getLine() const {
    if (!_tree) return 0;
    pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
    if (!token) return 0;
    return token->getLine(token);
}

int PortugolAST::getEndLine() const {
    return _endLine > 0 ? _endLine : getLine();
}

std::string PortugolAST::getFilename() const {
    return _filename;
}

PortugolAST* PortugolAST::getFirstChild() const {
    if (!_tree) return nullptr;
    ANTLR3_UINT32 count = _tree->getChildCount(_tree);
    if (count == 0) return nullptr;
    return create((pANTLR3_BASE_TREE)_tree->getChild(_tree, 0));
}

PortugolAST* PortugolAST::getNextSibling() const {
    if (!_tree) return nullptr;
    pANTLR3_BASE_TREE parent = (pANTLR3_BASE_TREE)_tree->getParent(_tree);
    if (!parent) return nullptr;
    
    ANTLR3_UINT32 idx = _tree->getChildIndex(_tree);
    ANTLR3_UINT32 count = parent->getChildCount(parent);
    if (idx + 1 >= count) return nullptr;
    
    return create((pANTLR3_BASE_TREE)parent->getChild(parent, idx + 1));
}

ANTLR3_UINT32 PortugolAST::getChildCount() const {
    if (!_tree) return 0;
    return _tree->getChildCount(_tree);
}

PortugolAST* PortugolAST::getChild(ANTLR3_UINT32 i) const {
    if (!_tree) return nullptr;
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)_tree->getChild(_tree, i);
    if (!child) return nullptr;
    return create(child);
}

PortugolAST* PortugolAST::getParent() const {
    if (!_tree) return nullptr;
    pANTLR3_BASE_TREE parent = (pANTLR3_BASE_TREE)_tree->getParent(_tree);
    if (!parent) return nullptr;
    return create(parent);
}

void PortugolAST::setEvalType(int type) {
    _evalType = type;
}

int PortugolAST::getEvalType() const {
    return _evalType;
}

void PortugolAST::setEndLine(int line) {
    _endLine = line;
}

void PortugolAST::setFilename(const std::string& filename) {
    _filename = filename;
}

PortugolAST* PortugolAST::create(pANTLR3_BASE_TREE tree) {
    if (!tree) return nullptr;
    
    // Check cache first
    auto it = astCache.find(tree);
    if (it != astCache.end()) {
        return it->second;
    }
    
    // Create new and cache it
    PortugolAST* ast = new PortugolAST(tree);
    astCache[tree] = ast;
    return ast;
}

void PortugolAST::release(PortugolAST* ast) {
    // This could be used for cleanup if needed
    // For now, we let the cache manage the memory
}
