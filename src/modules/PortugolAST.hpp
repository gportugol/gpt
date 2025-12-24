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

#ifndef PORTUGOLAST_HPP
#define PORTUGOLAST_HPP

#include <antlr3.h>
#include <string>
#include <memory>

/**
 * Custom AST node class for Portugol
 * Adapted for ANTLR3 C runtime
 */
class PortugolAST {
public:
    PortugolAST();
    PortugolAST(pANTLR3_BASE_TREE tree);
    PortugolAST(const PortugolAST& other);
    ~PortugolAST();

    PortugolAST& operator=(const PortugolAST& other);

    // Null check
    bool isNull() const;
    operator bool() const { return !isNull(); }

    // Token info
    std::string getText() const;
    int getType() const;
    int getLine() const;
    int getEndLine() const;
    std::string getFilename() const;

    // Tree navigation
    PortugolAST* getFirstChild() const;
    PortugolAST* getNextSibling() const;
    ANTLR3_UINT32 getChildCount() const;
    PortugolAST* getChild(ANTLR3_UINT32 i) const;
    PortugolAST* getParent() const;

    // Custom properties
    void setEvalType(int type);
    int getEvalType() const;
    void setEndLine(int line);
    void setFilename(const std::string& filename);

    // Internal access
    pANTLR3_BASE_TREE getTree() const { return _tree; }

    // Comparison with null
    bool operator==(std::nullptr_t) const { return isNull(); }
    bool operator!=(std::nullptr_t) const { return !isNull(); }

    // Static factory methods
    static PortugolAST* create(pANTLR3_BASE_TREE tree);
    static void release(PortugolAST* ast);

private:
    pANTLR3_BASE_TREE _tree;
    int _evalType;
    int _endLine;
    std::string _filename;
};

// Type alias for compatibility with old code
typedef PortugolAST* RefPortugolAST;

// Null AST for comparisons
namespace antlr {
    extern PortugolAST* nullAST;
}

#endif // PORTUGOLAST_HPP
