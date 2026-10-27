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

#include "PortugolErrorStrategy.hpp"

#include "BasePortugolParser.hpp"
#include "GPTDisplay.hpp"
#include "PortugolParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdlib.h>

using namespace antlr4;
using namespace std;

namespace {
struct LexicalError {
  size_t line;
  size_t column;
  string msg;
};

// The lexer reports to PortugolLexerErrorListener while validateTokens()
// fills the token stream. The errors wait here until validateTokens() knows
// which of them are inside an unterminated comment.
vector<LexicalError> lexicalErrors;

const string expecting_var_or_inicio =
    "\"variáveis\" ou \"início\" após declaração de algoritmo";
} // namespace

// Consumers strip the quotes themselves, as the ANTLR2 lexer did: an empty
// literal cannot be rewritten into the token, setText("") falls back to the
// input text.
string unquote(const string &text) {
  if (text.size() >= 2) {
    return text.substr(1, text.size() - 2);
  }
  return text;
}

static string tokenText(Token *token) {
  if (token->getType() == PortugolParser::T_STRING_LIT ||
      token->getType() == PortugolParser::T_CARAC_LIT) {
    return unquote(token->getText());
  }
  return token->getText();
}

static bool isKeyword(size_t type) {
  return type >= PortugolParser::T_KW_ALGORITMO &&
         type <= PortugolParser::T_KW_PASSO;
}

static bool isDatatype(size_t type) {
  switch (type) {
  case PortugolParser::T_KW_INTEIRO:
  case PortugolParser::T_KW_INTEIROS:
  case PortugolParser::T_KW_REAL:
  case PortugolParser::T_KW_REAIS:
  case PortugolParser::T_KW_CARACTERE:
  case PortugolParser::T_KW_CARACTERES:
  case PortugolParser::T_KW_LITERAL:
  case PortugolParser::T_KW_LITERAIS:
  case PortugolParser::T_KW_LOGICO:
  case PortugolParser::T_KW_LOGICOS:
    return true;
  }
  return false;
}

// ANTLR 4.10+ escapes non-ASCII characters of literal names as \uXXXX.
static string unescapeUnicode(const string &str) {
  string out;
  for (string::size_type i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 5 < str.size() && str[i + 1] == 'u') {
      unsigned cp = (unsigned)strtoul(str.substr(i + 2, 4).c_str(), NULL, 16);
      if (cp < 0x80) {
        out += (char)cp;
      } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
      } else {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
      }
      i += 5;
    } else {
      out += str[i];
    }
  }
  return out;
}

static string literalText(Parser *recognizer, size_t type) {
  string lit(recognizer->getVocabulary().getLiteralName(type));
  if (lit.size() >= 2 && lit[0] == '\'' && lit[lit.size() - 1] == '\'') {
    lit = lit.substr(1, lit.size() - 2);
  }
  return unescapeUnicode(lit);
}

string PortugolErrorStrategy::tokenName(Parser *recognizer, size_t type) {
  switch (type) {
  case Token::EOF:
    return "fim de arquivo (EOF)";
  case PortugolParser::T_IDENTIFICADOR:
    return "identificador";
  case PortugolParser::T_INT_LIT:
    return "número inteiro";
  case PortugolParser::T_REAL_LIT:
    return "número real";
  case PortugolParser::T_CARAC_LIT:
    return "caractere";
  case PortugolParser::T_STRING_LIT:
    return "literal";
  case PortugolParser::T_BIT_OU:
  case PortugolParser::T_BIT_XOU:
  case PortugolParser::T_BIT_E:
  case PortugolParser::T_BIT_NOT:
  case PortugolParser::T_IGUAL:
  case PortugolParser::T_DIFERENTE:
  case PortugolParser::T_MAIOR:
  case PortugolParser::T_MENOR:
  case PortugolParser::T_MAIOR_EQ:
  case PortugolParser::T_MENOR_EQ:
  case PortugolParser::T_MAIS:
  case PortugolParser::T_MENOS:
  case PortugolParser::T_DIV:
  case PortugolParser::T_MULTIP:
  case PortugolParser::T_MOD:
    return "operador '" + literalText(recognizer, type) + "'";
  }
  if (isKeyword(type)) {
    return "\"" + literalText(recognizer, type) + "\"";
  }
  return "'" + literalText(recognizer, type) + "'";
}

