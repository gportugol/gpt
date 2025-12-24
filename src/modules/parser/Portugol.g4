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

grammar Portugol;

/******************************** GRAMATICA *************************************************/

algoritmo
  : declaracao_algoritmo var_decl_block? stm_block func_decls* EOF
  ;

declaracao_algoritmo
  : T_KW_ALGORITMO T_IDENTIFICADOR T_SEMICOL
  ;

var_decl_block
  : T_KW_VARIAVEIS (var_decl T_SEMICOL)+ T_KW_FIM_VARIAVEIS
  ;

var_decl
  : T_IDENTIFICADOR (T_COMMA T_IDENTIFICADOR)* T_COLON tipo_decl
  ;

tipo_decl
  : tp_prim                                    # tipoPrimitivo
  | tp_matriz                                  # tipoMatriz
  ;

tp_prim
  : T_KW_INTEIRO
  | T_KW_REAL
  | T_KW_CARACTERE
  | T_KW_LITERAL
  | T_KW_LOGICO
  ;

tp_matriz
  : T_KW_MATRIZ dimensoes T_KW_DE tp_prim_pl
  ;

dimensoes
  : (T_ABREC T_INT_LIT T_FECHAC)+
  ;

tp_prim_pl
  : T_KW_INTEIROS
  | T_KW_REAIS
  | T_KW_CARACTERES
  | T_KW_LITERAIS
  | T_KW_LOGICOS
  ;

stm_block
  : T_KW_INICIO stm_list T_KW_FIM
  ;

stm_list
  : stm*
  ;

stm
  : stm_attr T_SEMICOL                         # stmAtribuicao
  | fcall T_SEMICOL                            # stmChamadaFunc
  | stm_ret T_SEMICOL                          # stmRetorno
  | stm_se                                     # stmCondicional
  | stm_enquanto                               # stmEnquanto
  | stm_repita T_SEMICOL                       # stmRepita
  | stm_para                                   # stmPara
  ;

stm_ret
  : T_KW_RETORNE expr                          # retorneComExpr
  | T_KW_RETORNE                               # retorneSemExpr
  ;

array_sub
  : (T_ABREC expr T_FECHAC)*
  ;

lvalue
  : T_IDENTIFICADOR array_sub
  ;

stm_attr
  : lvalue T_ATTR expr
  ;

stm_se
  : T_KW_SE expr T_KW_ENTAO stm_list senao_part? T_KW_FIM_SE
  ;

senao_part
  : T_KW_SENAO stm_list
  ;

stm_enquanto
  : T_KW_ENQUANTO expr T_KW_FACA stm_list T_KW_FIM_ENQUANTO
  ;

stm_repita
  : T_KW_REPITA stm_list T_KW_ATE expr
  ;

stm_para
  : T_KW_PARA lvalue T_KW_DE inicio=expr T_KW_ATE fim=expr passo? T_KW_FACA stm_list T_KW_FIM_PARA
  ;

passo
  : T_KW_PASSO (T_MAIS | T_MENOS)? T_INT_LIT
  ;


/* ----------------------------- Expressoes ---------------------------------- */

expr
  : expr_e (T_KW_OU expr_e)*
  ;

expr_e
  : expr_bit_ou (T_KW_E expr_bit_ou)*
  ;

expr_bit_ou
  : expr_bit_xou (T_BIT_OU expr_bit_xou)*
  ;

expr_bit_xou
  : expr_bit_e (T_BIT_XOU expr_bit_e)*
  ;

expr_bit_e
  : expr_igual (T_BIT_E expr_igual)*
  ;

expr_igual
  : expr_relacional ((T_IGUAL | T_DIFERENTE) expr_relacional)*
  ;

expr_relacional
  : expr_ad ((T_MAIOR | T_MAIOR_EQ | T_MENOR | T_MENOR_EQ) expr_ad)*
  ;

expr_ad
  : expr_multip ((T_MAIS | T_MENOS) expr_multip)*
  ;

expr_multip
  : expr_unario ((T_DIV | T_MULTIP | T_MOD) expr_unario)*
  ;

expr_unario
  : T_MENOS expr_elemento                      # exprNegativa
  | T_MAIS expr_elemento                       # exprPositiva
  | T_KW_NOT expr_elemento                     # exprNot
  | T_BIT_NOT expr_elemento                    # exprBitNot
  | expr_elemento                              # exprElemento
  ;

expr_elemento
  : fcall                                      # elemFcall
  | lvalue                                     # elemLvalue
  | literal                                    # elemLiteral
  | T_ABREP expr T_FECHAP                      # elemParenteses
  ;

fcall
  : T_IDENTIFICADOR T_ABREP fargs T_FECHAP
  ;

fargs
  : (expr (T_COMMA expr)*)?
  ;

