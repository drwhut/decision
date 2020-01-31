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

##################
Embedding Decision
##################

Prerequisites
=============

To embed Decision into your C/C++ application, you will need 2 things:

* The Decision header files.

* A compiled Decision library, which can be static or dynamic.

The best way to get both is to build Decision from the source code. There are
instructions on how to build Decision from source both in the ``README.md``
file in the root of the project, and more detailed instructions in the
`user manual <https://drwhut.github.io/decision/user/getting_started/install_decision.html#from-the-source-code>`_.

When you build Decision, not only do you get the executable you probably
intended, but you also get a compiled library as well!

.. note::

   On Linux, you get both the static (.a) and dynamic (.so) libraries, whereas
   on Windows by default you get the static (.lib) library. To get the
   dynamic (.dll) library, you need to specify an argument when using
   ``cmake``, see ``README.md`` for more details.

So if you have the header files and the libraries at hand, here is how you can
embed Decision into your program:

Windows
=======

This section assumes you are using Visual Studio to build your C/C++ project.

To get Visual Studio to see the header files, you can do the following:

1. In the Solution Explorer, right-click on the project and select
   **Properties**.

2. In **C/C++**/**General**, in the **Additional Include Directories** entry,
   either insert the path to the header files manually, or click on the arrow
   button to the right to use the in-built path editor.

3. Now, no errors should appear about missing Decision header files, and it
   should start auto-completing functions for you.

To get Visual Studio to link with the compiled Decision library:

1. In the Solution Explorer, right-click on the project and select
   **Properties**.

2. In **Linker**/**General**, in the **Additional Library Directories** entry,
   either intert the path to the folder where the library is located manually,
   or click on the arrow button on the right to use the in-built path editor.

3. In **Linker**/**Input**, in the **Additional Dependencies** entry, enter
   into the semi-colon seperated list "decision.lib", without quotes.

4. Now, when you build and run the project, it should link properly.

.. note::

   If you built a DLL version of the Decision library, if your program is
   complaining that it cannot find decision.dll, copy that file to the same
   folder as the program that is being run.

Linux
=====

This section assumes you are using gcc manually to build your C/C++ project.

To get your program to see the header files, add the ``-I`` flag when you are
compiling the C/C++ file:

.. code-block::

   gcc -I/home/USERNAME/decision/src -c main.c

To get your program to link with the library, use the ``-ldecision`` and ``-L``
flags:

.. code-block::

   gcc main.o -o main -L/home/USERNAME/decision/build -ldecision

.. note::

   If you installed Decision with ``sudo make install`` into a common prefix
   like ``/usr/local``, then you may omit the ``-L`` flag.

   Installing Decision is generally recommended, as it means that if you link
   with the dynamic library (libdecision.so), you don't have to give an
   environment variable every time to tell the executable where the library
   lives, it should just find it under a known list of directories.
