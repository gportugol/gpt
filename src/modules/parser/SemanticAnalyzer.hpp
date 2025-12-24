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

#ifndef SEMANTICANALYZER_HPP
#define SEMANTICANALYZER_HPP

#include <antlr3.h>
#include "SymbolTable.hpp"
#include "SemanticEval.hpp"
#include <string>
#include <list>
#include <vector>

class SemanticAnalyzer {
public:
    SemanticAnalyzer(SymbolTable& st, const std::string& sourceContent = "");
    
    void analyze(pANTLR3_BASE_TREE tree);
    
private:
    SemanticEval evaluator;
    pANTLR3_BASE_TREE topnode;
    
    // Original source content for text extraction (ANTLR3 UTF-8 workaround)
    std::string _sourceContent;
    std::vector<size_t> _lineOffsets;
    
    void buildLineOffsets();
    
    // Helper functions
    std::string getNodeText(pANTLR3_BASE_TREE node);
    int getNodeType(pANTLR3_BASE_TREE node);
    pANTLR3_BASE_TREE getChild(pANTLR3_BASE_TREE node, int i);
    int getChildCount(pANTLR3_BASE_TREE node);
    int getLine(pANTLR3_BASE_TREE node);
    
    // AST walkers
    void algoritmo(pANTLR3_BASE_TREE node);
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
    void fcall(pANTLR3_BASE_TREE node);
    ExpressionValue expr(pANTLR3_BASE_TREE node);
    ExpressionValue element(pANTLR3_BASE_TREE node);
    void lvalue(pANTLR3_BASE_TREE node);
    ExpressionValue literal(pANTLR3_BASE_TREE node);
    void func_decl(pANTLR3_BASE_TREE node);
    void func_proto(pANTLR3_BASE_TREE node);
};

#endif // SEMANTICANALYZER_HPP

