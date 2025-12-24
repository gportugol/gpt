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

options {
  language = C;
  output = AST;
  ASTLabelType = pANTLR3_BASE_TREE;
  k = 2;
}

tokens {
  // Imaginary tokens for AST
  TI_UN_POS;
  TI_UN_NEG;
  TI_UN_NOT;
  TI_UN_BNOT;
  TI_PARENTHESIS;
  TI_FCALL;
  TI_FRETURN;
  TI_VAR_PRIMITIVE;
  TI_VAR_MATRIX;
  TI_NULL;
}

/******************************** GRAMATICA *************************************************/

algoritmo
  : declaracao_algoritmo var_decl_block? stm_block func_decls* EOF
  ;

declaracao_algoritmo
  : T_KW_ALGORITMO T_IDENTIFICADOR T_SEMICOL
    -> ^(T_KW_ALGORITMO T_IDENTIFICADOR)
  ;

var_decl_block
  : T_KW_VARIAVEIS (var_decl T_SEMICOL)+ T_KW_FIM_VARIAVEIS
    -> ^(T_KW_VARIAVEIS var_decl+)
  ;

var_decl
  : T_IDENTIFICADOR var_more T_COLON tipo_decl
    -> ^(tipo_decl T_IDENTIFICADOR var_more)
  ;

tipo_decl
  : tp_prim -> ^(TI_VAR_PRIMITIVE tp_prim)
  | tp_matriz -> ^(TI_VAR_MATRIX tp_matriz)
  ;

var_more
  : (T_COMMA T_IDENTIFICADOR)*
    -> T_IDENTIFICADOR*
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
    -> ^(tp_prim_pl dimensoes)
  ;

dimensoes
  : (T_ABREC T_INT_LIT T_FECHAC)+
    -> T_INT_LIT+
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
    -> ^(T_KW_INICIO stm_list)
  ;

stm_list
  : stm*
  ;

stm
  : (lvalue T_ATTR)=> stm_attr T_SEMICOL!
  | (T_IDENTIFICADOR T_ABREP)=> fcall T_SEMICOL!
  | stm_ret T_SEMICOL!
  | stm_se
  | stm_enquanto
  | stm_repita T_SEMICOL!
  | stm_para
  ;

stm_ret
  : T_KW_RETORNE expr
    -> ^(T_KW_RETORNE expr)
  | T_KW_RETORNE
    -> ^(T_KW_RETORNE TI_NULL)
  ;

array_sub
  : (T_ABREC expr T_FECHAC)*
    -> expr*
  ;

lvalue
  : T_IDENTIFICADOR array_sub
    -> ^(T_IDENTIFICADOR array_sub)
  ;

stm_attr
  : lvalue T_ATTR expr
    -> ^(T_ATTR lvalue expr)
  ;

stm_se
  : T_KW_SE expr T_KW_ENTAO stm_list senao_part? T_KW_FIM_SE
    -> ^(T_KW_SE expr stm_list senao_part?)
  ;

senao_part
  : T_KW_SENAO stm_list
    -> ^(T_KW_SENAO stm_list)
  ;

stm_enquanto
  : T_KW_ENQUANTO expr T_KW_FACA stm_list T_KW_FIM_ENQUANTO
    -> ^(T_KW_ENQUANTO expr stm_list)
  ;

stm_repita
  : T_KW_REPITA stm_list T_KW_ATE expr
    -> ^(T_KW_REPITA stm_list expr)
  ;

stm_para
  : T_KW_PARA lvalue T_KW_DE e1=expr T_KW_ATE e2=expr passo? T_KW_FACA stm_list T_KW_FIM_PARA
    -> ^(T_KW_PARA lvalue $e1 $e2 passo? stm_list)
  ;

passo
  : T_KW_PASSO (T_MAIS|T_MENOS)? T_INT_LIT
    -> ^(T_KW_PASSO T_MAIS? T_MENOS? T_INT_LIT)
  ;


/* ----------------------------- Expressoes ---------------------------------- */

expr
  : expr_e (T_KW_OU^ expr_e)*
  ;

expr_e
  : expr_bit_ou (T_KW_E^ expr_bit_ou)*
  ;

expr_bit_ou
  : expr_bit_xou (T_BIT_OU^ expr_bit_xou)*
  ;

expr_bit_xou
  : expr_bit_e (T_BIT_XOU^ expr_bit_e)*
  ;

expr_bit_e
  : expr_igual (T_BIT_E^ expr_igual)*
  ;

expr_igual
  : expr_relacional ((T_IGUAL^ | T_DIFERENTE^) expr_relacional)*
  ;

expr_relacional
  : expr_ad ((T_MAIOR^ | T_MAIOR_EQ^ | T_MENOR^ | T_MENOR_EQ^) expr_ad)*
  ;

expr_ad
  : expr_multip ((T_MAIS^ | T_MENOS^) expr_multip)*
  ;

expr_multip
  : expr_unario ((T_DIV^ | T_MULTIP^ | T_MOD^) expr_unario)*
  ;

expr_unario
  : T_MENOS expr_elemento -> ^(TI_UN_NEG expr_elemento)
  | T_MAIS expr_elemento -> ^(TI_UN_POS expr_elemento)
  | T_KW_NOT expr_elemento -> ^(TI_UN_NOT expr_elemento)
  | T_BIT_NOT expr_elemento -> ^(TI_UN_BNOT expr_elemento)
  | expr_elemento
  ;

expr_elemento
  : (T_IDENTIFICADOR T_ABREP)=> fcall
  | lvalue
  | literal
  | T_ABREP expr T_FECHAP -> ^(TI_PARENTHESIS expr)
  ;

