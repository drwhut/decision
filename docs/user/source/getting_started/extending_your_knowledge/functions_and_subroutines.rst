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

Functions and Subroutines
=========================

Functions and Subroutines are ways of creating your own nodes that you can
call upon whenever you like!

Functions
---------

Functions are a custom set of *non-execution* nodes called with a
*non-execution* node. You can have as many inputs and outputs as you like.

To create a function, you can use the ``Function`` property, then you can use
the ``FunctionInput`` and ``FunctionOutput`` properties to state the inputs
and outputs of the function.

``Function`` properties have two arguments:

1. The name of the function.
2. (Optional) A description of the function.

``FunctionInput`` properties have several arguments:

1. The name of the function the input is for.
2. The name of the input itself.
3. The data type of the input, i.e. ``Integer``, ``Float``, ...
4. (Optional) The default value of the input. This is not used by the compiler,
   but it is used by editors.
5. (Optional) A description of the input.

``FunctionOutput`` properties have several arguments:

1. The name of the function the output is for.
2. The name of the output itself.
3. The data type of the output, i.e. ``String``, ``Boolean``, ...
4. (Optional) A description of the output.

For functions, you need to have one and only one ``Return`` node to return
the output of your function. You will also need a ``Define`` node if your
function takes any inputs, but if it doesn't, then it is not nessesary.

.. code-block:: decision

   [Function(Double, "Doubles a given number.")]
   [FunctionInput(Double, num, Float, 1.0, "The number to double.")]
   [FunctionOutput(Double, doubled, Float, "The input number doubled.")]

   [Function(CanLegallyDrink, "Determine if someone is allowed to drink.")]
   [FunctionInput(CanLegallyDrink, age, Integer, 0, "The person's age.")]
   [FunctionOutput(CanLegallyDrink, canDrink, Boolean, "If the person can drink.")]

   Define(Double)~#1
   Multiply(#1, 2)~#2
   Return(Double, #2)

   Define(CanLegallyDrink)~#3
   MoreThanOrEqual(#3, 18)~#4
   Return(CanLegallyDrink, #4)

   Start~#10
   Double(20.5)~#11
   Print(#10, #11)~#12

   CanLegallyDrink(25)~#13
   Print(#12, #13)

.. code-block::

   $ decision functions.dc

   41
   true

Subroutines
-----------

Subroutines are a custom set of both *execution* and *non-execution* nodes
called with an *execution* node. You can have as many inputs and outputs as
you like, but there will always be an input and output execution socket.

To create a subroutine, you can use the ``Subroutine`` property, then you use
the ``FunctionInput`` and ``FunctionOutput`` properties like before to state
your inputs and outputs.

For subroutines, you need to have one and only one ``Define`` node, and you
need at least one ``Return`` node if the subroutine returns any values
(otherwise you don't need any), but you can have as many ``Return`` nodes
as you need (since your flow of execution could go down different paths).

.. code-block:: decision

   [Subroutine(CountTo, "Count from 1 to a given integer.")]
   [FunctionInput(CountTo, upTo, Integer, 10, "The number to count up to.")]

   [Subroutine(PrintFactorsOf, "Print off the factors of a given number.")]
   [FunctionInput(PrintFactorsOf, num, Integer, 1, "The number to print the factors of.")]
   [FunctionOutput(PrintFactorsOf, numFactors, Integer, "The number of factors the number has.")]

   Define(CountTo)~#1, #2
   For(#1, 1, #2, 1)~#3, #4, #5
   Print(#3, #4)
   Return(CountTo, #5)
   
   [Variable(numFactors, Integer, 0, "Incremented every time a factor is found.")]

   Define(PrintFactorsOf)~#10, #11
   Set(numFactors, #10, 0)~#12
   Div(#11, 2)~#13

   For(#12, 1, #13, 1)~#14, #15, #16
   Mod(#11, #15)~#17
   Equal(#17, 0)~#18
   IfThen(#14, #18)~#19
   Print(#19, #15)~#20

   numFactors~#21
   Add(#21, 1)~#22
   Set(numFactors, #20, #22)

   Return(PrintFactorsOf, #16, #21)

   Start~#30
   CountTo(#30, 10)~#31
   Print(#31, "Factors of 360:")~#32
   PrintFactorsOf(#32, 360)

.. code-block::

   $ decision subroutines.dc

   1
   2
   3
   4
   5
   6
   7
   8
   9
   10
   Factors of 360:
   1
   2
   3
   4
   5
   6
   8
   9
   10
   12
   15
   18
   20
   24
   30
   36
   40
   45
   60
   72
   90
   120
   180
