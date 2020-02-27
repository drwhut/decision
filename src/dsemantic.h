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
 * \file dsemantic.h
 * \brief This header checks that the program we're about to compile is correct.
 */

#ifndef DSEMANTIC_H
#define DSEMANTIC_H

#include "dcfg.h"
#include "dcfunc.h"
#include "dcore.h"
#include "dlex.h"
#include "dsheet.h"
#include "dsyntax.h"
#include "dtype.h"
#include <stdbool.h>

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/*
    Trust me, there was stuff here once...
    But then I made a circular dependency that the compiler didn't want.
    So I created dname.c and dname.h,
    And it saved the compiler and me from so much pain.
*/

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_semantic_scan_properties(Sheet *sheet, SyntaxNode *root,
 *                                     Sheet **priors, bool debugIncluded)
 * \brief Sets the properties of the sheet, given the syntax tree.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 * \param priors A NULL-terminated list of sheets that, if included, will
 * produce an error.
 * \param debugIncluded If true, compile included sheets in debug mode.
 */
DECISION_API void d_semantic_scan_properties(Sheet *sheet, SyntaxNode *root,
                                             Sheet **priors,
                                             bool debugIncluded);

/**
 * \fn void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root)
 * \brief Sets the nodes of the sheet, given the syntax tree.
 *
 * **NOTE:** This function also sets the connections between the nodes.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
DECISION_API void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root);

/**
 * \fn void d_semantic_reduce_types(Sheet *sheet)
 * \brief Take the connections of a sheet which may have "vague" connections,
 * and reduce them to unique types with the information we have.
 *
 * e.g. If `Multiply` has at least one `Float` input, the output must be a
 * `Float`.
 *
 * \param sheet The sheet to reduce the types on.
 */
DECISION_API void d_semantic_reduce_types(Sheet *sheet);

/**
 * \fn void d_semantic_detect_loops(Sheet *sheet)
 * \brief After a sheet has been connected in `d_semantic_scan_nodes`, go
 * through the sheet and see if there are any loops.
 *
 * "Loops are bad, and should be given coal at christmas."
 *
 * In retrospect, this was a bad quote. Instead, give it something that won't
 * cause climate change, like a really cheap sticker.
 *
 * While we are here, we also check to see if there are any redundant nodes.
 *
 * \param sheet The connected sheet to check for loops.
 */
DECISION_API void d_semantic_detect_loops(Sheet *sheet);

/**
 * \fn void d_semantic_scan(Sheet *sheet, SyntaxNode *root, Sheet **priors,
 *                          bool debugIncluded)
 * \brief Perform Semantic Analysis on a syntax tree.
 *
 * \param sheet The sheet to put everything into.
 * \param root The *valid* syntax tree to scan everything from.
 * \param priors A NULL-terminated list of sheets that, if included, will
 * produce an error. This is to prevent circular includes.
 * \param debugIncluded If true, compile any included sheets in debug mode.
 */
DECISION_API void d_semantic_scan(Sheet *sheet, SyntaxNode *root,
                                  Sheet **priors, bool debugIncluded);

#endif // DSEMANTIC_H
