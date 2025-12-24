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

#include "Interpreter.hpp"
#include "SemanticWalkerTokenTypes.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;

Interpreter::Interpreter(SymbolTable& st, const std::string& host, int port,
                         const std::string& sourceContent)
    : interpreter(st, host, port), _returning(false), topnode(nullptr),
      _sourceContent(sourceContent) {
    buildLineOffsets();
}

void Interpreter::buildLineOffsets() {
    _lineOffsets.clear();
    _lineOffsets.push_back(0);  // Line 1 starts at offset 0
    
    for (size_t i = 0; i < _sourceContent.size(); i++) {
        if (_sourceContent[i] == '\n') {
            _lineOffsets.push_back(i + 1);  // Next line starts after \n
        }
    }
}

std::string Interpreter::getNodeText(pANTLR3_BASE_TREE node) {
    if (!node) return "";
    
    std::string result;
    
    // First, try node's getText - works for imaginary tokens like TI_FCALL
    pANTLR3_STRING str = node->getText(node);
    if (str && str->chars) {
        result = std::string((const char*)str->chars);
        // Check for null bytes that indicate corruption
        if (!result.empty() && result.find('\0') == std::string::npos) {
            return result;  // No corruption, use this
        }
        result.clear();  // Corrupted or empty, try fallback
    }
    
    // Fallback: extract from original source using line/column
    // This avoids the ANTLR3 C UTF-8 getText bug
    pANTLR3_COMMON_TOKEN token = node->getToken(node);
    if (token && !_sourceContent.empty()) {
        ANTLR3_UINT32 line = token->line;
        ANTLR3_UINT32 charPos = token->charPosition;
        
        if (line > 0 && line <= _lineOffsets.size()) {
            size_t lineStart = _lineOffsets[line - 1];  // Lines are 1-based
            size_t pos = lineStart;
            
            // charPosition is in Unicode code points, need to convert to bytes
            // Walk through the line counting UTF-8 characters
            size_t charCount = 0;
            while (pos < _sourceContent.size() && charCount < charPos) {
                unsigned char c = _sourceContent[pos];
                if (c < 0x80) {
                    pos++;  // ASCII
                } else if ((c & 0xE0) == 0xC0) {
                    pos += 2;  // 2-byte UTF-8
                } else if ((c & 0xF0) == 0xE0) {
                    pos += 3;  // 3-byte UTF-8
                } else if ((c & 0xF8) == 0xF0) {
                    pos += 4;  // 4-byte UTF-8
                } else {
                    pos++;  // Invalid, skip
                }
                charCount++;
            }
            
            // Now extract the identifier (until whitespace or special char)
            size_t start = pos;
            while (pos < _sourceContent.size()) {
                unsigned char c = _sourceContent[pos];
                // Check if it's an identifier character
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c >= 0x80) {
                    // UTF-8 continuation or valid char
                    if (c < 0x80) {
                        pos++;
                    } else if ((c & 0xE0) == 0xC0) {
                        pos += 2;
                    } else if ((c & 0xF0) == 0xE0) {
                        pos += 3;
                    } else if ((c & 0xF8) == 0xF0) {
                        pos += 4;
                    } else {
                        pos++;
                    }
                } else {
                    break;  // End of identifier
                }
            }
            
            if (pos > start) {
                result = _sourceContent.substr(start, pos - start);
                return result;
            }
        }
    }
    
    // Last fallback: use token getText and skip leading nulls
    if (token) {
        str = token->getText(token);
        if (str && str->chars && str->len > 0) {
            const unsigned char* chars = (const unsigned char*)str->chars;
            ANTLR3_UINT32 len = str->len;
            
            while (len > 0 && chars[0] == 0) {
                chars++;
                len--;
            }
            
            if (len > 0) {
                result = std::string((const char*)chars, len);
            }
        }
    }
    
    return result;
}

int Interpreter::getNodeType(pANTLR3_BASE_TREE node) {
    if (!node) return 0;
    return node->getType(node);
}