string PortugolErrorStrategy::tokenDescription(Parser *recognizer,
                                               Token *token) {
  if (token->getType() == PortugolParser::T_IDENTIFICADOR) {
    return "\"" + token->getText() + "\"";
  } else if (isKeyword(token->getType())) {
    return "a palavra-chave " + tokenName(recognizer, token->getType());
  } else if (token->getType() == Token::EOF) {
    return "fim de arquivo (EOF)";
  }
  return tokenText(token);
}

static bool endsExpression(size_t type) {
  switch (type) {
  case PortugolParser::T_SEMICOL:
    return true;
  case PortugolParser::T_KW_E:
  case PortugolParser::T_KW_OU:
  case PortugolParser::T_KW_NOT:
  case PortugolParser::T_KW_VERDADEIRO:
  case PortugolParser::T_KW_FALSO:
    return false;
  }
  return isKeyword(type);
}

// Where an operand is missing, ANTLR deletes the unexpected token if the next
// one can start an operand: "x := 1 +;" followed by "y := 2;" took "y" as
// the operand and then failed on ":=". The ANTLR2 parser stopped the
// expression at the ';' instead.
Token *PortugolErrorStrategy::singleTokenDeletion(Parser *recognizer) {
  size_t rule = recognizer->getContext()->getRuleIndex();
  if (rule >= PortugolParser::RuleExpr &&
      rule <= PortugolParser::RuleExpr_elemento &&
      endsExpression(recognizer->getCurrentToken()->getType())) {
    return nullptr;
  }
  return DefaultErrorStrategy::singleTokenDeletion(recognizer);
}

misc::IntervalSet
PortugolErrorStrategy::getErrorRecoverySet(Parser *recognizer) {
  misc::IntervalSet set = DefaultErrorStrategy::getErrorRecoverySet(recognizer);
  // A bad token after "função f()" is reported as a missing "início";
  // resuming at an identifier would read what follows as local variables.
  if (recognizer->getContext()->getRuleIndex() == PortugolParser::RuleRettype) {
    set.remove((size_t)PortugolParser::T_IDENTIFICADOR);
  }
  return set;
}

void PortugolErrorStrategy::reportError(Parser *recognizer,
                                        const RecognitionException &e) {
  if (inErrorRecoveryMode(recognizer)) {
    return; // already reported; stay quiet until the parser resynchronizes
  }
  beginErrorCondition(recognizer);
  Token *offending = e.getOffendingToken();
  if (auto nva = dynamic_cast<const NoViableAltException *>(&e)) {
    if (nva->getStartToken()) {
      offending = nva->getStartToken();
    }
  }
  if (!offending) {
    offending = recognizer->getCurrentToken();
  }
  report(recognizer, offending);
}

void PortugolErrorStrategy::reportUnwantedToken(Parser *recognizer) {
  if (inErrorRecoveryMode(recognizer)) {
    return;
  }
  beginErrorCondition(recognizer);
  report(recognizer, recognizer->getCurrentToken());
}

void PortugolErrorStrategy::reportMissingToken(Parser *recognizer) {
  if (inErrorRecoveryMode(recognizer)) {
    return;
  }
  beginErrorCondition(recognizer);
  report(recognizer, recognizer->getCurrentToken());
}

