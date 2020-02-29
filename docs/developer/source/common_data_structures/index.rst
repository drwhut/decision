..
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

**********************
Common Data Structures
**********************

DType
=====

Defined in ``dtype.h``.

.. doxygentypedef:: DType
   :no-link:

.. doxygenenum:: _dType
   :no-link:

LexData
=======

Defined in ``dlex.h``.

.. doxygentypedef:: LexData
   :no-link:

.. doxygenunion:: _lexData
   :no-link:

Graph Structures
================

The graph of a sheet is the set of nodes, and the wires connecting those nodes.

SocketMeta
----------

Defined in ``dgraph.h``.

.. doxygentypedef:: SocketMeta
   :no-link:

.. doxygenstruct:: _socketMeta
   :no-link:
   :members:

NodeDefinition
--------------

Defined in ``dgraph.h``.

.. doxygentypedef:: NodeDefinition
   :no-link:

.. doxygenstruct:: _nodeDefinition
   :no-link:
   :members:

NodeSocket
----------

Defined in ``dgraph.h``.

.. doxygentypedef:: NodeSocket
   :no-link:

.. doxygenstruct:: _nodeSocket
   :no-link:
   :members:

Wire
----

Defined in ``dgraph.h``.

.. doxygentypedef:: Wire
   :no-link:

.. doxygenstruct:: _wire
   :no-link:
   :members:

Node
----

Defined in ``dgraph.h``.

.. doxygentypedef:: Node
   :no-link:

.. doxygenstruct:: _node
   :no-link:
   :members:

Graph
-----

Defined in ``dgraph.h``.

.. doxygentypedef:: Graph
   :no-link:

.. doxygenstruct:: _graph
   :no-link:
   :members:

Sheet Structures
================

Sheets contain graphs, as well as extra properties like variables, functions,
and the compiled bytecode.

SheetVariable
-------------

Defined in ``dsheet.h``.

.. doxygentypedef:: SheetVariable
   :no-link:

.. doxygenstruct:: _sheetVariable
   :no-link:
   :members:

SheetFunction
-------------

Defined in ``dsheet.h``.

.. doxygentypedef:: SheetFunction
   :no-link:

.. doxygenstruct:: _sheetFunction
   :no-link:
   :members:

Sheet
-----

Defined in ``dsheet.h``.

.. doxygentypedef:: Sheet
   :no-link:

.. doxygenstruct:: _sheet
   :no-link:
   :members:
