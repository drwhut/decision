/*
    Decision
    Copyright (C) 2019  Benjamin Beddows

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

/* A macro to help with syntax definitions. */
#define STXDEF(d) STX_##d

/* Macro for defining META-variables. */
#define META(f)                                                          \
    static SyntaxResult f(SyntaxContext *context, bool saveNode) {       \
        VERBOSE(5, "ENTER\t%s\tWITH\t%i\n", #f, context->currentType)    \
        SyntaxNode *definitionNode = NULL;                               \
        if (saveNode)                                                    \
            definitionNode =                                             \
                d_syntax_create_node(STXDEF(f), NULL, context->lineNum); \
        SyntaxResult out;                                                \
        out.node      = definitionNode;                                  \
        bool addChild = false;

#define END_META }

/* Macros for if meta variables got accepted or not. */
#define ACCEPT(f)                       \
    {                                   \
        VERBOSE(5, "SUCCESS\t%s\n", #f) \
        out.success = true;             \
        return out;                     \
    }

#define DECLINE(f)                   \
    {                                \
        VERBOSE(5, "FAIL\t%s\n", #f) \
        out.success = false;         \
        if (saveNode)                \
            free(definitionNode);    \
        return out;                  \
    }

/*
    Macros for whether we start or end adding children to the definition node.
    NOTE: If the parent doesn't want to add the current node as a child, then
    we don't bother adding children to this node, since they would never be in
    the final syntax tree.
*/
#define START_ADD addChild = true;
#define END_ADD   addChild = false;

/* Macro for getting the current token's type. */
#define currentType currentToken->type

/*
    Some handy macros for switch statements.
    NOTE: Only use switch for tokens, NOT definitions.
 */
#define START_SWITCH switch (context->currentType) {

#define SWITCH_IF(t) case t:

#define SWITCH_THEN(f)                                                         \
    if (saveNode && addChild) {                                                \
        SyntaxNode *n = d_syntax_create_node(STX_TOKEN, context->currentToken, \
                                             context->lineNum);                \
        d_syntax_add_child(definitionNode, n);                                 \
    }                                                                          \
    nextToken(context);                                                        \
    ACCEPT(f);

#define END_SWITCH(f) \
    default:          \
        DECLINE(f);   \
        }

/* A structure to keep track of tokens during the syntax check. */
typedef struct {
    LexStream lexicalStream;
    LexToken *currentToken;
    long tokenIndex;
    size_t numTokens;
    size_t lineNum;
    const char *currentFilePath;
} SyntaxContext;

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
    SyntaxNode *n = (SyntaxNode *)d_malloc(sizeof(SyntaxNode));

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
    SyntaxNode **found =
        (SyntaxNode **)d_malloc(currentSize * sizeof(SyntaxNode *));

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
                found = (SyntaxNode **)d_realloc(
                    found, currentSize * sizeof(SyntaxNode **));
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
    found = (SyntaxNode **)d_realloc(found, (n + 1) * sizeof(SyntaxNode *));
    out.numOccurances = n;
    out.occurances    = found;
    out.occurances[n] = 0;

    return out;
}

/*
=== PARSER FUNCTIONS ============================
*/

/*
    void nextToken(SyntaxContext* context)
    Get the next lexical token.

    SyntaxContext* context: The context to use to get the next token.
*/
static void nextToken(SyntaxContext *context) {
    context->tokenIndex++;

    if (!context->currentToken)
        context->currentToken = context->lexicalStream.tokenArray;

    else if (context->tokenIndex >= (long)context->numTokens)
        context->currentType = -1;

    else
        context->currentToken++;

    if (context->currentToken->type == TK_EOSNL)
        context->lineNum++;
}

/*
    void error(const char* message)
    Take the blue pill. Forget anything happened.

    const char* message: The message to display.
    SyntaxContext* context: The context that contains the info needed to
    display a meaningful error.
*/
static void error(const char *message, SyntaxContext *context) {
    d_error_compiler_push(message, context->currentFilePath, context->lineNum,
                          true);
}

/* A macro version of the above error message that calls ERROR_COMPILER. */
#define SYNTAX_ERROR(contextPtr, ...)                                          \
    ERROR_COMPILER((contextPtr)->currentFilePath, (contextPtr)->lineNum, true, \
                   __VA_ARGS__)

/*
    bool accept_t(
        LexType t,
        SyntaxNode* root,
        bool addChild,
        SyntaxContext* context
    )

    Returns: If the token was the one we wanted.

    LexType t: The token type we want.
    SyntaxNode* root: The parent of the child we may want to create.
    bool addChild: Do we want to add a child if this succeeds?
    SyntaxContext* context: The context to check the type against.
*/
static bool accept_t(LexType t, SyntaxNode *root, bool addChild,
                     SyntaxContext *context) {
    if (context->currentType == t) {
        if (addChild) {
            SyntaxNode *n = d_syntax_create_node(
                STX_TOKEN, context->currentToken, context->lineNum);

            d_syntax_add_child(root, n);
        }

        nextToken(context);
        return true;
    }

    return false;
}

/*
    void expect_t(LexType t, const char* message, SyntaxNode* root, bool
   addChild) If we don't get what we want, EXPLODE EVERYTHING.

    LexType t: The token type we want to see.
    const char message*: The error message to show if the token isn't the type
   we want. SyntaxNode* root: The parent of the child we may want to create.
    bool addChild: Do we want to add a child if this succeeds?
    SyntaxContext* context: The context to check the type against.
*/
static void expect_t(LexType t, const char *message, SyntaxNode *root,
                     bool addChild, SyntaxContext *context) {
    if (accept_t(t, root, addChild, context))
        return;
    error(message, context);
}

/* A macro version of expect_t which calls SYNTAX_ERROR. */
#define EXPECT_T(t, root, addChild, context, ...)     \
    if (accept_t((t), (root), (addChild), (context))) \
        return;                                       \
    SYNTAX_ERROR(context, __VA_ARGS__);

/*
    Same functions as above, but accept and expect definitions instead of
   tokens.
*/
static bool accept_d(SyntaxResult (*f)(SyntaxContext *, bool), SyntaxNode *root,
                     bool addChild, SyntaxContext *context) {
    SyntaxResult r = f(context, addChild);

    if (addChild && r.success) {
        d_syntax_add_child(root, r.node);
    }

    return r.success;
}

static void expect_d(SyntaxResult (*f)(SyntaxContext *, bool),
                     const char *message, SyntaxNode *root, bool addChild,
                     SyntaxContext *context) {
    if (accept_d(f, root, addChild, context))
        return;
    error(message, context);
}

/* A macro version of expect_d which calls SYNTAX_ERROR. */
#define EXPECT_D(f, root, addChild, context, ...)     \
    if (accept_d((f), (root), (addChild), (context))) \
        return;                                       \
    SYNTAX_ERROR((context), __VA_ARGS__);

/* Macros of the above functions using the global variables. */
#define acceptToken(t) accept_t(t, definitionNode, saveNode &&addChild, context)
#define expectToken(t, s) \
    expect_t(t, s, definitionNode, saveNode &&addChild, context)
#define expectTokenDetailed(t, ...) \
    EXPECT_T(t, definitionNode, saveNode &&addChild, context, __VA_ARGS__)
#define acceptDefinition(f) \
    accept_d(f, definitionNode, saveNode &&addChild, context)
#define expectDefinition(f, s) \
    expect_d(f, s, definitionNode, saveNode &&addChild, context)
#define expectDefinitionDetailed(f, ...) \
    EXPECT_D(f, definitionNode, saveNode &&addChild, context, __VA_ARGS__)

/*
=== LANGUAGE DEFINITIONS ========================

* Notation is in BNF form.

* Meta-Variables beginning in an upper-case are
  represented as tokens in code.

* Lower-case is represented as functions.

*/

/* <lineIdentifier> ::= <Line><IntegerLiteral> */
META(lineIdentifier)

if (acceptToken(TK_LINE)) {
    START_ADD
    expectToken(TK_INTEGERLITERAL,
                "Integer expected after line declaration (#)");
    END_ADD

    ACCEPT(lineIdentifier);
} else {
    DECLINE(lineIdentifier);
}

END_META

/* <listOfLineIdentifier> ::=
 * <lineIdentifier>|<lineIdentifier><Comma><lineIdentifier>|... */
META(listOfLineIdentifier)

START_ADD
if (acceptDefinition(lineIdentifier)) {
    END_ADD
    if (acceptToken(TK_COMMA)) {
        do {
            START_ADD
            expectDefinition(lineIdentifier,
                             "Expected line identifier after comma (,)");
            END_ADD

        } while (acceptToken(TK_COMMA));
    }

    ACCEPT(listOfLineIdentifier);
} else {
    DECLINE(listOfLineIdentifier);
}

END_META

/* <dataType> ::= <IntegerType>|... */
META(dataType)

START_ADD

START_SWITCH

SWITCH_IF(TK_EXECUTIONTYPE)
SWITCH_IF(TK_INTEGERTYPE)
SWITCH_IF(TK_FLOATTYPE)
SWITCH_IF(TK_STRINGTYPE)
SWITCH_IF(TK_BOOLEANTYPE)

SWITCH_THEN(dataType)

END_SWITCH(dataType)

END_META

/* <literal> ::=
 * <IntegerLiteral>|<FloatLiteral>|<StringLiteral>|<BooleanLiteral> */
META(literal)

START_ADD

START_SWITCH

SWITCH_IF(TK_INTEGERLITERAL)
SWITCH_IF(TK_FLOATLITERAL)
SWITCH_IF(TK_STRINGLITERAL)
SWITCH_IF(TK_BOOLEANLITERAL)

SWITCH_THEN(literal)

END_SWITCH(literal)

END_META

/* <oneeos> ::= <EosNL>|<EosSC> */
META(oneeos)

START_SWITCH

SWITCH_IF(TK_EOSNL)
SWITCH_IF(TK_EOSSC)

SWITCH_THEN(oneeos)

END_SWITCH(oneeos)

END_META

/* <eos> ::= <oneeos>|<oneeos><oneeos>|... */
META(eos)

if (acceptDefinition(oneeos)) {
    while (acceptDefinition(oneeos)) {
    }
    ACCEPT(eos);
} else {
    DECLINE(eos);
}

END_META

/* <argument> ::= <Name>|<literal>|<lineIdentifier> */
META(argument)

START_ADD

if (acceptToken(TK_NAME) || acceptDefinition(literal) ||
    acceptDefinition(lineIdentifier)) {
    ACCEPT(argument);
} else {
    DECLINE(argument);
}

END_META

/* <propertyArgument> ::= <Name>|<literal>|<listOfDataType> */
META(propertyArgument)

START_ADD

if (acceptToken(TK_NAME) || acceptDefinition(literal) ||
    acceptDefinition(dataType)) {
    ACCEPT(propertyArgument);
} else {
    DECLINE(propertyArgument);
}

END_META

/* <listOfArguments> ::= <argument>|<argument><Comma><argument>|... */
META(listOfArguments)

START_ADD

if (acceptDefinition(argument)) {
    END_ADD

    if (acceptToken(TK_COMMA)) {
        do {
            START_ADD
            expectDefinition(
                argument, "Argument is not a name, literal or line identifier");
            END_ADD

        } while (acceptToken(TK_COMMA));
    }

    ACCEPT(listOfArguments);

} else {
    DECLINE(listOfArguments);
}

END_META

/* <listOfPropertyArguments> ::=
 * <propertyArgument>|<propertyArgument><Comma><propertyArgument>|... */
META(listOfPropertyArguments)

START_ADD

if (acceptDefinition(propertyArgument)) {
    END_ADD

    if (acceptToken(TK_COMMA)) {
        do {
            START_ADD
            expectDefinition(
                propertyArgument,
                "Property argument is not a name, literal, or data type");
            END_ADD

        } while (acceptToken(TK_COMMA));
    }

    ACCEPT(listOfPropertyArguments);

} else {
    DECLINE(listOfPropertyArguments);
}

END_META

/* <call> ::= <Lbracket><listOfArguments><Rbracket> */
META(call)

if (acceptToken(TK_LBRACKET)) {
    START_ADD
    acceptDefinition(listOfArguments); // There dosen't nessesarily have to be
                                       // anything in the backets.
    END_ADD

    expectToken(TK_RBRACKET, "Expected ) symbol at end of call");
    ACCEPT(call);
} else {
    DECLINE(call);
}

END_META

/* <propertyCall> ::= <Lbracket><listOfPropertyArguments><Rbracket> */
META(propertyCall)

if (acceptToken(TK_LBRACKET)) {
    START_ADD
    acceptDefinition(listOfPropertyArguments); // There dosen't nessesarily have
                                               // to be anything in the backets.
    END_ADD

    expectToken(TK_RBRACKET, "Expected ) symbol at end of call");
    ACCEPT(propertyCall);
} else {
    DECLINE(propertyCall);
}

END_META

/* <expression> ::= <Name>|<Name><call> */
META(expression)

START_ADD

if (acceptToken(TK_NAME)) {
    acceptDefinition(call);
    ACCEPT(expression);
} else {
    DECLINE(expression);
}

END_META

/* <propertyExpression> ::= <Name>|<Name><propertyCall> */
META(propertyExpression)

START_ADD

if (acceptToken(TK_NAME)) {
    acceptDefinition(propertyCall);
    ACCEPT(propertyExpression);
} else {
    DECLINE(propertyExpression);
}

END_META

/* <statement> ::=
 * <expression><eos>|<expression><Output><listofLineIdentifier><eos> */
META(statement)

START_ADD

if (acceptDefinition(expression)) {
    END_ADD

    if (acceptDefinition(eos)) {
        ACCEPT(statement);
    }

    else if (acceptToken(TK_OUTPUT)) {
        START_ADD
        expectDefinition(listOfLineIdentifier,
                         "Expected line identifier(s) after ~ symbol");
        END_ADD

        expectDefinition(eos, "Expected end-of-statement symbol (either "
                              "newline or semi-colon ;) after statement");

        ACCEPT(statement);
    }

    else {
        error("Expected end-of-statement symbol (either newline or semi-colon "
              ";) or ~ symbol after expression, got neither",
              context);
        DECLINE(statement);
    }
} else {
    DECLINE(statement);
}

END_META

/* <propertyStatement> ::= <Lproperty><propertyExpression><Rproperty><eos> */
META(propertyStatement)

if (acceptToken(TK_LPROPERTY)) {
    START_ADD
    expectDefinition(propertyExpression,
                     "Expected property expression inside property statement");
    END_ADD

    expectToken(TK_RPROPERTY, "Expected ] symbol at end of property");
    expectDefinition(eos, "Expected end-of-statement symbol (either newline or "
                          "semi-colon ;) at end of property");

    ACCEPT(propertyStatement);
} else {
    DECLINE(propertyStatement);
}

END_META

/* <generalStatement> ::= (<statement>|<propertyStatement>)(<eos> | NULL) */
META(generalStatement)

START_ADD

if (acceptDefinition(statement)) {
    END_ADD

    while (acceptDefinition(eos)) {
    }
    ACCEPT(generalStatement);
} else if (acceptDefinition(propertyStatement)) {
    END_ADD

    acceptDefinition(eos);
    ACCEPT(generalStatement);
} else {
    error("Statement is neither a general statement or a property statement",
          context);
    DECLINE(generalStatement);
}

END_META

/* <program> ::= (<eos> |
 * NULL)(<generalStatement>|<generalStatement><generalStatement>|...) */
META(program)

acceptDefinition(eos);

// If there's nothing here... well... can't really do much.
if (context->currentType == -1) {
    DECLINE(program);
}

START_ADD

if (acceptDefinition(generalStatement)) {
    do {
        // If we've reached the end of the lex stream, we need to stop.
        if (context->currentType == -1)
            break;

    } while (acceptDefinition(generalStatement));

    ACCEPT(program);
} else
    DECLINE(program);

END_META

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

    return program(&context, true);
}
