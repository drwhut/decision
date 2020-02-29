#!/bin/bash

# Variables to track the number of successes and fails.
export DECISION_TEST_SUCCESS=0
export DECISION_TEST_FAIL=0
export DECISION_FAILED_TESTS="List of failed tests :"

# Set the working directory to this file.
cd "$(dirname $0)"
# Find all of the test files.
for test in */*.test; do
	echo "Running test $test ..."
	export DECISION_CURRENT_TEST=$test
	cd $(dirname $test) # Go to the directory and
	source ./$(basename $test) # run the script.
	cd ..
done

# Print the results.
echo "# Successful tests   : $DECISION_TEST_SUCCESS"
echo "# Failed tests       : $DECISION_TEST_FAIL"

if [ $DECISION_TEST_FAIL -gt 0 ]
then
	echo -e "$DECISION_FAILED_TESTS"
	exit 1
fi
