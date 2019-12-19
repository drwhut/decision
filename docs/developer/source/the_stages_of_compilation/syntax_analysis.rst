..
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

.. _syntax-analysis:

###############
Syntax Analysis
###############

Syntax Analysis is performed in ``dsyntax.c`` and ``dsyntax.h``.

The aim of Syntax Analysis is to determine whether the user wrote the source
code *correctly* in terms of the structure of the program. This is done by
generating a **syntax tree**, which not only helps us later with analysing
the code but also helps us find any syntax errors in the code.

The syntax tree has a root node, which represents the entire program.
The children of the root node would then be the statements in the program,
then those statements would have the name, the arguments, and the outputs
as their children, etc.

Since the syntax of the language doesn't allow for nested statements like in
other languages, we use a *top-down* method of creating the syntax tree
rather than using grammars and *bottom-up* methods which can handle nested
statements. Therefore in the compiler, we use the
`recursive-descent <https://en.wikipedia.org/wiki/Recursive_descent_parser>`_
method to convert the lexical stream from :ref:`lexical-analysis` to a
syntax tree.

Structures
==========

``SyntaxDefinition``
    An enumerator representing the definitions of syntax statements, for
    example, there will be one for line identifiers, another for arguments,
    another for statements as a whole, etc.

.. note::

   There is one special value in ``SyntaxDefinition`` which is
   ``STX_TOKEN = 0``, which represents a lexical token.

``SyntaxContext``
    A structure that, during Syntax Analysis, keeps track of data like where
    we are in the lexical stream, what the current ``LexToken`` is, etc.

``SytaxNode``
    A node in the syntax tree. It contains a ``SyntaxDefinition``, and a
    reference to a lexical token (``LexToken*``) to preserve the info gathered
    by :ref:`lexical-analysis` - however, it only stores the said reference is
    the ``SyntaxDefinition`` is ``STX_TOKEN = 0``.

    Each node also stores 2 pointers to other syntax nodes (``SyntaxNode*``),
    which make up the structure of the tree. One is the *direct child* of the
    node, and the other is the *next sibling* of the node. The reason for
    this structure is so that we don't have to store a dynamic list of
    children, which in C is a pain.

    Therefore, the syntax tree as a whole is represented as a pointer
    (``SyntaxNode*``) to the root node.

Functions
=========

Meta-variables
--------------

Each ``SyntaxDefinition`` has an associated function called a
**meta-variable** in ``dsyntax.c``, which returns if the section of lexical
tokens we're looking at fits the syntax rule of the meta-variable, and may
potentially throw an error if it specifically breaks the rules (but not if
none of the lexical tokens are valid, as they may be valid in another
definition)

.. note::

   These functions have a *lot* of preprocessor macros that hide a lot of
   what is going on under the hood - this is because all of these functions
   do a lot of similar things, and it makes it easier in the long run to
   create new rules, as the macros represent concepts similar to something
   you would see in, say, Backus-Naur form.

Here is an example of a meta-variable for a *call*, which is a list of
arguments (that could be empty), surrounded by brackets:

.. code-block:: c

   /* <call> ::= <Lbracket><listOfArguments><Rbracket> */
   META(call)

       if (acceptToken(TK_LBRACKET))
       {
           START_ADD
           acceptDefinition(listOfArguments); // There dosen't nessesarily have to
           END_ADD                            // be anything in the backets.

           expectToken(TK_RBRACKET, "Expected ) symbol at end of call");
           ACCEPT(call)
       }
       else
       {
           DECLINE(call)
       }

   END_META

Here it will accept a list of arguments if it finds one, but if it doesn't
then it will not error - but it will error if after there isn't a right
bracket. Also notice that if there is no left bracket it doesn't error, but
simply says "this cannot be a call, try something else".

Syntax functions
----------------

In ``dsyntax.h``, there are a few functions to help you traverse and
manipulate a syntax tree, for instance:

.. doxygenfunction:: d_syntax_add_child
   :no-link:

The ``child`` node is added as the next child of the ``parent`` node.

.. doxygenfunction:: d_syntax_get_num_children
   :no-link:

Returns the number of children that ``parent`` has.

There is also a mechanism to help you find all of the syntax nodes with a
given ``SyntaxDefinition``:

.. doxygenfunction:: d_syntax_get_all_nodes_with
   :no-link:

Returns a list of ``SyntaxNode*`` which have the same ``SyntaxDefinition`` as
``definition``. It also gives you the choice of whether you want to continue
traversing the children of a satisfactory node to try and find more, although
this will mostly be false as certain syntax definitions will only appear in
certain layers of the tree.

Because C is our bestest friend in the world, we need to free the results once
we're done with them:

.. doxygenfunction:: d_syntax_free_results
   :no-link:

However, the *most important function* is:

.. doxygenfunction:: d_syntax_parse
   :no-link:

Returns the root node of a syntax tree, given the ``stream`` from
:ref:`lexical-analysis`. In the event there is an error, it blames
``filePath``.

.. doxygenfunction:: d_syntax_free_tree
   :no-link:

Frees the syntax tree after we are done with it.

Like in :ref:`lexical-analysis`, there is a debugging method:

.. doxygenfunction:: d_syntax_dump_tree
   :no-link:

Dumps the syntax tree with indentation depending on the depth of the node.

.. note::

   The syntax gets dumped automatically whenever you compile source code with
   a ``VERBOSE_LEVEL`` of 4 or higher.
