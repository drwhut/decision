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

#include "dlex.h"

#include "decision.h"
#include "derror.h"
#include "dmalloc.h"

#include <stdlib.h>
#include <string.h>

/**
 * \fn bool d_lex_is_alpha(char c)
 * \brief Checks if a character is a letter of the alphabet.
 *
 * \return If the character is a letter of the alphabet.
 *
 * \param c The character to query.
 */
bool d_lex_is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/**
 * \fn bool d_lex_is_digit(char c)
 * \brief Check if a character is a digit.
 *
 * \return If the character is a digit.
 *
 * \param c The character to query.
 */
bool d_lex_is_digit(char c) {
    return (c >= '0' && c <= '9');
}

/**
 * \fn bool d_lex_is_hex(char c)
 * \brief Check if a character is a hexadecimal digit.
 *
 * \return If the character is a hexadecimal digit.
 *
 * \param c The character to query.
 */
bool d_lex_is_hex(char c) {
    return d_lex_is_digit(c) || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

/**
 * \fn bool d_lex_is_octal(char c)
 * \brief Check if a character is an octal digit.
 *
 * \return If the character is an octal digit.
 *
 * \param c The character to query.
 */
bool d_lex_is_octal(char c) {
    return (c >= '0' && c <= '7');
}

/**
 * \fn bool d_lex_is_name_char(char c)
 * \brief Check if a character is a valid name character.
 *
 * \return If the character is a valid name character.
 *
 * \param c The character to query.
 */
bool d_lex_is_name_char(char c) {
    return d_lex_is_alpha(c) || d_lex_is_digit(c) || (c == '_');
}

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
const char *d_lex_get_string_literal(const char *source, size_t *i,
                                     const char *filePath, size_t lineNum) {
    // Is the first character a quote?
    if (!(source[*i] == '"' || source[*i] == '\'')) {
        d_error_compiler_push("String does not start with a quote \" or '",
                              filePath, lineNum, true);
        return NULL;
    }

    const size_t len = strlen(source);

    char *out;

    bool endFound = false;
    size_t lenStr = 0;
    size_t oldi   = *i;

    // Finding the length of the string.
    for (size_t j = *i + 1; j < len; j++) {
        // We need to deal with escape characters!
        if (source[j] == '\\') {
            lenStr++;
            j++;
        }
        // Have we reached the end?
        else if (source[j] == source[*i]) {
            endFound = true;
            *i       = j;
            break;
        } else {
            lenStr++;
        }
    }

    if (!endFound) {
        d_error_compiler_push("End of string not found", filePath, lineNum,
                              true);
        return NULL;
    }

    // Now we are building the string.
    out = (char *)d_malloc((lenStr + 1) * sizeof(char));

    size_t pos = oldi + 1;

    for (size_t c = 0; c < lenStr; c++) {
        char original = source[pos];

        if (original == '\\') {
            char escapeChar;

            switch (source[c + oldi + 2]) {
                case '\'':
                    escapeChar = '\'';
                    break;
                case '"':
                    escapeChar = '"';
                    break;
                case '\\':
                    escapeChar = '\\';
                    break;
                case 'n':
                    escapeChar = '\n';
                    break;
                case 'r':
                    escapeChar = '\r';
                    break;
                case 't':
                    escapeChar = '\t';
                    break;
                case 'b':
                    escapeChar = '\b';
                    break;
                case 'f':
                    escapeChar = '\f';
                    break;
                case 'v':
                    escapeChar = '\v';
                    break;
                case '0':
                    escapeChar = '\0';
                    break;
                default:
                    escapeChar = '\\';

                    ERROR_COMPILER(filePath, lineNum, true,
                                   "Unidentified escape character %u",
                                   escapeChar);
                    break;
            }

            out[c] = escapeChar;
            pos++;
        } else {
            out[c] = original;
        }

        pos++;
    }

    out[lenStr] = '\0';

    return out;
}

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
const char *d_lex_get_name(const char *source, size_t *i, const char *filePath,
                           size_t lineNum) {
    // A name can only start with a letter or underscore.
    if (!(d_lex_is_alpha(source[*i]) || source[*i] == '_')) {
        d_error_compiler_push(
            "Names can only start with letters or underscores", filePath,
            lineNum, true);
        return NULL;
    }

    size_t len = strlen(source);

    char *out;

    bool endFound = false;
    long lenStr   = 1;
    size_t oldi   = *i;

    // Finding the length of the string.
    for (size_t j = *i + 1; j < len; j++) {
        if (d_lex_is_name_char(source[j])) {
            lenStr++;
        }
        // Have we reached the end?
        else {
            endFound = true;
            *i       = j - 1;
            break;
        }
    }

    if (!endFound) {
        d_error_compiler_push("End of string not found", filePath, lineNum,
                              true);
        return NULL;
    }

    // Now we are building the string.
    out = (char *)d_malloc(((size_t)lenStr + 1) * sizeof(char));

    memcpy(out, source + oldi, lenStr);
    out[lenStr] = '\0';

    return out;
}

/*
    Macro used to add currentToken to the stream.
    It sets the data, and if the token type is defined, adds it to the array.
*/
#define ADD_TOKEN()                  \
    currentToken.data = currentData; \
    if ((int)currentToken.type > -1) \
        lexArray[n++] = currentToken;

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
LexStream d_lex_create_stream(const char *source, const char *filePath) {
    size_t sourceLength = strlen(source);

    LexStream out;
    out.numTokens = sourceLength;

    // We set the size now to the length of the source code,
    // but we will later resize it to the actual amount of tokens.
    LexToken *lexArray = (LexToken *)d_malloc(sourceLength * sizeof(LexToken));

    if (lexArray != NULL) {
        size_t i       = 0; // The character we're on in the source.
        size_t n       = 0; // The number of tokens.
        size_t lineNum = 1;

        LexToken currentToken;
        LexData currentData;

        bool inComment = false;

        for (;;) {
            /* Reset Values */
            currentToken.type        = -1;
            currentData.integerValue = 0;

            // Always include newlines, even if we're in a comment.
            if (source[i] == '\n') {
                currentToken.type = TK_EOSNL;
                lineNum++;
            }

            else if (!inComment)
                switch (source[i]) {
                    case ' ': // Whitespace, ignore.
                        break;

                    case '<':; // Comment, ignore until we hit a '>'.
                        inComment = true;
                        break;

                    case '~': // OUTPUT
                        currentToken.type = TK_OUTPUT;
                        break;

                    case '#': // LINE
                        currentToken.type = TK_LINE;
                        break;

                    case ',': // COMMA
                        currentToken.type = TK_COMMA;
                        break;

                    case '|': // OR
                        currentToken.type = TK_OR;
                        break;

                    case '\r': // Windows carriage return, ignore.
                        break;

                    case ';': // EOS (semi colon)
                        currentToken.type = TK_EOSSC;
                        break;

                    case '(': // LBRACKET
                        currentToken.type = TK_LBRACKET;
                        break;

                    case '[': // LPROPERTY
                        currentToken.type = TK_LPROPERTY;
                        break;

                    case '{': // LARRAY
                        currentToken.type = TK_LARRAY;
                        break;

                    case ')': // RBRACKET
                        currentToken.type = TK_RBRACKET;
                        break;

                    case ']': // RPROPERTY
                        currentToken.type = TK_RPROPERTY;
                        break;

                    case '}': // RARRAY
                        currentToken.type = TK_RARRAY;
                        break;

                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '-':
                    case '+':
                    case '.': // Integer + Float
                        if (source[i] == '0' && source[i + 1] != '.') {
                            currentToken.type = TK_INTEGERLITERAL;

                            if (source[i + 1] == 'x') // Hex
                            {
                                char *endptr;
                                currentData.integerValue =
                                    strtol(&source[i], &endptr, 0);
                                i = i + (endptr - &source[i]) - 1;
                            } else // Octal
                            {
                                char *endptr;
                                currentData.integerValue =
                                    strtol(&source[i], &endptr, 8);
                                i = i + (endptr - &source[i]) - 1;
                            }
                        } else // Decimal
                        {
                            char numDots = (source[i] == '.');
                            size_t end   = i;

                            for (size_t j = i + 1; j < sourceLength; j++) {
                                if (d_lex_is_digit(source[j]))
                                    ;
                                else if (source[j] == '.') {
                                    if (++numDots > 1) {
                                        d_error_compiler_push(
                                            "Invalid number literal, too many "
                                            "'.' symbols",
                                            filePath, lineNum, true);
                                    }
                                } else {
                                    end = j - 1;
                                    break;
                                }
                            }

                            // Error-Checking.
                            if ((source[i] == '+' || source[i] == '-') &&
                                end == i) {
                                d_error_compiler_push("Invalid number literal, "
                                                      "sign with no magnitude",
                                                      filePath, lineNum, true);
                            }

                            // We can deduce if it's an integer or float based
                            // on the number of dots.
                            if (numDots == 1) {
                                currentToken.type = TK_FLOATLITERAL;
                                currentData.floatValue =
#ifdef DECISION_32
                                    strtof(&source[i], NULL);
#else
                                    strtod(&source[i], NULL);
#endif // DECISION_32
                            } else if (numDots == 0) {
                                currentToken.type = TK_INTEGERLITERAL;
                                currentData.integerValue =
                                    strtol(&source[i], NULL, 10);
                            }

                            // Skip past this bit, we're done here.
                            i = end;
                        }
                        break;

                    case '"':
                    case '\'':; // String
                        currentToken.type = TK_STRINGLITERAL;

                        currentData.stringValue =
                            (char *)d_lex_get_string_literal(source, &i,
                                                             filePath, lineNum);

                        break;

                    default:; // Names + Keywords + Booleans
                        char *name = (char *)d_lex_get_name(source, &i,
                                                            filePath, lineNum);

                        char isBool = 2; // 2 - Not a boolean literal, 0, 1 are
                                         // self-explanitory.

                        if (name != NULL) {
                            // Check to see if it is a keyword.
                            if (strcmp(name, "Execution") == 0)
                                currentToken.type = TK_EXECUTIONTYPE;
                            else if (strcmp(name, "Integer") == 0)
                                currentToken.type = TK_INTEGERTYPE;
                            else if (strcmp(name, "Float") == 0)
                                currentToken.type = TK_FLOATTYPE;
                            else if (strcmp(name, "String") == 0)
                                currentToken.type = TK_STRINGTYPE;
                            else if (strcmp(name, "Boolean") == 0)
                                currentToken.type = TK_BOOLEANTYPE;

                            // Check to see if it is a boolean value.
                            else if (strcmp(name, "true") == 0) {
                                currentToken.type = TK_BOOLEANLITERAL;
                                isBool            = 1;
                            } else if (strcmp(name, "false") == 0) {
                                currentToken.type = TK_BOOLEANLITERAL;
                                isBool            = 0;
                            }

                            // Otherwise, it's just a name.
                            else
                                currentToken.type = TK_NAME;

                            if (isBool == 2)
                                currentData.stringValue = name;
                            else
                                currentData.booleanValue = isBool;

                            // If it's not a keyword (i.e. we need to keep the
                            // name for something), we can just free the
                            // malloc'd string, since we know what it
                            // represents.
                            if (currentToken.type != TK_NAME)
                                free(name);
                        } else {
                            d_error_compiler_push("Unidentified character",
                                                  filePath, lineNum, true);
                        }

                        break;
                }

            ADD_TOKEN()

            // We are now out of a comment!
            if (source[i] == '>')
                inComment = false;

            if (++i >= sourceLength)
                break;
        }

        // Format output and reduce size of stream.
        if (n > 0) {
            lexArray = (LexToken *)d_realloc(lexArray, n * sizeof(LexToken));
        } else {
            free(lexArray);
            lexArray = NULL;
        }

        out.tokenArray = lexArray;
        out.numTokens  = n;

        return out;
    } else {
        out.numTokens = 0;
    }

    return out;
}

/**
 * \fn void d_lex_free_stream(LexStream stream)
 * \brief Frees the token array from a stream. The stream should not be used
 * after this function is called on it.
 *
 * \param stream The stream to free from memory.
 */
void d_lex_free_stream(LexStream stream) {
    if (stream.tokenArray != NULL) {
        free(stream.tokenArray);
    }
}

/**
 * \fn void d_lex_dump_stream(LexStream stream)
 * \brief Print the contents of a `LexStream`.
 *
 * \param stream The stream to print to the screen.
 */
void d_lex_dump_stream(LexStream stream) {
    printf("\nLEX STREAM DUMP\n===============\nn: %zu\ni\ttype\tdata\n",
           stream.numTokens);

    LexToken t;

    for (size_t i = 0; i < stream.numTokens; i++) {
        t = stream.tokenArray[i];
        printf("%zi\t%i\t", i, t.type);

        switch (t.type) {
            case TK_INTEGERLITERAL:
                printf("%" DINT_PRINTF_d, t.data.integerValue);
                break;
            case TK_FLOATLITERAL:
                printf("%f", t.data.floatValue);
                break;
            case TK_STRINGLITERAL:
            case TK_NAME:
                printf("%s (%p)", t.data.stringValue, t.data.stringValue);
                break;
            case TK_BOOLEANLITERAL:
                printf("%s", (t.data.booleanValue) ? "True" : "False");
                break;
            default:
                break;
        }

        printf("\n");
    }
}
