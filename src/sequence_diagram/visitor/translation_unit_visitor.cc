/**
 * src/sequence_diagram/visitor/translation_unit_visitor.cc
 *
 * Copyright (c) 2021-2022 Bartek Kryza <bkryza@gmail.com>
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

#include "translation_unit_visitor.h"

#include "common/model/namespace.h"
#include "cx/util.h"

namespace clanguml::sequence_diagram::visitor {

std::string to_string(const clang::FunctionTemplateDecl *decl)
{
    std::vector<std::string> template_parameters;
    // Handle template function
    for (const auto *parameter : *decl->getTemplateParameters()) {
        if (clang::dyn_cast_or_null<clang::TemplateTypeParmDecl>(parameter)) {
            const auto *template_type_parameter =
                clang::dyn_cast_or_null<clang::TemplateTypeParmDecl>(parameter);

            std::string template_parameter{
                template_type_parameter->getNameAsString()};

            if (template_type_parameter->isParameterPack())
                template_parameter += "...";

            template_parameters.emplace_back(std::move(template_parameter));
        }
        else {
            // TODO
        }
    }
    return fmt::format("{}<{}>({})", decl->getQualifiedNameAsString(),
        fmt::join(template_parameters, ","), "");
}

translation_unit_visitor::translation_unit_visitor(clang::SourceManager &sm,
    clanguml::sequence_diagram::model::diagram &diagram,
    const clanguml::config::sequence_diagram &config)
    : common::visitor::translation_unit_visitor{sm, config}
    , diagram_{diagram}
    , config_{config}
    , call_expression_context_{}
{
}

bool translation_unit_visitor::shouldVisitTemplateInstantiations()
{
    return true;
}

call_expression_context &translation_unit_visitor::context()
{
    return call_expression_context_;
}

clanguml::sequence_diagram::model::diagram &translation_unit_visitor::diagram()
{
    return diagram_;
}

const clanguml::config::sequence_diagram &
translation_unit_visitor::config() const
{
    return config_;
}

bool translation_unit_visitor::VisitCXXRecordDecl(clang::CXXRecordDecl *cls)
{
    // Skip system headers
    if (source_manager().isInSystemHeader(cls->getSourceRange().getBegin()))
        return true;

    if (!diagram().should_include(cls->getQualifiedNameAsString()))
        return true;

    if (cls->isTemplated() && cls->getDescribedTemplate()) {
        // If the described templated of this class is already in the model
        // skip it:
        if (get_unique_id(cls->getDescribedTemplate()->getID()))
            return true;
    }

    // TODO: Add support for classes defined in function/method bodies
    if (cls->isLocalClass())
        return true;

    // Build the class declaration and store it in the diagram, even
    // if we don't need it for any of the participants of this diagram
    auto c_ptr = create_class_declaration(cls);

    if (!c_ptr)
        return true;

    context().reset();

    const auto cls_id = c_ptr->id();

    set_unique_id(cls->getID(), cls_id);

    auto &class_model =
        diagram()
            .get_participant<sequence_diagram::model::class_>(cls_id)
            .has_value()
        ? *diagram()
               .get_participant<sequence_diagram::model::class_>(cls_id)
               .get()
        : *c_ptr;

    auto id = class_model.id();
    if (!cls->isCompleteDefinition()) {
        forward_declarations_.emplace(id, std::move(c_ptr));
        return true;
    }
    else {
        forward_declarations_.erase(id);
    }

    if (diagram().should_include(class_model)) {
        LOG_DBG("Adding class {} with id {}", class_model.full_name(false),
            class_model.id());

        assert(class_model.id() == cls_id);

        context().set_caller_id(cls_id);
        context().update(cls);

        diagram().add_participant(std::move(c_ptr));
    }
    else {
        LOG_DBG("Skipping class {} with id {}", class_model.full_name(),
            class_model.id());
    }

    return true;
}

bool translation_unit_visitor::VisitClassTemplateDecl(
    clang::ClassTemplateDecl *cls)
{
    if (source_manager().isInSystemHeader(cls->getSourceRange().getBegin()))
        return true;

    if (!diagram().should_include(cls->getQualifiedNameAsString()))
        return true;

    LOG_DBG("= Visiting class template declaration {} at {} [{}]",
        cls->getQualifiedNameAsString(),
        cls->getLocation().printToString(source_manager()), (void *)cls);

    auto c_ptr = create_class_declaration(cls->getTemplatedDecl());

    if (!c_ptr)
        return true;

    // Override the id with the template id, for now we don't care about the
    // underlying templated class id
    process_template_parameters(*cls, *c_ptr);

    const auto cls_full_name = c_ptr->full_name(false);
    const auto id = common::to_id(cls_full_name);

    c_ptr->set_id(id);

    set_unique_id(cls->getID(), id);

    if (!cls->getTemplatedDecl()->isCompleteDefinition()) {
        forward_declarations_.emplace(id, std::move(c_ptr));
        return true;
    }
    else {
        forward_declarations_.erase(id);
    }

    if (diagram().should_include(*c_ptr)) {
        LOG_DBG("Adding class template {} with id {}", cls_full_name, id);

        context().set_caller_id(id);
        context().update(cls);

        diagram().add_participant(std::move(c_ptr));
    }

    return true;
}

bool translation_unit_visitor::VisitClassTemplateSpecializationDecl(
    clang::ClassTemplateSpecializationDecl *cls)
{
    if (source_manager().isInSystemHeader(cls->getSourceRange().getBegin()))
        return true;

    if (!diagram().should_include(cls->getQualifiedNameAsString()))
        return true;

    LOG_DBG("Visiting template specialization declaration {} at {}",
        cls->getQualifiedNameAsString(),
        cls->getLocation().printToString(source_manager()));

    // TODO: Add support for classes defined in function/method bodies
    if (cls->isLocalClass())
        return true;

    auto template_specialization_ptr = process_template_specialization(cls);

    if (!template_specialization_ptr)
        return true;

    const auto cls_full_name = template_specialization_ptr->full_name(false);
    const auto id = common::to_id(cls_full_name);

    template_specialization_ptr->set_id(id);

    set_unique_id(cls->getID(), id);

    if (!cls->isCompleteDefinition()) {
        forward_declarations_.emplace(
            id, std::move(template_specialization_ptr));
        return true;
    }
    else {
        forward_declarations_.erase(id);
    }

    if (diagram().should_include(*template_specialization_ptr)) {
        LOG_DBG("Adding class template specialization {} with id {}",
            cls_full_name, id);

        context().set_caller_id(id);
        context().update(cls);

        diagram().add_participant(std::move(template_specialization_ptr));
    }

    return true;
}

bool translation_unit_visitor::VisitCXXMethodDecl(clang::CXXMethodDecl *m)
{
    if (context().current_class_decl_ == nullptr &&
        context().current_class_template_decl_ == nullptr &&
        context().current_class_template_specialization_decl_ == nullptr)
        return true;

    LOG_DBG("= Processing method {} in class {} [{}]",
        m->getQualifiedNameAsString(),
        m->getParent()->getQualifiedNameAsString(), (void *)m->getParent());

    context().update(m);

    auto m_ptr = std::make_unique<sequence_diagram::model::method>(
        config().using_namespace());

    common::model::namespace_ ns{m->getQualifiedNameAsString()};
    auto method_name = ns.name();
    m_ptr->set_method_name(method_name);
    ns.pop_back();
    m_ptr->set_name(ns.name());
    ns.pop_back();

    clang::Decl *parent_decl = m->getParent();

    if (context().current_class_template_decl_)
        parent_decl = context().current_class_template_decl_;

    LOG_DBG("Getting method's class with local id {}", parent_decl->getID());

    const auto &method_class =
        diagram()
            .get_participant<model::class_>(
                get_unique_id(parent_decl->getID()).value())
            .value();

    m_ptr->set_class_id(method_class.id());
    m_ptr->set_class_full_name(method_class.full_name(false));
    m_ptr->set_name(
        diagram().participants.at(m_ptr->class_id())->full_name_no_ns() +
        "::" + m->getNameAsString());

    m_ptr->set_id(common::to_id(
        diagram().participants.at(m_ptr->class_id())->full_name(false) +
        "::" + m->getNameAsString()));

    LOG_DBG("Set id {} for method name {}", m_ptr->id(),
        diagram().participants.at(m_ptr->class_id())->full_name(false) +
            "::" + m->getNameAsString());

    context().update(m);

    context().set_caller_id(m_ptr->id());

    set_unique_id(m->getID(), m_ptr->id());

    diagram().add_participant(std::move(m_ptr));

    return true;
}

bool translation_unit_visitor::VisitFunctionDecl(clang::FunctionDecl *f)
{
    if (f->isCXXClassMember())
        return true;

    const auto function_name = f->getQualifiedNameAsString();

    if (!diagram().should_include(function_name))
        return true;

    LOG_DBG("Visiting function declaration {} at {}", function_name,
        f->getLocation().printToString(source_manager()));

    if (f->isTemplated()) {
        if (f->getDescribedTemplate()) {
            // If the described templated of this function is already in the
            // model skip it:
            if (get_unique_id(f->getDescribedTemplate()->getID()))
                return true;
        }
    }

    if (f->isFunctionTemplateSpecialization()) {
        auto f_ptr = build_function_template_instantiation(*f);

        f_ptr->set_id(common::to_id(f_ptr->full_name(false)));

        context().update(f);

        context().set_caller_id(f_ptr->id());

        set_unique_id(f->getID(), f_ptr->id());

        // TODO: Handle overloaded functions with different arguments
        diagram().add_participant(std::move(f_ptr));
    }
    else {
        auto f_ptr = std::make_unique<sequence_diagram::model::function>(
            config().using_namespace());

        common::model::namespace_ ns{function_name};
        f_ptr->set_name(ns.name());
        ns.pop_back();
        f_ptr->set_namespace(ns);
        f_ptr->set_id(common::to_id(function_name));

        context().update(f);

        context().set_caller_id(f_ptr->id());

        set_unique_id(f->getID(), f_ptr->id());

        // TODO: Handle overloaded functions with different arguments
        diagram().add_participant(std::move(f_ptr));
    }

    return true;
}

bool translation_unit_visitor::VisitFunctionTemplateDecl(
    clang::FunctionTemplateDecl *function_template)
{
    const auto function_name = function_template->getQualifiedNameAsString();

    if (!diagram().should_include(function_name))
        return true;

    LOG_DBG("Visiting function template declaration {} at {}", function_name,
        function_template->getLocation().printToString(source_manager()));

    auto f_ptr = std::make_unique<sequence_diagram::model::function_template>(
        config().using_namespace());

    common::model::namespace_ ns{function_name};
    f_ptr->set_name(ns.name());
    ns.pop_back();
    f_ptr->set_namespace(ns);

    process_template_parameters(*function_template, *f_ptr);

    f_ptr->set_id(common::to_id(f_ptr->full_name(false)));

    context().update(function_template);
    context().set_caller_id(f_ptr->id());

    set_unique_id(function_template->getID(), f_ptr->id());

    // TODO: Handle overloaded functions with different arguments
    diagram().add_participant(std::move(f_ptr));

    return true;
}

bool translation_unit_visitor::VisitCallExpr(clang::CallExpr *expr)
{
    using clanguml::common::model::message_t;
    using clanguml::common::model::namespace_;
    using clanguml::sequence_diagram::model::activity;
    using clanguml::sequence_diagram::model::message;

    // Skip casts, moves and such
    if (expr->isCallToStdMove())
        return true;

    if (expr->isImplicitCXXThis())
        return true;

    if (clang::dyn_cast_or_null<clang::ImplicitCastExpr>(expr))
        return true;

    if (!context().valid())
        return true;

    message m;
    m.type = message_t::kCall;
    m.from = context().caller_id();

    const auto &current_ast_context = *context().get_ast_context();

    LOG_DBG("Visiting call expression at {}",
        expr->getBeginLoc().printToString(source_manager()));

    if (const auto *operator_call_expr =
            clang::dyn_cast_or_null<clang::CXXOperatorCallExpr>(expr);
        operator_call_expr != nullptr) {
        // TODO: Handle C++ operator calls
    }
    //
    // Call to a class method
    //
    else if (const auto *method_call_expr =
                 clang::dyn_cast_or_null<clang::CXXMemberCallExpr>(expr);
             method_call_expr != nullptr) {

        // Get callee declaration as methods parent
        const auto *method_decl = method_call_expr->getMethodDecl();
        std::string method_name = method_decl->getQualifiedNameAsString();

        auto *callee_decl = method_decl ? method_decl->getParent() : nullptr;

        if (!(callee_decl &&
                diagram().should_include(
                    callee_decl->getQualifiedNameAsString())))
            return true;

        const auto *callee_template_specialization =
            clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(
                callee_decl);

        if (callee_template_specialization) {
            LOG_DBG("Callee is a template specialization declaration {}",
                callee_template_specialization->getQualifiedNameAsString());

            const auto &participant =
                diagram()
                    .get_participant<model::class_>(
                        get_unique_id(callee_template_specialization->getID())
                            .value())
                    .value();

            if (participant.is_implicit()) {
                /*
                const auto *parent_template =
                    callee_template_specialization->getSpecializedTemplate();

                const auto &parent_template_participant =
                    diagram()
                        .get_participant<model::class_>(
                            get_unique_id(parent_template->getID()).value())
                        .value();

                const auto parent_method_name =
                    parent_template_participant.full_name(false) +
                    "::" + method_decl->getNameAsString();

                m.to = common::to_id(parent_method_name);
                m.message_name = participant.full_name_no_ns() +
                    "::" + method_decl->getNameAsString();
                */
                const auto specialization_method_name =
                    participant.full_name(false) +
                    "::" + method_decl->getNameAsString();

                m.to = common::to_id(method_name);
                m.message_name = method_decl->getNameAsString();

                // Since this is an implicit instantiation it might not exist
                // so we have to create this participant here and it to the
                // diagram
                if (!diagram()
                         .get_participant<model::class_>(m.to)
                         .has_value()) {
                    auto m_ptr =
                        std::make_unique<sequence_diagram::model::method>(
                            config().using_namespace());
                    m_ptr->set_id(m.to);
                    m_ptr->set_method_name(method_decl->getNameAsString());
                    m_ptr->set_name(method_decl->getNameAsString());
                    m_ptr->set_class_id(participant.id());
                    m_ptr->set_class_full_name(participant.full_name(false));
                    set_unique_id(method_decl->getID(), m_ptr->id());
                    diagram().add_active_participant(m_ptr->id());
                    diagram().add_participant(std::move(m_ptr));
                }
            }
            else {
                const auto &specialization_participant =
                    diagram()
                        .get_participant<model::class_>(get_unique_id(
                            callee_template_specialization->getID())
                                                            .value())
                        .value();
                const auto specialization_method_name =
                    specialization_participant.full_name(false) +
                    "::" + method_decl->getNameAsString();

                m.to = common::to_id(specialization_method_name);
                m.message_name = method_decl->getNameAsString();
            }
        }
        else {
            // TODO: The method can be called before it's declaration has been
            //       encountered by the visitor - for now it's not a problem
            //       as overloaded methods are not supported
            m.to = common::to_id(method_decl->getQualifiedNameAsString());
            m.message_name = method_decl->getNameAsString();
        }
        m.return_type = method_call_expr->getCallReturnType(current_ast_context)
                            .getAsString();

        LOG_DBG("Set callee method id {} for method name {}", m.to,
            method_decl->getQualifiedNameAsString());

        if (get_unique_id(callee_decl->getID()))
            diagram().add_active_participant(
                get_unique_id(callee_decl->getID()).value());
    }
    //
    // Call to a function
    //
    else if (const auto *function_call_expr =
                 clang::dyn_cast_or_null<clang::CallExpr>(expr);
             function_call_expr != nullptr) {
        auto *callee_decl = function_call_expr->getCalleeDecl();

        if (callee_decl == nullptr) {
            LOG_DBG("Cannot get callee declaration - trying direct callee...");
            callee_decl = function_call_expr->getDirectCallee();
        }

        if (!callee_decl) {
            //
            // Call to a method of a class template
            //
            if (clang::dyn_cast_or_null<clang::CXXDependentScopeMemberExpr>(
                    function_call_expr->getCallee())) {
                auto *dependent_member_callee =
                    clang::dyn_cast_or_null<clang::CXXDependentScopeMemberExpr>(
                        function_call_expr->getCallee());

                if (!dependent_member_callee->getBaseType().isNull()) {
                    const auto *primary_template =
                        dependent_member_callee->getBaseType()
                            ->getAs<clang::TemplateSpecializationType>()
                            ->getTemplateName()
                            .getAsTemplateDecl();

                    auto callee_method_full_name =
                        diagram()
                            .participants
                            .at(get_unique_id(primary_template->getID())
                                    .value())
                            ->full_name(false) +
                        "::" +
                        dependent_member_callee->getMember().getAsString();

                    auto callee_id = common::to_id(callee_method_full_name);
                    m.to = callee_id;

                    m.message_name =
                        dependent_member_callee->getMember().getAsString();
                    m.return_type = "";

                    if (get_unique_id(primary_template->getID()))
                        diagram().add_active_participant(
                            get_unique_id(primary_template->getID()).value());
                }
            }
            //
            // Call to a template function
            //
            else if (clang::dyn_cast_or_null<clang::UnresolvedLookupExpr>(
                         function_call_expr->getCallee())) {
                // This is probably a template
                auto *unresolved_expr =
                    clang::dyn_cast_or_null<clang::UnresolvedLookupExpr>(
                        function_call_expr->getCallee());

                if (unresolved_expr) {
                    for (const auto *decl : unresolved_expr->decls()) {
                        if (clang::dyn_cast_or_null<
                                clang::FunctionTemplateDecl>(decl)) {
                            // Yes, it's a template
                            auto *ftd = clang::dyn_cast_or_null<
                                clang::FunctionTemplateDecl>(decl);

                            m.to = get_unique_id(ftd->getID()).value();
                            auto message_name =
                                diagram()
                                    .get_participant<model::function_template>(
                                        m.to)
                                    .value()
                                    .full_name(false)
                                    .substr();
                            m.message_name =
                                message_name.substr(0, message_name.size() - 2);

                            break;
                        }
                    }
                }
            }
        }
        else {
            if (callee_decl->isTemplateDecl())
                LOG_DBG("Call to template function");

            const auto *callee_function = callee_decl->getAsFunction();

            if (!callee_function)
                return true;

            auto callee_name =
                callee_function->getQualifiedNameAsString() + "()";

            std::unique_ptr<model::function_template> f_ptr;

            if (!get_unique_id(callee_function->getID()).has_value()) {
                // This is hopefully not an interesting call...
                return true;
            }
            else {
                m.to = get_unique_id(callee_function->getID()).value();
            }

            auto message_name = callee_name;
            m.message_name = message_name.substr(0, message_name.size() - 2);

            if (f_ptr)
                diagram().add_participant(std::move(f_ptr));
        }

        //
        // This crashes on LLVM <= 12, for now just return empty type
        //
        // const auto &return_type =
        //    function_call_expr->getCallReturnType(current_ast_context);
        // m.return_type = return_type.getAsString();
        m.return_type = "";
    }
    else {
        return true;
    }

    if (m.from > 0 && m.to > 0) {
        if (diagram().sequences.find(m.from) == diagram().sequences.end()) {
            activity a;
            //            a.usr = m.from;
            a.from = m.from;
            diagram().sequences.insert({m.from, std::move(a)});
        }

        diagram().add_active_participant(m.from);
        diagram().add_active_participant(m.to);

        LOG_DBG("Found call {} from {} [{}] to {} [{}] ", m.message_name,
            m.from, m.from, m.to, m.to);

        diagram().sequences[m.from].messages.emplace_back(std::move(m));

        assert(!diagram().sequences.empty());
    }

    return true;
}

