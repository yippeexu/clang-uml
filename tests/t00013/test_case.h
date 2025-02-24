/**
 * tests/t00013/test_case.cc
 *
 * Copyright (c) 2021-2023 Bartek Kryza <bkryza@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

TEST_CASE("t00013", "[test-case][class]")
{
    auto [config, db] = load_config("t00013");

    auto diagram = config.diagrams["t00013_class"];

    REQUIRE(diagram->name == "t00013_class");

    auto model = generate_class_diagram(*db, diagram);

    REQUIRE(model->name() == "t00013_class");

    {
        auto src = generate_class_puml(diagram, *model);
        AliasMatcher _A(src);

        REQUIRE_THAT(src, StartsWith("@startuml"));
        REQUIRE_THAT(src, EndsWith("@enduml\n"));
        REQUIRE_THAT(src, IsClass(_A("A")));
        REQUIRE_THAT(src, IsClass(_A("B")));
        REQUIRE_THAT(src, IsClass(_A("C")));
        REQUIRE_THAT(src, IsClass(_A("D")));
        REQUIRE_THAT(src, IsClassTemplate("E", "T"));
        REQUIRE_THAT(src, IsClassTemplate("G", "T,Args..."));

        REQUIRE_THAT(src, !IsDependency(_A("R"), _A("R")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("A")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("B")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("C")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("D")));
        REQUIRE_THAT(src, IsDependency(_A("D"), _A("R")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("E<T>")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("E<int>")));
        REQUIRE_THAT(src, IsInstantiation(_A("E<T>"), _A("E<int>")));
        REQUIRE_THAT(src, IsInstantiation(_A("E<T>"), _A("E<std::string>")));
        REQUIRE_THAT(
            src, IsAggregation(_A("R"), _A("E<std::string>"), "-estring"));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("ABCD::F<T>")));
        REQUIRE_THAT(
            src, IsInstantiation(_A("ABCD::F<T>"), _A("ABCD::F<int>")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("ABCD::F<int>")));

        REQUIRE_THAT(src,
            IsInstantiation(
                _A("G<T,Args...>"), _A("G<int,float,std::string>")));

        save_puml(config.output_directory(), diagram->name + ".puml", src);
    }
    {
        auto j = generate_class_json(diagram, *model);

        using namespace json;

        REQUIRE(IsClass(j, "A"));
        REQUIRE(IsClass(j, "B"));
        REQUIRE(IsClass(j, "C"));
        REQUIRE(IsClass(j, "D"));
        REQUIRE(IsInstantiation(j, "E<T>", "E<int>"));
        REQUIRE(IsDependency(j, "R", "A"));
        REQUIRE(IsDependency(j, "R", "B"));
        REQUIRE(IsDependency(j, "R", "C"));
        REQUIRE(IsDependency(j, "R", "D"));
        REQUIRE(IsDependency(j, "D", "R"));
        REQUIRE(IsDependency(j, "R", "E<int>"));
        REQUIRE(IsInstantiation(j, "G<T,Args...>", "G<int,float,std::string>"));

        save_json(config.output_directory(), diagram->name + ".json", j);
    }
    {
        auto src = generate_class_mermaid(diagram, *model);

        mermaid::AliasMatcher _A(src);

        REQUIRE_THAT(src, IsClass(_A("A")));
        REQUIRE_THAT(src, IsClass(_A("B")));
        REQUIRE_THAT(src, IsClass(_A("C")));
        REQUIRE_THAT(src, IsClass(_A("D")));
        REQUIRE_THAT(src, IsClass(_A("E<T>")));
        REQUIRE_THAT(src, IsClass(_A("G<T,Args...>")));

        REQUIRE_THAT(src, !IsDependency(_A("R"), _A("R")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("A")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("B")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("C")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("D")));
        REQUIRE_THAT(src, IsDependency(_A("D"), _A("R")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("E<T>")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("E<int>")));
        REQUIRE_THAT(src, IsInstantiation(_A("E<T>"), _A("E<int>")));
        REQUIRE_THAT(src, IsInstantiation(_A("E<T>"), _A("E<std::string>")));
        REQUIRE_THAT(
            src, IsAggregation(_A("R"), _A("E<std::string>"), "-estring"));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("ABCD::F<T>")));
        REQUIRE_THAT(
            src, IsInstantiation(_A("ABCD::F<T>"), _A("ABCD::F<int>")));
        REQUIRE_THAT(src, IsDependency(_A("R"), _A("ABCD::F<int>")));

        REQUIRE_THAT(src,
            IsInstantiation(
                _A("G<T,Args...>"), _A("G<int,float,std::string>")));

        save_mermaid(config.output_directory(), diagram->name + ".mmd", src);
    }
}