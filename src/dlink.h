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
 * \file dlink.h
 * \brief This header deals with linking sheets together before they are run.
 */

#ifndef DLINK_H
#define DLINK_H

#include "dcfg.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/**
 * \enum _linkType
 * \brief Describes what type of object we want to link.
 *
 * \typedef enum _linkType LinkType
 */
typedef enum _linkType {
    LINK_DATA_STRING_LITERAL,           ///< A string literal.
    LINK_VARIABLE,                      ///< A variable that stores it's data
                                        ///< directly.
    LINK_VARIABLE_POINTER,              ///< A variable that stores it's data
                                        ///< indirectly, e.g. a String.
    LINK_VARIABLE_STRING_DEFAULT_VALUE, ///< The default value of a string
                                        ///< variable.
    LINK_FUNCTION,                      ///< A function.
    LINK_CFUNCTION,                     ///< A C function.
} LinkType;

/**
 * \struct _linkMeta
 * \brief A structure describing a link.
 *
 * \typedef struct _linkMeta LinkMeta
 */
typedef struct _linkMeta {
    const char *name; ///< The name of the object we are linking.

    void *meta; ///< A generic pointer to the metadata of the thing we are
                ///< linking to, e.g. it can be a poiner to a `SheetVariable`
                ///< or `SheetFunction`. If the value is `-1`, then we don't
                ///< know where the metadata is, so we will need to find out
                ///< when linking.

    char *_ptr; ///< If the thing we're linking to is in the sheet's own data
                ///< section, `_ptr` will store the index of the first byte
                ///< before linking. Or if it is a function, it will be the
                ///< index of the function in the text section instead.
                ///< Otherwise, it is `-1`, which implies that the thing we
                ///< want to link is in another castle... sorry, Mario.

    LinkType type; ///< The type of object we are linking.
} LinkMeta;

/**
 * \struct _linkMetaList
 * \brief A list of LinkMeta.
 *
 * \typedef struct _linkMetaList LinkMetaList
 */
typedef struct _linkMetaList {
    LinkMeta *list;
    size_t size;
} LinkMetaList;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn LinkMeta d_link_new_meta(LinkType type, const char *name, void *meta)
 * \brief Create a new LinkMeta structure.
 *
 * \return A new `LinkMeta` with the given parameters.
 *
 * \param type The type of object this link will point to.
 * \param name The name of the object this link will point to.
 * \param meta A pointer to the metadata of the object the link points to.
 */
DECISION_API LinkMeta d_link_new_meta(LinkType type, const char *name,
                                      void *meta);

/**
 * \fn LinkMetaList d_link_new_meta_list()
 * \brief Create an empty LinkMetaList.
 *
 * \return An empty LinkMetaList.
 */
DECISION_API LinkMetaList d_link_new_meta_list();

/**
 * \fn void d_link_meta_list_push(LinkMetaList *list, LinkMeta item)
 * \brief Add a LinkMeta item to a list.
 *
 * \param list The list to push the item onto.
 * \param item The item to push onto.
 */
DECISION_API void d_link_meta_list_push(LinkMetaList *list, LinkMeta item);

/**
 * \fn void d_link_free_list(LinkMetaList *list)
 * \brief Free a LinkMetaList object.
 *
 * \param list The list to free.
 */
DECISION_API void d_link_free_list(LinkMetaList *list);

/**
 * \fn void d_link_replace_fimmediate(char *ins, char *ptr)
 * \brief Change an instruction's full immediate to point somewhere.
 *
 * **NOTE:** If you don't like the fact that you can't run 32-bit Decision code
 * on 64-bit machines and vice versa, blame it on this function.
 *
 * \param ins A pointer to first byte of the instruction.
 * \param ptr The memory address for the instruction to load.
 */
DECISION_API void d_link_replace_fimmediate(char *ins, char *ptr);

/**
 * \fn void d_link_precalculate_ptr(Sheet *sheet)
 * \brief Precalculate the pointers to external variables and functions for
 * linking.
 *
 * \param sheet The sheet to precalculate the pointers for.
 */
DECISION_API void d_link_precalculate_ptr(struct _sheet *sheet);

/**
 * \fn void d_link_self(Sheet *sheet)
 * \brief Link a sheet's properties from itself to itself and included sheets.
 *
 * \param sheet The sheet to link.
 */
DECISION_API void d_link_self(struct _sheet *sheet);

/**
 * \fn void d_link_includes_recursive(Sheet *sheet)
 * \brief Recursively go through the tree of included sheets and link them all
 * with `d_link_self`.
 *
 * \param sheet The sheet to recurse from.
 */
DECISION_API void d_link_includes_recursive(struct _sheet *sheet);

/**
 * \fn void d_link_find_included(Sheet *sheet)
 * \brief If the sheet includes objects, but doesn't know where they live,
 * i.e. `linkMeta.meta == -1 && linkMeta._ptr == -1`, then find them and
 * reference them in the sheet.
 *
 * \param sheet The sheet to find the locations of includes.
 */
DECISION_API void d_link_find_included(struct _sheet *sheet);

/**
 * \fn void d_link_sheet(Sheet *sheet)
 * \brief Call `d_link_find_included`, `d_link_precalculate_ptr`, `d_link_self`,
 * and `d_link_includes_recursive` on a sheet.
 *
 * \param sheet The sheet to link.
 */
DECISION_API void d_link_sheet(struct _sheet *sheet);

#endif // DLINK_H