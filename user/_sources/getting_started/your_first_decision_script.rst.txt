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

##########################
Your First Decision Script
##########################

Now that you have the Decision compiler on your computer, you can now get
started with your first script!

Sheets
======

Every Decision file is called a **sheet**. All sheets have the following
things in them:

* A set of **properties** that give the sheet extra functionality.

* A set of **nodes** that represent functionality.

  * Nodes themselves are essentially a set of **input and output sockets**.

* A set of **wires** that connect the nodes together with their sockets, and
  give the "flow" of the program.

Nodes can also be split up into two categories:

* **Execution** nodes are nodes that explicitly perform an action.
  They directly control the flow of execution when activated.

* **Non-execution** nodes are nodes that obtain or calculate values.
  They are only activated **when they need to be activated**, i.e. whenever
  another node needs to get its outputs.

That's all well and good, but what do these things actually *look* like in
Decision code?

The Start Node
==============

If you want your Decision script to be able to do anything, it has to start
somewhere, right? You'd be correct, and it just so happens that it starts
at the ``Start`` node!

The ``Start`` node is an execution node (because it performs an action by
starting the program), with no input sockets, and only 1 output socket:
What is the first thing you want to do?

So before we write anything else, we need to create this ``Start`` node.
Open your favourite text editor of choice and create it:

.. code-block:: decision

   Start~#1

Let's go through this step by step:

* By simply typing in the name of the node, you've created it! But by default
  it will not have any wires connected to it's sockets. If the ``Start`` node
  doesn't have any wires connected to it, your program won't do anything!

* So we tell the compiler we want to connect the output socket to some wire.
  We do this with the ``~`` (tilda) symbol.

* After the tilda, we then say *what* wires we should connect the output
  socket to. We do this by putting a ``#`` symbol followed by a positive
  integer of our choice, say, ``1``, so ``#1``.

  * You can think of it like ``Start`` will "trigger" it's outut socket, which
    will then send a signal down the ``#1`` wire to the next node.

.. note::

   Since the language is **strongly typed**, Whenever you create a wire like
   this, it automatically gains a "data type" based on which output of the node
   it is. In this example, it is of type ``Execution``, so we can only plug it
   in later to input ``Execution`` sockets.

We've created a wire, but we've only connected it at one end (to the ``Start``
node)! So, let's create another node to connect it to!

Hello, world!
=============

Another important node is the ``Print`` node, which once activated, will print
a value to the console. It has 2 inputs and 1 output: The first input is the
execution input (the input where ``Print`` will actually print once it's been
activated by the previous node, in our case, the ``Start`` node), and the
second input is the value you want to print to the screen. The output is
another execution socket so once the value has been printed to the screen,
you can go and do something else.

So now we have a node to connect the ``Start`` node to, let's add it to our
script we've got going:

.. code-block:: decision

   Start~#1
   Print(#1, "Hello, world!")

Let's examine what's changed:

* We've created the ``Print`` node simply by typing it's name by default,
  but in this case we *need* to provide some inputs or the compiler will
  complain.

* The way we provide the inputs is in a comma-seperated list surrounded by
  round brackets, as shown above.

  * The first input is the execution socket, which if we connect it to the
    output socket of the ``Start`` node with the ``#1`` wire, means this
    ``Print`` will be the first node to activate when we run the script.

  * The second input is the value you want to print to the screen. In this
    case, we can just put the value directly into the input. But later on
    we'll be feeding in wires into these inputs to output calculations we
    make.

* Notice that we didn't put a tilda symbol at the end of the ``Print`` node.
  This is because if you don't want to connect any of the output sockets,
  you don't have to put it there. In this case, we're not doing anything
  after the ``Print`` node, so the program will automatically stop when it
  realises there is no connection, i.e. there is nothing else to do.

Great! You now have a working script! You can now save the file, e.g. as
``hello_world.dc``, and run it!

.. code-block::

   $ decision hello_world.dc

   Hello, world!

.. note::

   As well as having nodes on seperate lines, if you want to, you can have
   nodes on the same line but seperated with a semi-colon:

   .. code-block:: decision

      Start~#1 ; Print(#1, "Hello, world!")
