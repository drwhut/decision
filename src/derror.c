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

#include "derror.h"

#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A static global variable holding all of the run-time error messages. */
static char *errorMessages = NULL;

/* A static global variable holding the length of the errorMessages variable. */
static size_t lenErrorMessages = 0;

/* A static global variable saying if there's been an ERROR reported. */
static bool programHasError = false;

/*
    size_t add_length_to_messages(size_t length)
    Add a length of memory to the errorMessages variable. If there is no
    allocated memory, then malloc some.

    Returns: The index of the first char of available space. This is where the
    newline can be placed, followed by the new message.

    size_t length: The length of memory to "add on". This does NOT include the
    \0 character at the end.
*/
static size_t add_length_to_messages(size_t length) {
    size_t indexOfFirstFreeSpace = 0;

    if (errorMessages == NULL) {
        errorMessages = d_calloc((length + 1), sizeof(char));

        if (errorMessages != NULL)
            lenErrorMessages = length + 1;
    } else {
        indexOfFirstFreeSpace =
            lenErrorMessages - 1; // lenErrorMessages includes the \0.
        size_t newLength = lenErrorMessages + length + 1;
        errorMessages    = d_realloc(errorMessages, newLength * sizeof(char));

        if (errorMessages != NULL)
            lenErrorMessages = newLength;
    }

    return indexOfFirstFreeSpace;
}

/*
    void add_message_to_error_list(const char* message, const char seperator)
    Add a string to the error list.

    const char* message: The message itself. Ends in a \0 character.
    char seperator: The character to put inbetween the previous and new message.
*/
static void add_message_to_error_list(const char *message,
                                      const char seperator) {
    size_t lenMessage = strlen(message);

    // Reallocate the amount of memory errorMessages has.
    size_t indexToAdd = add_length_to_messages(lenMessage);

    // Get the pointer to the index we want to add to.
    char *ptr = errorMessages + indexToAdd;

    // Place the seperator in if it's not the first message.
    if (indexToAdd > 0) {
        errorMessages[indexToAdd] = seperator;
        ptr++;
    }

    // Now copy our message onto there.
    strcpy(ptr, message);
}

/**
 * \fn void d_error_compiler_push(const char *message, const char *filePath,
 *                                size_t lineNum, bool isError)
 * \brief Push a compile-time error message to the error list.
 *
 * \param message The error message itself.
 * \param filePath The file where the error occured.
 * \param lineNum The line number of the error.
 * \param isError If `true`, show ERROR, otherwise, show WARNING.
 */
void d_error_compiler_push(const char *message, const char *filePath,
                           size_t lineNum, bool isError) {
    // Does the program have an error?
    if (isError)
        programHasError = true;

    char errorMessage[MAX_ERROR_SIZE];

    // Create the error message with sprintf.
    sprintf(errorMessage, "%s: (%s:%zd) %s", (isError) ? "Fatal" : "Warning",
            filePath, lineNum, message);

    // Add the error to the error list.
    add_message_to_error_list(errorMessage, '\n');
}

/**
 * \fn bool d_error_report()
 * \brief Report if there were any compile-time errors or warnings to `stdout`.
 *
 * \return `true` if there were errors (warnings do not count as errors),
 * `false` otherwise.
 */
bool d_error_report() {
    if (errorMessages != NULL) {
        printf("%s\n", errorMessages);
    }

    return programHasError;
}

/**
 * \fn void d_error_free()
 * \brief Free any compile-time error messages saved.
 *
 * Should be called at the end of compilation.
 */
void d_error_free() {
    if (errorMessages != NULL) {
        free(errorMessages);
        errorMessages    = NULL;
        lenErrorMessages = 0;
    }
}