std::unique_ptr<clanguml::sequence_diagram::model::class_>
translation_unit_visitor::create_class_declaration(clang::CXXRecordDecl *cls)
{
    assert(cls != nullptr);

    auto c_ptr{std::make_unique<clanguml::sequence_diagram::model::class_>(
        config().using_namespace())};
    auto &c = *c_ptr;

    // TODO: refactor to method get_qualified_name()
    auto qualified_name =
        cls->getQualifiedNameAsString(); //  common::get_qualified_name(*cls);

    if (!diagram().should_include(qualified_name))
        return {};

    auto ns = common::get_tag_namespace(*cls);

    const auto *parent = cls->getParent();

    if (parent && parent->isRecord()) {
        // Here we have 2 options, either:
        //  - the parent is a regular C++ class/struct
        //  - the parent is a class template declaration/specialization
        std::optional<common::model::diagram_element::id_t> id_opt;
        int64_t local_id =
            static_cast<const clang::RecordDecl *>(parent)->getID();

        // First check if the parent has been added to the diagram as regular
        // class
        id_opt = get_unique_id(local_id);

        // If not, check if the parent template declaration is in the model
        if (!id_opt) {
            local_id = static_cast<const clang::RecordDecl *>(parent)
                           ->getDescribedTemplate()
                           ->getID();
            if (static_cast<const clang::RecordDecl *>(parent)
                    ->getDescribedTemplate())
                id_opt = get_unique_id(local_id);
        }

        assert(id_opt);

        auto parent_class =
            diagram_.get_participant<clanguml::sequence_diagram::model::class_>(
                *id_opt);

        assert(parent_class);

        c.set_namespace(ns);
        if (cls->getNameAsString().empty()) {
            // Nested structs can be anonymous
            if (anonymous_struct_relationships_.count(cls->getID()) > 0) {
                const auto &[label, hint, access] =
                    anonymous_struct_relationships_[cls->getID()];

                c.set_name(parent_class.value().name() + "##" +
                    fmt::format("({})", label));

                parent_class.value().add_relationship(
                    {hint, common::to_id(c.full_name(false)), access, label});
            }
            else
                c.set_name(parent_class.value().name() + "##" +
                    fmt::format(
                        "(anonymous_{})", std::to_string(cls->getID())));
        }
        else {
            c.set_name(
                parent_class.value().name() + "##" + cls->getNameAsString());
        }

        c.set_id(common::to_id(c.full_name(false)));

        c.nested(true);
    }
    else {
        c.set_name(common::get_tag_name(*cls));
        c.set_namespace(ns);
        c.set_id(common::to_id(c.full_name(false)));
    }

    c.is_struct(cls->isStruct());

    process_comment(*cls, c);
    set_source_location(*cls, c);

    if (c.skip())
        return {};

    c.set_style(c.style_spec());

    return c_ptr;
}

