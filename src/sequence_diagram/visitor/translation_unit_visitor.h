/**
 * @file src/sequence_diagram/visitor/translation_unit_visitor.h
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

#include "call_expression_context.h"
#include "common/visitor/translation_unit_visitor.h"
#include "config/config.h"
#include "sequence_diagram/model/diagram.h"

#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

#include <stack>

namespace clanguml::sequence_diagram::visitor {

using common::model::namespace_;
using common::model::template_parameter;

std::string to_string(const clang::FunctionTemplateDecl *decl);

/**
 * @brief Sequence diagram translation unit visitor
 *
 * This class implements the `clang::RecursiveASTVisitor` interface
 * for selected visitors relevant to generating sequence diagrams.
 */
class translation_unit_visitor
    : public clang::RecursiveASTVisitor<translation_unit_visitor>,
      public common::visitor::translation_unit_visitor {
public:
    /**
     * @brief Constructor.
     *
     * @param sm Current source manager reference
     * @param diagram Diagram model
     * @param config Diagram configuration
     */
    translation_unit_visitor(clang::SourceManager &sm,
        clanguml::sequence_diagram::model::diagram &diagram,
        const clanguml::config::sequence_diagram &config);

    ~translation_unit_visitor() override = default;

    /**
     * \defgroup Implementation of ResursiveASTVisitor methods
     * @{
     */
    bool shouldVisitTemplateInstantiations();

    bool VisitCallExpr(clang::CallExpr *expr);

    bool TraverseCallExpr(clang::CallExpr *expr);

    bool TraverseVarDecl(clang::VarDecl *VD);

    bool TraverseCXXMemberCallExpr(clang::CXXMemberCallExpr *expr);

    bool TraverseCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr);

    bool VisitCXXConstructExpr(clang::CXXConstructExpr *expr);

    bool TraverseCXXConstructExpr(clang::CXXConstructExpr *expr);

    bool TraverseCXXTemporaryObjectExpr(clang::CXXTemporaryObjectExpr *expr);

    bool VisitLambdaExpr(clang::LambdaExpr *expr);

    bool TraverseLambdaExpr(clang::LambdaExpr *expr);

    bool VisitCXXMethodDecl(clang::CXXMethodDecl *declaration);

    bool VisitCXXRecordDecl(clang::CXXRecordDecl *declaration);

    bool VisitClassTemplateDecl(clang::ClassTemplateDecl *declaration);

    bool VisitClassTemplateSpecializationDecl(
        clang::ClassTemplateSpecializationDecl *declaration);

    bool VisitFunctionDecl(clang::FunctionDecl *declaration);

    bool VisitFunctionTemplateDecl(
        clang::FunctionTemplateDecl *function_declaration);

    bool TraverseCompoundStmt(clang::CompoundStmt *stmt);

    bool TraverseIfStmt(clang::IfStmt *stmt);

    bool TraverseWhileStmt(clang::WhileStmt *stmt);

    bool TraverseDoStmt(clang::DoStmt *stmt);

    bool TraverseForStmt(clang::ForStmt *stmt);

    bool TraverseCXXForRangeStmt(clang::CXXForRangeStmt *stmt);

    bool TraverseCXXTryStmt(clang::CXXTryStmt *stmt);

    bool TraverseCXXCatchStmt(clang::CXXCatchStmt *stmt);

    bool TraverseSwitchStmt(clang::SwitchStmt *stmt);

    bool TraverseCaseStmt(clang::CaseStmt *stmt);

    bool TraverseDefaultStmt(clang::DefaultStmt *stmt);

    bool TraverseConditionalOperator(clang::ConditionalOperator *stmt);
    /** @} */

    /**
     * @brief Get diagram model reference
     *
     * @return Reference to diagram model created by the visitor
     */
    clanguml::sequence_diagram::model::diagram &diagram();

    /**
     * @brief Get diagram model reference
     *
     * @return Reference to diagram model created by the visitor
     */
    const clanguml::sequence_diagram::model::diagram &diagram() const;

    /**
     * @brief Get diagram config instance
     *
     * @return Reference to config instance
     */
    const clanguml::config::sequence_diagram &config() const;

    /**
     * @brief Get current call expression context reference
     *
     * @return Reference to the current call expression context
     */
    call_expression_context &context();

    /**
     * @brief Get current call expression context reference
     *
     * @return Reference to the current call expression context
     */
    const call_expression_context &context() const;

    /**
     * @brief Get participant by declaration
     *
     * @tparam T Participant type
     * @param decl Clang entity declaration
     * @return Optional reference to participant diagram element
     */
    template <typename T = model::participant>
    common::optional_ref<T> get_participant(const clang::Decl *decl)
    {
        assert(decl != nullptr);

        auto unique_participant_id = get_unique_id(decl->getID());
        if (!unique_participant_id.has_value())
            return {};

        return get_participant<T>(unique_participant_id.value());
    }

    /**
     * @brief Get participant by declaration
     *
     * @tparam T Participant type
     * @param decl Clang entity declaration
     * @return Optional reference to participant diagram element
     */
    template <typename T = model::participant>
    common::optional_ref<T> get_participant(const clang::Decl *decl) const
    {
        assert(decl != nullptr);

        auto unique_participant_id = get_unique_id(decl->getID());
        if (!unique_participant_id.has_value())
            return {};

        return get_participant<T>(unique_participant_id.value());
    }

    /**
     * @brief Get participant by global element id
     *
     * @tparam T Participant type
     * @param id Global element id
     * @return Optional reference to participant diagram element
     */
    template <typename T = model::participant>
    common::optional_ref<T> get_participant(
        const common::model::diagram_element::id_t id)
    {
        if (diagram().participants().find(id) == diagram().participants().end())
            return {};

        return common::optional_ref<T>(
            *(static_cast<T *>(diagram().participants().at(id).get())));
    }

    /**
     * @brief Get participant by global element id
     *
     * @tparam T Participant type
     * @param id Global element id
     * @return Optional reference to participant diagram element
     */
    template <typename T = model::participant>
    common::optional_ref<T> get_participant(
        common::model::diagram_element::id_t id) const
    {
        if (diagram().participants().find(id) == diagram().participants().end())
            return {};

        return common::optional_ref<T>(
            *(static_cast<T *>(diagram().participants().at(id).get())));
    }

    /**
     * @brief Store the mapping from local clang entity id (obtained using
     *        getID()) method to clang-uml global id
     *
     * @todo Refactor to @ref ast_id_mapper
     *
     * @param local_id Local AST element id
     * @param global_id Globa diagram element id
     */
    void set_unique_id(
        int64_t local_id, common::model::diagram_element::id_t global_id);

    /**
     * @brief Retrieve the global `clang-uml` entity id based on the Clang
     *        local id
     * @param local_id AST local element id
     * @return Global diagram element id
     */
    std::optional<common::model::diagram_element::id_t> get_unique_id(
        int64_t local_id) const;

    /**
     * @brief Finalize diagram model for this translation unit
     */
    void finalize();