literal
  : T_STRING_LIT                               # litString
  | T_INT_LIT                                  # litInteiro
  | T_REAL_LIT                                 # litReal
  | T_CARAC_LIT                                # litCaractere
  | T_KW_VERDADEIRO                            # litVerdadeiro
  | T_KW_FALSO                                 # litFalso
  ;

func_decls
  : T_KW_FUNCAO T_IDENTIFICADOR T_ABREP fparams T_FECHAP rettype fvar_decl stm_block
  ;

rettype
  : T_COLON tp_prim                            # rettypeComTipo
  |                                            # rettypeSemTipo
  ;

fvar_decl
  : (var_decl T_SEMICOL)+                      # fvarDeclComVars
  |                                            # fvarDeclSemVars
  ;

fparams
  : (fparam (T_COMMA fparam)*)?
  ;

fparam
  : T_IDENTIFICADOR T_COLON tipo_decl
  ;

/*------------------------- Lexer Rules -----------------------*/

// Keywords - accepting both with and without accents
// UTF-8 accented characters are written directly in the grammar

T_KW_ALGORITMO : 'algoritmo' ;
T_KW_VARIAVEIS : 'variáveis' ;
T_KW_FIM_VARIAVEIS : 'fim-variáveis' ;
T_KW_INTEIRO : 'inteiro' ;
T_KW_REAL : 'real' ;
T_KW_CARACTERE : 'caractere' ;
T_KW_LITERAL : 'literal' ;
T_KW_LOGICO : 'lógico' ;
T_KW_INICIO : 'início' ;
T_KW_VERDADEIRO : 'verdadeiro' ;
T_KW_FALSO : 'falso' ;
T_KW_FIM : 'fim' ;
T_KW_OU : 'ou' ;
T_KW_E : 'e' ;
T_KW_NOT : 'não' ;
T_KW_SE : 'se' ;
T_KW_SENAO : 'senão' ;
T_KW_ENTAO : 'então' ;
T_KW_FIM_SE : 'fim-se' ;
T_KW_ENQUANTO : 'enquanto' ;
T_KW_FACA : 'faça' ;
T_KW_FIM_ENQUANTO : 'fim-enquanto' ;
T_KW_PARA : 'para' ;
T_KW_DE : 'de' ;
T_KW_ATE : 'até' ;
T_KW_FIM_PARA : 'fim-para' ;
T_KW_REPITA : 'repita' ;
T_KW_MATRIZ : 'matriz' ;
T_KW_INTEIROS : 'inteiros' ;
T_KW_REAIS : 'reais' ;
T_KW_CARACTERES : 'caracteres' ;
T_KW_LITERAIS : 'literais' ;
T_KW_LOGICOS : 'lógicos' ;
T_KW_FUNCAO : 'função' ;
T_KW_RETORNE : 'retorne' ;
T_KW_PASSO : 'passo' ;

// Operators
T_BIT_OU : '|' ;
T_BIT_XOU : '^' ;
T_BIT_E : '&' ;
T_BIT_NOT : '~' ;
T_IGUAL : '=' ;
T_DIFERENTE : '<>' ;
T_MAIOR : '>' ;
T_MENOR : '<' ;
T_MAIOR_EQ : '>=' ;
T_MENOR_EQ : '<=' ;
T_MAIS : '+' ;
T_MENOS : '-' ;
T_DIV : '/' ;
T_MULTIP : '*' ;
T_MOD : '%' ;
T_ABREP : '(' ;
T_FECHAP : ')' ;
T_ABREC : '[' ;
T_FECHAC : ']' ;
T_ATTR : ':=' ;
T_SEMICOL : ';' ;
T_COLON : ':' ;
T_COMMA : ',' ;

T_REAL_LIT
  : T_DIGIT+ '.' T_DIGIT+
  ;

T_INT_LIT
  : T_OCTAL_LIT
  | T_HEX_LIT
  | T_BIN_LIT
  | T_DIGIT+
  ;

fragment T_OCTAL_LIT
  : '0' [cC] [0-7]+
  ;

fragment T_HEX_LIT
  : '0' [xX] [0-9a-fA-F]+
  ;

fragment T_BIN_LIT
  : '0' [bB] [01]+
  ;

T_CARAC_LIT
  : '\'' ( ~['\\\r\n] | ESC )? '\''
  ;

T_STRING_LIT
  : '"' ( ~["\\\r\n] | ESC)* '"'
  ;

fragment ESC
  : '\\' .
  ;

T_IDENTIFICADOR
  : T_ID_START T_ID_CHAR*
  ;

fragment T_ID_START
  : [a-zA-Z_] | [\u0080-\u00FF]
  ;

fragment T_ID_CHAR
  : [a-zA-Z0-9_] | [\u0080-\u00FF]
  ;

fragment T_DIGIT
  : [0-9]
  ;

T_WS
  : [ \t\r\n]+ -> skip
  ;

SL_COMMENT
  : '//' ~[\r\n]* -> skip
  ;

ML_COMMENT
  : '/*' .*? '*/' -> skip
  ;