bool translation_unit_visitor::process_template_parameters(
    const clang::TemplateDecl &template_declaration,
    sequence_diagram::model::template_trait &c)
{
    using class_diagram::model::template_parameter;

    LOG_DBG("Processing class {} template parameters...",
        common::get_qualified_name(template_declaration));

    if (template_declaration.getTemplateParameters() == nullptr)
        return false;

    for (const auto *parameter :
        *template_declaration.getTemplateParameters()) {
        if (clang::dyn_cast_or_null<clang::TemplateTypeParmDecl>(parameter)) {
            const auto *template_type_parameter =
                clang::dyn_cast_or_null<clang::TemplateTypeParmDecl>(parameter);
            template_parameter ct;
            ct.set_type("");
            ct.is_template_parameter(true);
            ct.set_name(template_type_parameter->getNameAsString());
            ct.set_default_value("");
            ct.is_variadic(template_type_parameter->isParameterPack());

            c.add_template(std::move(ct));
        }
        else if (clang::dyn_cast_or_null<clang::NonTypeTemplateParmDecl>(
                     parameter)) {
            const auto *template_nontype_parameter =
                clang::dyn_cast_or_null<clang::NonTypeTemplateParmDecl>(
                    parameter);
            template_parameter ct;
            ct.set_type(template_nontype_parameter->getType().getAsString());
            ct.set_name(template_nontype_parameter->getNameAsString());
            ct.is_template_parameter(false);
            ct.set_default_value("");
            ct.is_variadic(template_nontype_parameter->isParameterPack());

            c.add_template(std::move(ct));
        }
        else if (clang::dyn_cast_or_null<clang::TemplateTemplateParmDecl>(
                     parameter)) {
            const auto *template_template_parameter =
                clang::dyn_cast_or_null<clang::TemplateTemplateParmDecl>(
                    parameter);
            template_parameter ct;
            ct.set_type("");
            ct.set_name(template_template_parameter->getNameAsString() + "<>");
            ct.is_template_parameter(true);
            ct.set_default_value("");
            ct.is_variadic(template_template_parameter->isParameterPack());

            c.add_template(std::move(ct));
        }
        else {
            // pass
        }
    }

    return false;
}

