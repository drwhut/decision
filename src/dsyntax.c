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

#include "dsyntax.h"

#include "decision.h"
#include "derror.h"
#include "dlex.h"
#include "dmalloc.h"

#include <stddef.h>
#include <stdlib.h>

/*
=== SYNTAX TREE FUNCTIONS =======================
*/

/**
 * \fn SyntaxNode *d_syntax_create_node(SyntaxDefinition d, LexToken *info,
 *                                      size_t line)
 * \brief Create a malloc'd syntax node with default nulled family pointers.
 *
 * \return A pointer to a malloc'd `SyntaxNode`.
 *
 * \param d What definition does this syntax node have?
 * \param info The corresponding lexical token, if any.
 * \param line The line number this syntax definition appears in.
 */
SyntaxNode *d_syntax_create_node(SyntaxDefinition d, LexToken *info,
                                 size_t line) {
    SyntaxNode *n = d_malloc(sizeof(SyntaxNode));

    n->definition = d;
    n->info       = info;
    n->child      = NULL;
    n->sibling    = NULL;
    n->onLineNum  = line;

    return n;
}

/**
 * \fn SyntaxNode *d_syntax_last_sibling(SyntaxNode *node)
 * \brief Get the pointer to the last of a chain of children with the same
 * common parent.
 *
 * \return The last sibling.
 *
 * \param node The node to find the last sibling of.
 */
SyntaxNode *d_syntax_last_sibling(SyntaxNode *node) {
    SyntaxNode *last = node;

    for (;;) {
        // If this node has no sibling, it's the last one.
        if (last->sibling == NULL)
            return last;
        else
            last = last->sibling;
    }
}

/**
 * \fn void d_syntax_add_child(SyntaxNode *parent, SyntaxNode *child)
 * \brief Add a child node to a parent node.
 *
 * \param parent The parent node to attach the child onto.
 * \param child The child node to add onto the parent.
 */
void d_syntax_add_child(SyntaxNode *parent, SyntaxNode *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    // If the parent has no child, just add it straight away.
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        SyntaxNode *lastSibling = d_syntax_last_sibling(parent->child);

        // Add the new child as the last sibling's new sibling.
        lastSibling->sibling = child;
    }
}

/**
 * \fn size_t d_syntax_get_num_children(SyntaxNode *parent)
 * \brief Get how many children a node has.
 *
 * \return The number of children `parent` has.
 *
 * \param parent The node to query.
 */
size_t d_syntax_get_num_children(SyntaxNode *parent) {
    if (parent->child == NULL)
        return 0;
    else {
        SyntaxNode *currentChild = parent->child;

        size_t index = 0;

        for (;;) {
            currentChild = currentChild->sibling;

            if (currentChild == NULL)
                break;
            else
                index++;
        }

        return index + 1;
    }
}

/**
 * \fn SyntaxNode *d_syntax_get_child_by_index(SyntaxNode *parent, size_t index)
 * \brief Get a node's child from it's index.
 *
 * \return The `index`th child of the parent. `NULL` if the index is out of
 * range.
 *
 * \param parent The parent node to get the child from.
 * \param index The index of the child we want.
 */
SyntaxNode *d_syntax_get_child_by_index(SyntaxNode *parent, size_t index) {
    if (parent->child == NULL)
        return NULL;
    else {
        SyntaxNode *currentChild = parent->child;

        if (index == 0)
            return currentChild;
        else {
            for (size_t i = 1; i <= index; i++) {
                currentChild = currentChild->sibling;

                if (currentChild == NULL)
                    return NULL;
            }

            return currentChild;
        }
    }
}

/**
 * \fn SyntaxNode *d_syntax_get_child_by_definition(SyntaxNode *parent,
 *                                                  SyntaxDefinition definition)
 * \brief Find the first occurance of a child with a specific definition.
 *
 * \return The first child of parent with the given definition. NULL if there
 * is none.
 *
 * \param parent The parent to get the child from.
 * \param definition The definition we want from the child.
 */
SyntaxNode *d_syntax_get_child_by_definition(SyntaxNode *parent,
                                             SyntaxDefinition definition) {
    if (parent->child == NULL)
        return NULL;
    else {
        SyntaxNode *child = parent->child;

        do {

            if (child->definition == definition)
                return child;
            else
                child = child->sibling;

        } while (child != NULL);

        return NULL;
    }
}

