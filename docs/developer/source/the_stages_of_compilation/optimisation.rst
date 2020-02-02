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

.. _optimisation:

############
Optimisation
############

Optimisation is performed in ``doptimize.c`` and ``doptimize.h``, because I'm
trying to appeal to an American audience, and it totally isn't the case that
I messed up the spelling the first time round.

The best way to describe Optimisation is like this:
:ref:`code-generation` is very good at creating bytecode - Optimisation is
very good at removing it.

The less instructions in the final bytecode the better. We can reduce the
number of instructions by finding specific situations in the bytecode and
changing it so it is simpler or simply by removing redundant instructions.

Here is a really basic example of optimisation (this should actually be
reduced in the code):

::

   #      Instruction
   41 ... AND
   42 ... NOT
   43 ... NOT
   44 ... JRCON

There is no point in those 2 NOT instructions being there, they cancel each
other out!

.. note::

   Another example is since code generation, for simplicity, only works with
   full immediate variations of instructions, optimisation tries to reduce
   those instructions to half or byte immediate variations of the same
   instruction.

Implementation
==============

Each "situation" like the example above is in its own function in
``doptimize.h``. Theese functions take in a ``Sheet`` and find their
situations in the code, and reduces the bytecode using a helper function:

.. doxygenfunction:: d_optimize_remove_bytecode
   :no-link:

Which removes ``len`` instructions from the ``.text`` section of ``sheet``,
starting from the index ``start``.

The individual functions also return a boolean to say if they found a way to
optimise the bytecode (i,e, they return ``true`` if they changed the
bytecode). This way, you can check for new situations that crop up when
bytecode is removed, but only if bytecode was removed.

All of the optimisations are attempted in one big helper function:

.. doxygenfunction:: d_optimize_all
   :no-link:
