#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <regex>

//why would i do this? because macros can be made separate color and thus all const values become distinguishable from kerywords at glance
//paired with obsessive const correctness this increases readability immensely
#define VAL const auto

typedef uint32_t uint;

//global std using since this is cpp
namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::map;
using std::optional;
using std::move;
using std::istreambuf_iterator;
using std::ifstream;
using std::sregex_iterator;

//filetypes that we consider source files
static const string CPP_EXTENSIONS[] = { ".cpp", ".cxx", ".cc", ".c" };

static bool confirmCppFilename(fs::path const&file_path)
{
  for(VAL &ext: CPP_EXTENSIONS)
    if(file_path.extension() == ext)
      return true;

  return false;
};

//printer helpers
static void PRINT_HELP() {
  std::cout<<"Usage: analyzer [DIR]\n"
          <<"Prints include tree for every source file in DIR containing main() function\n"
         <<"\n"
        <<"appends status for special cases:\n"
       <<"(Ext) - external include\n"
      <<"(C) - circular dependency\n"
     <<"(!) - file not found\n"
    <<"\n"
   <<"Exit Status:\n"
  <<"Returns 0 unless an invalid option is given\n";
}
static void PRINT_INCLUDE(uint level, string const&include)  { std::cout<<string(level * 2, ' ')<<include<<"\n"; };
static void PRINT_EXTERNAL(uint level, string const&include) { std::cout<<string(level * 2, ' ')<<include<<" (Ext)\n"; };
static void PRINT_ERROR(uint level, string const&include)    { std::cout<<string(level * 2, ' ')<<include<<" (!)\n"; };
static void PRINT_CYCLIC(uint level, string const&include)   { std::cout<<string(level * 2, ' ')<<include<<" (C)\n"; };




//helper for putting file into string
static optional<string> readFile(fs::path const&file_path)
{
  if(ifstream file(file_path, ifstream::in); file.is_open())
  {
    file.exceptions(ifstream::failbit | ifstream::badbit);

    try {
      return { string{ istreambuf_iterator<char>(file), istreambuf_iterator<char>() } };
    } catch(ifstream::failure const&) { }
  }

  std::cerr<<"Error accessing file "<<file_path<<"\n";

  return { };
}


//map to store parsed files - their type and includes, if any
struct FileDescription {
  enum Flags : uint { None = 0x0, External = 0x1, Missing = 0x2 };

  Flags m_flags;
  vector<string> m_includes;
};
typedef map<string, FileDescription> IncludeCache;


