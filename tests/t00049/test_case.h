/**
 * tests/t00049/test_case.h
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

TEST_CASE("t00049", "[test-case][class]")
{
    auto [config, db] = load_config("t00049");

    auto diagram = config.diagrams["t00049_class"];

    REQUIRE(diagram->name == "t00049_class");

    auto model = generate_class_diagram(*db, diagram);

    REQUIRE(model->name() == "t00049_class");

    {
        auto src = generate_class_puml(diagram, *model);
        AliasMatcher _A(src);

        REQUIRE_THAT(src, StartsWith("@startuml"));
        REQUIRE_THAT(src, EndsWith("@enduml\n"));

        // Check if all classes exist
        REQUIRE_THAT(src, IsClass(_A("R")));

        // Check if class templates exist
        REQUIRE_THAT(src, IsClassTemplate("A", "T"));

        // Check if all methods exist
        REQUIRE_THAT(src, (IsMethod<Public>("get_int_map", "A<intmap>")));
        REQUIRE_THAT(src,
            (IsMethod<Public>("set_int_map", "void", "A<intmap> && int_map")));

        // Check if all fields exist
        REQUIRE_THAT(src, (IsField<Public>("a_string", "A<thestring>")));
        REQUIRE_THAT(
            src, (IsField<Public>("a_vector_string", "A<string_vector>")));
        REQUIRE_THAT(src, (IsField<Public>("a_int_map", "A<intmap>")));

        // Check if all relationships exist
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<string_vector>")));
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<thestring>")));
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<intmap>")));

        save_puml(config.output_directory(), diagram->name + ".puml", src);
    }
    {
        auto j = generate_class_json(diagram, *model);

        using namespace json;

        REQUIRE(IsClass(j, "R"));
        REQUIRE(IsClassTemplate(j, "A<T>"));
        REQUIRE(IsInstantiation(j, "A<T>", "A<string_vector>"));

        save_json(config.output_directory(), diagram->name + ".json", j);
    }
    {
        auto src = generate_class_mermaid(diagram, *model);

        mermaid::AliasMatcher _A(src);
        using mermaid::IsField;
        using mermaid::IsMethod;

        // Check if all classes exist
        REQUIRE_THAT(src, IsClass(_A("R")));

        // Check if class templates exist
        REQUIRE_THAT(src, IsClass(_A("A<T>")));

        // Check if all methods exist
        REQUIRE_THAT(src, (IsMethod<Public>("get_int_map", "A<intmap>")));
        REQUIRE_THAT(src,
            (IsMethod<Public>("set_int_map", "void", "A<intmap> && int_map")));

        // Check if all fields exist
        REQUIRE_THAT(src, (IsField<Public>("a_string", "A<thestring>")));
        REQUIRE_THAT(
            src, (IsField<Public>("a_vector_string", "A<string_vector>")));
        REQUIRE_THAT(src, (IsField<Public>("a_int_map", "A<intmap>")));

        // Check if all relationships exist
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<string_vector>")));
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<thestring>")));
        REQUIRE_THAT(src, IsInstantiation(_A("A<T>"), _A("A<intmap>")));

        save_mermaid(config.output_directory(), diagram->name + ".mmd", src);
    }
}