static string enclosingBlockExpectation(ParserRuleContext *ctx) {
  for (; ctx; ctx = dynamic_cast<ParserRuleContext *>(ctx->parent)) {
    if (dynamic_cast<PortugolParser::Stm_seContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimse;
    if (dynamic_cast<PortugolParser::Stm_enquantoContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimenq;
    if (dynamic_cast<PortugolParser::Stm_paraContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimpara;
    if (dynamic_cast<PortugolParser::Stm_repitaContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_ate;
    if (dynamic_cast<PortugolParser::Stm_blockContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fim;
  }
  return "";
}

string PortugolErrorStrategy::expectingText(Parser *recognizer,
                                            const misc::IntervalSet &expected,
                                            Token * /*offending*/) {
  ParserRuleContext *ctx = recognizer->getContext();
  size_t rule = ctx ? ctx->getRuleIndex() : (size_t)-1;

  bool hasIdent = expected.contains((size_t)PortugolParser::T_IDENTIFICADOR);

  // Inside a statement list the expected set carries the terminator of the
  // enclosing block, which is what the ANTLR2 messages named.
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_SE))
    return BasePortugolParser::expecting_stm_or_fimse;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_ENQUANTO))
    return BasePortugolParser::expecting_stm_or_fimenq;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_PARA))
    return BasePortugolParser::expecting_stm_or_fimpara;
  // "até" also follows the initial value of "para", where no command fits.
  if (expected.contains((size_t)PortugolParser::T_KW_ATE) &&
      !expected.contains((size_t)PortugolParser::T_KW_PASSO) &&
      !dynamic_cast<PortugolParser::Stm_paraContext *>(ctx))
    return BasePortugolParser::expecting_stm_or_ate;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM))
    return BasePortugolParser::expecting_stm_or_fim;

  if (expected.contains((size_t)PortugolParser::T_KW_SE) &&
      expected.contains((size_t)PortugolParser::T_KW_RETORNE)) {
    string s = enclosingBlockExpectation(ctx);
    if (!s.empty())
      return s;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_VARIAVEIS) &&
      expected.contains((size_t)PortugolParser::T_KW_INICIO)) {
    return expecting_var_or_inicio;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_FUNCAO) &&
      expected.contains((size_t)Token::EOF)) {
    return BasePortugolParser::expecting_eof_or_function;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_FIM_VARIAVEIS) &&
      hasIdent) {
    return BasePortugolParser::expecting_fimvar_or_var;
  }

  switch (rule) {
  case PortugolParser::RuleDeclaracao_algoritmo:
    if (hasIdent)
      return BasePortugolParser::expecting_algorithm_name;
    break;
  case PortugolParser::RuleFunc_decls:
    if (hasIdent)
      return BasePortugolParser::expecting_function_name;
    break;
  case PortugolParser::RuleFparams:
    return BasePortugolParser::expecting_param_or_fparen;
  case PortugolParser::RuleRettype:
  case PortugolParser::RuleFvar_decl:
    // The ANTLR2 parser took the empty alternative here and failed on the
    // "início" of stm_block, so that is the token its message named.
    if (expected.contains((size_t)PortugolParser::T_KW_INICIO))
      return tokenName(recognizer, PortugolParser::T_KW_INICIO);
    break;
  case PortugolParser::RuleVar_decl_block:
  case PortugolParser::RuleVar_decl:
  case PortugolParser::RuleFparam:
  case PortugolParser::RuleLvalue:
  case PortugolParser::RuleStm_para:
    if (hasIdent && !expected.contains((size_t)PortugolParser::T_INT_LIT))
      return BasePortugolParser::expecting_variable;
    if (expected.contains((size_t)PortugolParser::T_COLON))
      return tokenName(recognizer, PortugolParser::T_COLON);
    break;
  case PortugolParser::RulePasso:
  case PortugolParser::RuleDimensoes:
    if (expected.contains((size_t)PortugolParser::T_INT_LIT))
      return tokenName(recognizer, PortugolParser::T_INT_LIT);
    break;
  case PortugolParser::RuleTp_prim_pl:
    return BasePortugolParser::expecting_datatype_pl;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_INTEIROS)) {
    return BasePortugolParser::expecting_datatype_pl;
  }
  if (expected.contains((size_t)PortugolParser::T_KW_INTEIRO)) {
    return BasePortugolParser::expecting_datatype;
  }

  if (expected.contains((size_t)PortugolParser::T_INT_LIT) &&
      expected.contains((size_t)PortugolParser::T_STRING_LIT)) {
    return BasePortugolParser::expecting_expression;
  }

  vector<ptrdiff_t> types = expected.toList();
  string str;
  for (size_t i = 0; i < types.size(); ++i) {
    if (i > 0) {
      str += " ou ";
    }
    str += tokenName(recognizer, (size_t)types[i]);
  }
  return str;
}