void translation_unit_visitor::set_unique_id(
    int64_t local_id, common::model::diagram_element::id_t global_id)
{
    LOG_DBG("== Setting local element mapping {} --> {}", local_id, global_id);

    local_ast_id_map_[local_id] = global_id;
}

std::optional<common::model::diagram_element::id_t>
translation_unit_visitor::get_unique_id(int64_t local_id) const
{
    if (local_ast_id_map_.find(local_id) == local_ast_id_map_.end())
        return {};

    return local_ast_id_map_.at(local_id);
}

std::unique_ptr<model::function_template>
translation_unit_visitor::build_function_template_instantiation(
    const clang::FunctionDecl &decl)
{
    //
    // Here we'll hold the template base params to replace with the
    // instantiated values
    //
    std::deque<std::tuple</*parameter name*/ std::string, /* position */ int,
        /*is variadic */ bool>>
        template_base_params{};

    auto template_instantiation_ptr =
        std::make_unique<model::function_template>(config_.using_namespace());
    auto &template_instantiation = *template_instantiation_ptr;

    //
    // Set function template instantiation name
    //
    auto template_decl_qualified_name = decl.getQualifiedNameAsString();
    common::model::namespace_ ns{template_decl_qualified_name};
    ns.pop_back();
    template_instantiation.set_name(decl.getNameAsString());
    template_instantiation.set_namespace(ns);

    //
    // Instantiate the template arguments
    //
    model::template_trait *parent{nullptr};
    build_template_instantiation_process_template_arguments(parent,
        template_base_params, decl.getTemplateSpecializationArgs()->asArray(),
        template_instantiation, "", decl.getPrimaryTemplate());

    return template_instantiation_ptr;
}

