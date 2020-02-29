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

##############
Semantic Rules
##############

Properties
==========

* ``Variable`` properties must have **name** and **data type** arguments,
  but the **default value** argument and **description string literal**
  arguments are optional.

  * The data type argument **cannot be vague**.

  * The default value, if given, must be a **literal**.

  * The default value, if given, it's data type **must match that of the
    variable**.

* You cannot create 2 variables with the **same name**.

* ``Include`` properties must have a single **string literal argument**,
  or a single **name argument**.

  * Any sheet that produces errors should also **produce errors in sheet that
    include it**.

.. note::

   As of now, only the string literal argument is functional. See the
   ``add_property_Include`` function in ``dsemantic.c``.

* ``Function`` properties must have a **name argument**, and can have an
  optional **description string literal argument.**

* ``Subroutine`` properties must have a **name argument**, and can have an
  optional **description string literal argument.**

* ``FunctionInput`` properties must have a **function name argument**, a
  **socket name argument**, a **data type argument**, and an optional
  **default value argument** and an optional **description string literal
  argument**.

  * The function name argument must reference a **defined** function.

  * The default value, if given, must be a **literal**.

  * The default value, if given, it's data type **must match that of the
    function input**.

* ``FunctionOutput`` properties must have a **function name argument**, a
  **socket name argument**, a **data type argument**, and an optional
  **description string literal** argument.

  * The function name argument must reference a **defined** function.

* You cannot create 2 functions / subroutines with the **same name**.

Statements
==========

* For any node, if a literal is put in the place of a socket, then the literal
  must be the **same data type as the socket**.

* Node names must reference **defined** nodes.

* Wire identifiers as node inputs must be **defined in the same sheet**.

  * Both end-sockets of wires must have the **same data type.**

* A sheet can only have up to 1 ``Start`` node, and it cannot be run directly
  if it is absent.

* All functions and subroutines must have exactly 1 ``Define`` node.

  * All functions, not subroutines, must have exactly 1 ``Return`` node.

* ``Define`` and ``Return`` nodes must reference **defined** functions.

  * ``Define`` and ``Return`` nodes must reference functions and subroutines
    defined **in the same sheet**.

* For any bitwise operators, all inputs **must be of the same type**.

* Comparison nodes cannot compare between numbers and strings.

* The program should not have any **cycles**.
