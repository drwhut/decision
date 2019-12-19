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

**************
Core Reference
**************

.. todo::

   This page was created manually.
   Find a suitable method to automatically generate this page.

Core Functions
==============

``Add``
    Calculate the addition of all number inputs.

    Inputs:

    * Any number of ``Integer``/``Float`` inputs.

    Outputs:

    1. The addition of all of the inputs. Is a ``Float`` if at least one of
       the inputs was a ``Float``, otherwise it is an ``Integer``.

``And``
    Output the bitwise AND of two boolean values.

    Inputs:

    1. ``Boolean``: The first boolean.
    2. ``Boolean``: The second boolean.

    Outputs:

    1. ``Boolean``: The bitwise AND of the two inputs.

``Div``
    Take the division of two numbers, and truncate the answer.

    Inputs:

    1. ``Integer``/``Float``: The numerator.
    2. ``Integer``/``Float``: The denominator.

    Outputs:

    1. ``Integer``: The truncated division.

``Divide``
    Take the division of two numbers.

    Inputs:

    1. ``Integer``/``Float``: The numerator.
    2. ``Integer``/``Float``: The denominator.

    Outputs:

    1. ``Float``: The division.

``Equal``
    Check if two numbers or two strings are equal in value.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the values were equal, ``false`` otherwise.

``LessThan``
    Check if a number or string is less in value than a second number or
    string.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the first value was less than the second,
    ``false`` otherwise.

``LessThanOrEqual``
    Check if a number or string is less than or equal in value than a second
    number or string.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the first value was less than or equal to the
    second, ``false`` otherwise.

``Mod``
    Calculate the remainder after division of two integers.

    Inputs:

    1. ``Integer``: The numerator.
    2. ``Integer``: The denominator.

    Outputs:

    1. ``Integer``: The remainder after division.

``MoreThan``
    Check if a number or string is more in value than a second number or
    string.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the first value was more than the second,
    ``false`` otherwise.

``MoreThanOrEqual``
    Check if a number or string is more than or equal in value than a second
    number or string.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the first value was more than or equal to the
    second, ``false`` otherwise.

``Multiply``
    Calculate the multiplication of all number inputs.

    Inputs:

    * Any number of ``Integer``/``Float`` inputs.

    Outputs:

    1. The multiplication of all of the inputs. Is a ``Float`` if at least one
       of the inputs was a ``Float``, otherwise it is an ``Integer``.

``Not``
    Output the bitwise NOT of a boolean input.

    Inputs:

    1. ``Boolean``: The input.

    Outputs:

    1. ``Boolean``: The inverse of the input.

``NotEqual``
    Check if two numbers or two strings are not equal in value.

    Inputs:

    1. ``Integer``/``Float``/``String``: The first value.
    2. ``Integer``/``Float``/``String``: The second value.

    Outputs:

    1. ``Boolean``: ``true`` if the values were not equal, ``false``
       otherwise.

``Or``
    Output the bitwise OR of two boolean values.

    Inputs:

    1. ``Boolean``: The first boolean.
    2. ``Boolean``: The second boolean.

    Outputs:

    1. ``Boolean``: The bitwise OR of the two inputs.

``Subtract``
    Take the subtraction of two numbers.

    Inputs:

    1. ``Integer``/``Float``: The first number.
    2. ``Integer``/``Float``: The second number.

    Outputs:

    1. The subtraction of the second number from the first number. Is a
       ``Float`` if at least one of the inputs was a ``Float``, otherwise it
       is an ``Integer``.

``Ternary``
    Return one value or another depending on a condition.

    Inputs:

    1. ``Boolean``: The condition to evaluate.
    2. ``Integer``/``Float``/``String``/``Boolean``: The value to return if the
       condition is true.
    3. ``Integer``/``Float``/``String``/``Boolean``: The value to return if the
       condition is false.
    
    Outputs:

    1. ``Integer``/``Float``/``String``/``Boolean``: The selected input.

