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

#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * \fn void *d_malloc(size_t size)
 * \brief A wrapper function for `malloc` with error checking.
 *
 * \return A pointer to the allocated memory.
 *
 * \param size The size of the allocation.
 */
void *d_malloc(size_t size) {
    void *ptr = malloc(size);

    if (ptr == NULL) {
        printf("Fatal: malloc returned NULL\n");
        exit(1);
    }

    return ptr;
}

/**
 * \fn void *d_realloc(void *ptr, size_t size)
 * \brief A wrapper function for `realloc` with error checking.
 *
 * \return A pointer to the reallocated memory.
 *
 * \param ptr The memory allocation to reallocate.
 * \param size The size of the new allocation.
 */
void *d_realloc(void *ptr, size_t size) {
    void *newptr = realloc(ptr, size);

    if (newptr == NULL) {
        printf("Fatal: realloc returned NULL\n");
        exit(1);
    }

    return newptr;
}

/**
 * \fn void *d_calloc(size_t num, size_t size)
 * \brief A wrapper function for `calloc` with error checking.
 * 
 * \return A pointer to the allocated memory.
 * 
 * \param num The number of elements to allocate.
 * \param size The size of each element.
 */
void *d_calloc(size_t num, size_t size) {
    void *newptr = calloc(num, size);

    if (newptr == NULL) {
        printf("Fatal: calloc returned NULL\n");
        exit(1);
    }

    return newptr;
}