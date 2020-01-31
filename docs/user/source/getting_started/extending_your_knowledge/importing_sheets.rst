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

Importing Sheets
================

Once you start creating lots and lots of functions and subroutines, you'll
want to start using them in other sheets you create as well. Why create it
multiple times when you can create it once and use it elsewhere?

This is why the ``Include`` property exists. With it, you can include another
sheet by using either it's absolute path, or it's path relative to the sheet
including it. Once you do, you then have access to **all of the sheet's
functions, subroutines, and variables**.

.. code-block:: decision

   < my_math.dc >

   [Function(IsEven)]
   [FunctionInput(IsEven, Integer)]
   [FunctionOutput(IsEven, Boolean)]

   Define(IsEven)~#1
   Mod(#1, 2)~#2
   Return(IsEven, #2)

   [Function(IsOdd)]
   [FunctionInput(IsOdd, Integer)]
   [FunctionOutput(IsOdd, Boolean)]

   Define(IsOdd)~#3
   IsEven(#3)~#4
   Not(#4)~#5
   Return(IsOdd, #5)

   [Variable(PI, Float, 3.14159)]

   [Subroutine(AreaOfCircle)]
   [FunctionInput(AreaOfCircle, Float)]

   Define(AreaOfCircle)~#10, #11
   PI~#12
   Multiply(#11, #11, #12)~#13
   Print(#10, "Given a circle with radius:")~#14
   Print(#14, #11)~#15
   Print(#15, "It's area is:")~#16
   Print(#16, #13)

.. code-block:: decision

   < main.dc >

   [Include("my_math.dc")]

   Start~#1
   IsEven(6)~#2
   IsOdd(6)~#3

   Print(#1, #2)~#4
   Print(#4, #3)~#5

   Print(#5, "PI is approximately:")~#6
   PI~#7
   Print(#6, #7)~#8

   AreaOfCircle(#8, 2.5)

.. code-block::

   $ decision main.dc

   true
   false
   PI is approximately:
   3.14159
   Given a circle with radius:
   2.5
   It's area is:
   19.6349
