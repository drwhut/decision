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

************
Introduction
************

Who is Decision?
================

Original Author
---------------

Hi, I'm **Ben Beddows**, a.k.a. **drwhut**, and I'm a second-year Computer
Science student at Durham University.

What is Decision?
=================

Inspired by Unreal Engine 4's *Blueprints* and the programming language *Lua*,
Decision is a programming language that is:

* **General-purpose**: You can use Decision in almost any environment, whether
  it's on its own, or as a scripting language for another program.
* **Visual**: The syntax of the source code is in the form of a flowchart -
  data flows from nodes through wires into other nodes.
* **Compiled**: The language compiles source code into its own bytecode that
  is then executed. Decision allows you to run compiled bytecode directly for
  performance-heavy applications.

Where is Decision?
==================

Decision is being developed on
`this GitHub respository <https://github.com/drwhut/decision>`_.

When is Decision?
=================

Decision is currently in version 0.1.1

The version format is ``[Major Build].[Minor Build].[Patch]``, where an
increase in the major build indicates a major rework, an increase in the minor
build indicates new features, and an increase in the patch number indicates
bug fixes with each minor build.

I'll do you one better... why is Decision??
===========================================

One day in 6th form, I had just learnt the 5 stages of compilation in an
A-Level Computer Science lesson. Shortly after this lesson, I got bored and
wanted a big programming project to work on, so I decided to use what I learnt
in the lesson, plus some research of my own to create Decision.

However, for this project, I wanted to set some goals for myself:

* Decision should have lots of **documentation**, so if contributors wanted to
  help improve the project, that process is as easy as possible.
* Decision should be **adaptable**, so that you could find a use for it in any
  environment or situation.
* Decision as a language (including GUI editors) should be **intuitive** to
  use, so even if you've never programmed before, or can write code in a
  billion different languages, you would enjoy using this language.

How is Decision?
================

Good, you?

Decision is made purely with the C programming language. This is so you can
include it into any C or C++ project and will guarantee that the language is
small and as efficient as possible. (It also means pain where there shouldn't
be any...)

The project itself is set up with `CMake <https://cmake.org/>`_. This guarantees
that the language is cross-platform. See the ``README.md`` file at the root
directory of the project for instructions as to how to compile the project on
your local machine from the source.
