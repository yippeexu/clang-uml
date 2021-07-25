# clang-uml - UML diagram generator based on Clang and PlantUML

![linux build](https://github.com/bkryza/clang-uml/actions/workflows/build.yml/badge.svg)

`clang-uml` is an automatic [PlantUML](https://plantuml.com) class and sequence
diagram generator, driven by YAML configuration files. The main idea behind the
project is to easily maintain up-to-date diagrams within a code-base or document
existing project code. The configuration file or files for `clang-uml` define the
type and contents of each diagram.

## Features
Main features supported so far include:

* Class diagram generation
    * Basic class properties and methods including visibility
    * Class relationships including associations, aggregations, dependencies and friendship
    * Template instantiation relationships
    * Relationship inference from C++ containers and smart pointers
    * Namespace based content filtering
* Sequence diagram generation
    * Generation of sequence diagram from one code location to another

## Installation

### Building from source
Currently the only method to install `clang-uml` is from source. First make sure
that you have the following dependencies installed (example for Ubuntu):

```bash
apt install ccache cmake libyaml-cpp-dev libfmt-dev libspdlog-dev clang-12 libclang-12-dev libclang-cpp12-dev
```

Then proceed with building the sources:

```bash
git clone https://github.com/bkryza/clang-uml
cd clang-uml
make submodules
# Please note that top level Makefile is just a convenience wrapper for CMake
make release
release/clang-uml --help

# Optionally
make install
# or
export PATH=$PATH:$PWD/release
```

## Usage

### Generating compile commands database
`clang-uml` requires an up-to-date
[compile_commands.json](https://clang.llvm.org/docs/JSONCompilationDatabase.html)
file, containing the list of commands used for compiling the source code.
Nowadays, this file can be generated rather easily using multiple methods:
  * For [CMake](https://cmake.org/) projects, simply invoke the `cmake` command
    as `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ...`
  * For Make projects checkout [compiledb](https://github.com/nickdiego/compiledb) or [Bear](https://github.com/rizsotto/Bear)
  * For Boost-based projects try [commands_to_compilation_database](https://github.com/tee3/commands_to_compilation_database)

### Invocation
By default, `config-uml` will assume that the configuration file `.clanguml`
and compilation database `compile_commands.json` files are in the
current directory, so if they are in the top level directory of a project,
simply run:
```bash
clang-uml
```

The output path for diagrams, as well as alternative location of
compilation database can be specified in `.clanguml` configuration file.

For other options checkout help:
```bash
clang-uml --help
```

### Configuration file format and examples

Configuration files are written in YAML, and provide a list of diagrams
which should be generated by `clang-uml`. Basic example is as follows:

```yaml
compilation_database_dir: .
output_directory: puml
diagrams:
  myproject_class:
    type: class
    glob:
      - src/**.h
      - src/**.cc
    using_namespace:
      - myproject
    include:
      namespaces:
        - myproject
    exclude:
      namespaces:
        - myproject::detail
    plantuml:
      after:
        - 'note left of @A(MyProjectMain) : Main class of myproject library.'
```

See ![here](docs/configuration_file.md) for detailed configuration file reference guide.

### Class diagrams

#### Example

Source code:

```cpp
#include <string>
#include <vector>

namespace clanguml {
namespace t00009 {

template <typename T> class A {
public:
    T value;
};

class B {
public:
    A<int> aint;
    A<std::string> *astring;
    A<std::vector<std::string>> &avector;
};
}
}
```

generates the following diagram (via PlantUML):

![class_diagram_example](docs/test_cases/t00009_class.png)

#### Default mappings

| UML                            | C++                                                                                                |
| ----                           | ---                                                                                                |
| Inheritance (A is kind of B)   | Public, protected or private inheritance                                                           |
| Association (A knows of B)     | Class A has a pointer or a reference to class B, or any container with a pointer or reference to B |
| Dependency (A uses B)          | Any method of class A has argument of type B                                                       |
| Aggregation (A has B)          | Class A has a field of type B or an owning pointer of type B                                       |
| Composition (A has B)          | Class A has a field of type container of B                                                         |
| Template (T specializes A)     | Class A has a template parameter T                                                                 |
| Nesting (A has inner class B)  | Class B is an inner class of A
| Friendship (A is a friend of B)| Class A is an friend class of B

#### Inline directives

TODO

### Sequence diagrams

#### Example

The following C++ code:

```cpp
#include <algorithm>
#include <numeric>
#include <vector>

namespace clanguml {
namespace t20001 {

namespace detail {
struct C {
    auto add(int x, int y) { return x + y; }
};
}

class A {
public:
    A() {}

    int add(int x, int y) { return m_c.add(x, y); }

    int add3(int x, int y, int z)
    {
        std::vector<int> v;
        v.push_back(x);
        v.push_back(y);
        v.push_back(z);
        auto res = add(v[0], v[1]) + v[2];
        log_result(res);
        return res;
    }

    void log_result(int r) {}

private:
    detail::C m_c{};
};

class B {
public:
    B(A &a)
        : m_a{a}
    {
    }

    int wrap_add(int x, int y)
    {
        auto res = m_a.add(x, y);
        m_a.log_result(res);
        return res;
    }

    int wrap_add3(int x, int y, int z)
    {
        auto res = m_a.add3(x, y, z);
        m_a.log_result(res);
        return res;
    }

private:
    A &m_a;
};

int tmain()
{
    A a;
    B b(a);

    return b.wrap_add3(1, 2, 3);
}
}
}
```

generates the following diagram (via PlantUML):

![sequence_diagram_example](docs/test_cases/t20001_sequence.png)

### Test cases

The build-in test cases used for unit testing of the `clang-uml`, can be browsed [here](./docs/test_cases.md).


## LICENSE

    Copyright 2021-present Bartek Kryza <bkryza@gmail.com>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
