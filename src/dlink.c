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

#include "dlink.h"

#include "dmalloc.h"
#include "dsheet.h"
#include "dvm.h"

#include <stdlib.h>
#include <string.h>

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
LinkMeta d_link_new_meta(LinkType type, const char *name, void *meta) {
    LinkMeta out;
    out.type = type;
    out.name = name;
    out.meta = meta;
    out._ptr = (char *)-1;

    return out;
}

/**
 * \fn LinkMetaList d_link_new_meta_list()
 * \brief Create an empty LinkMetaList.
 *
 * \return An empty LinkMetaList.
 */
LinkMetaList d_link_new_meta_list() {
    return (LinkMetaList){NULL, 0};
}

/**
 * \fn void d_link_meta_list_push(LinkMetaList *list, LinkMeta item)
 * \brief Add a LinkMeta item to a list.
 *
 * \param list The list to push the item onto.
 * \param item The item to push onto.
 */
void d_link_meta_list_push(LinkMetaList *list, LinkMeta item) {
    size_t newSize = list->size + 1;

    if (list->list != NULL) {
        list->list =
            (LinkMeta *)d_realloc(list->list, newSize * sizeof(LinkMeta));
    } else {
        list->list = (LinkMeta *)d_malloc(newSize * sizeof(LinkMeta));
    }

    list->list[newSize - 1] = item;
    list->size              = newSize;
}

/**
 * \fn void d_link_free_list(LinkMetaList *list)
 * \brief Free a LinkMetaList object.
 *
 * \param list The list to free.
 */
void d_link_free_list(LinkMetaList *list) {
    if (list->list != NULL) {
        // Free the names of the metadata items.
        for (size_t i = 0; i < list->size; i++) {
            free((char *)list->list[i].name);
        }

        free(list->list);
    }

    list->size = 0;
}

/**
 * \fn void d_link_replace_load_ins(char *ins, char *ptr)
 * \brief Change a LOADUI/ORI instruction combination to load a specific
 * pointer.
 *
 * **NOTE:** If you don't like the fact that you can't run 32-bit Decision code
 * on 64-bit machines and vice versa, blame it on this function.
 *
 * \param ins A pointer to first byte of the LOADUI instruction.
 * \param ptr The memory address for the instructions to load.
 */
void d_link_replace_load_ins(char *ins, char *ptr) {
    // LOADUI
    *(immediate_t *)(ins + 2) =
        (immediate_t)((dint)ptr >> IMMEDIATE_SIZE * 8) & IMMEDIATE_MASK;

    ins += d_vm_ins_size(OP_LOADUI);

    // ORI
    *(immediate_t *)(ins + 2) = (immediate_t)((dint)ptr & IMMEDIATE_MASK);
}

/**
 * \fn void d_link_self(Sheet *sheet)
 * \brief Link a sheet's properties from itself to itself and included sheets.
 *
 * \param sheet The sheet to link.
 */
void d_link_self(Sheet *sheet) {
    if (sheet->_isCompiled) {
        // Go through the relational structures of linking instructions to the
        // thing that they should point to.
        for (size_t insLinkIndex = 0; insLinkIndex < sheet->_insLinkListSize;
             insLinkIndex++) {
            InstructionToLink itl = sheet->_insLinkList[insLinkIndex];

            char *ins     = sheet->_text + itl.ins;
            LinkMeta meta = sheet->_link.list[itl.link];

            // If the metadata references the name of the data section, we can
            // link that here.
            // Or if it's a variable, which are also stored in the data section.
            if (meta.type == LINK_DATA_STRING_LITERAL ||
                meta.type == LINK_VARIABLE ||
                meta.type == LINK_VARIABLE_POINTER) {
                char *dataStart = sheet->_data;
                size_t offset   = (size_t)meta._ptr;

                // If the variable is in another castle (sheet), get the pointer
                // to that sheet's data section, and find out what the offset
                // is.
                if (meta.type == LINK_VARIABLE ||
                    meta.type == LINK_VARIABLE_POINTER) {
                    SheetVariable *var   = (SheetVariable *)meta.meta;
                    Sheet *externalSheet = var->sheet;

                    if (sheet != externalSheet) {
                        dataStart = externalSheet->_data;
                        offset    = 0;

                        // TODO: Pre-calculate this!
                        for (size_t i = 0; i < externalSheet->_link.size; i++) {
                            LinkMeta externalMeta =
                                externalSheet->_link.list[i];
                            if (meta.type == externalMeta.type &&
                                strcmp(meta.name, externalMeta.name) == 0) {
                                offset = (size_t)externalMeta._ptr;
                                break;
                            }
                        }
                    }
                }

                d_link_replace_load_ins(ins, dataStart + offset);
            }
            // Functions need to link to an instruction in the text section.
            else if (meta.type == LINK_FUNCTION) {
                char *textStart = sheet->_text;
                size_t offset   = (size_t)meta._ptr;

                // If the function is in another castle (sheet), get the pointer
                // to that sheet's text section, and find out what the offset
                // is.
                SheetFunction *func  = (SheetFunction *)meta.meta;
                Sheet *externalSheet = func->sheet;

                if (sheet != externalSheet) {
                    textStart = externalSheet->_text;
                    offset    = 0;

                    // TODO: Pre-calculate this!
                    for (size_t i = 0; i < externalSheet->_link.size; i++) {
                        LinkMeta externalMeta = externalSheet->_link.list[i];
                        if (meta.type == externalMeta.type &&
                            strcmp(meta.name, externalMeta.name) == 0) {
                            offset = (size_t)externalMeta._ptr;
                            break;
                        }
                    }
                }

                d_link_replace_load_ins(ins, textStart + offset);
            }
        }

        // Some things that need to be linked don't need to change any
        // instructions, like string variables getting their default value.
        for (size_t metaIndex = 0; metaIndex < sheet->_link.size; metaIndex++) {
            LinkMeta meta = sheet->_link.list[metaIndex];

            // Variable strings need their default values malloc'd before
            // running. The pointer in the metadata should say where the
            // default value (stored like a string literal) starts in data.
            if (meta.type == LINK_VARIABLE_STRING_DEFAULT_VALUE) {
                const char *varName = meta.name;

                // Firstly, we need to find where the variable pointer is
                // stored in data.
                LinkMeta varMeta = (LinkMeta){0, NULL, 0};
                bool varFound    = false;
                for (size_t i = 0; i < sheet->_link.size; i++) {
                    varMeta = sheet->_link.list[i];

                    if (varMeta.type == LINK_VARIABLE_POINTER &&
                        strcmp(varName, varMeta.name) == 0) {
                        varFound = true;
                        break;
                    }
                }

                if (varFound) {
                    // We've found the variable! Now where is it stored?
                    char *strVarPtr = sheet->_data + (size_t)varMeta._ptr;
                    char *strVarDefaultValue = sheet->_data + (size_t)meta._ptr;
                    size_t defaultValueLength = strlen(strVarDefaultValue);

                    // Now we malloc, copy the default value into the new
                    // location, and store the new pointer.
                    char *newPtr = (char *)d_malloc(defaultValueLength + 1);
                    memcpy(newPtr, strVarDefaultValue, defaultValueLength + 1);

                    memcpy(strVarPtr, &newPtr, sizeof(dint));
                }
            }
        }

        sheet->_isLinked = true;
    }
}

