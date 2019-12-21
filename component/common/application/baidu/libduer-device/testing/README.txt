required:
    cmake(compile system)
    cmocka(unit test framework with mock func support)
    lcov(code coverage tool)

test folder structure is same with the source code;
test is file-based: every file with one test file;
test is function-based: every function in the file with one test function;

run command:
# create the build folder and enter
mkdir build
cd build
# create the Makefile
cmake -DCMAKE_BUILD_TYPE=Debug ..
# compile
make
# run the test cases
make ut_test
#brower the coverage report
ut_test/index.html
