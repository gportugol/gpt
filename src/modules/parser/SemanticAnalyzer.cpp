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

#include "SemanticAnalyzer.hpp"
#include "SemanticWalkerTokenTypes.hpp"
#include "GPTDisplay.hpp"
#include <sstream>

using namespace std;

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& st, const std::string& sourceContent)
    : evaluator(st), topnode(nullptr), _sourceContent(sourceContent) {
    buildLineOffsets();
}

void SemanticAnalyzer::buildLineOffsets() {
    _lineOffsets.clear();
    _lineOffsets.push_back(0);  // Line 1 starts at offset 0
    
    for (size_t i = 0; i < _sourceContent.size(); i++) {
        if (_sourceContent[i] == '\n') {
            _lineOffsets.push_back(i + 1);  // Next line starts after \n
        }
    }
}

std::string SemanticAnalyzer::getNodeText(pANTLR3_BASE_TREE node) {
    if (!node) return "";
    
    std::string result;
    
    // First, try node's getText - works for imaginary tokens
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

int SemanticAnalyzer::getNodeType(pANTLR3_BASE_TREE node) {
    if (!node) return 0;
    return node->getType(node);
}

pANTLR3_BASE_TREE SemanticAnalyzer::getChild(pANTLR3_BASE_TREE node, int i) {
    if (!node) return nullptr;
    return (pANTLR3_BASE_TREE)node->getChild(node, i);
}

int SemanticAnalyzer::getChildCount(pANTLR3_BASE_TREE node) {
    if (!node) return 0;
    return node->getChildCount(node);
}

int SemanticAnalyzer::getLine(pANTLR3_BASE_TREE node) {
    if (!node) return 0;
    pANTLR3_COMMON_TOKEN token = node->getToken(node);
    if (!token) return 0;
    return token->getLine(token);
}

void SemanticAnalyzer::analyze(pANTLR3_BASE_TREE tree) {
    topnode = tree;
    evaluator.setCurrentScope(SymbolTable::GlobalScope);
    
    // First pass: register functions (prototypes)
    int count = getChildCount(tree);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(tree, i);
        int type = getNodeType(child);
        // Check if it's a function declaration (identifier with children that include inicio)
        if (type == SemanticWalkerTokenTypes::T_IDENTIFICADOR) {
            // It might be a function
            int childCount = getChildCount(child);
            for (int j = 0; j < childCount; j++) {
                if (getNodeType(getChild(child, j)) == SemanticWalkerTokenTypes::T_KW_INICIO) {
                    func_proto(child);
                    break;
                }
            }
        }
    }
    
    // Second pass: analyze algorithm
    algoritmo(tree);
}

void SemanticAnalyzer::algoritmo(pANTLR3_BASE_TREE node) {
    evaluator.setCurrentScope(SymbolTable::GlobalScope);
    
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::T_KW_ALGORITMO) {
            // Algorithm declaration - skip
            continue;
        } else if (type == SemanticWalkerTokenTypes::T_KW_VARIAVEIS) {
            variaveis(child);
        } else if (type == SemanticWalkerTokenTypes::T_KW_INICIO) {
            inicio(child);
        } else if (type == SemanticWalkerTokenTypes::T_IDENTIFICADOR) {
            // Function declaration
            func_decl(child);
        }
    }
    
    evaluator.setCurrentScope(SymbolTable::GlobalScope);
}

void SemanticAnalyzer::variaveis(pANTLR3_BASE_TREE node) {
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

void SemanticAnalyzer::primitivo(pANTLR3_BASE_TREE node) {
    // Structure: ^(TI_VAR_PRIMITIVE (^(TI_VAR_PRIMITIVE tipo))? tipo? T_IDENTIFICADOR+)
    int count = getChildCount(node);
    int tipoToken = 0;
    std::list<std::string> ids;
    
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::TI_VAR_PRIMITIVE) {
            // Nested - get type from first child
            if (getChildCount(child) > 0) {
                tipoToken = getNodeType(getChild(child, 0));
            }
        } else if (type == SemanticWalkerTokenTypes::T_KW_INTEIRO ||
                   type == SemanticWalkerTokenTypes::T_KW_REAL ||
                   type == SemanticWalkerTokenTypes::T_KW_CARACTERE ||
                   type == SemanticWalkerTokenTypes::T_KW_LITERAL ||
                   type == SemanticWalkerTokenTypes::T_KW_LOGICO) {
            tipoToken = type;
        } else if (type == SemanticWalkerTokenTypes::T_IDENTIFICADOR) {
            ids.push_back(getNodeText(child));
        }
    }
    
    // Convert token type to TIPO_*
    int primitiveType = TIPO_NULO;
    switch (tipoToken) {
        case SemanticWalkerTokenTypes::T_KW_INTEIRO: primitiveType = TIPO_INTEIRO; break;
        case SemanticWalkerTokenTypes::T_KW_REAL: primitiveType = TIPO_REAL; break;
        case SemanticWalkerTokenTypes::T_KW_CARACTERE: primitiveType = TIPO_CARACTERE; break;
        case SemanticWalkerTokenTypes::T_KW_LITERAL: primitiveType = TIPO_LITERAL; break;
        case SemanticWalkerTokenTypes::T_KW_LOGICO: primitiveType = TIPO_LOGICO; break;
    }
    
    // Register variables
    for (const std::string& id : ids) {
        evaluator.declareVar(id, primitiveType, getLine(node));
    }
}

