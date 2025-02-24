/**
 * tests/t00059/test_case.h
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

TEST_CASE("t00059", "[test-case][class]")
{
    auto [config, db] = load_config("t00059");

    auto diagram = config.diagrams["t00059_class"];

    REQUIRE(diagram->name == "t00059_class");

    auto model = generate_class_diagram(*db, diagram);

    REQUIRE(model->name() == "t00059_class");

    {
        auto src = generate_class_puml(diagram, *model);
        AliasMatcher _A(src);

        REQUIRE_THAT(src, StartsWith("@startuml"));
        REQUIRE_THAT(src, EndsWith("@enduml\n"));

        REQUIRE_THAT(src, IsConcept(_A("fruit_c<T>")));
        REQUIRE_THAT(src, IsConcept(_A("apple_c<T>")));
        REQUIRE_THAT(src, IsConcept(_A("orange_c<T>")));

        REQUIRE_THAT(
            src, IsConstraint(_A("apple_c<T>"), _A("fruit_c<T>"), "T"));
        REQUIRE_THAT(
            src, IsConstraint(_A("orange_c<T>"), _A("fruit_c<T>"), "T"));

        REQUIRE_THAT(
            src, IsConceptRequirement(_A("apple_c<T>"), "t.get_sweetness()"));
        REQUIRE_THAT(
            src, IsConceptRequirement(_A("apple_c<T>"), "t.get_bitterness()"));

        REQUIRE_THAT(src, IsClass(_A("gala_apple")));
        REQUIRE_THAT(src, IsClass(_A("empire_apple")));
        REQUIRE_THAT(src, IsClass(_A("valencia_orange")));
        REQUIRE_THAT(src, IsClass(_A("lima_orange")));
        REQUIRE_THAT(src, IsClass(_A("R")));

        REQUIRE_THAT(
            src, IsClassTemplate("fruit_factory", "apple_c TA,orange_c TO"));

        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<gala_apple,valencia_orange>"),
                _A("gala_apple")));
        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<gala_apple,valencia_orange>"),
                _A("valencia_orange")));

        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<empire_apple,lima_orange>"),
                _A("empire_apple")));
        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<empire_apple,lima_orange>"),
                _A("lima_orange")));

        REQUIRE_THAT(src,
            IsAggregation(_A("R"),
                _A("fruit_factory<gala_apple,valencia_orange>"), "+factory_1"));
        REQUIRE_THAT(src,
            IsAggregation(_A("R"),
                _A("fruit_factory<empire_apple,lima_orange>"), "+factory_2"));

        REQUIRE_THAT(src,
            IsInstantiation(_A("fruit_factory<apple_c TA,orange_c TO>"),
                _A("fruit_factory<gala_apple,valencia_orange>")));
        REQUIRE_THAT(src,
            IsInstantiation(_A("fruit_factory<apple_c TA,orange_c TO>"),
                _A("fruit_factory<empire_apple,lima_orange>")));

        save_puml(config.output_directory(), diagram->name + ".puml", src);
    }

    {
        auto j = generate_class_json(diagram, *model);

        using namespace json;

        REQUIRE(IsConcept(j, "fruit_c<T>"));
        REQUIRE(IsConcept(j, "apple_c<T>"));
        REQUIRE(IsConcept(j, "orange_c<T>"));

        save_json(config.output_directory(), diagram->name + ".json", j);
    }
    {
        auto src = generate_class_mermaid(diagram, *model);

        mermaid::AliasMatcher _A(src);
        using mermaid::IsConcept;
        using mermaid::IsConceptRequirement;
        using mermaid::IsConstraint;

        REQUIRE_THAT(src, IsConcept(_A("fruit_c<T>")));
        REQUIRE_THAT(src, IsConcept(_A("apple_c<T>")));
        REQUIRE_THAT(src, IsConcept(_A("orange_c<T>")));

        REQUIRE_THAT(
            src, IsConstraint(_A("apple_c<T>"), _A("fruit_c<T>"), "T"));
        REQUIRE_THAT(
            src, IsConstraint(_A("orange_c<T>"), _A("fruit_c<T>"), "T"));

        REQUIRE_THAT(
            src, IsConceptRequirement(_A("apple_c<T>"), "t.get_sweetness()"));
        REQUIRE_THAT(
            src, IsConceptRequirement(_A("apple_c<T>"), "t.get_bitterness()"));

        REQUIRE_THAT(src, IsClass(_A("gala_apple")));
        REQUIRE_THAT(src, IsClass(_A("empire_apple")));
        REQUIRE_THAT(src, IsClass(_A("valencia_orange")));
        REQUIRE_THAT(src, IsClass(_A("lima_orange")));
        REQUIRE_THAT(src, IsClass(_A("R")));

        REQUIRE_THAT(src, IsClass(_A("fruit_factory<apple_c TA,orange_c TO>")));

        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<gala_apple,valencia_orange>"),
                _A("gala_apple")));
        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<gala_apple,valencia_orange>"),
                _A("valencia_orange")));

        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<empire_apple,lima_orange>"),
                _A("empire_apple")));
        REQUIRE_THAT(src,
            IsDependency(_A("fruit_factory<empire_apple,lima_orange>"),
                _A("lima_orange")));

        REQUIRE_THAT(src,
            IsAggregation(_A("R"),
                _A("fruit_factory<gala_apple,valencia_orange>"), "+factory_1"));
        REQUIRE_THAT(src,
            IsAggregation(_A("R"),
                _A("fruit_factory<empire_apple,lima_orange>"), "+factory_2"));

        REQUIRE_THAT(src,
            IsInstantiation(_A("fruit_factory<apple_c TA,orange_c TO>"),
                _A("fruit_factory<gala_apple,valencia_orange>")));
        REQUIRE_THAT(src,
            IsInstantiation(_A("fruit_factory<apple_c TA,orange_c TO>"),
                _A("fruit_factory<empire_apple,lima_orange>")));

        save_mermaid(config.output_directory(), diagram->name + ".mmd", src);
    }
}