void translation_unit_visitor::
    build_template_instantiation_process_template_arguments(
        model::template_trait *parent,
        std::deque<std::tuple<std::string, int, bool>> &template_base_params,
        const clang::ArrayRef<clang::TemplateArgument> &template_args,
        model::template_trait &template_instantiation,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl)
{
    auto arg_index = 0U;
    for (const auto &arg : template_args) {
        const auto argument_kind = arg.getKind();
        class_diagram::model::template_parameter argument;
        if (argument_kind == clang::TemplateArgument::Template) {
            build_template_instantiation_process_template_argument(
                arg, argument);
        }
        else if (argument_kind == clang::TemplateArgument::Type) {
            build_template_instantiation_process_type_argument(parent,
                full_template_specialization_name, template_decl, arg,
                template_instantiation, argument);
        }
        else if (argument_kind == clang::TemplateArgument::Integral) {
            build_template_instantiation_process_integral_argument(
                arg, argument);
        }
        else if (argument_kind == clang::TemplateArgument::Expression) {
            build_template_instantiation_process_expression_argument(
                arg, argument);
        }
        else {
            LOG_ERROR("Unsupported argument type {}", arg.getKind());
        }

        simplify_system_template(
            argument, argument.to_string(config().using_namespace(), false));

        template_instantiation.add_template(std::move(argument));

        arg_index++;
    }
}

void translation_unit_visitor::
    build_template_instantiation_process_template_argument(
        const clang::TemplateArgument &arg,
        class_diagram::model::template_parameter &argument) const
{
    argument.is_template_parameter(true);
    auto arg_name =
        arg.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString();
    argument.set_type(arg_name);
}

void translation_unit_visitor::
    build_template_instantiation_process_integral_argument(
        const clang::TemplateArgument &arg,
        class_diagram::model::template_parameter &argument) const
{
    assert(arg.getKind() == clang::TemplateArgument::Integral);

    argument.is_template_parameter(false);
    argument.set_type(std::to_string(arg.getAsIntegral().getExtValue()));
}