void PortugolErrorStrategy::report(Parser *recognizer, Token *offending) {
  misc::IntervalSet expected = recognizer->getExpectedTokens();
  string expecting = expectingText(recognizer, expected, offending);

  Token *last = recognizer->getTokenStream()->LT(-1);
  ParserRuleContext *ctx0 = recognizer->getContext();
  size_t rule0 = ctx0 ? ctx0->getRuleIndex() : (size_t)-1;

  // The ANTLR2 handlers of these rules always reported "encontrado X" on the
  // line of the unexpected token, the others only when it shared the line
  // of the last consumed token.
  bool alwaysFound =
      expecting == BasePortugolParser::expecting_stm_or_fim ||
      expecting == BasePortugolParser::expecting_fimvar_or_var ||
      expecting == BasePortugolParser::expecting_eof_or_function ||
      expecting == tokenName(recognizer, PortugolParser::T_KW_INICIO) ||
      expecting == expecting_var_or_inicio ||
      rule0 == PortugolParser::RuleAlgoritmo;
  bool alwaysAfter = last && (rule0 == PortugolParser::RulePasso ||
                              rule0 == PortugolParser::RuleDimensoes);

  int line;
  string found, after;
  if (alwaysAfter ||
      (!alwaysFound && last && last->getLine() != offending->getLine())) {
    line = (int)last->getLine();
    after = tokenText(last);
  } else {
    line = (int)offending->getLine();
    found = tokenDescription(recognizer, offending);
    if (expecting == BasePortugolParser::expecting_expression && last) {
      after = tokenText(last);
    }
  }

  int cd = BasePortugolParser::reportParserError(line, expecting, found, after);

  size_t rule = recognizer->getContext()
                    ? recognizer->getContext()->getRuleIndex()
                    : (size_t)-1;
  size_t ftype = offending->getType();

  if (expected.contains((size_t)PortugolParser::T_COLON) && last &&
      (rule == PortugolParser::RuleVar_decl ||
       rule == PortugolParser::RuleFparam) &&
      found.length()) {
    if (ftype == PortugolParser::T_IDENTIFICADOR) {
      BasePortugolParser::printTip(
          string("Coloque uma vírgula entre as variáveis \"") +
              last->getText() + "\" e \"" + offending->getText() + "\"",
          line, cd);
    } else if (isDatatype(ftype)) {
      BasePortugolParser::printTip(string("Coloque ':' entre \"") +
                                       last->getText() + "\" e " +
                                       tokenName(recognizer, ftype),
                                   line, cd);
    }
  } else if (expecting == BasePortugolParser::expecting_stm_or_fim &&
             ftype == PortugolParser::T_IDENTIFICADOR) {
    BasePortugolParser::printTip(
        string("Verifique o uso de \"[]\" (caso \"") + offending->getText() +
            "\" seja um conjunto/matriz), do operador \":=\" (caso seja um "
            "comando de atribuição) e do uso de parêntesis (caso \"" +
            offending->getText() + "\" seja uma chamada de função)",
        line, cd);
  } else if (expecting == BasePortugolParser::expecting_variable &&
             rule == PortugolParser::RuleVar_decl_block) {
    BasePortugolParser::printTip("Pelo menos uma variável deve ser declarada",
                                 line, cd);
  } else if ((rule == PortugolParser::RuleFcall ||
              rule == PortugolParser::RuleFargs) &&
             expected.contains((size_t)PortugolParser::T_FECHAP) &&
             (ftype == PortugolParser::T_IDENTIFICADOR ||
              ftype == PortugolParser::T_STRING_LIT ||
              ftype == PortugolParser::T_INT_LIT ||
              ftype == PortugolParser::T_REAL_LIT ||
              ftype == PortugolParser::T_CARAC_LIT)) {
    BasePortugolParser::printTip(string("Coloque uma vírgula antes de \"") +
                                     tokenText(offending) + "\"",
                                 line, cd);
  }
}