pANTLR3_BASE_TREE Interpreter::getChild(pANTLR3_BASE_TREE node, int i) {
    if (!node) return nullptr;
    return (pANTLR3_BASE_TREE)node->getChild(node, i);
}

int Interpreter::getChildCount(pANTLR3_BASE_TREE node) {
    if (!node) return 0;
    return node->getChildCount(node);
}

std::string Interpreter::parseLiteral(const std::string& str) {
    std::string result = str;
    // Remove quotes
    if (result.length() >= 2 && result[0] == '"' && result[result.length()-1] == '"') {
        result = result.substr(1, result.length() - 2);
    }
    
    std::string::size_type idx = 0;
    char c;
    while ((idx = result.find('\\', idx)) != std::string::npos) {
        if (idx + 1 >= result.length()) break;
        switch (result[idx+1]) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            case '"': c = '"'; break;
            default: c = result[idx+1];
        }
        result.replace(idx, 2, 1, c);
        idx++;
    }
    return result;
}

std::string Interpreter::parseChar(const std::string& str) {
    std::stringstream ret;
    std::string s = str;
    // Remove quotes
    if (s.length() >= 2 && s[0] == '\'' && s[s.length()-1] == '\'') {
        s = s.substr(1, s.length() - 2);
    }
    
    if (s.empty()) {
        ret << 0;
        return ret.str();
    }
    
    if (s[0] == '\\' && s.length() > 1) {
        switch (s[1]) {
            case 't': ret << (int)'\t'; break;
            case 'n': ret << (int)'\n'; break;
            case 'r': ret << (int)'\r'; break;
            default: ret << (int)s[1];
        }
        return ret.str();
    } else {
        ret << (int)s[0];
        return ret.str();
    }
}

pANTLR3_BASE_TREE Interpreter::getFunctionNode(const std::string& name) {
    pANTLR3_BASE_TREE node = topnode;
    // Find function declaration with matching name
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        if (getNodeText(child) == name) {
            return child;
        }
    }
    // Also check siblings at top level
    // The AST structure might have functions as siblings
    return nullptr;
}

int Interpreter::run(pANTLR3_BASE_TREE tree) {
    return algoritmo(tree);
}

int Interpreter::algoritmo(pANTLR3_BASE_TREE node) {
    int ret = 0;
    topnode = node;
    
    // Initialize interpreter
    interpreter.init("");

    // AST structure: ^(T_KW_ALGORITMO T_IDENTIFICADOR) variaveis? inicio func_decls*
    int count = getChildCount(node);
    pANTLR3_BASE_TREE inicioNode = nullptr;
    
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::T_KW_ALGORITMO) {
            // Skip algorithm declaration
            continue;
        } else if (type == SemanticWalkerTokenTypes::T_KW_VARIAVEIS) {
            variaveis(child);
        } else if (type == SemanticWalkerTokenTypes::T_KW_INICIO) {
            inicioNode = child;
        }
    }
    
    // Execute main block
    if (inicioNode) {
        inicio(inicioNode);
    }
    
    if (_returning) {
        ret = interpreter.getReturning();
    }
    
    return ret;
}

void Interpreter::variaveis(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::TI_VAR_PRIMITIVE) {
            primitivo(child);
        } else if (type == SemanticWalkerTokenTypes::TI_VAR_MATRIX) {
            matriz(child);
        }
    }
}

void Interpreter::primitivo(pANTLR3_BASE_TREE node) {
    // Primitive variable declaration - already handled by symbol table
}

void Interpreter::matriz(pANTLR3_BASE_TREE node) {
    // Matrix declaration - already handled by symbol table
}

void Interpreter::inicio(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        if (_returning) break;
        pANTLR3_BASE_TREE child = getChild(node, i);
        stm(child);
    }
}

