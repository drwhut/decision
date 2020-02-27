/*
    Decision
    Copyright (C) 2019-2020  Benjamin Beddows

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file dlex.h
 * \brief This header provides Lexical Analysis for Decision.
 */

#ifndef DLEX_H
#define DLEX_H

#include "dcfg.h"
#include <stdbool.h>

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/**
 * \enum _lexType
 * \brief An enum for all the types of lexical tokens.
 *
 * \typedef enum _lexType LexType
 */
typedef enum _lexType {

    TK_NAME, ///< The name token represents the name of all nodes, variables
             ///< and properties.

    // Keywords.
    TK_EXECUTIONTYPE, // = 1
    TK_INTEGERTYPE,   // = 2
    TK_FLOATTYPE,     // = 3
    TK_STRINGTYPE,    // = 4
    TK_BOOLEANTYPE,   // = 5

    // Literals
    TK_INTEGERLITERAL, // = 6
    TK_FLOATLITERAL,   // = 7
    TK_STRINGLITERAL,  // = 8
    TK_BOOLEANLITERAL, // = 9

    // Statement symbols
    TK_IDENTIFIER, // = 10
    TK_OUTPUT,     // = 11
    TK_LINE,       // = 12
    TK_COMMA,      // = 13
    TK_OR,         // = 14

    // End-of-statements
    TK_EOSNL, // = 15
    TK_EOSSC, // = 16

    // Beginnings and ends of collections
    TK_LBRACKET,  // = 17
    TK_LPROPERTY, // = 18
    TK_LARRAY,    // = 19
    TK_RBRACKET,  // = 20
    TK_RPROPERTY, // = 21
    TK_RARRAY     // = 22

} LexType;

/**
 * \def LEX_DATATYPE_START
 * \brief The starting token of the data types.
 */
#define LEX_DATATYPE_START TK_EXECUTIONTYPE

/**
 * \def LEX_DATATYPE_END
 * \brief The ending token of the data types.
 */
#define LEX_DATATYPE_END TK_BOOLEANTYPE

/**
 * \def LEX_VARTYPE_START
 * \brief The starting token of the variable types.
 */
#define LEX_VARTYPE_START TK_INTEGERTYPE

/**
 * \def LEX_VARTYPE_END
 * \brief The ending token of the variable types.
 */
#define LEX_VARTYPE_END TK_BOOLEANTYPE

/**
 * \def LEX_LITERAL_START
 * \brief The starting token of the literal types.
 */
#define LEX_LITERAL_START TK_INTEGERLITERAL

/**
 * \def LEX_LITERAL_END
 * \brief The ending token of the literal types.
 */
#define LEX_LITERAL_END TK_BOOLEANLITERAL

/**
 * \union _lexData
 * \brief A union for storing data in a lexical token.
 *
 * \typedef union _lexData LexData
 */
typedef union _lexData {
    dint integerValue;
    dfloat floatValue;
    bool booleanValue;
    char *stringValue;
} LexData;

/**
 * \struct _lexToken
 * \brief The sturcture of a lexical token.
 *
 * \typedef struct _lexToken LexToken
 */
typedef struct _lexToken {
    LexType type;
    LexData data;
} LexToken;

/**
 * \struct _lexStream
 * \brief The structure of a lexical stream.
 *
 * **NOTE:** It is actually implemented as an array.
 *
 * \typedef struct _lexStream LexStream
 */
typedef struct _lexStream {
    LexToken *tokenArray;
    size_t numTokens;
} LexStream;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn bool d_lex_is_alpha(char c)
 * \brief Checks if a character is a letter of the alphabet.
 *
 * \return If the character is a letter of the alphabet.
 *
 * \param c The character to query.
 */
DECISION_API bool d_lex_is_alpha(char c);

/**
 * \fn bool d_lex_is_digit(char c)
 * \brief Check if a character is a digit.
 *
 * \return If the character is a digit.
 *
 * \param c The character to query.
 */
DECISION_API bool d_lex_is_digit(char c);

/**
 * \fn bool d_lex_is_hex(char c)
 * \brief Check if a character is a hexadecimal digit.
 *
 * \return If the character is a hexadecimal digit.
 *
 * \param c The character to query.
 */
DECISION_API bool d_lex_is_hex(char c);

/**
 * \fn bool d_lex_is_octal(char c)
 * \brief Check if a character is an octal digit.
 *
 * \return If the character is an octal digit.
 *
 * \param c The character to query.
 */
DECISION_API bool d_lex_is_octal(char c);

/**
 * \fn bool d_lex_is_name_char(char c)
 * \brief Check if a character is a valid name character.
 *
 * \return If the character is a valid name character.
 *
 * \param c The character to query.
 */
DECISION_API bool d_lex_is_name_char(char c);

/**
 * \fn const char *d_lex_get_string_literal(const char *source, size_t *i,
 *                                          const char *filePath,
 *                                          size_t lineNum)
 * \brief For some source text, starting at the character at index `*i`, extract
 * a string literal.
 *
 * The first character (i.e. `source[*i]`) must be a string quote, or an error
 * will occur.
 *
 * \return A malloc'd string. NULL if the string is errorneous.
 *
 * \param source The source text.
 * \param i A pointer to an index, where the start of a string literal should
 * be. It will be set to just after where the literal ends when the function
 * returns.
 * \param filePath In the event we error, say what the file path was.
 * \param lineNum In the event we error, say what line it was on.
 */
DECISION_API const char *d_lex_get_string_literal(const char *source, size_t *i,
                                                  const char *filePath,
                                                  size_t lineNum);

/**
 * \fn const char *d_lex_get_name(const char *source, size_t *i,
 *                                const char *filePath, size_t lineNum)
 * \brief For some source text, starting at the character at index `*i`, extract
 * a name.
 *
 * It must follow the syntax rules for names.
 *
 * \return A malloc'd string representing the name. NULL if the name is
 * errorneous.
 *
 * \param source The source text.
 * \param i A pointer to an index, where the start of a name should be. It will
 * be set to just after where the name ends when the function returns.
 * \param filePath In the event we error, say what the file path was.
 * \param lineNum In the event we error, say what line it was on.
 */
DECISION_API const char *d_lex_get_name(const char *source, size_t *i,
                                        const char *filePath, size_t lineNum);

/**
 * \fn LexStream d_lex_create_stream(const char *source, const char *filePath)
 * \brief Create a stream of lexical tokens from some source text that you can
 * extract tokens from.
 *
 * **NOTE:** The source code needs to have a newline (`\n`) as it's last
 * non-NULL character!
 *
 * \return A new malloc'd `LexStream`. If there was an error, `numTokens = 0`
 * and `tokenArray = NULL`.
 *
 * \param source The source text. Can be text read from a source file.
 * \param filePath In case we error, say what the file path was.
 */
DECISION_API LexStream d_lex_create_stream(const char *source,
                                           const char *filePath);

/**
 * \fn void d_lex_free_stream(LexStream stream)
 * \brief Frees the token array from a stream. The stream should not be used
 * after this function is called on it.
 *
 * \param stream The stream to free from memory.
 */
DECISION_API void d_lex_free_stream(LexStream stream);

/**
 * \fn void d_lex_dump_stream(LexStream stream)
 * \brief Print the contents of a `LexStream`.
 *
 * \param stream The stream to print to the screen.
 */
DECISION_API void d_lex_dump_stream(LexStream stream);

#endif // DLEX_H
