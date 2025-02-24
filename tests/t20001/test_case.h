/**
 * tests/t20001/test_case.cc
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

TEST_CASE("t20001", "[test-case][sequence]")
{
    auto [config, db] = load_config("t20001");

    auto diagram = config.diagrams["t20001_sequence"];

    REQUIRE(diagram->name == "t20001_sequence");

    auto model = generate_sequence_diagram(*db, diagram);

    REQUIRE(model->name() == "t20001_sequence");

    {
        auto src = generate_sequence_puml(diagram, *model);
        AliasMatcher _A(src);

        REQUIRE_THAT(src, StartsWith("@startuml"));
        REQUIRE_THAT(src, EndsWith("@enduml\n"));

        REQUIRE_THAT(src, HasTitle("Basic sequence diagram example"));

        REQUIRE_THAT(src, HasCall(_A("B"), _A("A"), "add3(int,int,int)"));
        REQUIRE_THAT(src, HasCall(_A("A"), "add(int,int)"));
        REQUIRE_THAT(src, !HasCall(_A("A"), _A("detail::C"), "add(int,int)"));
        REQUIRE_THAT(src, HasCall(_A("A"), "__log_result(int)__"));
        REQUIRE_THAT(src, HasCall(_A("B"), _A("A"), "__log_result(int)__"));

        REQUIRE_THAT(src, HasComment("t20001 test diagram of type sequence"));

        REQUIRE_THAT(
            src, HasMessageComment(_A("tmain()"), "Just add 2 numbers"));

        REQUIRE_THAT(
            src, HasMessageComment(_A("tmain()"), "And now add another 2"));

        save_puml(config.output_directory(), diagram->name + ".puml", src);
    }
    {
        auto j = generate_sequence_json(diagram, *model);

        using namespace json;

        REQUIRE(HasTitle(j, "Basic sequence diagram example"));

        REQUIRE(IsFunctionParticipant(j, "tmain()"));
        REQUIRE(IsClassParticipant(j, "A"));
        REQUIRE(IsClassParticipant(j, "B"));

        std::vector<int> messages = {
            FindMessage(j, "tmain()", "A", "add(int,int)"),
            FindMessage(j, "tmain()", "B", "wrap_add3(int,int,int)"),
            FindMessage(j, "B", "A", "add3(int,int,int)"),
            FindMessage(j, "A", "A", "add(int,int)"),
            FindMessage(j, "A", "A", "log_result(int)"),
            FindMessage(j, "B", "A", "log_result(int)")};

        REQUIRE(std::is_sorted(messages.begin(), messages.end()));

        save_json(config.output_directory(), diagram->name + ".json", j);
    }
    {
        auto src = generate_sequence_mermaid(diagram, *model);
        mermaid::SequenceDiagramAliasMatcher _A(src);
        using mermaid::HasCall;
        using mermaid::HasComment;
        using mermaid::HasTitle;

        REQUIRE_THAT(src, HasTitle("Basic sequence diagram example"));

        REQUIRE_THAT(src, HasCall(_A("B"), _A("A"), "add3(int,int,int)"));
        REQUIRE_THAT(src, HasCall(_A("A"), "add(int,int)"));
        REQUIRE_THAT(src, !HasCall(_A("A"), _A("detail::C"), "add(int,int)"));
        REQUIRE_THAT(src, HasCall(_A("A"), "log_result(int)"));
        REQUIRE_THAT(src, HasCall(_A("B"), _A("A"), "log_result(int)"));

        REQUIRE_THAT(src, HasComment("t20001 test diagram of type sequence"));

        save_mermaid(config.output_directory(), diagram->name + ".mmd", src);
    }
}