void Interpreter::stm(pANTLR3_BASE_TREE node) {
    if (!node || _returning) return;
    
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::T_ATTR:
            stm_attr(node);
            break;
        case SemanticWalkerTokenTypes::TI_FCALL:
            fcall(node);  // Discard return value
            break;
        case SemanticWalkerTokenTypes::T_KW_RETORNE:
            stm_ret(node);
            break;
        case SemanticWalkerTokenTypes::T_KW_SE:
            stm_se(node);
            break;
        case SemanticWalkerTokenTypes::T_KW_ENQUANTO:
            stm_enquanto(node);
            break;
        case SemanticWalkerTokenTypes::T_KW_REPITA:
            stm_repita(node);
            break;
        case SemanticWalkerTokenTypes::T_KW_PARA:
            stm_para(node);
            break;
        default:
            // Unknown statement type
            break;
    }
}

void Interpreter::stm_attr(pANTLR3_BASE_TREE node) {
    // ^(T_ATTR lvalue expr)
    if (getChildCount(node) < 2) return;
    
    pANTLR3_BASE_TREE lvalNode = getChild(node, 0);
    pANTLR3_BASE_TREE exprNode = getChild(node, 1);
    
    LValue l = lvalueRef(lvalNode);
    ExprValue v = expr(exprNode);
    
    interpreter.execAttribution(l, v);
}

LValue Interpreter::lvalueRef(pANTLR3_BASE_TREE node) {
    LValue l;
    l.name = getNodeText(node);
    
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        ExprValue e = expr(child);
        l.addMatrixIndex(e);
    }
    
    return l;
}

ExprValue Interpreter::lvalue(pANTLR3_BASE_TREE node, bool getref) {
    LValue l = lvalueRef(node);
    return interpreter.getLValueValue(l);
}

void Interpreter::stm_se(pANTLR3_BASE_TREE node) {
    // ^(T_KW_SE expr stm_list senao_part?)
    int count = getChildCount(node);
    if (count < 2) return;
    
    pANTLR3_BASE_TREE condNode = getChild(node, 0);
    ExprValue cond = expr(condNode);
    bool exec = cond.ifTrue();
    
    // Find then and else parts
    for (int i = 1; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::T_KW_SENAO) {
            // Else part
            if (!exec) {
                int elseCount = getChildCount(child);
                for (int j = 0; j < elseCount; j++) {
                    if (_returning) break;
                    stm(getChild(child, j));
                }
            }
        } else if (exec) {
            // Then part
            stm(child);
        }
    }
}

void Interpreter::stm_enquanto(pANTLR3_BASE_TREE node) {
    // ^(T_KW_ENQUANTO expr stm*)
    int count = getChildCount(node);
    if (count < 1) return;
    
    pANTLR3_BASE_TREE condNode = getChild(node, 0);
    
    while (!_returning && expr(condNode).ifTrue()) {
        for (int i = 1; i < count; i++) {
            if (_returning) break;
            stm(getChild(node, i));
        }
    }
}

void Interpreter::stm_repita(pANTLR3_BASE_TREE node) {
    // ^(T_KW_REPITA stm_list expr)
    int count = getChildCount(node);
    if (count < 1) return;
    
    pANTLR3_BASE_TREE condNode = getChild(node, count - 1);
    
    do {
        for (int i = 0; i < count - 1; i++) {
            if (_returning) break;
            stm(getChild(node, i));
        }
    } while (!_returning && !expr(condNode).ifTrue());
}