/**
 * \fn SyntaxSearchResult d_syntax_get_all_nodes_with(
 *     SyntaxNode *root,
 *     SyntaxDefinition definition,
 *     bool traverseChildrenOfFound)
 * \brief Return all occurances of a tree with a given definition as a malloc'd
 * SyntaxSearchResult.
 *
 * \return A malloc'd SyntaxSearchResult.
 *
 * \param root The root node to search from.
 * \param definition The definition we want our found nodes to have.
 * \param traverseChildrenOfFound If we find a node that we want, should w
 * also traverse the children of that found node?
 */
SyntaxSearchResult d_syntax_get_all_nodes_with(SyntaxNode *root,
                                               SyntaxDefinition definition,
                                               bool traverseChildrenOfFound) {
    SyntaxSearchResult out;
    size_t n = 0;

    size_t currentSize = 16;
    SyntaxNode **found = d_calloc(currentSize, sizeof(SyntaxNode *));

    // Iterative traversal.
    SyntaxNode *nodeStack[32];
    short nodeStackPtr = -1;

    nodeStack[++nodeStackPtr] = root;

    // While the stack is not empty.
    while (nodeStackPtr >= 0) {
        // Visit node on top of stack.
        SyntaxNode *top = nodeStack[nodeStackPtr--];

        bool isDefinition = false;

        // Is this node what we are looking after?
        if (top->definition == definition) {
            isDefinition = true;

            // Resize if the array is too small!
            if (n + 1 > currentSize) {
                currentSize++;
                found = d_realloc(found, currentSize * sizeof(SyntaxNode **));
            }

            found[n++] = top;
        }

        // Visit sibling.
        if (top->sibling != NULL)
            nodeStack[++nodeStackPtr] = top->sibling;

        // Visit child. (Has piority over sibling so we can
        // say that the nodes are "in order".
        if (top->child != NULL && (!isDefinition || traverseChildrenOfFound))
            nodeStack[++nodeStackPtr] = top->child;
    }

    // Cleaning up and outputting.
    found             = d_realloc(found, (n + 1) * sizeof(SyntaxNode *));
    out.numOccurances = n;
    out.occurances    = found;
    out.occurances[n] = 0;

    return out;
}

/*
=== PARSER FUNCTIONS ============================
*/

/* A structure to keep track of tokens during the syntax check. */
typedef struct {
    LexStream lexicalStream;
    LexToken *currentToken;
    int tokenIndex;
    size_t numTokens;
    size_t lineNum;
    const char *currentFilePath;
} SyntaxContext;

/*
    void nextToken(SyntaxContext* context)
    Get the next lexical token.

    SyntaxContext* context: The context to use to get the next token.
*/
static void nextToken(SyntaxContext *context) {
    context->tokenIndex++;

    if (!context->currentToken) {
        context->currentToken = context->lexicalStream.tokenArray;
    } else if (context->tokenIndex >= (long)context->numTokens) {
        context->currentToken->type = -1;
    } else {
        context->currentToken++;
    }

    if (context->currentToken->type == TK_EOSNL) {
        context->lineNum++;
    }
}

/*
    void syntax_error(const char* message)
    Take the blue pill. Forget anything happened.

    const char* message: The message to display.
    SyntaxContext* context: The context that contains the info needed to
    display a meaningful error.
*/
static void syntax_error(const char *message, SyntaxContext *context) {
    d_error_compiler_push(message, context->currentFilePath, context->lineNum,
                          true);
}

/* A macro version of the above error message that calls ERROR_COMPILER. */
#define SYNTAX_ERROR(contextPtr, ...)                                          \
    ERROR_COMPILER((contextPtr)->currentFilePath, (contextPtr)->lineNum, true, \
                   __VA_ARGS__)

/* What if a given definition fails? */
static void fail_definition(SyntaxResult *out) {
    out->success = false;

    if (out->node != NULL) {
        d_syntax_free_tree(out->node);
        out->node = NULL;
    }
}

/*
=== LANGUAGE DEFINITIONS ========================

* Notation is in BNF form.

* Meta-Variables beginning in an upper-case are represented as tokens in code.

* Lower-case is represented as functions.

* NOTE: The grammar is technically a LL(1) grammar, meaning that at any given
  time, you only need to look ahead one token to know what node you're making.
  Because of this, we first check the current token before proceeding with the
  definition every time.

*/

