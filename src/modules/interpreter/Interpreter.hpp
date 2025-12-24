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

#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <antlr3.h>
#include "SymbolTable.hpp"
#include "InterpreterEval.hpp"
#include <string>
#include <list>
#include <vector>

class Interpreter {
public:
    class ReturnException {};
    
    Interpreter(SymbolTable& st, const std::string& host = "", int port = 0,
                const std::string& sourceContent = "");
    
    int run(pANTLR3_BASE_TREE tree);
    
private:
    InterpreterEval interpreter;
    bool _returning;
    pANTLR3_BASE_TREE topnode;
    
    // Original source content for text extraction (ANTLR3 UTF-8 workaround)
    std::string _sourceContent;
    std::vector<size_t> _lineOffsets;  // Offset of each line start
    
    void buildLineOffsets();
    
    // Helper functions
    std::string getNodeText(pANTLR3_BASE_TREE node);
    int getNodeType(pANTLR3_BASE_TREE node);
    pANTLR3_BASE_TREE getChild(pANTLR3_BASE_TREE node, int i);
    int getChildCount(pANTLR3_BASE_TREE node);
    pANTLR3_BASE_TREE getFunctionNode(const std::string& name);
    
    std::string parseLiteral(const std::string& str);
    std::string parseChar(const std::string& str);
    
    // AST walkers
    int algoritmo(pANTLR3_BASE_TREE node);
    void variaveis(pANTLR3_BASE_TREE node);
    void primitivo(pANTLR3_BASE_TREE node);
    void matriz(pANTLR3_BASE_TREE node);
    void inicio(pANTLR3_BASE_TREE node);
    void stm(pANTLR3_BASE_TREE node);
    void stm_attr(pANTLR3_BASE_TREE node);
    void stm_se(pANTLR3_BASE_TREE node);
    void stm_enquanto(pANTLR3_BASE_TREE node);
    void stm_repita(pANTLR3_BASE_TREE node);
    void stm_para(pANTLR3_BASE_TREE node);
    void stm_ret(pANTLR3_BASE_TREE node);
    ExprValue fcall(pANTLR3_BASE_TREE node);
    ExprValue expr(pANTLR3_BASE_TREE node);
    ExprValue element(pANTLR3_BASE_TREE node);
    ExprValue lvalue(pANTLR3_BASE_TREE node, bool getref = false);
    LValue lvalueRef(pANTLR3_BASE_TREE node);
    ExprValue literal(pANTLR3_BASE_TREE node);
    void func_decl(pANTLR3_BASE_TREE node, std::list<ExprValue>& args, int line);
    ExprValue execFunc(const std::string& fname, std::list<ExprValue>& args);
};

#endif // INTERPRETER_HPP
