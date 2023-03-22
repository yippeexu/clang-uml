/**
 * tests/t00021/test_case.cc
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

TEST_CASE("t00021", "[test-case][class]")
{
    auto [config, db] = load_config("t00021");

    auto diagram = config.diagrams["t00021_class"];

    REQUIRE(diagram->name == "t00021_class");

    auto model = generate_class_diagram(*db, diagram);

    REQUIRE(model->name() == "t00021_class");
    REQUIRE(model->should_include("clanguml::t00021::Visitor"));

    {
        auto puml = generate_class_puml(diagram, *model);
        AliasMatcher _A(puml);

        REQUIRE_THAT(puml, StartsWith("@startuml"));
        REQUIRE_THAT(puml, EndsWith("@enduml\n"));
        REQUIRE_THAT(puml, IsAbstractClass(_A("Item")));
        REQUIRE_THAT(puml, IsAbstractClass(_A("Visitor")));
        REQUIRE_THAT(puml, IsClass(_A("A")));
        REQUIRE_THAT(puml, IsClass(_A("B")));
        REQUIRE_THAT(puml, IsClass(_A("Visitor1")));
        REQUIRE_THAT(puml, IsClass(_A("Visitor2")));
        REQUIRE_THAT(puml, IsClass(_A("Visitor3")));

        save_puml(
            config.output_directory() + "/" + diagram->name + ".puml", puml);
    }
    {
        auto j = generate_class_json(diagram, *model);

        using namespace json;

        REQUIRE(IsClass(j, "Visitor1"));
        REQUIRE(IsClass(j, "Visitor2"));
        REQUIRE(IsAbstractClass(j, "Item"));

        save_json(config.output_directory() + "/" + diagram->name + ".json", j);
    }
}