/* <lineIdentifier> ::= <Line><IntegerLiteral> */
static SyntaxResult lineIdentifier(SyntaxContext *context) {
    SyntaxResult out;
    out.node = d_syntax_create_node(STX_lineIdentifier, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tlineIdentifier\tWITH\t%i\n",
            context->currentToken->type);

    if (context->currentToken->type == TK_LINE) {
        nextToken(context);

        if (context->currentToken->type == TK_INTEGERLITERAL) {

            SyntaxNode *literal = d_syntax_create_node(
                STX_TOKEN, context->currentToken, context->lineNum);
            d_syntax_add_child(out.node, literal);

            nextToken(context);

        } else {
            syntax_error(
                "Expected integer literal to follow the line symbol (#)",
                context);
            fail_definition(&out);
        }
    } else {
        syntax_error(
            "Expected line identifier to start with the line symbol (#)",
            context);
        fail_definition(&out);
    }

    return out;
}

/* <listOfLineIdentifier> ::= <lineIdentifier>(<Comma><lineIdentifier>)* */
static SyntaxResult listOfLineIdentifier(SyntaxContext *context) {
    SyntaxResult out;
    out.node =
        d_syntax_create_node(STX_listOfLineIdentifier, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tlistOfLineIdentifier\tWITH\t%i\n",
            context->currentToken->type);

    if (context->currentToken->type == TK_LINE) {

        SyntaxResult identifier = lineIdentifier(context);

        if (identifier.success) {

            d_syntax_add_child(out.node, identifier.node);

            // Great, we got at least one line identifier! Now, as long as the
            // current token is a comma, a line identifier should follow.
            while (context->currentToken->type == TK_COMMA) {
                nextToken(context);

                identifier = lineIdentifier(context);

                if (identifier.success) {
                    d_syntax_add_child(out.node, identifier.node);
                } else {
                    syntax_error("Expected line identifier to follow comma (,)",
                                 context);
                    fail_definition(&out);
                    break;
                }
            }

        } else {
            syntax_error("Expected list of line identifiers to start with a "
                         "line identifier",
                         context);
            fail_definition(&out);
        }
    } else {
        syntax_error("Expected list of line identifiers to start with the line "
                     "symbol (#)",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <dataType> ::= <IntegerType>|... */
static bool is_data_type(LexType type) {
    return (type == TK_INTEGERTYPE || type == TK_FLOATTYPE ||
            type == TK_STRINGTYPE || type == TK_BOOLEANTYPE);
}

static SyntaxResult dataType(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_dataType, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tdataType\tWITH\t%i\n", context->currentToken->type);

    if (is_data_type(context->currentToken->type)) {
        SyntaxNode *type = d_syntax_create_node(
            STX_TOKEN, context->currentToken, context->lineNum);
        d_syntax_add_child(out.node, type);

        nextToken(context);
    } else {
        syntax_error("Expected a data type keyword", context);
        fail_definition(&out);
    }

    return out;
}

/* <literal> ::=
 * <IntegerLiteral>|<FloatLiteral>|<StringLiteral>|<BooleanLiteral> */
static bool is_literal(LexType type) {
    return (type == TK_INTEGERLITERAL || type == TK_FLOATLITERAL ||
            type == TK_STRINGLITERAL || type == TK_BOOLEANLITERAL);
}

static SyntaxResult literal(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_literal, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tliteral\tWITH\t%i\n", context->currentToken->type);

    if (is_literal(context->currentToken->type)) {
        SyntaxNode *literal = d_syntax_create_node(
            STX_TOKEN, context->currentToken, context->lineNum);
        d_syntax_add_child(out.node, literal);

        nextToken(context);
    } else {
        syntax_error("Expected a literal", context);
        fail_definition(&out);
    }

    return out;
}

/* <argument> ::= <Name>|<literal>|<lineIdentifier> */
static bool is_argument(LexType type) {
    return (type == TK_NAME || is_literal(type) || type == TK_LINE);
}

static SyntaxResult argument(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_argument, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\targument\tWITH\t%i\n", context->currentToken->type);

    if (context->currentToken->type == TK_NAME) {
        SyntaxNode *name = d_syntax_create_node(
            STX_TOKEN, context->currentToken, context->lineNum);
        d_syntax_add_child(out.node, name);

        nextToken(context);
    } else if (is_literal(context->currentToken->type)) {
        SyntaxResult lit = literal(context);

        if (lit.success) {
            d_syntax_add_child(out.node, lit.node);
        } else {
            syntax_error("Invalid literal argument", context);
            fail_definition(&out);
        }
    } else if (context->currentToken->type == TK_LINE) {
        SyntaxResult line = lineIdentifier(context);

        if (line.success) {
            d_syntax_add_child(out.node, line.node);
        } else {
            syntax_error("Invalid line identifier argument", context);
            fail_definition(&out);
        }
    } else {
        syntax_error("Invalid argument: not a name, literal or line identifier",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <propertyArgument> ::= <Name>|<literal>|<dataType> */
static bool is_property_argument(LexType type) {
    return (type == TK_NAME || is_literal(type) || is_data_type(type));
}

static SyntaxResult propertyArgument(SyntaxContext *context) {
    SyntaxResult out;
    out.node =
        d_syntax_create_node(STX_propertyArgument, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tpropertyArgument\tWITH\t%i\n",
            context->currentToken->type);

    if (context->currentToken->type == TK_NAME) {
        SyntaxNode *name = d_syntax_create_node(
            STX_TOKEN, context->currentToken, context->lineNum);
        d_syntax_add_child(out.node, name);

        nextToken(context);
    } else if (is_literal(context->currentToken->type)) {
        SyntaxResult lit = literal(context);

        if (lit.success) {
            d_syntax_add_child(out.node, lit.node);
        } else {
            syntax_error("Invalid literal property argument", context);
            fail_definition(&out);
        }
    } else if (is_data_type(context->currentToken->type)) {
        SyntaxResult type = dataType(context);

        if (type.success) {
            d_syntax_add_child(out.node, type.node);
        } else {
            syntax_error("Invalid data type property argument", context);
            fail_definition(&out);
        }
    } else {
        syntax_error("Invalid property argument: not a name, literal or data "
                     "type keyword",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <listOfArguments> ::= <argument>(<Comma><argument>)* */
static SyntaxResult listOfArguments(SyntaxContext *context) {
    SyntaxResult out;
    out.node =
        d_syntax_create_node(STX_listOfArguments, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tlistOfArguments\tWITH\t%i\n",
            context->currentToken->type);

    SyntaxResult arg = argument(context);

    if (arg.success) {

        d_syntax_add_child(out.node, arg.node);

        // This will be similar to the list of line identifiers.
        while (context->currentToken->type == TK_COMMA) {
            nextToken(context);

            arg = argument(context);

            if (arg.success) {
                d_syntax_add_child(out.node, arg.node);
            } else {
                syntax_error("Expected an argument to follow a comma (,)",
                             context);
                fail_definition(&out);
                break;
            }
        }
    } else {
        syntax_error("Expected an argument to start a list of arguments",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <listOfPropertyArguments> ::=
 * <propertyArgument>(<Comma><propertyArgument>)* */
static SyntaxResult listOfPropertyArguments(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_listOfPropertyArguments, NULL,
                                    context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tlistOfPropertyArguments\tWITH\t%i\n",
            context->currentToken->type);

    SyntaxResult arg = propertyArgument(context);

    if (arg.success) {

        d_syntax_add_child(out.node, arg.node);

        // This will be similar to the list of line identifiers.
        while (context->currentToken->type == TK_COMMA) {
            nextToken(context);

            arg = propertyArgument(context);

            if (arg.success) {
                d_syntax_add_child(out.node, arg.node);
            } else {
                syntax_error(
                    "Expected a property argument to follow a comma (,)",
                    context);
                fail_definition(&out);
                break;
            }
        }
    } else {
        syntax_error("Expected a property argument to start a list of property "
                     "arguments",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <eos> ::= (<EosNL>|<EosSC>)(<EosNL>|<EosSC>)* */
static bool is_eos(LexType type) {
    return (type == TK_EOSNL || type == TK_EOSSC);
}

// Ultimately we don't want to add eos nodes to the tree.
static bool eos(SyntaxContext *context) {
    if (is_eos(context->currentToken->type)) {

        nextToken(context);

        while (is_eos(context->currentToken->type)) {
            nextToken(context);
        }

        return true;
    } else {
        return false;
    }
}

/* <statement> ::=
 * <Name>((<Lbracket>(<listOfArguments>|NULL)<Rbracket>)|NULL)((<Output><listOfLineIdentifier>)|NULL)<eos>
 */

static SyntaxResult statement(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_statement, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tstatement\tWITH\t%i\n", context->currentToken->type);

    if (context->currentToken->type == TK_NAME) {
        SyntaxNode *name = d_syntax_create_node(
            STX_TOKEN, context->currentToken, context->lineNum);
        d_syntax_add_child(out.node, name);

        nextToken(context);

        if (context->currentToken->type == TK_LBRACKET) {
            nextToken(context);

            if (is_argument(context->currentToken->type)) {
                SyntaxResult argList = listOfArguments(context);

                if (argList.success) {
                    d_syntax_add_child(out.node, argList.node);
                } else {
                    syntax_error("Invalid list of arguments", context);
                    fail_definition(&out);
                }
            }

            if (context->currentToken->type == TK_RBRACKET) {
                nextToken(context);
            } else {
                syntax_error(
                    "Expected list of arguments to end with a right bracket",
                    context);
                fail_definition(&out);
            }
        }

        if (context->currentToken->type == TK_OUTPUT) {
            nextToken(context);

            SyntaxResult lineList = listOfLineIdentifier(context);

            if (lineList.success) {
                d_syntax_add_child(out.node, lineList.node);
            } else {
                syntax_error(
                    "Invalid list of line identifiers after output (~)",
                    context);
                fail_definition(&out);
            }
        }

        if (!eos(context)) {
            syntax_error(
                "Expected end-of-statement (\\n, ;) after the statement",
                context);
            fail_definition(&out);
        }
    } else {
        syntax_error("Expected statement to start with a name", context);
        fail_definition(&out);
    }

    return out;
}

/* <propertyStatement> ::=
 * <Lproperty><Name>(<Lbracket>(<listOfPropertyArguments>|NULL)<Rbracket>|NULL)<Rproperty><eos>
 */
static SyntaxResult propertyStatement(SyntaxContext *context) {
    SyntaxResult out;
    out.node =
        d_syntax_create_node(STX_propertyStatement, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tpropertyStatement\tWITH\t%i\n",
            context->currentToken->type);

    if (context->currentToken->type == TK_LPROPERTY) {
        nextToken(context);

        if (context->currentToken->type == TK_NAME) {
            SyntaxNode *name = d_syntax_create_node(
                STX_TOKEN, context->currentToken, context->lineNum);
            d_syntax_add_child(out.node, name);

            nextToken(context);

            if (context->currentToken->type == TK_LBRACKET) {
                nextToken(context);

                if (is_property_argument(context->currentToken->type)) {
                    SyntaxResult argList = listOfPropertyArguments(context);

                    if (argList.success) {
                        d_syntax_add_child(out.node, argList.node);
                    } else {
                        syntax_error("Invalid list of property arguments",
                                     context);
                        fail_definition(&out);
                    }
                }

                if (context->currentToken->type == TK_RBRACKET) {
                    nextToken(context);
                } else {
                    syntax_error("Expected list of property arguments to end "
                                 "with a right bracket",
                                 context);
                    fail_definition(&out);
                }
            }

            if (context->currentToken->type == TK_RPROPERTY) {
                nextToken(context);

                if (!eos(context)) {
                    syntax_error("Expected end-of-statement (\\n, ;) after the "
                                 "property statement",
                                 context);
                    fail_definition(&out);
                }
            } else {
                syntax_error("Expected property statement to end with a right "
                             "squared bracket (])",
                             context);
                fail_definition(&out);
            }
        } else {
            syntax_error("Expected property statement to start with a name",
                         context);
            fail_definition(&out);
        }
    } else {
        syntax_error("Expected property statement to start with a left squared "
                     "bracket ([)",
                     context);
        fail_definition(&out);
    }

    return out;
}

/* <program> ::= (NULL|<eos>)(<statement>|<propertyStatement>)* */
static bool is_statement(LexType type) {
    return (type == TK_NAME || type == TK_LPROPERTY);
}

static SyntaxResult program(SyntaxContext *context) {
    SyntaxResult out;
    out.node    = d_syntax_create_node(STX_program, NULL, context->lineNum);
    out.success = true;

    VERBOSE(5, "ENTER\tprogram\tWITH\t%i\n", context->currentToken->type);

    // If there are end-of-statements here, then ignore all of them.
    eos(context);

    // If there's nothing left... then... yeah, can't do much.
    if ((int)context->currentToken->type == -1) {
        return out;
    }

    while (is_statement(context->currentToken->type)) {
        SyntaxResult s;
        if (context->currentToken->type == TK_NAME) {
            s = statement(context);

            if (s.success) {
                d_syntax_add_child(out.node, s.node);
            } else {
                syntax_error("Invalid statement", context);
                fail_definition(&out);
                break;
            }
        } else if (context->currentToken->type == TK_LPROPERTY) {
            s = propertyStatement(context);

            if (s.success) {
                d_syntax_add_child(out.node, s.node);
            } else {
                syntax_error("Invalid property statement", context);
                fail_definition(&out);
                break;
            }
        }
    }

    // This SHOULD be the end of the token stream.
    if (out.success && (int)context->currentToken->type != -1) {
        syntax_error("Expected statement to start with a name or a left square "
                     "bracket ([) for a property",
                     context);
        fail_definition(&out);
    }

    return out;
}

/*
=== END OF LANGUAGE DEFINITIONS =================
*/

/**
 * \fn void d_syntax_dump_tree_raw(SyntaxNode *root, int n)
 * \brief Dump the contents of a tree recursively.
 *
 * \param root The root node to start from.
 * \param n The "level" of the recursion, so we can print the tree with proper
 * indentation.
 */
void d_syntax_dump_tree_raw(SyntaxNode *root, int n) {
    // Visit root.
    for (int i = 0; i < n - 1; i++)
        printf("   ");

    if (n != 0)
        printf("|");

    printf("%i ", root->definition);

    if (root->definition == STX_TOKEN) {
        printf("(%i)", root->info->type);
    }

    printf("\n");

    // Traverse child.
    if (root->child != NULL)
        d_syntax_dump_tree_raw(root->child, n + 1);

    // Traverse sibling.
    if (root->sibling != NULL)
        d_syntax_dump_tree_raw(root->sibling, n);
}

/**
 * \fn void d_syntax_dump_tree(SyntaxNode *root)
 * \brief Dump the contents of a tree recursively.
 *
 * \param root The root node to start from.
 */
void d_syntax_dump_tree(SyntaxNode *root) {
    printf("\nSYNTAX TREE DUMP\n================\n");

    d_syntax_dump_tree_raw(root, 0);
}

/**
 * \fn void d_syntax_free_tree(SyntaxNode *root)
 * \brief Free all the nodes from a syntax tree.
 *
 * \param root The root node of the tree.
 */
void d_syntax_free_tree(SyntaxNode *root) {
    // Traverse child
    if (root->child != NULL) {
        d_syntax_free_tree(root->child);
    }

    // Traverse sibling
    if (root->sibling != NULL) {
        d_syntax_free_tree(root->sibling);
    }

    // Free self.
    free(root);
}

/**
 * \fn void d_syntax_free_results(SyntaxSearchResult results)
 * \brief Free search results from memory.
 *
 * \param results The results to free.
 */
DECISION_API void d_syntax_free_results(SyntaxSearchResult results) {
    free(results.occurances);
}

/**
 * \fn SyntaxResult d_syntax_parse(LexStream stream, const char *filePath)
 * \brief Parse a lexical stream, and generate a syntax tree.
 *
 * \return The malloc'd root node of the syntax tree, and whether the parsing
 * was successful or not.
 *
 * \param stream The stream to parse from.
 * \param filePath In case we error, say what the file path was.
 */
SyntaxResult d_syntax_parse(LexStream stream, const char *filePath) {
    // Creating the syntax context structure.
    SyntaxContext context;
    context.lexicalStream   = stream;
    context.numTokens       = stream.numTokens;
    context.currentToken    = NULL;
    context.tokenIndex      = -1;
    context.lineNum         = 1;
    context.currentFilePath = filePath;

    // Set currentToken to the first token.
    nextToken(&context);

    return program(&context);
}