/**
 * \fn void d_link_includes_recursive(Sheet *sheet)
 * \brief Recursively go through the tree of included sheets and link them all
 * with `d_link_self`.
 *
 * \param sheet The sheet to recurse from.
 */
void d_link_includes_recursive(Sheet *sheet) {
    for (size_t i = 0; i < sheet->numIncludes; i++) {
        Sheet *include = sheet->includes[i];

        if (include != NULL) {
            d_link_self(include);
        }
    }
}

/*
    static void* recursive_find_link_meta(Sheet* sheet, LinkMeta* linkMeta)
    A helper function for d_link_find_included() that recursively searches
    through includes to find where the metadata is defined.

    Returns: The pointer to where the object is defined, -1 if not found.

    Sheet* sheet: The sheet to check the includes of.
    LinkMeta* linkMeta: The link meta to search for.
*/
static void *recursive_find_link_meta(Sheet *sheet, LinkMeta *linkMeta) {
    for (size_t includeIndex = 0; includeIndex < sheet->numIncludes;
         includeIndex++) {
        Sheet *include = sheet->includes[includeIndex];

        for (size_t linkIndex = 0; linkIndex < include->_link.size;
             linkIndex++) {
            LinkMeta includeLinkMeta = include->_link.list[linkIndex];

            // Is this the thing we're looking for?
            if (linkMeta->type == includeLinkMeta.type &&
                strcmp(linkMeta->name, includeLinkMeta.name) == 0) {
                // Is this the sheet where it is defined?
                if (includeLinkMeta._ptr != (char *)-1) {
                    // Bingo! Now we just need to find where the metadata is
                    // defined and we're good to go!
                    if (includeLinkMeta.type == LINK_VARIABLE ||
                        includeLinkMeta.type == LINK_VARIABLE_POINTER) {
                        // Find the SheetVariable entry.
                        // TODO: Error if not found.
                        for (size_t var = 0; var < include->numVariables;
                             var++) {
                            if (strcmp(includeLinkMeta.name,
                                       include->variables[var].name) == 0) {
                                return &(include->variables[var]);
                            }
                        }
                    } else if (includeLinkMeta.type == LINK_FUNCTION) {
                        // Find the SheetFunction entry.
                        // TODO: Error if not found.
                        for (size_t func = 0; func < include->numFunctions;
                             func++) {
                            if (strcmp(includeLinkMeta.name,
                                       include->functions[func].name) == 0) {
                                return &(include->functions[func]);
                            }
                        }
                    }
                }
            }
        }

        // If we can't find it here, maybe it's in this include's includes?
        void *recurse = recursive_find_link_meta(include, linkMeta);

        if (recurse != (void *)-1)
            return recurse;
    }

    return (void *)-1;
}

/**
 * \fn void d_link_find_included(Sheet *sheet)
 * \brief If the sheet includes objects, but doesn't know where they live,
 * i.e. `linkMeta.meta == -1 && linkMeta._ptr == -1`, then find them and
 * reference them in the sheet.
 *
 * \param sheet The sheet to find the locations of includes.
 */
void d_link_find_included(Sheet *sheet) {
    if (sheet->_isCompiled) {
        for (size_t linkIndex = 0; linkIndex < sheet->_link.size; linkIndex++) {
            LinkMeta linkMeta = sheet->_link.list[linkIndex];

            // We don't know where this thing is!
            if (linkMeta._ptr == (char *)-1 && linkMeta.meta == (void *)-1) {
                void *newMeta = recursive_find_link_meta(sheet, &linkMeta);

                if (newMeta != (void *)-1) {
                    sheet->_link.list[linkIndex].meta = newMeta;
                }
                // TODO: Error if not found?
            }
        }
    }
}

/**
 * \fn void d_link_sheet(Sheet *sheet)
 * \brief Call `d_link_find_included`, `d_link_self`, and
 * `d_link_includes_recursive` on a sheet.
 *
 * \param sheet The sheet to link.
 */
void d_link_sheet(Sheet *sheet) {
    d_link_find_included(sheet);
    d_link_self(sheet);
    d_link_includes_recursive(sheet);
}