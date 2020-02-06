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

Comments
========

Comments are blocks of text within scripts that are not read by the compiler.
They are mostly used to help the reader, and to help GUI programs store extra
data about the nodes.

Comments are anything after and including the ``>`` symbol on the same line,
as long as the symbol isn't in a string!

.. code-block:: decision

   Start~#1

   > Now this is a story all about how my life got flipped-turned upside down...

   > This next line is in a comment, so the program won't actually print here.
   > Print(#1, "Hello, world!")

   > This next line still prints as you'd expect.
   Print(#1, "So, 2 > 1? Wow.")

.. code-block::

   $ decision comment.dc
