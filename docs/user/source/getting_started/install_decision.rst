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

###################
Installing Decision
###################

From a pre-built binary
=======================

You can get the pre-built executables and libraries of Decision from
`the GitHub repository <https://github.com/drwhut/decision/releases>`_,
which will come in either a zip file or a tarball, depending on the platform.
You can then extract the files and start using Decision!

From the source code
====================

If for some reason a pre-built binary is not available, or you want to build
a custom version of Decision, or just want to try compiling from source,
it's quite easy! Just follow the steps below for your platform:

Windows
-------

Dependencies
^^^^^^^^^^^^

Visual Studio IDE
    The program used to compile the project. When installing, make sure that
    you install C++ support as well!

    You can download it `here <https://visualstudio.microsoft.com/>`_.

CMake
    The program used to automatically create the Visual Studio project.

    You can download it `here <https://cmake.org/download/>`_.

Git for Windows (Optional)
    If you want to clone the GitHub repository rather than downloading a
    zip of the source code, you can use git to do so. This also has the added
    bonus of allowing you to update the code easily rather than re-downloading
    the zip every release. It also has **Git Bash**, which is a thousand times
    better than Powershell, *fight me*.

    You can download it `here <https://gitforwindows.org/>`_.

.. note::

   If you already use a developer environment like **MinGW**, CMake *should*
   detect it.

Installation
^^^^^^^^^^^^

1. Download the source code.

   * If you installed git, you can do this by simply entering this command
     into Git Bash:

     .. code-block::

        git clone https://github.com/drwhut/decision.git

   * Otherwise, you can visit the
     `GitHub page <https://github.com/drwhut/decision>`_ to download the zip
     file and extract it.

2. Navigate to the root folder of the project, i.e. where the ``README.md``
   file exists, and create a folder called ``build``.

3. Inside that ``build`` folder, open up a command prompt (either cmd.exe,
   Powershell or Git Bash), and enter this command:

   .. code-block::

      cmake -DCOMPILER_32=ON ..

   * This will create the Visual Studio solution inside the ``build`` folder.

.. note::

   By default, this step will create a Visual Studio solution for 32-bit
   Windows (x86). If you want to create a Visual Studio solution for 64-bit
   Windows (x64), you need to specify this:

   .. code-block::

      cmake -DCOMPILER_32=OFF -A x64 ..

4. Build the appropiate executable with one of the following commands:

   .. code-block::

      cmake --build . --config Debug
      cmake --build . --config Release

   **And that's it!** However, if you want to build and run Decision directly
   from Visual Studio, follow steps 5-8.

5. Open the Visual Studio solution.

6. (Optional) If you want to run Decision from within Visual Studio, you
   should set the "decision" project to be the start-up project by
   right-clicking on the "decision" project in the solution explorer and
   clicking "Set as StartUp Project".

7. Select either the "Debug" or "Release" version at the top, depending on
   your needs.

8. Click Build > Build Solution.

Linux
-----

Dependencies
^^^^^^^^^^^^

GCC
    This is the main C/C++ compiler used on Linux.

make
    This is the program that builds the project.

CMake
    The program used to automatically create the file that make uses to build
    the project.

git (Optional)
    If you want to clone the GitHub repository rather than downloading a
    tarball of the source code, you can use git to do so.

.. note::

   On Ubuntu, you can install all the dependencies you need by running these
   commands:

   .. code-block::

      sudo apt-get update
      sudo apt-get install build-essential cmake git

Installation
^^^^^^^^^^^^

1. Download the source code.

   * If you installed git, you can do this by simply entering this command:

     .. code-block::

        git clone https://github.com/drwhut/decision.git

   * Otherwise, you can visit the
     `GitHub page <https://github.com/drwhut/decision>`_ to download the
     tarball file and extract it:

     .. code-block::

        tar -xf decision-master.tar.gz

2. Navigate to the root folder of the project, i.e. where the ``README.md``
   file exists, and create a directory called ``build``:

   .. code-block::

      mkdir build && cd build

3. Inside the ``build`` directory, create the CMake project:

   .. code-block::

      cmake ..

4. If the CMake project was created, build the project:

   .. code-block::

      make

5. (Optional) If you want to, you can install Decision into a standard path
   on the system so you can run it from anywhere:

   .. code-block::

      sudo make install

   * By default, CMake installs Decision in the ``/usr/local`` directory.
     If you want to change this, add this argument when creating the CMake
     project, e.g.:

     .. code-block::

        cmake -DCMAKE_INSTALL_PREFIX=/usr ...

Options
-------

32-Bit Mode
^^^^^^^^^^^

If you want the interpreter and compiler to store data in 32 bits, add this
argument when creating the CMake project:

.. code-block::

   cmake -DCOMPILER_32=ON ..

.. note::

   This option will only work properly on 32-bit machines! Applying this
   option on 64-bit machines will not work since it will not be able to store
   full 64-bit pointers.

Build a DLL
^^^^^^^^^^^

On Linux, both the static library (.a) and the shared library (.so) are built.
On Windows however, only the static library (.lib) is built by default.
If you want to create a DLL instead of a static library on Windows, add
this argument when creating the CMake project:

.. code-block::

   cmake -DCOMPILER_DLL=ON ..

Enable C API Tests
^^^^^^^^^^^^^^^^^^

If you want CMake to generate tests testing Decision's C API, add this argument:

```bash
cmake -DCOMPILER_C_TESTS=ON ..
```

Note that this option will generate a lot more executables alongside the
compiler executable. See ``tests/README.md`` for more details.