void translation_unit_visitor::
    build_template_instantiation_process_expression_argument(
        const clang::TemplateArgument &arg,
        class_diagram::model::template_parameter &argument) const
{
    assert(arg.getKind() == clang::TemplateArgument::Expression);

    argument.is_template_parameter(false);
    argument.set_type(common::get_source_text(
        arg.getAsExpr()->getSourceRange(), source_manager()));
}

void translation_unit_visitor::
    build_template_instantiation_process_tag_argument(
        model::template_trait &template_instantiation,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl,
        const clang::TemplateArgument &arg,
        class_diagram::model::template_parameter &argument) const
{
    assert(arg.getKind() == clang::TemplateArgument::Type);

    argument.is_template_parameter(false);

    argument.set_name(
        common::to_string(arg.getAsType(), template_decl->getASTContext()));
}

void translation_unit_visitor::
    build_template_instantiation_process_type_argument(
        model::template_trait *parent,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl,
        const clang::TemplateArgument &arg,
        model::template_trait &template_instantiation,
        class_diagram::model::template_parameter &argument)
{
    assert(arg.getKind() == clang::TemplateArgument::Type);

    argument.is_template_parameter(false);

    // If this is a nested template type - add nested templates as
    // template arguments
    if (arg.getAsType()->getAs<clang::FunctionType>()) {

        //        for (const auto &param_type :
        //            arg.getAsType()->getAs<clang::FunctionProtoType>()->param_types())
        //            {
        //
        //            if (!param_type->getAs<clang::RecordType>())
        //                continue;
        //
        //            auto classTemplateSpecialization =
        //                llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(
        //                    param_type->getAsRecordDecl());
        //
        //            if (classTemplateSpecialization) {
        //                // Read arg info as needed.
        //                auto nested_template_instantiation =
        //                    build_template_instantiation_from_class_template_specialization(
        //                        *classTemplateSpecialization,
        //                        *param_type->getAs<clang::RecordType>(),
        //                        diagram().should_include(
        //                            full_template_specialization_name)
        //                            ?
        //                            std::make_optional(&template_instantiation)
        //                            : parent);
        //            }
        //        }
    }
    else if (arg.getAsType()->getAs<clang::TemplateSpecializationType>()) {
        const auto *nested_template_type =
            arg.getAsType()->getAs<clang::TemplateSpecializationType>();

        const auto nested_template_name =
            nested_template_type->getTemplateName()
                .getAsTemplateDecl()
                ->getQualifiedNameAsString();

        auto [tinst_ns, tinst_name] = cx::util::split_ns(nested_template_name);

        argument.set_name(nested_template_name);

        //        auto nested_template_instantiation =
        //        build_template_instantiation(
        //            *arg.getAsType()->getAs<clang::TemplateSpecializationType>(),
        //            diagram().should_include(full_template_specialization_name)
        //                ? std::make_optional(&template_instantiation)
        //                : parent);
        //
        //        argument.set_id(nested_template_instantiation->id());
        //
        //        for (const auto &t :
        //        nested_template_instantiation->templates())
        //            argument.add_template_param(t);

        // Check if this template should be simplified (e.g. system
        // template aliases such as 'std:basic_string<char>' should
        // be simply 'std::string')
        simplify_system_template(
            argument, argument.to_string(config().using_namespace(), false));
    }
    else if (arg.getAsType()->getAs<clang::TemplateTypeParmType>()) {
        argument.is_template_parameter(true);
        argument.set_name(
            common::to_string(arg.getAsType(), template_decl->getASTContext()));
    }
    else {
        // This is just a regular record type
        build_template_instantiation_process_tag_argument(
            template_instantiation, full_template_specialization_name,
            template_decl, arg, argument);
    }
}

std::unique_ptr<model::class_>
translation_unit_visitor::process_template_specialization(
    clang::ClassTemplateSpecializationDecl *cls)
{
    auto c_ptr{std::make_unique<model::class_>(config_.using_namespace())};
    auto &template_instantiation = *c_ptr;

    // TODO: refactor to method get_qualified_name()
    auto qualified_name = cls->getQualifiedNameAsString();
    util::replace_all(qualified_name, "(anonymous namespace)", "");
    util::replace_all(qualified_name, "::::", "::");

    common::model::namespace_ ns{qualified_name};
    ns.pop_back();
    template_instantiation.set_name(cls->getNameAsString());
    template_instantiation.set_namespace(ns);

    template_instantiation.is_struct(cls->isStruct());

    process_comment(*cls, template_instantiation);
    set_source_location(*cls, template_instantiation);

    if (template_instantiation.skip())
        return {};

    const auto template_args_count = cls->getTemplateArgs().size();
    for (auto arg_it = 0U; arg_it < template_args_count; arg_it++) {
        const auto arg = cls->getTemplateArgs().get(arg_it);
        process_template_specialization_argument(
            cls, template_instantiation, arg, arg_it);
    }

    template_instantiation.set_id(
        common::to_id(template_instantiation.full_name(false)));

    set_unique_id(cls->getID(), template_instantiation.id());

    return c_ptr;
}