void Interpreter::stm_para(pANTLR3_BASE_TREE node) {
    // ^(T_KW_PARA lvalue expr expr passo? stm*)
    int count = getChildCount(node);
    if (count < 3) return;
    
    pANTLR3_BASE_TREE lvalNode = getChild(node, 0);
    pANTLR3_BASE_TREE deNode = getChild(node, 1);
    pANTLR3_BASE_TREE ateNode = getChild(node, 2);
    
    LValue lv = lvalueRef(lvalNode);
    ExprValue de = expr(deNode);
    interpreter.execAttribution(lv, de);
    
    int ps = 1;
    int stmStart = 3;
    
    // Check for passo
    if (count > 3) {
        pANTLR3_BASE_TREE maybePassoNode = getChild(node, 3);
        if (getNodeType(maybePassoNode) == SemanticWalkerTokenTypes::T_KW_PASSO) {
            // Parse passo
            int passoCount = getChildCount(maybePassoNode);
            bool neg = false;
            for (int i = 0; i < passoCount; i++) {
                pANTLR3_BASE_TREE passoChild = getChild(maybePassoNode, i);
                int passoType = getNodeType(passoChild);
                if (passoType == SemanticWalkerTokenTypes::T_MENOS) {
                    neg = true;
                } else if (passoType == SemanticWalkerTokenTypes::T_INT_LIT) {
                    ps = atoi(getNodeText(passoChild).c_str());
                    if (neg) ps = -ps;
                }
            }
            stmStart = 4;
        }
    }
    
    while (!_returning) {
        ExprValue ate = expr(ateNode);
        if (ps > 0) {
            if (!interpreter.execLowerEq(lv, ate)) break;
        } else {
            if (!interpreter.execBiggerEq(lv, ate)) break;
        }
        
        for (int i = stmStart; i < count; i++) {
            if (_returning) break;
            stm(getChild(node, i));
        }
        
        interpreter.execPasso(lv, ps);
    }
    
    // Set final value
    ExprValue ate = expr(ateNode);
    interpreter.execAttribution(lv, ate);
}

void Interpreter::stm_ret(pANTLR3_BASE_TREE node) {
    // ^(T_KW_RETORNE (TI_NULL|expr))
    ExprValue eval;
    
    if (getChildCount(node) > 0) {
        pANTLR3_BASE_TREE child = getChild(node, 0);
        if (getNodeType(child) != SemanticWalkerTokenTypes::TI_NULL) {
            eval = expr(child);
        }
    }
    
    interpreter.setReturnExprValue(eval);
    _returning = true;
}

ExprValue Interpreter::fcall(pANTLR3_BASE_TREE node) {
    // ^(TI_FCALL T_IDENTIFICADOR expr*)
    ExprValue v;
    
    if (getChildCount(node) < 1) return v;
    
    pANTLR3_BASE_TREE idNode = getChild(node, 0);
    std::string fname = getNodeText(idNode);
    
    std::list<ExprValue> args;
    int count = getChildCount(node);
    for (int i = 1; i < count; i++) {
        args.push_back(expr(getChild(node, i)));
    }
    
    if (interpreter.isBuiltInFunction(fname)) {
        v = interpreter.execBuiltInFunction(fname, args);
    } else {
        // User-defined function
        pANTLR3_BASE_TREE fnode = getFunctionNode(fname);
        if (fnode) {
            func_decl(fnode, args, 0);
            _returning = false;
            v = interpreter.getReturnExprValue(fname);
        }
    }
    
    return v;
}

