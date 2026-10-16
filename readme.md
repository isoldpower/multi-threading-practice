# C++ Project Template
This is a template for C++ projects. It includes a Makefile for building the project and running tests. 
The project is set up to use the Google Test framework for unit testing.

**_NOTE_**: Under the default configuration, the first run of the project will be slow because of the external libraries downloading and building. 

## Usage
To manage the project, template provides a Makefile with the following commands:
- `make rebuild` - build the project
- `make test` - build the project and run tests

## For developers
This section targets those who wants to better understand the essence behind
the project structure and module relations

### Modules structure

```
[project_name]
  ├── .github                       # GitHub actions directory
  │   └── workflows                 # GitHub actions workflows directory
  │       └── multi-platform.yaml   # Actions scenario where the project is built and tested on different platforms
  │
  ├── .clang-format                 # Clang format file, adjust it how you want
  ├── .gitignore                    # Git ignore file
  ├── CMakeLists.txt                # Main config file with general project settings
  ├── Makefile                      # Makefile with project management commands. Adjust it to add more commands
  │
  ├── [project_name]                # Main project directory
  │   ├── CMakeLists.txt            # Libraries config file
  │   ├── tools                     # Example reusable library, owing nothing to a particular application
  │   │   ├── CMakeLists.txt        # Library config file
  │   │   ├── include/tools         # Public headers, included as <tools/named_tools.h>
  │   │   │   └── named_tools.h
  │   │   ├── src/tools             # Library sources, mirroring the headers layout
  │   │   │   └── named_tools.cpp
  │   │   └── tests                 # Library tests directory
  │   │       └── main.cpp
  │   │
  │   └── application               # Application definitions built on top of the libraries
  │       ├── CMakeLists.txt
  │       └── structures_scenarios  # Example application layer, included as <application/structures_scenarios/...>
  │           ├── CMakeLists.txt
  │           ├── include/application/structures_scenarios
  │           └── src/application/structures_scenarios
  │
  ├── executables                   # Thin entry points wiring application layers together
  │   ├── CMakeLists.txt            # Executables config file
  │   └── benchmarks                # Group of related executables
  │       ├── CMakeLists.txt
  │       └── print_hello.cpp       # Example executable source file
  │
  ├── external                      # Directory for external libraries, i.e. Google Test, JSON, etc.
  │   └── CMakeLists.txt            # External libraries config file
  └── readme.md                     # Project README file
```

## Examples
The example of a working project with this template structure
### AI Warships Game
Warships game against AI with skills, game saves, and progression.

[![Readme Card](https://github-readme-stats.vercel.app/api/pin/?username=TrofimovVladislav5&theme=tokyonight&height=200&repo=cpp-warships)](https://github.com/TrofimovVladislav5/cpp-warships)

TODO:
- [ ] Write benchmarks tests
- [ ] Complete two other laboratory works