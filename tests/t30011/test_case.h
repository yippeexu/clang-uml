/**
 * tests/t30011/test_case.h
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

TEST_CASE("t30011", "[test-case][package]")
{
    auto [config, db] = load_config("t30011");

    auto diagram = config.diagrams["t30011_package"];

    REQUIRE(diagram->name == "t30011_package");

    auto model = generate_package_diagram(*db, diagram);

    REQUIRE(model->name() == "t30011_package");

    {
        auto src = generate_package_puml(diagram, *model);
        AliasMatcher _A(src);

        REQUIRE_THAT(src, StartsWith("@startuml"));
        REQUIRE_THAT(src, EndsWith("@enduml\n"));

        REQUIRE_THAT(src, IsPackage("app"));
        REQUIRE_THAT(src, IsPackage("libraries"));
        REQUIRE_THAT(src, IsPackage("lib1"));
        REQUIRE_THAT(src, IsPackage("lib2"));
        REQUIRE_THAT(src, !IsPackage("library1"));
        REQUIRE_THAT(src, !IsPackage("library2"));

        REQUIRE_THAT(src, IsDependency(_A("app"), _A("lib1")));
        REQUIRE_THAT(src, IsDependency(_A("app"), _A("lib2")));
        REQUIRE_THAT(src, IsDependency(_A("app"), _A("lib3")));
        REQUIRE_THAT(src, IsDependency(_A("app"), _A("lib4")));

        save_puml(config.output_directory(), diagram->name + ".puml", src);
    }

    {
        auto j = generate_package_json(diagram, *model);

        using namespace json;

        save_json(config.output_directory(), diagram->name + ".json", j);
    }

    {
        auto src = generate_package_mermaid(diagram, *model);
        mermaid::AliasMatcher _A(src);
        using mermaid::IsPackage;
        using mermaid::IsPackageDependency;

        REQUIRE_THAT(src, IsPackage(_A("app")));
        REQUIRE_THAT(src, IsPackage(_A("libraries")));
        REQUIRE_THAT(src, IsPackage(_A("lib1")));
        REQUIRE_THAT(src, IsPackage(_A("lib2")));
        REQUIRE_THAT(src, !IsPackage(_A("library1")));
        REQUIRE_THAT(src, !IsPackage(_A("library2")));

        REQUIRE_THAT(src, IsPackageDependency(_A("app"), _A("lib1")));
        REQUIRE_THAT(src, IsPackageDependency(_A("app"), _A("lib2")));
        REQUIRE_THAT(src, IsPackageDependency(_A("app"), _A("lib3")));
        REQUIRE_THAT(src, IsPackageDependency(_A("app"), _A("lib4")));

        save_mermaid(config.output_directory(), diagram->name + ".mmd", src);
    }
}