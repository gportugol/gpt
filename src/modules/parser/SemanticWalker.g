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

tree grammar SemanticWalker;

options {
  language = C;
  tokenVocab = Portugol;
  ASTLabelType = pANTLR3_BASE_TREE;
  backtrack = true;
}

/****************************** TREE WALKER *********************************************/

algoritmo
  : ^(T_KW_ALGORITMO T_IDENTIFICADOR)
    (variaveis)?
    inicio
    (func_decl)*
  ;

variaveis
  : ^(T_KW_VARIAVEIS decl_var+)
  ;

decl_var
  : primitivo
  | matriz
  ;

primitivo
  : ^(TI_VAR_PRIMITIVE ^(TI_VAR_PRIMITIVE tipo_prim) T_IDENTIFICADOR+)
  | ^(TI_VAR_PRIMITIVE tipo_prim T_IDENTIFICADOR+)
  ;

tipo_prim
  : T_KW_INTEIRO
  | T_KW_REAL
  | T_KW_CARACTERE
  | T_KW_LITERAL
  | T_KW_LOGICO
  ;

matriz
  : ^(TI_VAR_MATRIX ^(TI_VAR_MATRIX tipo_matriz) T_IDENTIFICADOR+)
  | ^(TI_VAR_MATRIX tipo_matriz T_IDENTIFICADOR+)
  ;

tipo_matriz
  : ^(T_KW_INTEIROS T_INT_LIT+)
  | ^(T_KW_REAIS T_INT_LIT+)
  | ^(T_KW_CARACTERES T_INT_LIT+)
  | ^(T_KW_LITERAIS T_INT_LIT+)
  | ^(T_KW_LOGICOS T_INT_LIT+)
  ;


inicio
  : ^(T_KW_INICIO stm*)
  ;

stm
  : stm_attr
  | fcall
  | stm_ret
  | stm_se
  | stm_enquanto
  | stm_repita
  | stm_para
  ;

stm_attr
  : ^(T_ATTR lvalue expr)
  ;

lvalue
  : ^(T_IDENTIFICADOR expr*)
  ;

fcall
  : ^(TI_FCALL T_IDENTIFICADOR expr*)
  ;

stm_ret
  : ^(T_KW_RETORNE TI_NULL)
  | ^(T_KW_RETORNE expr)
  ;

stm_se
  : ^(T_KW_SE expr stm_list senao_part?)
  ;

stm_list
  : stm*
  ;

senao_part
  : ^(T_KW_SENAO stm*)
  ;

stm_enquanto
  : ^(T_KW_ENQUANTO expr stm*)
  ;

stm_repita
  : ^(T_KW_REPITA stm_list expr)
  ;

stm_para
  : ^(T_KW_PARA lvalue expr expr passo? stm*)
  ;

passo
  : ^(T_KW_PASSO T_MAIS? T_MENOS? T_INT_LIT)
  ;

expr
  : ^(T_KW_OU expr expr)
  | ^(T_KW_E expr expr)
  | ^(T_BIT_OU expr expr)
  | ^(T_BIT_XOU expr expr)
  | ^(T_BIT_E expr expr)
  | ^(T_IGUAL expr expr)
  | ^(T_DIFERENTE expr expr)
  | ^(T_MAIOR expr expr)
  | ^(T_MENOR expr expr)
  | ^(T_MAIOR_EQ expr expr)
  | ^(T_MENOR_EQ expr expr)
  | ^(T_MAIS expr expr)
  | ^(T_MENOS expr expr)
  | ^(T_DIV expr expr)
  | ^(T_MULTIP expr expr)
  | ^(T_MOD expr expr)
  | ^(TI_UN_NEG element)
  | ^(TI_UN_POS element)
  | ^(TI_UN_NOT element)
  | ^(TI_UN_BNOT element)
  | element
  ;

element
  : literal
  | fcall
  | lvalue
  | ^(TI_PARENTHESIS expr)
  ;

literal
  : T_STRING_LIT
  | T_INT_LIT
  | T_REAL_LIT
  | T_CARAC_LIT
  | T_KW_VERDADEIRO
  | T_KW_FALSO
  ;

func_decl
  : ^(T_IDENTIFICADOR param_list ret_type? fvar_decl? inicio)
  ;

param_list
  : (primitivo | matriz)*
  ;

fvar_decl
  : ^(T_KW_VARIAVEIS decl_var+)
  ;

ret_type
  : ^(TI_FRETURN tipo_prim)
  ;
