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

.. _error-reporting:

###############
Error Reporting
###############

Error reporting is perfomed in ``derror.c`` and ``derror.h``.

Throughout the compilation process, we are checking to see if the program the
user has written is *correct*. If it is not, we need to let the user know why.

Thankfully, there are functions to allow errors and warnings to get to the
user:

.. doxygenfunction:: d_error_compiler_push
   :no-link:

This function adds an error message to a list of error messages to be reported
to the user. If you want to give a more detailed error however, there is an
alternative macro function for you to use:

.. doxygendefine:: ERROR_COMPILER
   :no-link:

The arguments after ``isError`` are put directly into an ``sprintf`` call, so
you can fill out the arguments like any format call. For example:

.. code-block:: c

   ERROR_COMPILER("main.dc", 4, true, "My friend %s did a bad.", "Bob");

We can then call:

.. doxygenfunction:: d_error_report
   :no-link:

to print all of the errors and warnings we've stored to ``stdout``. It also
returns true if there was at least one error.

.. doxygenfunction:: d_error_free
   :no-link:

This function needs to be called after we report errors, to free the error
messages themselves from memory. We love you, C.

Whenever you load a string or a file, after :ref:`semantic-analysis` is
complete (or :ref:`syntax-analysis`, depending on which went wrong first),
``d_error_report()`` is called to report errors. If there were errors, i.e.
it returned true, then **the compiler does not bother generating bytecode**
with :ref:`code-generation`.
