### Include Analyzer

Alexander Drozdov

last updated 19.11.2018

### Files

analyzer.cxx

### Overview

The program will print include tree for a c++ project. We will recursively go through the project directory, find all instances of main() to determine executables within project, and print include hierarchy for each of them into standard output.

### Structure

The program will consist of a recursive function that parses a file for includes and then calls itself for every include.
As an optimization we shall cache files which we already parsed.

### Dependencies

We shall use only a c++17 compliant standard library implementation.

### Constraints

Program execution speed shall not greatly exceed the filesystem access speed.

### Standing issues

None



### Implementations details

### Data structures

# IncludeCache

map<string, FileDescription> to cache parsed files.

# FileDescription

a structure containing information on parsed file.

Consists of:

# m_flags 

enumeration, a flag to indicate the type of file state may be None - normal include, External - file is an external include, Missing - file is missing from filesystem.

# m_includes 

vector<string> to hold names of included files. Shall be identical to IncludeCache Key for respective files.

### Functions

# optional<string> readFile(file_path)

a function to read specified file into standard string. Shall return file content on success; shall print error into standard error output and return non-value if read error occurs or file can't be read.

# printIncludes(include_cache, include_stack, this_path, level)

main recursive function. Shall take reference to include_cache of type IncludeCache; include_stack of type stack<file_path>, containing the files which already have been visited in current recursion branch, in order, and in which the first entry shall be used to determine project root directory; this_path of type file_path, containing file to parse; an integer level, denoting current depth of recursion. Shall return nothing.

Shall print files as tabulated tree to standard output, in order they are included. Shall also implicitly include source file for every included header, if present, and parse it before header. Shall cache parsed files into include_cache.

# main()

entry point, shall take standard input, verify it, and run printIncludes on it. Shall return 0 on successful parsing and -1 otherwise.
