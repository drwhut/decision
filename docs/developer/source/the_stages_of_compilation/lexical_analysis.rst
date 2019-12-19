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

.. _lexical-analysis:

################
Lexical Analysis
################

Lexical Analysis is performed in ``dlex.c`` and ``dlex.h``.

The aim of Lexical Analysis is to change the way the source code is
represented in memory, such that the later compilation stages have an
easier job understanding what they are reading. Lexical Analysis essentially
abstracts the source code (by ignoring comments and whitespace) and
categorises it such that we can identify what a particular section of the
source code represents in one comparison (Is it an integer? Is it the name of
something? Is it a plane?).

Structures
==========

``LexType``
    An ``enum`` representing the type of data a token represents. The values
    range from very basic (``TK_COMMA``) to representing a large amount of
    characters (``TK_INTEGERLITERAL``).

``LexData``
    A ``union`` of the most basic data types, which are strings (``char*``),
    integers (``dint``), floats (``dfloat``), and booleans (``bool``). This
    union is used for a token to store important information it needs to save
    for later, for example, the name of a node, or the value of a literal.

.. note::

   ``dint`` and ``dfloat`` are defined in ``dcfg.h`` and represent different
   sizes depending on whether the pre-processor macro ``DECISION_32`` is
   defined.

``LexToken``
    A ``struct`` that contains ``LexType`` and ``LexData`` elements.

``LexStream``
    A list of ``LexToken``.

Functions
=========

Most of the functions declared in ``dlex.h`` are to help identify characters
or to help group up some characters into say, a string literal.

These functions are ultimately used by:

.. doxygenfunction:: d_lex_create_stream
   :no-link:

to create a ``LexStream`` from some source code ``source``. It is essentially
a giant for loop that iterates over the characters of the source string and
builds up a ``LexToken`` each iteration to add to the ``LexStream``.
``filePath`` is used in producing compilation errors, if any should occur.

.. warning::

   For safety reasons, the ``source`` string NEEDS a newline character (``\n``)
   as the last non-null character for the function to work without producing
   any errors.

Since ``LexStream`` is a dynamic list, and this is C, once you are done with
the list, you will need to free it from memory with this function, because
hell on Earth actually exists:

.. doxygenfunction:: d_lex_free_stream
   :no-link:

.. note::

   ``d_lex_free_stream`` frees the list of tokens, but not anything that may
   have been allocated inside the tokens themselves, i.e. string names and
   literals inside of ``LexData``. These will need to be freed later on.

If you want to dump the contents of a ``LexStream`` into ``stdout``, you can
use the function:

.. doxygenfunction:: d_lex_dump_stream
   :no-link:

to do so. The output of this function for a section of source code that prints
"Hello, world!" to the screen may look like this:

::

   i     type  data
   ...
   17    0     Start (00000127F23739A0)
   18    11
   19    12
   20    6     1
   21    15
   22    0     Print (00000127F2376700)
   23    17
   24    12
   25    6     1
   26    13
   27    8     Hello, world! (00000127F2376720)
   28    20
   29    15
   ...

.. note::

   This table gets printed whenever you compile source code with a
   ``VERBOSE_LEVEL`` of 4 or higher.

The first column states the index of the ``LexToken`` in the ``LexStream``,
the second column states the ``LexType`` value (you can look up what each value
corresponds to which type in ``dlex.h``), and the third column states the
value in ``LexData`` (for specific types). Strings in the third columns are
also given alongside their pointer values, which can help with debugging.
