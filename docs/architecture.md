# Architecture

<!-- toc -->

* [Overview](#overview)
* [Configuration model](#configuration-model)
* [Diagram model](#diagram-model)
  * [Common model](#common-model)
  * [Diagram filters](#diagram-filters)
* [Translation unit visitors](#translation-unit-visitors)
* [Diagram generators](#diagram-generators)
* [Command line handler](#command-line-handler)
* [Tests](#tests)
  * [Unit tests](#unit-tests)
  * [Test cases](#test-cases)
  * [Real code tests](#real-code-tests)

<!-- tocstop -->

This section presents general architecture and components of `clang-uml`.

> All diagrams below are generated by `clang-uml` and updated automatically.

## Overview

`clang-uml` is written in C++17 and
uses [Clang LibTooling API](https://releases.llvm.org/16.0.0/tools/clang/docs/LibTooling.html)
to traverse
the AST (Abstract Syntax Tree) of the source code and extract any information
relevant for a specified diagram.

The code is divided into several packages (namespaces), the main of them are:

- [`clanguml::config`](./namespaceclanguml_1_1config.html) - configuration
  handling
- [`clanguml::common`](./namespaceclanguml_1_1common.html) - common interfaces
- [`clanguml::class_diagram`](./namespaceclanguml_1_1class__diagram.html) -
  specializations for class diagrams
- [`clanguml::sequence_diagram`](./namespaceclanguml_1_1sequence__diagram.html) -
  specializations for sequence diagrams
- [`clanguml::include_diagram`](./namespaceclanguml_1_1include__diagram.html) -
  specializations for include diagrams
- [`clanguml::package_diagram`](./namespaceclanguml_1_1package__diagram.html) -
  specializations for package diagrams

![clang-uml packages](./architecture_package.svg)

## Configuration model

The configuration model consists of classes representing the configuration
specified in the YAML configuration files.

Depending on the option, it can either:

- be specified only at the top level of the configuration file
- only in the specific diagram configuration
- either of the above

The first group of options are stored in
the [`config::config`](structclanguml_1_1config_1_1config.html) class.

The second group is stored in a specific diagram config subclass, e.g.
[`config::sequence_diagram`](structclanguml_1_1config_1_1sequence__diagram.html)

The options in the last group are modeled in the
[`config::inheritable_diagram_options`](./structclanguml_1_1config_1_1inheritable__diagram__options.html).

![configuration model](./config_class.svg)

The YAML configuration file is parsed
using [yaml-cpp](https://github.com/jbeder/yaml-cpp) library:

![configuration load sequence](./load_config_sequence.svg)

For each possible option type, there must an implementation of a
YAML decoder - e.g.
[`YAML::convert<filter>`](./structYAML_1_1convert_3_01filter_01_4.html)
(for converting YAML nodes to configuration model classes)
and a YAML emitter - e.g.
[`operator<<`](./group__yaml__emitters.html#ga4c8bc075684b08daa379aef609bb6297)
(for generating YAML from configuration model classes).

## Diagram model

The diagram model namespace is divided into the [`common`](#common-model) model
namespace and 1 namespace for each supported diagram type.

### Common model

The [common diagram model namespace](./namespaceclanguml_1_1common_1_1model.html),
provides a set of classes representing typical UML and C++ concepts such as
diagram elements, packages, templates, and others which are shared by more than
1 diagram type.

![clang-uml packages](./common_model_class.svg)

The diagram elements are composed into a hierarchy spanning all major
namespaces,
depending on whether the element is specific for a single diagram type (
e.g. [`participant`](./structclanguml_1_1sequence__diagram_1_1model_1_1participant.html)),
or whether it's common for several diagram types (
e.g. [`package`](./classclanguml_1_1common_1_1model_1_1package.html)).

### Diagram filters

In order to ease the generation of diagrams, `clang-uml` has a (very) simple
intermediate UML model, which covers only the features necessary for
generation of the supported diagram types. The model can be extended if
necessary to add new features.

![diagram filter context](./diagram_filter_context_class.svg)

## Translation unit visitors

The first stage in the diagram generation involves traversing the AST of
each translation unit from the `compile_commands.json` compilation database,
which matched at least one pattern specified in the `glob` pattern of the
configuration file.

Each visitor is implemented in a subclass of
[`translation_unit_visitor`](./classclanguml_1_1common_1_1visitor_1_1translation__unit__visitor.html),
and must also implement relevant methods from Clang's
[RecursiveASTVisitor](https://clang.llvm.org/doxygen/classclang_1_1RecursiveASTVisitor.html).

![AST visitors](./architecture_visitors_class.svg)

The output of the `translation_unit_visitor` for each diagram type is an
intermediate diagram model, which is then passed to the relevant diagram
generator.

## Diagram generators

Diagram generators convert the `clang-uml`'s internal UML model into actual
diagram in one of the supported formats:

- PlantUML
- MermaidJS
- JSON

Each diagram generator extends a common interface appropriate for the
selected output format, i.e.:

- [PlantUML](classclanguml_1_1common_1_1generators_1_1plantuml_1_1generator.html)
- [MermaidJS](classclanguml_1_1common_1_1generators_1_1mermaid_1_1generator.html)
- [JSON](classclanguml_1_1common_1_1generators_1_1json_1_1generator.html)

and renders the output to a file. For each diagram type there is a separate
generator for each supported output format.

## Command line handler

The [cli_handler](classclanguml_1_1cli_1_1cli__handler.html) is a command line
handler class is a wrapper around [CLI11](https://github.com/CLIUtils/CLI11),
and implements handlers for various actions, validates command line parameters
and reports errors.

## Tests

### Unit tests

Basic set of units tests are stored in
[tests/test_*.cc](https://github.com/bkryza/clang-uml/tree/master/tests)
test files. The unit tests do not aim to cover the entire codebase, only
specific algorithms or methods, which should behave as expected and if their
errors can be difficult to diagnose when running the test cases on C++ code.

### Test cases

These tests are the main tests of `clang-uml`. Each test case tests one or
more feature of a specific diagram type. Each of them has a separated directory
in the `tests` directory and its own `.clang-uml` with diagram configuration
as well as a `test_case.h` file which contains the tests assertions.

Any other sources in that directory are compiled and then used to generate the
diagrams, whose contents should be then verified within `test_case.h` 
for correctness. All the sources should be wrapped within a namespace:
`clanguml::`

These test directories are numbered in consecutive numbers using the following
convention:
- Start with a letter `t`
- The first digit of the number is the diagram type:
  - `0` - class diagram
  - `2` - sequence diagram
  - `3` - package diagram
  - `4` - include diagram
  - `9` - other test cases
- The rest of the name is the consecutive number of the test case

Each test case is also referenced in
[test_cases.yaml](https://github.com/bkryza/clang-uml/blob/master/tests/test_cases.yaml)
where it has assigned a title. That file is used to generate the [test cases 
documentation page](./md_docs_2test__cases.html).

### Real code tests

Each release is tested on a set of open-source C++ projects, to be sure that
at least the new version does not crash or introduce some obvious regressions.

The tests are stored in a separate
repository: [clang-uml-examples](https://github.com/bkryza/clang-uml-examples).

