# Decision Documentation

The documentation is split up into 2 manuals: A **user** manual and a
**developer** manual.

## Which manual should I read?

* If you are going to be using this compiler for general use, or don't care how
the compiler works under the hood, read the **user** manual.
* If you are going to be contributing to the development of the compiler, want
to understand how it works under the hood, or want to include the code in your
own project, read the **developer** manual.

## How do I build the documentation?

The documentation is created in *reStructuredText* format, and is built using a
Python tool called [Sphinx](http://www.sphinx-doc.org/en/master/) to convert
the text into an HTML format thats easy to read.

The **developer** documentation also includes a reference to all of the
functions, structures, etc. defined in the header files of the project.
This information is compiled into XML format with **Doxygen**, and then used
by **Breathe** (Haha, get it? Because it *breathes* oxygen? Bet the programmer
who came up with that one is chuffed with themselves) to include it into the
**Sphinx** documents.

### Installing Dependencies

Firstly, you will need `pip` installed alongside Python. `pip` is a package
manager for Python that allows you to install libraries and tools made in
Python.

With pip installed, you need to run this command to install the dependencies:
```bash
pip install -U sphinx sphinx_rtd_theme
```

For the **developer** documentation, you will also need to install **Doxygen**
and **Breathe**:
```bash
pip install -U breathe

sudo apt install doxygen
```

You can download **Doxygen** [here](http://www.doxygen.nl/download.html) if
you are not on Linux.

### Building the Documentation

If you are building the **developer** documentation, you should first generate
the XML reference using **Doxygen**. To do this, go to the root directory of
the project, i.e. where `Doxyfile` is located, and run:
```bash
doxygen Doxyfile
```

This will generate XML files in `docs/developer/xml`, which **Breathe** will
then use in the next step:

With sphinx installed, you should now go to the manual directory of your choice,
and run the following command:
```bash
# On Linux:
make html

# On Windows:
make.bat html
```