void PortugolLexerErrorListener::syntaxError(
    Recognizer * /*recognizer*/, Token * /*offendingSymbol*/, size_t line,
    size_t charPositionInLine, const string &msg, std::exception_ptr /*e*/) {
  // msg is "token recognition error at: '<text>'"
  string text;
  string::size_type pos = msg.find("at: '");
  if (pos != string::npos) {
    text = msg.substr(pos + 5);
    if (!text.empty() && text[text.size() - 1] == '\'') {
      text.erase(text.size() - 1);
    }
  }

  string c;
  if (!text.empty()) {
    unsigned char b = (unsigned char)text[0];
    size_t len = (b < 0x80) ? 1 : (b >> 5) == 0x6 ? 2 : (b >> 4) == 0xE ? 3 : 4;
    c = text.substr(0, len);
  }
  stringstream s;
  s << "Caractere inválido: \"" << c << "\"";
  lexicalErrors.push_back({line, charPositionInLine, s.str()});
}

// Base of an integer literal with a 0c, 0x or 0b prefix, 0 for a decimal one.
static int literalBase(const string &text) {
  if (text.size() < 2 || text[0] != '0') {
    return 0;
  }
  switch (text[1]) {
  case 'c':
  case 'C':
    return 8;
  case 'x':
  case 'X':
    return 16;
  case 'b':
  case 'B':
    return 2;
  }
  return 0;
}

static bool validDigits(const string &text, int base) {
  if (text.size() == 2) {
    return false;
  }
  for (string::size_type i = 2; i < text.size(); ++i) {
    unsigned char c = (unsigned char)text[i];
    bool valid = (base == 16)  ? isxdigit(c) != 0
                 : (base == 8) ? (c >= '0' && c <= '7')
                               : (c == '0' || c == '1');
    if (!valid) {
      return false;
    }
  }
  return true;
}

