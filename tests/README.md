# Decision Testing

This directory contains a series of tests to make sure that the final
executable is working properly.

## Environment variables

In order for the tests to know where the Decision executable lives, you will
need to define an environment variable before running the tests. The variable
`EXECUTABLE` needs to hold an *absolute* path to a working Decision executable.
If you have installed Decision to a location that is accessible via the `PATH`
variable, then the value can simply be `decision`.

```bash
# If Decision is accessible via PATH:
EXECUTABLE="decision" tests/run_tests.sh

# Otherwise, set EXECUTABLE to be an absolute path to the executable:
EXECUTABLE="/home/user/decision/build/decision" tests/run_tests.sh

# A shortcut is to use the output of the pwd command:
EXECUTABLE="$(pwd)/build/decision" tests/run_tests.sh
```

## Running Tests

To run all of the tests, simply run the bash script, remembering to create the
environment variable as mentioned earlier:
```bash
EXECUTABLE="/path/to/decision" tests/run_tests.sh
```

## Adding Tests

To add a test, follow these steps:

1. Create a subdirectory inside `tests/`:
```bash
mkdir tests/my_new_test
cd tests/my_new_test
```

2. Create the source file that you want to check the output for.
```
tests/my_new_test/my_new_test.dc

Start~#1
Print(#1, "Hello, world!")
```

3. Create a text file whose content is what you expect to be output from the
source file.
```
tests/my_new_test/my_new_test.out

Hello, world!
```

4. Create a `.test` file, which is a bash script to call the functions defined
in `tests/test_functions.sh`.
```bash
tests/my_new_test/my_new_test.test

#!/bin/bash

source ../test_functions.sh

# Take the output of running the source code directly, and diff it with the
# content in the .out file.
testdecision my_new_test.dc my_new_test.out

# Compile the source file into an object file, run the object file, and diff it
# with the content in the .out file.
testdecisioncompile my_new_test.dc my_new_test.out
```

And you're done! This new test file should be automatically picked up and run
by `tests/run_tests.sh`