void SemanticAnalyzer::matriz(pANTLR3_BASE_TREE node) {
    // Structure: ^(TI_VAR_MATRIX tipo T_IDENTIFICADOR+)
    int count = getChildCount(node);
    int tipoToken = 0;
    std::list<int> dimensions;
    std::list<std::string> ids;
    
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::TI_VAR_MATRIX) {
            // Nested - process recursively
            int nestedCount = getChildCount(child);
            for (int j = 0; j < nestedCount; j++) {
                pANTLR3_BASE_TREE nestedChild = getChild(child, j);
                int nestedType = getNodeType(nestedChild);
                if (nestedType == SemanticWalkerTokenTypes::T_KW_INTEIROS ||
                    nestedType == SemanticWalkerTokenTypes::T_KW_REAIS ||
                    nestedType == SemanticWalkerTokenTypes::T_KW_CARACTERES ||
                    nestedType == SemanticWalkerTokenTypes::T_KW_LITERAIS ||
                    nestedType == SemanticWalkerTokenTypes::T_KW_LOGICOS) {
                    tipoToken = nestedType;
                    // Get dimensions from children
                    int dimCount = getChildCount(nestedChild);
                    for (int k = 0; k < dimCount; k++) {
                        pANTLR3_BASE_TREE dimChild = getChild(nestedChild, k);
                        if (getNodeType(dimChild) == SemanticWalkerTokenTypes::T_INT_LIT) {
                            dimensions.push_back(atoi(getNodeText(dimChild).c_str()));
                        }
                    }
                }
            }
        } else if (type == SemanticWalkerTokenTypes::T_KW_INTEIROS ||
                   type == SemanticWalkerTokenTypes::T_KW_REAIS ||
                   type == SemanticWalkerTokenTypes::T_KW_CARACTERES ||
                   type == SemanticWalkerTokenTypes::T_KW_LITERAIS ||
                   type == SemanticWalkerTokenTypes::T_KW_LOGICOS) {
            tipoToken = type;
            // Get dimensions
            int dimCount = getChildCount(child);
            for (int j = 0; j < dimCount; j++) {
                pANTLR3_BASE_TREE dimChild = getChild(child, j);
                if (getNodeType(dimChild) == SemanticWalkerTokenTypes::T_INT_LIT) {
                    dimensions.push_back(atoi(getNodeText(dimChild).c_str()));
                }
            }
        } else if (type == SemanticWalkerTokenTypes::T_IDENTIFICADOR) {
            ids.push_back(getNodeText(child));
        }
    }
    
    // Convert token type to TIPO_*
    int primitiveType = TIPO_NULO;
    switch (tipoToken) {
        case SemanticWalkerTokenTypes::T_KW_INTEIROS: primitiveType = TIPO_INTEIRO; break;
        case SemanticWalkerTokenTypes::T_KW_REAIS: primitiveType = TIPO_REAL; break;
        case SemanticWalkerTokenTypes::T_KW_CARACTERES: primitiveType = TIPO_CARACTERE; break;
        case SemanticWalkerTokenTypes::T_KW_LITERAIS: primitiveType = TIPO_LITERAL; break;
        case SemanticWalkerTokenTypes::T_KW_LOGICOS: primitiveType = TIPO_LOGICO; break;
    }
    
    // Register variables
    for (const std::string& id : ids) {
        evaluator.declareMatrix(id, primitiveType, dimensions, getLine(node));
    }
}