void translation_unit_visitor::process_template_specialization_argument(
    const clang::ClassTemplateSpecializationDecl *cls,
    model::class_ &template_instantiation, const clang::TemplateArgument &arg,
    size_t argument_index, bool in_parameter_pack)
{
    const auto argument_kind = arg.getKind();

    if (argument_kind == clang::TemplateArgument::Type) {
        class_diagram::model::template_parameter argument;
        argument.is_template_parameter(false);

        // If this is a nested template type - add nested templates as
        // template arguments
        if (arg.getAsType()->getAs<clang::TemplateSpecializationType>()) {
            const auto *nested_template_type =
                arg.getAsType()->getAs<clang::TemplateSpecializationType>();

            const auto nested_template_name =
                nested_template_type->getTemplateName()
                    .getAsTemplateDecl()
                    ->getQualifiedNameAsString();

            argument.set_name(nested_template_name);

            auto nested_template_instantiation = build_template_instantiation(
                *arg.getAsType()->getAs<clang::TemplateSpecializationType>(),
                &template_instantiation);

            argument.set_id(nested_template_instantiation->id());

            for (const auto &t : nested_template_instantiation->templates())
                argument.add_template_param(t);

            // Check if this template should be simplified (e.g. system
            // template aliases such as 'std:basic_string<char>' should be
            // simply 'std::string')
            simplify_system_template(argument,
                argument.to_string(config().using_namespace(), false));
        }
        else if (arg.getAsType()->getAs<clang::TemplateTypeParmType>()) {
            auto type_name =
                common::to_string(arg.getAsType(), cls->getASTContext());

            // clang does not provide declared template parameter/argument
            // names in template specializations - so we have to extract
            // them from raw source code...
            if (type_name.find("type-parameter-") == 0) {
                auto declaration_text = common::get_source_text_raw(
                    cls->getSourceRange(), source_manager());

                declaration_text = declaration_text.substr(
                    declaration_text.find(cls->getNameAsString()) +
                    cls->getNameAsString().size() + 1);

                auto template_params =
                    cx::util::parse_unexposed_template_params(
                        declaration_text, [](const auto &t) { return t; });

                if (template_params.size() > argument_index)
                    type_name = template_params[argument_index].to_string(
                        config().using_namespace(), false);
                else {
                    LOG_DBG("Failed to find type specialization for argument "
                            "{} at index {} in declaration \n===\n{}\n===\n",
                        type_name, argument_index, declaration_text);
                }
            }

            argument.set_name(type_name);
        }
        else {
            auto type_name =
                common::to_string(arg.getAsType(), cls->getASTContext());
            if (type_name.find('<') != std::string::npos) {
                // Sometimes template instantiation is reported as
                // RecordType in the AST and getAs to
                // TemplateSpecializationType returns null pointer so we
                // have to at least make sure it's properly formatted
                // (e.g. std:integral_constant, or any template
                // specialization which contains it - see t00038)
                process_unexposed_template_specialization_parameters(
                    type_name.substr(type_name.find('<') + 1,
                        type_name.size() - (type_name.find('<') + 2)),
                    argument, template_instantiation);

                argument.set_name(type_name.substr(0, type_name.find('<')));
            }
            else if (type_name.find("type-parameter-") == 0) {
                auto declaration_text = common::get_source_text_raw(
                    cls->getSourceRange(), source_manager());

                declaration_text = declaration_text.substr(
                    declaration_text.find(cls->getNameAsString()) +
                    cls->getNameAsString().size() + 1);

                auto template_params =
                    cx::util::parse_unexposed_template_params(
                        declaration_text, [](const auto &t) { return t; });

                if (template_params.size() > argument_index)
                    type_name = template_params[argument_index].to_string(
                        config().using_namespace(), false);
                else {
                    LOG_DBG("Failed to find type specialization for argument "
                            "{} at index {} in declaration \n===\n{}\n===\n",
                        type_name, argument_index, declaration_text);
                }

                // Otherwise just set the name for the template argument to
                // whatever clang says
                argument.set_name(type_name);
            }
            else
                argument.set_name(type_name);
        }

        LOG_DBG("Adding template instantiation argument {}",
            argument.to_string(config().using_namespace(), false));

        simplify_system_template(
            argument, argument.to_string(config().using_namespace(), false));

        template_instantiation.add_template(std::move(argument));
    }
    else if (argument_kind == clang::TemplateArgument::Integral) {
        class_diagram::model::template_parameter argument;
        argument.is_template_parameter(false);
        argument.set_type(std::to_string(arg.getAsIntegral().getExtValue()));
        template_instantiation.add_template(std::move(argument));
    }
    else if (argument_kind == clang::TemplateArgument::Expression) {
        class_diagram::model::template_parameter argument;
        argument.is_template_parameter(false);
        argument.set_type(common::get_source_text(
            arg.getAsExpr()->getSourceRange(), source_manager()));
        template_instantiation.add_template(std::move(argument));
    }
    else if (argument_kind == clang::TemplateArgument::TemplateExpansion) {
        class_diagram::model::template_parameter argument;
        argument.is_template_parameter(true);

        cls->getLocation().dump(source_manager());
    }
    else if (argument_kind == clang::TemplateArgument::Pack) {
        // This will only work for now if pack is at the end
        size_t argument_pack_index{argument_index};
        for (const auto &template_argument : arg.getPackAsArray()) {
            process_template_specialization_argument(cls,
                template_instantiation, template_argument,
                argument_pack_index++, true);
        }
    }
    else {
        LOG_ERROR("Unsupported template argument kind {} [{}]", arg.getKind(),
            cls->getLocation().printToString(source_manager()));
    }
}