private:
    /**
     * @brief Check if the diagram should include a declaration.
     *
     * @param decl Clang declaration.
     * @return True, if the entity should be included in the diagram.
     */
    bool should_include(const clang::TagDecl *decl) const;

    /**
     * @brief Check if the diagram should include a lambda expression.
     *
     * @param expr Lambda expression.
     * @return True, if the expression should be included in the diagram.
     */
    bool should_include(const clang::LambdaExpr *expr) const;

    /**
     * @brief Check if the diagram should include a call expression.
     *
     * @param expr Call expression.
     * @return True, if the expression should be included in the diagram.
     */
    bool should_include(const clang::CallExpr *expr) const;

    /**
     * @brief Check if the diagram should include a declaration.
     *
     * @param decl Clang declaration.
     * @return True, if the entity should be included in the diagram.
     */
    bool should_include(const clang::CXXMethodDecl *decl) const;

    /**
     * @brief Check if the diagram should include a declaration.
     *
     * @param decl Clang declaration.
     * @return True, if the entity should be included in the diagram.
     */
    bool should_include(const clang::FunctionDecl *decl) const;

    /**
     * @brief Check if the diagram should include a declaration.
     *
     * @param decl Clang declaration.
     * @return True, if the entity should be included in the diagram.
     */
    bool should_include(const clang::FunctionTemplateDecl *decl) const;

    /**
     * @brief Check if the diagram should include a declaration.
     *
     * @param decl Clang declaration.
     * @return True, if the entity should be included in the diagram.
     */
    bool should_include(const clang::ClassTemplateDecl *decl) const;

    /**
     * @todo Refactor this group of methods to @ref template_builder
     *
     * @{
     */
    std::unique_ptr<clanguml::sequence_diagram::model::class_>
    create_class_model(clang::CXXRecordDecl *cls);

    std::unique_ptr<clanguml::sequence_diagram::model::method>
    create_method_model(clang::CXXMethodDecl *cls);

    bool process_template_parameters(
        const clang::TemplateDecl &template_declaration,
        common::model::template_trait &c);

    std::unique_ptr<model::function_template>
    build_function_template_instantiation(const clang::FunctionDecl &pDecl);

    std::unique_ptr<model::function> build_function_model(
        const clang::FunctionDecl &declaration);

    std::unique_ptr<model::function_template> build_function_template(
        const clang::FunctionTemplateDecl &declaration);

    void build_template_instantiation_process_template_arguments(
        model::template_trait *parent,
        std::deque<std::tuple<std::string, int, bool>> &template_base_params,
        const clang::ArrayRef<clang::TemplateArgument> &template_args,
        model::template_trait &template_instantiation,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl);

    common::model::template_parameter
    build_template_instantiation_process_template_argument(
        const clang::TemplateArgument &arg) const;

    common::model::template_parameter
    build_template_instantiation_process_integral_argument(
        const clang::TemplateArgument &arg) const;

    common::model::template_parameter
    build_template_instantiation_process_expression_argument(
        const clang::TemplateArgument &arg) const;

    void build_template_instantiation_process_tag_argument(
        model::template_trait &template_instantiation,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl,
        const clang::TemplateArgument &arg,
        common::model::template_parameter &argument) const;

    common::model::template_parameter
    build_template_instantiation_process_type_argument(
        model::template_trait *parent,
        const std::string &full_template_specialization_name,
        const clang::TemplateDecl *template_decl,
        const clang::TemplateArgument &arg,
        model::template_trait &template_instantiation);

    std::unique_ptr<model::class_> process_template_specialization(
        clang::ClassTemplateSpecializationDecl *cls);

    void process_template_specialization_argument(
        const clang::ClassTemplateSpecializationDecl *cls,
        model::class_ &template_instantiation,
        const clang::TemplateArgument &arg, size_t argument_index,
        bool in_parameter_pack = false);

    void process_unexposed_template_specialization_parameters(
        const std::string &type_name, common::model::template_parameter &tp,
        model::class_ &c) const;

    std::unique_ptr<model::class_> build_template_instantiation(
        const clang::TemplateSpecializationType &template_type_decl,
        model::class_ *parent);

    bool simplify_system_template(common::model::template_parameter &ct,
        const std::string &full_name) const;

    std::string simplify_system_template(const std::string &full_name) const;
    /** }@ */

    /**
     * @brief Assuming `cls` is a lambda, create it's full name.
     *
     * @note Currently, lambda names are generated using their source code
     *       location including file path, line and column to ensure
     *       unique names.
     *
     * @param cls Lambda declaration
     * @return Full lambda unique name
     */
    std::string make_lambda_name(const clang::CXXRecordDecl *cls) const;

    /**
     * @brief Check if template is a smart pointer
     *
     * @param primary_template Template declaration
     * @return True, if template declaration is a smart pointer
     */
    bool is_smart_pointer(const clang::TemplateDecl *primary_template) const;

    /**
     * @brief Check, the callee is a template specialization
     *
     * @param dependent_member_expr Dependent member expression
     * @return True, if the callee is a template specialization
     */
    bool is_callee_valid_template_specialization(
        const clang::CXXDependentScopeMemberExpr *dependent_member_expr) const;

    /**
     * @brief Handle CXX constructor call
     *
     * @param m Message model
     * @param construct_expr CXX Construct expression
     * @return True, if `m` contains a valid constructor call
     */
    bool process_construct_expression(
        model::message &m, const clang::CXXConstructExpr *construct_expr);

    /**
     * @brief Handle a operator call expression
     *
     * @param m Message model
     * @param operator_call_expr Operator call expression
     * @return True, if `m` contains now a valid call expression model
     */
    bool process_operator_call_expression(model::message &m,
        const clang::CXXOperatorCallExpr *operator_call_expr);

    /**
     * @brief Handle a class method call expresion
     *
     * @param m Message model
     * @param method_call_expr Operator call expression
     * @return True, if `m` contains now a valid call expression model
     */
    bool process_class_method_call_expression(
        model::message &m, const clang::CXXMemberCallExpr *method_call_expr);

    /**
     * @brief Handle a class template method call expresion
     *
     * @param m Message model
     * @param expr Class template method call expression
     * @return True, if `m` contains now a valid call expression model
     */
    bool process_class_template_method_call_expression(
        model::message &m, const clang::CallExpr *expr);

    /**
     * @brief Handle a function call expresion
     *
     * @param m Message model
     * @param expr Function call expression
     * @return True, if `m` contains now a valid call expression model
     */
    bool process_function_call_expression(
        model::message &m, const clang::CallExpr *expr);

    /**
     * @brief Handle an unresolved lookup call expresion
     *
     * Unresolved lookup expression is a reference to a name which Clang was
     * not able to look up during parsing but could not resolve to a
     * specific declaration.
     *
     * @param m Message model
     * @param expr Call expression
     * @return True, if `m` contains now a valid call expression model
     */
    bool process_unresolved_lookup_call_expression(
        model::message &m, const clang::CallExpr *expr) const;

    /**
     * @brief Register a message model `m` with a call expression
     *
     * This is used to know whether a model for a specific call expression
     * has already been created, but not yet added to the diagram.
     *
     * @param expr Call expresion
     * @param m Message model
     */
    void push_message(clang::CallExpr *expr, model::message &&m);
    void push_message(clang::CXXConstructExpr *expr, model::message &&m);

    /**
     * @brief Move a message model to diagram.
     *
     * @param expr Call expression
     */
    void pop_message_to_diagram(clang::CallExpr *expr);
    void pop_message_to_diagram(clang::CXXConstructExpr *expr);

    std::optional<std::string> get_expression_comment(
        const clang::SourceManager &sm, const clang::ASTContext &context,
        int64_t caller_id, const clang::Stmt *stmt);

    // Reference to the output diagram model
    clanguml::sequence_diagram::model::diagram &diagram_;

    // Reference to class diagram config
    const clanguml::config::sequence_diagram &config_;

    call_expression_context call_expression_context_;

    /**
     * This is used to generate messages in proper order in case of nested call
     * expressions (e.g. a(b(c(), d())), as they need to be added to the diagram
     * sequence after the visitor leaves the call expression AST node
     */
    std::map<clang::CallExpr *, model::message> call_expr_message_map_;
    std::map<clang::CXXConstructExpr *, model::message>
        construct_expr_message_map_;

    std::map<common::model::diagram_element::id_t,
        std::unique_ptr<clanguml::sequence_diagram::model::class_>>
        forward_declarations_;

    /**
     * @todo Refactor to @ref ast_id_mapper
     */
    std::map</* local id from ->getID() */ int64_t,
        /* global ID based on full name */ common::model::diagram_element::id_t>
        local_ast_id_map_;

    std::map<int64_t /* local anonymous struct id */,
        std::tuple<std::string /* field name */, common::model::relationship_t,
            common::model::access_t>>
        anonymous_struct_relationships_;

    mutable unsigned within_static_variable_declaration_{0};
    mutable std::set<const clang::Expr *>
        already_visited_in_static_declaration_{};

    mutable std::set<std::pair<int64_t, const clang::RawComment *>>
        processed_comments_;
};
} // namespace clanguml::sequence_diagram::visitor