void SemanticAnalyzer::inicio(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        stm(getChild(node, i));
    }
}

void SemanticAnalyzer::stm(pANTLR3_BASE_TREE node) {
    if (!node) return;
    
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::T_ATTR:
            stm_attr(node);
            break;
        case SemanticWalkerTokenTypes::TI_FCALL:
            fcall(node);
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
    }
}

void SemanticAnalyzer::stm_attr(pANTLR3_BASE_TREE node) {
    // Semantic analysis of attribution
    if (getChildCount(node) >= 2) {
        lvalue(getChild(node, 0));
        expr(getChild(node, 1));
    }
}

void SemanticAnalyzer::stm_se(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    if (count < 1) return;
    
    expr(getChild(node, 0));
    
    for (int i = 1; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        if (type == SemanticWalkerTokenTypes::T_KW_SENAO) {
            int elseCount = getChildCount(child);
            for (int j = 0; j < elseCount; j++) {
                stm(getChild(child, j));
            }
        } else {
            stm(child);
        }
    }
}

void SemanticAnalyzer::stm_enquanto(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    if (count < 1) return;
    
    expr(getChild(node, 0));
    
    for (int i = 1; i < count; i++) {
        stm(getChild(node, i));
    }
}

void SemanticAnalyzer::stm_repita(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    if (count < 1) return;
    
    for (int i = 0; i < count - 1; i++) {
        stm(getChild(node, i));
    }
    
    expr(getChild(node, count - 1));
}

void SemanticAnalyzer::stm_para(pANTLR3_BASE_TREE node) {
    int count = getChildCount(node);
    if (count < 3) return;
    
    lvalue(getChild(node, 0));
    expr(getChild(node, 1));
    expr(getChild(node, 2));
    
    for (int i = 3; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        if (getNodeType(child) != SemanticWalkerTokenTypes::T_KW_PASSO) {
            stm(child);
        }
    }
}

void SemanticAnalyzer::stm_ret(pANTLR3_BASE_TREE node) {
    if (getChildCount(node) > 0) {
        pANTLR3_BASE_TREE child = getChild(node, 0);
        if (getNodeType(child) != SemanticWalkerTokenTypes::TI_NULL) {
            expr(child);
        }
    }
}

void SemanticAnalyzer::fcall(pANTLR3_BASE_TREE node) {
    if (getChildCount(node) < 1) return;
    
    std::string fname = getNodeText(getChild(node, 0));
    
    int count = getChildCount(node);
    for (int i = 1; i < count; i++) {
        expr(getChild(node, i));
    }
}

ExpressionValue SemanticAnalyzer::expr(pANTLR3_BASE_TREE node) {
    ExpressionValue v;
    if (!node) return v;
    
    int type = getNodeType(node);
    
    // Binary operations
    if (getChildCount(node) == 2) {
        switch (type) {
            case SemanticWalkerTokenTypes::T_KW_OU:
            case SemanticWalkerTokenTypes::T_KW_E:
            case SemanticWalkerTokenTypes::T_BIT_OU:
            case SemanticWalkerTokenTypes::T_BIT_XOU:
            case SemanticWalkerTokenTypes::T_BIT_E:
            case SemanticWalkerTokenTypes::T_IGUAL:
            case SemanticWalkerTokenTypes::T_DIFERENTE:
            case SemanticWalkerTokenTypes::T_MAIOR:
            case SemanticWalkerTokenTypes::T_MENOR:
            case SemanticWalkerTokenTypes::T_MAIOR_EQ:
            case SemanticWalkerTokenTypes::T_MENOR_EQ:
            case SemanticWalkerTokenTypes::T_MAIS:
            case SemanticWalkerTokenTypes::T_MENOS:
            case SemanticWalkerTokenTypes::T_DIV:
            case SemanticWalkerTokenTypes::T_MULTIP:
            case SemanticWalkerTokenTypes::T_MOD: {
                ExpressionValue left = expr(getChild(node, 0));
                ExpressionValue right = expr(getChild(node, 1));
                // Type checking would go here
                return v;
            }
        }
    }
    
    // Unary operations
    if (getChildCount(node) == 1) {
        switch (type) {
            case SemanticWalkerTokenTypes::TI_UN_NEG:
            case SemanticWalkerTokenTypes::TI_UN_POS:
            case SemanticWalkerTokenTypes::TI_UN_NOT:
            case SemanticWalkerTokenTypes::TI_UN_BNOT:
            case SemanticWalkerTokenTypes::TI_PARENTHESIS:
                return expr(getChild(node, 0));
        }
    }
    
    return element(node);
}