fcall
  : T_IDENTIFICADOR T_ABREP fargs T_FECHAP
    -> ^(TI_FCALL T_IDENTIFICADOR fargs)
  ;

fargs
  : (expr (T_COMMA expr)*)?
    -> expr*
  ;

literal
  : T_STRING_LIT
  | T_INT_LIT
  | T_REAL_LIT
  | T_CARAC_LIT
  | T_KW_VERDADEIRO
  | T_KW_FALSO
  ;

func_decls
  : T_KW_FUNCAO T_IDENTIFICADOR T_ABREP fparams T_FECHAP rettype fvar_decl stm_block
    -> ^(T_IDENTIFICADOR fparams rettype fvar_decl stm_block)
  ;

rettype
  : T_COLON tp_prim
    -> ^(TI_FRETURN tp_prim)
  |
    -> /* empty */
  ;

fffvar_decl
  : (var_decl T_SEMICOL)+
    -> var_decl+
  ;

fvar_decl
  : fffvar_decl
    -> ^(T_KW_VARIAVEIS fffvar_decl)
  |
    -> /* empty */
  ;

fparams
  : (fparam (T_COMMA fparam)*)?
    -> fparam*
  ;

fparam
  : T_IDENTIFICADOR T_COLON tipo_decl_param
    -> ^(tipo_decl_param T_IDENTIFICADOR)
  ;

tipo_decl_param
  : tp_prim -> ^(TI_VAR_PRIMITIVE tp_prim)
  | tp_matriz -> ^(TI_VAR_MATRIX tp_matriz)
  ;

/*------------------------- Lexer Rules -----------------------*/

// Keywords - accepting both with and without accents
// UTF-8 accented characters are written directly in the grammar

T_KW_ALGORITMO : 'algoritmo' ;
T_KW_VARIAVEIS : 'variaveis' | 'variáveis' ;
T_KW_FIM_VARIAVEIS : 'fim-variaveis' | 'fim-variáveis' ;
T_KW_INTEIRO : 'inteiro' ;
T_KW_REAL : 'real' ;
T_KW_CARACTERE : 'caractere' ;
T_KW_LITERAL : 'literal' ;
T_KW_LOGICO : 'logico' | 'lógico' ;
T_KW_INICIO : 'inicio' | 'início' ;
T_KW_VERDADEIRO : 'verdadeiro' ;
T_KW_FALSO : 'falso' ;
T_KW_FIM : 'fim' ;
T_KW_OU : 'ou' ;
T_KW_E : 'e' ;
T_KW_NOT : 'nao' | 'não' ;
T_KW_SE : 'se' ;
T_KW_SENAO : 'senao' | 'senão' ;
T_KW_ENTAO : 'entao' | 'então' ;
T_KW_FIM_SE : 'fim-se' ;
T_KW_ENQUANTO : 'enquanto' ;
T_KW_FACA : 'faca' | 'faça' ;
T_KW_FIM_ENQUANTO : 'fim-enquanto' ;
T_KW_PARA : 'para' ;
T_KW_DE : 'de' ;
T_KW_ATE : 'ate' | 'até' ;
T_KW_FIM_PARA : 'fim-para' ;
T_KW_REPITA : 'repita' ;
T_KW_MATRIZ : 'matriz' ;
T_KW_INTEIROS : 'inteiros' ;
T_KW_REAIS : 'reais' ;
T_KW_CARACTERES : 'caracteres' ;
T_KW_LITERAIS : 'literais' ;
T_KW_LOGICOS : 'logicos' | 'lógicos' ;
T_KW_FUNCAO : 'funcao' | 'função' ;
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

T_INT_LIT
  : T_OCTAL_LIT
  | T_HEX_LIT
  | T_BIN_LIT
  | T_INTEGER_LIT ('.' T_DIGIT+ { $type = T_REAL_LIT; })?
  ;

T_REAL_LIT : ; // Set by T_INT_LIT

fragment
T_INTEGER_LIT
  : T_DIGIT+
  ;

fragment
T_OCTAL_LIT
  : '0' ('c'|'C') T_LETTER_OR_DIGIT+
  ;

fragment
T_HEX_LIT
  : '0' ('x'|'X') T_LETTER_OR_DIGIT+
  ;

fragment
T_BIN_LIT
  : '0' ('b'|'B') T_LETTER_OR_DIGIT+
  ;

T_CARAC_LIT
  : '\'' ( ~('\''|'\\') | ESC )? '\''
  ;

T_STRING_LIT
  : '"' ( ~('"'|'\\'|'\n'|'\r') | ESC)* '"'
  ;

fragment
ESC
  : '\\' .
  ;

T_IDENTIFICADOR
  : T_ID_AUX ('-' T_LETTER_OR_DIGIT*)?
  ;

fragment
T_ID_AUX
  : (T_LETTER | '_') T_LETTER_OR_DIGIT*
  ;

fragment
T_LETTER_OR_DIGIT
  : T_LETTER | T_DIGIT | '_'
  ;

fragment
T_DIGIT
  : '0'..'9'
  ;

fragment
T_LETTER
  : 'a'..'z' | 'A'..'Z' | '\u0080'..'\u00FF'
  ;

T_WS
  : (' ' | '\t' | '\n' | '\r')+ { $channel = HIDDEN; }
  ;

SL_COMMENT
  : '//' (~'\n')* '\n'? { $channel = HIDDEN; }
  ;

ML_COMMENT
  : '/*' ( options {greedy=false;} : . )* '*/' { $channel = HIDDEN; }
  ;
