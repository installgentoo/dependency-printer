### OVERVIEW
This is a C++ program which prints include tree for C++ project.

The program recursively searches project directory for source files that contain main() function, and prints an include tree for every such file. Program searches for includes first in directory containing master file, and then in project directory.

Program also prints specifier for includes:
(Ext) - external include. We consider files included through #include< > to be external;
(C) - circular dependency;
(!) - missing include;

### COMPILING
Configure with cmake and compile with your build tool of choice. Program doesn't have any dependencies besides standard library.

On Linux
g++ -O2 -std=c++17 -Wall ./analyzer.cxx -o analyzer -lstdc++ -lstdc++fs

On Windows
use your favorite IDE, select c++17

### RUNNING
Run "analyzer --help" for command line parameters
