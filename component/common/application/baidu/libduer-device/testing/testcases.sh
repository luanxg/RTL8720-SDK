#!/bin/bash

#echo "XXXXXXXXXXXXYYYYYYYYYYYYYYYYYYYY:  $*"
#TEST_CASES=`find . -name "lightduer_*_test"`
TEST_CASES=$*

for test_case in $TEST_CASES
do
    $test_case
done