void validateTokens(CommonTokenStream &tokens) {
  tokens.fill();
  const vector<Token *> &all = tokens.getTokens();
  GPTDisplay *display = GPTDisplay::self();

  vector<LexicalError> errors;
  for (size_t i = 0; i + 1 < all.size(); ++i) {
    if (all[i]->getType() != PortugolParser::T_DIV ||
        all[i + 1]->getType() != PortugolParser::T_MULTIP ||
        all[i + 1]->getStartIndex() != all[i]->getStopIndex() + 1) {
      continue;
    }
    // The grammar has no rule for an unterminated comment, so "/*" lexes as
    // '/' '*'. The ANTLR2 lexer, one per input file, warned at the end of the
    // file and skipped the rest of it.
    size_t line = all[i]->getLine();
    size_t column = all[i]->getCharPositionInLine();
    int next = display->nextFileStart((int)line);
    size_t j = i;
    for (; all[j]->getType() != Token::EOF &&
           (next == 0 || all[j]->getLine() < (size_t)next);
         ++j) {
      if (auto wt = dynamic_cast<WritableToken *>(all[j]))
        wt->setChannel(Token::HIDDEN_CHANNEL);
    }
    for (const LexicalError &e : lexicalErrors) {
      bool inComment =
          (e.line > line || (e.line == line && e.column >= column)) &&
          (next == 0 || e.line < (size_t)next);
      if (!inComment) {
        errors.push_back(e);
      }
    }
    lexicalErrors.swap(errors);
    errors.clear();

    string file = display->getCurrentFile();
    int startLine, endLine;
    display->mapLine((int)line, file, startLine);
    if (next == 0) {
      display->mapLine((int)all.back()->getLine(), file, endLine);
    } else {
      display->mapLine(next - 1, file, endLine);
      endLine++;
    }
    stringstream s;
    s << "AVISO: comentário iniciado na linha " << startLine
      << " não termina com \"*/\"";
    display->add(s.str(), file, endLine);
    i = j - 1;
  }
  errors.swap(lexicalErrors);
  for (Token *t : all) {
    if (t->getChannel() != Token::DEFAULT_CHANNEL) {
      continue;
    }
    if (t->getType() == PortugolParser::T_QUOTE_ERROR) {
      errors.push_back(
          {t->getLine(), t->getCharPositionInLine(), "Faltando fechar aspas"});
      if (auto wt = dynamic_cast<WritableToken *>(t))
        wt->setChannel(Token::HIDDEN_CHANNEL);
      continue;
    }
    // The ANTLR2 lexer rewrote 0c17/0x1F/0b101 to decimal text, so every
    // consumer only ever sees base-10 integers.
    if (t->getType() == PortugolParser::T_INT_LIT) {
      const string &text = t->getText();
      int base = literalBase(text);
      if (base && !validDigits(text, base)) {
        stringstream s;
        s << "\"" << text << "\" não é um valor "
          << (base == 16  ? "hexadecimal"
              : base == 8 ? "octal"
                          : "binário")
          << " válido";
        errors.push_back({t->getLine(), t->getCharPositionInLine(), s.str()});
      } else if (base) {
        // Like the ANTLR2 lexer, keep the low 32 bits as a signed int, so
        // 0xFFFFFFFF reads as -1 in every backend.
        int v = (int)strtoul(text.c_str() + 2, NULL, base);
        stringstream s;
        s << v;
        if (auto wt = dynamic_cast<WritableToken *>(t))
          wt->setText(s.str());
      }
      continue;
    }
    if (t->getType() == PortugolParser::T_CARAC_LIT) {
      // A caractere holds one byte, so only Latin-1 (U+00C2 and U+00C3 lead
      // bytes included) fits; the ANTLR2 lexer rejected the rest.
      const string &text = t->getText();
      string::size_type pos = (text.size() > 2 && text[1] == '\\') ? 2 : 1;
      unsigned char lead = (unsigned char)text[pos];
      if (lead >= 0xC4) {
        size_t len = (lead >> 4) == 0xE ? 3 : (lead >> 4) == 0xF ? 4 : 2;
        stringstream s;
        s << "Caractere inválido: \"" << text.substr(pos, len) << "\"";
        errors.push_back({t->getLine(), t->getCharPositionInLine(), s.str()});
      }
      continue;
    }
    if (t->getType() != PortugolParser::T_IDENTIFICADOR) {
      continue;
    }
    const string &text = t->getText();
    bool hasLatim = false;
    for (string::size_type i = 0; i < text.size(); ++i) {
      if ((unsigned char)text[i] >= 0x80) {
        hasLatim = true;
        break;
      }
    }
    if (hasLatim) {
      stringstream s;
      s << "Identificador \"" << text << "\" não pode ter caracteres especiais";
      errors.push_back({t->getLine(), t->getCharPositionInLine(), s.str()});
    }
  }

  stable_sort(errors.begin(), errors.end(),
              [](const LexicalError &a, const LexicalError &b) {
                return a.line < b.line ||
                       (a.line == b.line && a.column < b.column);
              });
  for (const LexicalError &e : errors) {
    display->add(e.msg, (int)e.line);
  }
  // CommonToken::getText() decodes the text from the input stream on every
  // call, and the interpreter reads names and literals on every evaluation.
  for (Token *t : all) {
    if (t->getType() == Token::EOF)
      continue;
    if (auto wt = dynamic_cast<WritableToken *>(t))
      wt->setText(t->getText());
  }
  // fill() already positioned the stream, so a leading token that was just
  // hidden would still be the first one the parser sees.
  tokens.seek(0);
}