std::unique_ptr<model::class_>
translation_unit_visitor::build_template_instantiation(
    const clang::TemplateSpecializationType &template_type_decl,
    model::class_ *parent)
{
    // TODO: Make sure we only build instantiation once

    //
    // Here we'll hold the template base params to replace with the
    // instantiated values
    //
    std::deque<std::tuple</*parameter name*/ std::string, /* position */ int,
        /*is variadic */ bool>>
        template_base_params{};

    auto *template_type_ptr = &template_type_decl;
    if (template_type_decl.isTypeAlias() &&
        template_type_decl.getAliasedType()
            ->getAs<clang::TemplateSpecializationType>())
        template_type_ptr = template_type_decl.getAliasedType()
                                ->getAs<clang::TemplateSpecializationType>();

    auto &template_type = *template_type_ptr;

    //
    // Create class_ instance to hold the template instantiation
    //
    auto template_instantiation_ptr =
        std::make_unique<model::class_>(config_.using_namespace());
    auto &template_instantiation = *template_instantiation_ptr;
    std::string full_template_specialization_name = common::to_string(
        template_type.desugar(),
        template_type.getTemplateName().getAsTemplateDecl()->getASTContext());

    auto *template_decl{template_type.getTemplateName().getAsTemplateDecl()};

    auto template_decl_qualified_name =
        template_decl->getQualifiedNameAsString();

    auto *class_template_decl{
        clang::dyn_cast<clang::ClassTemplateDecl>(template_decl)};

    if (class_template_decl && class_template_decl->getTemplatedDecl() &&
        class_template_decl->getTemplatedDecl()->getParent() &&
        class_template_decl->getTemplatedDecl()->getParent()->isRecord()) {

        common::model::namespace_ ns{
            common::get_tag_namespace(*class_template_decl->getTemplatedDecl()
                                           ->getParent()
                                           ->getOuterLexicalRecordContext())};

        std::string ns_str = ns.to_string();
        std::string name = template_decl->getQualifiedNameAsString();
        if (!ns_str.empty()) {
            name = name.substr(ns_str.size() + 2);
        }

        util::replace_all(name, "::", "##");
        template_instantiation.set_name(name);

        template_instantiation.set_namespace(ns);
    }
    else {
        common::model::namespace_ ns{template_decl_qualified_name};
        ns.pop_back();
        template_instantiation.set_name(template_decl->getNameAsString());
        template_instantiation.set_namespace(ns);
    }

    // TODO: Refactor handling of base parameters to a separate method

    // We need this to match any possible base classes coming from template
    // arguments
    std::vector<
        std::pair</* parameter name */ std::string, /* is variadic */ bool>>
        template_parameter_names{};

    for (const auto *parameter : *template_decl->getTemplateParameters()) {
        if (parameter->isTemplateParameter() &&
            (parameter->isTemplateParameterPack() ||
                parameter->isParameterPack())) {
            template_parameter_names.emplace_back(
                parameter->getNameAsString(), true);
        }
        else
            template_parameter_names.emplace_back(
                parameter->getNameAsString(), false);
    }

    // Check if the primary template has any base classes
    int base_index = 0;

    const auto *templated_class_decl =
        clang::dyn_cast_or_null<clang::CXXRecordDecl>(
            template_decl->getTemplatedDecl());

    if (templated_class_decl && templated_class_decl->hasDefinition())
        for (const auto &base : templated_class_decl->bases()) {
            const auto base_class_name = common::to_string(
                base.getType(), templated_class_decl->getASTContext(), false);

            LOG_DBG("Found template instantiation base: {}, {}",
                base_class_name, base_index);

            // Check if any of the primary template arguments has a
            // parameter equal to this type
            auto it = std::find_if(template_parameter_names.begin(),
                template_parameter_names.end(),
                [&base_class_name](
                    const auto &p) { return p.first == base_class_name; });

            if (it != template_parameter_names.end()) {
                const auto &parameter_name = it->first;
                const bool is_variadic = it->second;
                // Found base class which is a template parameter
                LOG_DBG("Found base class which is a template parameter "
                        "{}, {}, {}",
                    parameter_name, is_variadic,
                    std::distance(template_parameter_names.begin(), it));

                template_base_params.emplace_back(parameter_name,
                    std::distance(template_parameter_names.begin(), it),
                    is_variadic);
            }
            else {
                // This is a regular base class - it is handled by
                // process_template
            }
            base_index++;
        }

    build_template_instantiation_process_template_arguments(parent,
        template_base_params, template_type.template_arguments(),
        template_instantiation, full_template_specialization_name,
        template_decl);

    // First try to find the best match for this template in partially
    // specialized templates
    std::string destination{};
    std::string best_match_full_name{};
    auto full_template_name = template_instantiation.full_name(false);
    int best_match{};
    common::model::diagram_element::id_t best_match_id{0};

    for (const auto &[id, c] : diagram().participants) {
        const auto *participant_as_class =
            dynamic_cast<model::class_ *>(c.get());
        if ((participant_as_class != nullptr) &&
            (*participant_as_class == template_instantiation))
            continue;

        auto c_full_name = participant_as_class->full_name(false);
        auto match =
            participant_as_class->calculate_template_specialization_match(
                template_instantiation, template_instantiation.name_and_ns());

        if (match > best_match) {
            best_match = match;
            best_match_full_name = c_full_name;
            best_match_id = participant_as_class->id();
        }
    }

    auto templated_decl_id =
        template_type.getTemplateName().getAsTemplateDecl()->getID();
    //    auto templated_decl_local_id =
    //        get_unique_id(templated_decl_id).value_or(0);

    if (best_match_id > 0) {
        destination = best_match_full_name;
    }
    else {
        LOG_DBG("== Cannot determine global id for specialization template {} "
                "- delaying until the translation unit is complete ",
            templated_decl_id);
    }

    template_instantiation.set_id(
        common::to_id(template_instantiation_ptr->full_name(false)));

    return template_instantiation_ptr;
}

void translation_unit_visitor::
    process_unexposed_template_specialization_parameters(
        const std::string &type_name,
        class_diagram::model::template_parameter &tp, model::class_ &c) const
{
    auto template_params = cx::util::parse_unexposed_template_params(
        type_name, [](const std::string &t) { return t; });

    for (auto &param : template_params) {
        tp.add_template_param(param);
    }
}

bool translation_unit_visitor::simplify_system_template(
    class_diagram::model::template_parameter &ct, const std::string &full_name)
{
    if (config().type_aliases().count(full_name) > 0) {
        ct.set_name(config().type_aliases().at(full_name));
        ct.clear_params();
        return true;
    }
    else
        return false;
}
}