ExprValue Interpreter::expr(pANTLR3_BASE_TREE node) {
    ExprValue v;
    if (!node) return v;
    
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::T_KW_OU: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateOu(left, right);
        }
        case SemanticWalkerTokenTypes::T_KW_E: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateE(left, right);
        }
        case SemanticWalkerTokenTypes::T_BIT_OU: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateBitOu(left, right);
        }
        case SemanticWalkerTokenTypes::T_BIT_XOU: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateBitXou(left, right);
        }
        case SemanticWalkerTokenTypes::T_BIT_E: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateBitE(left, right);
        }
        case SemanticWalkerTokenTypes::T_IGUAL: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateIgual(left, right);
        }
        case SemanticWalkerTokenTypes::T_DIFERENTE: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateDif(left, right);
        }
        case SemanticWalkerTokenTypes::T_MAIOR: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMaior(left, right);
        }
        case SemanticWalkerTokenTypes::T_MENOR: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMenor(left, right);
        }
        case SemanticWalkerTokenTypes::T_MAIOR_EQ: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMaiorEq(left, right);
        }
        case SemanticWalkerTokenTypes::T_MENOR_EQ: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMenorEq(left, right);
        }
        case SemanticWalkerTokenTypes::T_MAIS: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMais(left, right);
        }
        case SemanticWalkerTokenTypes::T_MENOS: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMenos(left, right);
        }
        case SemanticWalkerTokenTypes::T_DIV: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateDiv(left, right);
        }
        case SemanticWalkerTokenTypes::T_MULTIP: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMultip(left, right);
        }
        case SemanticWalkerTokenTypes::T_MOD: {
            ExprValue left = expr(getChild(node, 0));
            ExprValue right = expr(getChild(node, 1));
            return interpreter.evaluateMod(left, right);
        }
        case SemanticWalkerTokenTypes::TI_UN_NEG: {
            ExprValue right = element(getChild(node, 0));
            return interpreter.evaluateUnNeg(right);
        }
        case SemanticWalkerTokenTypes::TI_UN_POS: {
            ExprValue right = element(getChild(node, 0));
            return interpreter.evaluateUnPos(right);
        }
        case SemanticWalkerTokenTypes::TI_UN_NOT: {
            ExprValue right = element(getChild(node, 0));
            return interpreter.evaluateUnNot(right);
        }
        case SemanticWalkerTokenTypes::TI_UN_BNOT: {
            ExprValue right = element(getChild(node, 0));
            return interpreter.evaluateUnBNot(right);
        }
        default:
            return element(node);
    }
}

ExprValue Interpreter::element(pANTLR3_BASE_TREE node) {
    if (!node) return ExprValue();
    
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::TI_FCALL:
            return fcall(node);
        case SemanticWalkerTokenTypes::TI_PARENTHESIS:
            return expr(getChild(node, 0));
        case SemanticWalkerTokenTypes::T_IDENTIFICADOR:
            return lvalue(node);
        case SemanticWalkerTokenTypes::T_STRING_LIT:
        case SemanticWalkerTokenTypes::T_INT_LIT:
        case SemanticWalkerTokenTypes::T_REAL_LIT:
        case SemanticWalkerTokenTypes::T_CARAC_LIT:
        case SemanticWalkerTokenTypes::T_KW_VERDADEIRO:
        case SemanticWalkerTokenTypes::T_KW_FALSO:
            return literal(node);
        default:
            return ExprValue();
    }
}

ExprValue Interpreter::literal(pANTLR3_BASE_TREE node) {
    ExprValue v;
    int type = getNodeType(node);
    std::string text = getNodeText(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::T_STRING_LIT:
            v.setValue(parseLiteral(text));
            v.type = TIPO_LITERAL;
            break;
        case SemanticWalkerTokenTypes::T_INT_LIT:
            v.setValue(text);
            v.type = TIPO_INTEIRO;
            break;
        case SemanticWalkerTokenTypes::T_REAL_LIT:
            v.setValue(text);
            v.type = TIPO_REAL;
            break;
        case SemanticWalkerTokenTypes::T_CARAC_LIT:
            v.setValue(parseChar(text));
            v.type = TIPO_CARACTERE;
            break;
        case SemanticWalkerTokenTypes::T_KW_VERDADEIRO:
            v.setValue("1");
            v.type = TIPO_LOGICO;
            break;
        case SemanticWalkerTokenTypes::T_KW_FALSO:
            v.setValue("0");
            v.type = TIPO_LOGICO;
            break;
    }
    
    return v;
}

void Interpreter::func_decl(pANTLR3_BASE_TREE node, std::list<ExprValue>& args, int line) {
    std::string fname = getNodeText(node);
    interpreter.beginFunctionCall("", fname, args, line);
    
    // Find inicio node
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        if (getNodeType(child) == SemanticWalkerTokenTypes::T_KW_INICIO) {
            inicio(child);
            break;
        }
    }
    
    interpreter.endFunctionCall();
}

ExprValue Interpreter::execFunc(const std::string& fname, std::list<ExprValue>& args) {
    ExprValue v;
    if (interpreter.isBuiltInFunction(fname)) {
        v = interpreter.execBuiltInFunction(fname, args);
    }
    return v;
}

