/**
 * @file src/common/generators/json/generator.h
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
#pragma once

#include "common/generators/generator.h"
#include "common/model/diagram_filter.h"
#include "config/config.h"
#include "util/error.h"
#include "util/util.h"
#include "version.h"

#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <glob/glob.hpp>

#include <ostream>

namespace clanguml::common::model {
using nlohmann::json;

void to_json(nlohmann::json &j, const source_location &sl);

void to_json(nlohmann::json &j, const element &c);

void to_json(nlohmann::json &j, const template_parameter &c);

void to_json(nlohmann::json &j, const relationship &c);
} // namespace clanguml::common::model

namespace clanguml::common::generators::json {

using clanguml::common::model::access_t;
using clanguml::common::model::element;
using clanguml::common::model::message_t;
using clanguml::common::model::relationship_t;

/**
 * @brief Base class for diagram generators
 *
 * @tparam ConfigType Configuration type
 * @tparam DiagramType Diagram model type
 */
template <typename ConfigType, typename DiagramType>
class generator
    : public clanguml::common::generators::generator<ConfigType, DiagramType> {
public:
    using clanguml::common::generators::generator<ConfigType,
        DiagramType>::generator;

    ~generator() override = default;

    /**
     * @brief Generate diagram
     *
     * This is the main diagram generation entrypoint. It is responsible for
     * calling other methods in appropriate order to generate the diagram into
     * the output stream. It generates diagram elements, that are common
     * to all types of diagrams in a given generator.
     *
     * @param ostr Output stream
     */
    void generate(std::ostream &ostr) const override;

    /**
     * @brief Generate diagram model
     *
     * This method must be implemented in subclasses for specific diagram
     * types.
     *
     * @param ostr Output stream
     */
    virtual void generate_diagram(nlohmann::json &parent) const = 0;

    /**
     * @brief Generate metadata element with diagram metadata
     *
     * @param parent Root JSON object
     */
    void generate_metadata(nlohmann::json &parent) const;
};

template <typename DiagramModel, typename DiagramConfig>
std::ostream &operator<<(
    std::ostream &os, const generator<DiagramModel, DiagramConfig> &g)
{
    g.generate(os);
    return os;
}

template <typename C, typename D>
void generator<C, D>::generate(std::ostream &ostr) const
{
    nlohmann::json j;
    j["name"] = generators::generator<C, D>::model().name();
    j["diagram_type"] = to_string(generators::generator<C, D>::model().type());
    if (generators::generator<C, D>::config().title) {
        j["title"] = generators::generator<C, D>::config().title();
    }

    generate_diagram(j);

    generate_metadata(j);

    ostr << j;
}

template <typename C, typename D>
void generator<C, D>::generate_metadata(nlohmann::json &parent) const
{
    if (generators::generator<C, D>::config().generate_metadata()) {
        parent["metadata"]["clang_uml_version"] =
            clanguml::version::CLANG_UML_VERSION;
        parent["metadata"]["schema_version"] =
            clanguml::version::CLANG_UML_JSON_GENERATOR_SCHEMA_VERSION;
        parent["metadata"]["llvm_version"] = clang::getClangFullVersion();
    }
}

} // namespace clanguml::common::generators::json