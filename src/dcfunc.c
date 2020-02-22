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

#include "dcfunc.h"

#include "dmalloc.h"

#include <stdlib.h>
#include <string.h>

/**
 * \fn void d_create_c_function(DecisionCFunction function, const char *name,
 *                              const char *description, SocketMeta *sockets,
 *                              size_t numInputs, size_t numOutputs)
 * \brief Create a function that calls a C function. Any sheets that are loaded
 * after this call can use this new function.
 *
 * \param function The C function to call when this node is activated.
 * \param name The name of the function.
 * \param description The description of the function.
 * \param sockets An array of socket metadata. This array should have at least
 * `numInputs + numOutputs` elements in.
 * \param numInputs The number of input sockets the function has.
 * \param numOutputs The number of output sockets the function has.
 */
CFunction d_create_c_function(DecisionCFunction function, const char *name,
                              const char *description, SocketMeta *sockets,
                              size_t numInputs, size_t numOutputs) {
    // Create a node definition to define the function.

    char *newName = NULL;

    // Copy the name over.
    if (name != NULL) {
        size_t nameSize = strlen(name) + 1;
        newName         = (char *)d_malloc(nameSize);
        strcpy(newName, name);
    }

    char *newDescription = NULL;

    // Copy the description over.
    if (description != NULL) {
        size_t descriptionSize = strlen(description) + 1;
        newDescription         = (char *)d_malloc(descriptionSize);
        strcpy(newDescription, description);
    }

    SocketMeta *newSockets = NULL;
    size_t numSockets      = 0;

    // Copy the sockets array, and copy the names and descriptions over as well.
    if (sockets != NULL) {
        numSockets = numInputs + numOutputs;
        newSockets = (SocketMeta *)d_malloc(numSockets * sizeof(SocketMeta));
        memcpy(newSockets, sockets, numSockets * sizeof(SocketMeta));

        for (size_t i = 0; i < numSockets; i++) {
            const char *socketName = newSockets[i].name;
            const char *socketDesc = newSockets[i].description;

            if (socketName != NULL) {
                size_t nameSize     = strlen(socketName) + 1;
                char *newSocketName = d_malloc(nameSize);
                strcpy(newSocketName, socketName);
                newSockets[i].name = newSocketName;
            }

            if (socketDesc != NULL) {
                size_t descSize     = strlen(socketDesc) + 1;
                char *newSocketDesc = d_malloc(descSize);
                strcpy(newSocketDesc, socketDesc);
                newSockets[i].description = newSocketDesc;
            }
        }
    }

    NodeDefinition definition             = {NULL, NULL, NULL, 0, 0, false};
    definition.name                       = newName;
    definition.description                = newDescription;
    *(SocketMeta **)&(definition.sockets) = newSockets;
    definition.numSockets                 = numSockets;
    definition.startOutputIndex           = numInputs;

    CFunction newFunction;
    newFunction.function                           = function;
    *(NodeDefinition *)(&(newFunction.definition)) = definition;

    return newFunction;
}

/**
 * \fn void d_create_c_subroutine(DecisionCFunction function, const char *name,
 *                                const char *description, SocketMeta *sockets,
 *                                size_t numInputs, size_t numOutputs)
 * \brief Create a subroutine that calls a C function. Any sheets that are
 * loaded after this call can use this new function.
 *
 * **NOTE:** The `sockets` array should not have any execution sockets in,
 * these will automatically be added. Thus `numInputs` and `numOutputs` should
 * not account for any execution nodes either.
 *
 * \param function The C function to call when this node is activated.
 * \param name The name of the function.
 * \param description The description of the function.
 * \param sockets An array of socket metadata. This array should have at least
 * `numInputs + numOutputs` elements in.
 * \param numInputs The number of input sockets the function has.
 * \param numOutputs The number of output sockets the function has.
 */
CFunction d_create_c_subroutine(DecisionCFunction function, const char *name,
                                const char *description, SocketMeta *sockets,
                                size_t numInputs, size_t numOutputs) {
    // For this function, we're going to add a "before" and "after" socket to
    // what the user has already given us, and pass that onto
    // d_create_c_function.

    const size_t numSockets = numInputs + numOutputs;

    const size_t newNumInputs  = numInputs + 1;
    const size_t newNumOutputs = numOutputs + 1;
    const size_t newNumSockets = newNumInputs + newNumOutputs;

    SocketMeta *newSockets =
        (SocketMeta *)d_malloc(newNumSockets * sizeof(SocketMeta));

    // Copy the array we've been given to the middle of this new array, such
    // that there is a free spot either end.
    memcpy(newSockets + 1, sockets, numSockets * sizeof(SocketMeta));

    // Add the "before" node.
    SocketMeta before;
    before.name        = "before";
    before.description = "The node will activate when this input is activated.";
    before.type        = TYPE_EXECUTION;
    before.defaultValue.integerValue = 0;
    memcpy(newSockets, &before, sizeof(SocketMeta));

    // Add the "after" node.
    SocketMeta after;
    after.name = "after";
    after.description =
        "This output will activate once the node has finished executing.";
    after.type                      = TYPE_EXECUTION;
    after.defaultValue.integerValue = 0;
    memcpy(newSockets + newNumSockets - 1, &after, sizeof(SocketMeta));

    CFunction out = d_create_c_function(function, name, description, newSockets,
                                        newNumInputs, newNumOutputs);

    free(newSockets);

    return out;
}