// recursively parses files and prints includes for each one.
/// include_cache - map to store includes parsed from files;
/// include_stack - vector to store walked paths for current recursion branch, so that we may check for circular dependencies. first entry's location will be considered the project dir root
/// this_path - path for the file we wish to start parsing from
/// level - current recursion level for print formatting
static void printIncludes(IncludeCache &include_cache, vector<fs::path> include_stack, fs::path const&this_path, uint level)
{
  /* check and print if circular dependency */
  VAL CHECK_CIRCULAR = [&](fs::path const&path){
    for(VAL &i: include_stack)
      if(i == path)
      {
        PRINT_CYCLIC(level + 1, path.relative_path().string());
        return true;
      }

    return false;
  };

  /* using relative path from working dir for includes filenames */
  VAL relative_path = this_path.relative_path().string();

  /* print from cache if we already parsed this file */
  if(VAL existing = include_cache.find(relative_path); existing != include_cache.cend())
  {
    switch(existing->second.m_flags)
    {
      case FileDescription::External: PRINT_EXTERNAL(level, relative_path); return;
      case FileDescription::Missing:  PRINT_ERROR(level, relative_path);    return;
      case FileDescription::None:     PRINT_INCLUDE(level, relative_path);
    }

    include_stack.emplace_back(this_path);

    for(VAL &i: existing->second.m_includes)
      [&]{
      if(CHECK_CIRCULAR(i))
        return;

      printIncludes(include_cache, include_stack, i, level + 1);
    }();

    return;
  }

  /* print filename and parse the file */
  PRINT_INCLUDE(level, relative_path);

  include_stack.emplace_back(this_path);

  auto this_entry = std::make_pair(relative_path, FileDescription{ FileDescription::None, { } });
  auto &this_entry_includes = this_entry.second.m_includes;

  static VAL INCLUDE_RX = std::regex{ "#include *[<|\"].+?[>|\"]" };

  if(VAL content = readFile(this_path); content.has_value())
    /* find #include directives */
    for(auto i=sregex_iterator(content->cbegin(), content->cend(), INCLUDE_RX), end=sregex_iterator{}; i!=end; ++i)
    {
      auto include = i->str();
      /* we assume that parsed files conform to guidelines and <> are used for external libs, "" for files in project */
      VAL is_ext_lib = include.back() == '>';

      VAL name_start_pos = include.find_first_of("<\"") + 1;
      include = include.substr(name_start_pos, include.size() - name_start_pos - 1);

      if(is_ext_lib)
      {
        /* external libs do not require recursion */
        include_cache.emplace(std::make_pair(include, FileDescription{ FileDescription::External, { } }));
        this_entry_includes.emplace_back(include);
        PRINT_EXTERNAL(level + 1, include);
      }
      else
      {
        /* find include in the same dir or in project root */
        auto path = fs::path{ this_path.parent_path().string() + "/" + include };
        if(!fs::is_regular_file(path))
          path = fs::path{ include_stack.front().parent_path().string() + "/" + include };

        include = path.relative_path().string();

        if(!fs::is_regular_file(path))
        {
          include_cache.emplace(std::make_pair(include, FileDescription{ FileDescription::Missing, { } }));
          this_entry_includes.emplace_back(include);
          PRINT_ERROR(level + 1, include);
        }
        else
          [&]{
          this_entry_includes.emplace_back(include);

          /* if this is a cpp and include has same name consider it our header */
          VAL include_is_self_header = ((fs::path{ path }.replace_extension("") == fs::path{ this_path }.replace_extension("")) && confirmCppFilename(this_path) );

          /* print source file for include, if present */
          if(!include_is_self_header &&
             !confirmCppFilename(path))
            for(VAL &ext: CPP_EXTENSIONS)
              if(auto cpp_path = fs::path{ path }.replace_extension(ext); fs::is_regular_file(cpp_path))
              {
                if(CHECK_CIRCULAR(cpp_path))
                  return;

                printIncludes(include_cache, include_stack, cpp_path, level + 1);
              }

          if(CHECK_CIRCULAR(path))
            return;

          /* print header */
          printIncludes(include_cache, include_stack, path, level + 1);
        }();
      }
    }

  //finally put this entry into cache after recursion cached all of its branches
  include_cache.emplace(move(this_entry));
}


int main(int argc, char *argv[])
{
  /* check input */
  if(argc != 2 ||
     argv[1][0] == '\0' ||
     argv[1][0] == '-')
  {
    PRINT_HELP();
    return -1;
  }

  VAL base_dir_name = argv[1];
  VAL base_dir = fs::path{ base_dir_name };

  if(!fs::is_directory(base_dir))
  {
    std::cout<<"No directory "<<base_dir<<"\n\n";
    PRINT_HELP();
    return -1;
  }

  VAL contains_main = [](VAL &file_path){
    if(VAL content = readFile(file_path); content.has_value())
      return content->find("int main(") != string::npos;

    return false;
  };

  /* walk the dir and print include tree for every source containing main */
  IncludeCache includes_cache;

  for(VAL &file: fs::recursive_directory_iterator(base_dir))
    if(VAL path = file.path(); file.is_regular_file())
      if(confirmCppFilename(path) &&
         contains_main(path))
      {
        std::cout<<string(80, ':')<<"\n";
        printIncludes(includes_cache, { base_dir }, file, 0);
      }

  return 0;
}
