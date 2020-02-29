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

#####################
Decision Fundamentals
#####################

Sockets
=======

Sockets contain data of a pre-determined type (essentially data types):

Execution
    These kinds of sockets don't hold *data*, but they determine what actions
    occur when they are *activated*. If an input execution socket is
    activated, the **node's** action is performed. If an output execution
    socket is activated, it passes the execution of the program to the
    connected input execution socket.

Integer
    These sockets hold integer (whole number) values.

Float
    These sockets hold floating-point values.

String
    These sockets hold strings, essentially an array of characters.

Boolean
    These sockets hold *true* (1) or *false* (0) values.

.. note::
   The following data types have not been implemented yet. But they will be.
   Soon. Maybe.

Collection
    These sockets hold a collection of data.

Programmer-defined Structure
    These sockets hold defined structures of data.

Programmer-defined Class
    These sockets hold objects instantiated from classes.

.. note::

    Integers, floats, and booleans are passed from socket-to-socket by **value**,
    whereas strings, collections, structures and classes are passed from
    socket-to-socket by **reference**. This is because of the way these values
    are represented in memory.

Wires
=====

Wires connect 2 **sockets** together. The sockets they connect must be of the
same data type, however. Wires allow data to be transferred between **nodes**,
allowing you to take whatever output you want and *feed* it into the input of
your choice.

An alternative to wires (if you know an input is always going to be the same
value) are **literals**, which is data (that is the same data type as the
input socket) that the socket uses instead of getting the value from a wire.

Wires are analogous to *local variables* in most common programming languages.

Nodes
=====

Nodes are the building blocks of the Decision language. They dictate what
happens when they are activated, and what calculations happen. They are
analogous to *statements* in most common programming languages.

All nodes have inputs and outputs in the form of **sockets**. What the node
does with the inputs and what it outputs depends on the node.

Execution nodes
---------------

Execution nodes are nodes that have at least one **execution socket**. These
nodes perform an action once their primary input execution socket has been
activated. Once that action is done, the node will then activate it's primary
output execution socket to pass the execution of the program to the next
execution node.

Definition nodes
^^^^^^^^^^^^^^^^

Definition nodes are execution nodes that do not have any input sockets. These
nodes are therefore the start of a path of execution. Essentially, they define
what functions do.

Non-execution nodes
-------------------

Non-execution nodes have no **execution sockets**. These nodes perform a
calculation on their inputs without affecting the state of the program. These
nodes are used to manipulate values before they are fed into execution nodes.

Sheets
======

Every Decision source file represents a sheet. Sheets are essentially a
collection of **nodes**, **wires** and **properties**. (The fact that they are
a collection of nodes and wires means you can model sheets as directed graphs,
for you mathematicians out there)

Properties
==========

Properties aren't necessarily *stored* in sheets, but they manipulate the
sheet's metadata to provide extra information, or to specify something about
the sheet. For example:

* Properties can define **global variables** that nodes can get and set the
  value of.
* Properties can define **functions/subroutines**, and what their inputs and
  outputs are.
* Properties can be used to **import other sheets**, which allows you to use
  other people's programs or libraries for your convenience.