ExpressionValue SemanticAnalyzer::element(pANTLR3_BASE_TREE node) {
    ExpressionValue v;
    if (!node) return v;
    
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::TI_FCALL:
            fcall(node);
            break;
        case SemanticWalkerTokenTypes::T_IDENTIFICADOR:
            lvalue(node);
            break;
        case SemanticWalkerTokenTypes::T_STRING_LIT:
        case SemanticWalkerTokenTypes::T_INT_LIT:
        case SemanticWalkerTokenTypes::T_REAL_LIT:
        case SemanticWalkerTokenTypes::T_CARAC_LIT:
        case SemanticWalkerTokenTypes::T_KW_VERDADEIRO:
        case SemanticWalkerTokenTypes::T_KW_FALSO:
            return literal(node);
    }
    
    return v;
}

void SemanticAnalyzer::lvalue(pANTLR3_BASE_TREE node) {
    // Check variable exists
    std::string varName = getNodeText(node);
    // evaluator.checkVar(varName, getLine(node)); // Would need to implement this
}

ExpressionValue SemanticAnalyzer::literal(pANTLR3_BASE_TREE node) {
    ExpressionValue v;
    int type = getNodeType(node);
    
    switch (type) {
        case SemanticWalkerTokenTypes::T_STRING_LIT:
            v.setPrimitiveType(TIPO_LITERAL);
            break;
        case SemanticWalkerTokenTypes::T_INT_LIT:
            v.setPrimitiveType(TIPO_INTEIRO);
            break;
        case SemanticWalkerTokenTypes::T_REAL_LIT:
            v.setPrimitiveType(TIPO_REAL);
            break;
        case SemanticWalkerTokenTypes::T_CARAC_LIT:
            v.setPrimitiveType(TIPO_CARACTERE);
            break;
        case SemanticWalkerTokenTypes::T_KW_VERDADEIRO:
        case SemanticWalkerTokenTypes::T_KW_FALSO:
            v.setPrimitiveType(TIPO_LOGICO);
            break;
    }
    
    return v;
}

void SemanticAnalyzer::func_proto(pANTLR3_BASE_TREE node) {
    // Register function prototype
    std::string fname = getNodeText(node);
    
    std::list<std::pair<int, std::string>> params;
    int returnType = TIPO_NULO;
    
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::TI_VAR_PRIMITIVE ||
            type == SemanticWalkerTokenTypes::TI_VAR_MATRIX) {
            // Parameter declaration
            // Parse similar to primitivo/matriz
        } else if (type == SemanticWalkerTokenTypes::TI_FRETURN) {
            // Return type
            if (getChildCount(child) > 0) {
                int retType = getNodeType(getChild(child, 0));
                switch (retType) {
                    case SemanticWalkerTokenTypes::T_KW_INTEIRO: returnType = TIPO_INTEIRO; break;
                    case SemanticWalkerTokenTypes::T_KW_REAL: returnType = TIPO_REAL; break;
                    case SemanticWalkerTokenTypes::T_KW_CARACTERE: returnType = TIPO_CARACTERE; break;
                    case SemanticWalkerTokenTypes::T_KW_LITERAL: returnType = TIPO_LITERAL; break;
                    case SemanticWalkerTokenTypes::T_KW_LOGICO: returnType = TIPO_LOGICO; break;
                }
            }
        }
    }
    
    evaluator.declareFunction(fname, returnType, getLine(node));
}

void SemanticAnalyzer::func_decl(pANTLR3_BASE_TREE node) {
    std::string fname = getNodeText(node);
    evaluator.setCurrentScope(fname);
    
    int count = getChildCount(node);
    for (int i = 0; i < count; i++) {
        pANTLR3_BASE_TREE child = getChild(node, i);
        int type = getNodeType(child);
        
        if (type == SemanticWalkerTokenTypes::TI_VAR_PRIMITIVE) {
            primitivo(child);
        } else if (type == SemanticWalkerTokenTypes::TI_VAR_MATRIX) {
            matriz(child);
        } else if (type == SemanticWalkerTokenTypes::T_KW_VARIAVEIS) {
            variaveis(child);
        } else if (type == SemanticWalkerTokenTypes::T_KW_INICIO) {
            inicio(child);
        }
    }
    
    evaluator.setCurrentScope(SymbolTable::GlobalScope);
}

