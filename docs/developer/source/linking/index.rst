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

.. _linking:

*******
Linking
*******

This chapter describes how linking objects before runtime works. Linking
functionality can be found in ``dlink.c`` and ``dlink.h``.

Decision is a **compiled language**, and you compile individual sheets into
their respective **objects**. Most of the time you will want to just run a
sheet straight from the terminal. But sometimes, especially if the sheet is
very big, it may be worth your while to compile the sheet into an **object
file** to increase performance (since it doesn't compile the program every
time).

The fact that you can compile a sheet to an object file is the reason why
we need to link. Every time you load an object from an object file in memory,
it is **very likely** that it will not be places into the same memory address
every time. So, we will need to change where the instructions in the ``.text``
section point to, and that is exactly what we do when linking.

###############
Object Sections
###############

Objects have 2 sections that are purely used for linking:

``.lmeta``

    A list of ``LinkMeta``, where each ``LinkMeta`` describes a thing like a
    variable, function, or string literal, and where you can find it (if it's
    defined on the same sheet).

``.link``

    A table of instruction indexes and which ``LinkMeta`` they link to.

##########
Structures
##########

``LinkType``

    An *enum* which describes what kind of thing a link points to, whether it
    be a variable, function, or something else.

``LinkMeta``

    A *struct* that describes a link. It includes a ``LinkType``, and the
    ``const char*`` name of the item it is linking to. In the case of string
    literals, the name is the content of the literal. It also contains a
    generic ``void*`` pointer that is designed to point to a struct that
    contains metadata for the object being linked to, i.e. a ``SheetVariable``
    or ``SheetFunction``. It also contains a generic ``char*`` pointer which
    serves different purposes depending on what the ``LinkType`` is:

    * If it is a link to an item in the object's own ``.data`` section, the
      pointer's value is the starting index of the item in the ``.data``
      section.
    
    * If it is a link to an item in the object's own ``.text`` section, the
      pointer's value is the starting index of the item in the ``.text``
      section.

``LinkMetaList``

    A *struct* that is a list of ``LinkMeta``.

#########
Functions
#########

.. doxygenfunction:: d_link_new_meta
   :no-link:

``LinkMetaList`` by itself has a few helper functions to help create it, free
it, and to manipulate it's contents:

.. doxygenfunction:: d_link_new_meta_list
   :no-link:

.. doxygenfunction:: d_link_meta_list_push
   :no-link:

.. doxygenfunction:: d_link_free_list
   :no-link:

.. doxygenfunction:: d_link_replace_load_ins
   :no-link:

This function is what actually changes the instructions in ``.text`` to point
to the correct items. Since we use pointers to set links, the instructions
need to load either 32-bit or 64-bit addresses into a register. This is always
done with a ``LOADUI`` / ``ORI`` combination.

.. doxygenfunction:: d_link_self
   :no-link:

Takes a compiled sheet, gets the information it needs to link from the
``.lmeta`` and ``.link`` sections, and uses it to call
``d_link_replace_load_ins`` to replace all of the instructions that need
linking.
