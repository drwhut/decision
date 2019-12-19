#!/bin/bash

# NOTE: In order for the tests to work, a variable must be defined either in
# the global environment or the environment of the run_tests.sh file.
# Before running run_tests.sh, define EXECUTABLE to be a valid path to the
# Decision executable, whether you have just built it, or it is now in a PATH.
# e.g. EXECUTABLE="decision" ./run_tests.sh (If decision is in PATH)
# e.g. EXECUTABLE="/home/user/decision/build/decision" ./run_tests.sh

# Flags to pass to diff.
DIFF_FLAGS=""

# A function for what to do depending on if a test succeeded or not.
# The first argument should be 0 if the test succeeded, 1 if not.
function testdone {
	if [ $1 != 0 ]
	then
		# The test failed.
		echo "FAIL"
		let DECISION_TEST_FAIL++
		DECISION_FAILED_TESTS="$DECISION_FAILED_TESTS\n$DECISION_CURRENT_TEST"
	else
		# The test succeeded.
		echo "SUCCESS"
		let DECISION_TEST_SUCCESS++
	fi
}

# A function to run a Decision file, and compare it against some output we expect.
# The 1st argument is the source file.
# The 2nd argument is the output file.
function testdecision {
	echo "- Running diff on $EXECUTABLE $1 and $2 ..."
	diff $DIFF_FLAGS <("$EXECUTABLE" $1) <(cat $2)
	testdone $?
}

# A function to compile a Decision file, run the object file, and compare it
# against some output we expect.
# The 1st argument is the source file.
# The 2nd argument is the output file.
function testdecisioncompile {
	echo "- Compiling $1 ..."
	"$EXECUTABLE" -c $1
	testdecision $1o $2
}