``Xor``
    Output the bitwise XOR of two boolean values.

    Inputs:

    1. ``Boolean``: The first boolean.
    2. ``Boolean``: The second boolean.

    Outputs:

    1. ``Boolean``: The bitwise XOR of the two inputs.

Core Subroutines
================

.. note::

   "Standard execution input" means the execution socket that needs to be
   activated in order for the execution node to activate.

   "Standard execution output" means the execution socket that is activated
   by the execution node once the node is done with it's action.

``For``
    For each iteration of a numerical value, activate an execution path.

    Inputs:

    1. ``Execution``: The standard execution input.
    2. ``Integer``/``Float``: The starting value of the numerical value.
    3. ``Integer``/``Float``: The ending value of the numerical value.
    4. ``Integer``/``Float``: The increment/decrement amount of the numerical
       value.
    
    Outputs:

    1. ``Execution``: The exection socket to activate every iteration.
    2. The numerical value. Is a ``Float`` if at least one of the numerical
       inputs is a ``Float``, otherwise it is an ``Integer``.
    3. ``Execution``: The standard execution output.

``IfThen``
    Activate an execution path if a condition is true.

    Inputs:

    1. ``Execution``: The standard execution input.
    2. ``Boolean``: The condition to evaluate.

    Outputs:

    1. ``Execution``: The execution socket to activate is the condition is
       true.
    2. ``Execution``: The standard execution output.   

``IfThenElse``
    Activate an execution path if a condition is true, and another if the
    condition is false.

    Inputs:

    1. ``Execution``: The standard execution input.
    2. ``Boolean``: The condition to evaluate.

    Outputs:

    1. ``Execution``: The execution socket to activate if the condition is
       true.
    2. ``Execution``: The execution socket to activate if the condition is
       false.
    3. ``Execution``: The standard execution output.

``Print``
    Print a value to the standard output.

    Inputs:

    1. ``Execution``: The standard execution input.
    2. ``Integer``/``Float``/``String``/``Boolean``: The value to print.

    Outputs:

    1. ``Execution``: The standard execution output.

``Set``
    Set the value of a variable.

    Inputs:

    1. ``Name``: The name of the variable to set.
    2. ``Execution``: The standard execution input.
    3. ``Integer``/``Float``/``String``/``Boolean``: The value to set the
       variable to - must be the same type as the variable being set.
    
    Outputs:

    1. ``Execution``: The standard execution output.

``While``
    Repeat an execution path while a condition is true.

    Inputs:

    1. ``Execution``: The standard execution input.
    2. ``Boolean``: The condition to evaluate at the start of every loop.

    Outputs:

    1. ``Execution``: The execution socket to activate every loop.
    2. ``Execution``: The standard execution output.

Core Properties
===============

``Variable``
    Define a variable in the sheet.

    Arguments:

    1. ``Name``: The name of the variable.
    2. ``Type``: The data type of the variable.
    3. ``Literal``: The default value of the variable. Technically optional,
       but highly recommended to set a default value.

``Include``
    Include the functions, subroutine and variables of another sheet.

    Arguments:

    1. ``Name``/``String``: The file path of the sheet.

    .. note::

       Standard libraries will eventually be implemented by providing a name
       rather than a string literal, but this is not yet functional.

``Function``
    Create a custom function in the sheet.

    Arguments:

    1. ``Name``: The name of the function.

``Subroutine``
    Create a custom subroutine in the sheet.

    By default it will have an input and output execution socket, representing
    the standard execution input and output respectively.

    Arguments:

    1. ``Name``: The name of the subroutine.

``FunctionInput``
    Add an input socket to a function (or subroutine).

    Arguments:

    1. ``Name``: The name of the function/subroutine to add an input to.
    2. ``Type``: The data type of the input.
    
    .. note::

       There are also optional arguments for the name of the input and it's
       default value, but these are not yet functional.

``FunctionOutput``
    Add an output socket to a function (or subroutine).

    Arguments:

    1. ``Name``: The name of the function/subroutine to add an output to.
    2. ``Type``: The data type of the output.

    .. note::

       There is also an optional argument for the nmae of the output, but this
       is not yet functional.
