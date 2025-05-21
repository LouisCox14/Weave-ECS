#pragma once
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <iterator>
#include <stdexcept>
#include "SparseSet.h"
#include "Query.h"
#include "Events.h"
#include <iostream>

namespace Weave 
{
    namespace ECS
    {
        class IQueryNode 
        {
        public:
            virtual std::vector<std::size_t> GetValidEntities() = 0;
            virtual std::vector<ISparseSet*> GetSparseSets() = 0;
            virtual bool HasEntity(std::size_t entity) = 0;
            virtual std::unordered_set<std::type_index> GetGuaranteedComponents() = 0;

            Utilities::Event<> onNodeUpdated;
        };

        template<typename T>
        class SparseSetNode : public IQueryNode 
        {
        private:
            void HandleSetUpdated()
            {
                onNodeUpdated();
            }

        public:
            SparseSet<T>* sparseSet;

            SparseSetNode(SparseSet<T>* _sparseSet) : sparseSet(_sparseSet) 
            {
                sparseSet->onSetUpdated.Subscribe(std::ref(*this), &SparseSetNode::HandleSetUpdated);
            }

            std::vector<std::size_t> GetValidEntities() override 
            {
                return sparseSet->GetIndexes();
            }

            std::vector<ISparseSet*> GetSparseSets() override 
            {
                return std::vector<ISparseSet*>{ sparseSet };
            }

            bool HasEntity(std::size_t entity) override 
            {
                return sparseSet->HasIndex(entity);
            }

            std::unordered_set<std::type_index> GetGuaranteedComponents() override 
            {
                return { std::type_index(typeid(T)) };
            }
        };

        class IntersectionNode : public IQueryNode 
        {
        private:
            std::vector<IQueryNode*> children;

            void HandleNodeUpdated()
            {
                onNodeUpdated();
            }

        public:
            IntersectionNode(std::vector<IQueryNode*> _children) : children(_children)
            {
                for (IQueryNode* node : children)
                {
                    node->onNodeUpdated.Subscribe(*this, &IntersectionNode::HandleNodeUpdated);
                }
            }

            std::vector<std::size_t> GetValidEntities() override {
                if (children.empty()) return {};

                // Start with the smallest set to minimize intersection cost
                auto smallestChild = *std::min_element(children.begin(), children.end(), [](IQueryNode* a, IQueryNode* b) {
                    return a->GetValidEntities().size() < b->GetValidEntities().size();
                    });

                std::vector<std::size_t> baseEntities = smallestChild->GetValidEntities();
                std::unordered_set<std::size_t> resultSet(baseEntities.begin(), baseEntities.end());

                for (auto* child : children) {
                    if (child == smallestChild) continue;
                    std::vector<std::size_t> childEntities = child->GetValidEntities();
                    std::unordered_set<std::size_t> tempSet(childEntities.begin(), childEntities.end());

                    for (auto it = resultSet.begin(); it != resultSet.end();) {
                        if (tempSet.find(*it) == tempSet.end())
                            it = resultSet.erase(it);
                        else
                            ++it;
                    }
                }

                return { resultSet.begin(), resultSet.end() };
            }

            std::vector<ISparseSet*> GetSparseSets() override {
                std::vector<ISparseSet*> sparseSets;
                for (auto* child : children) {
                    auto childSets = child->GetSparseSets();
                    sparseSets.insert(sparseSets.end(), childSets.begin(), childSets.end());
                }
                return sparseSets;
            }

            bool HasEntity(std::size_t entity) override {
                return std::all_of(children.begin(), children.end(), [entity](IQueryNode* child) {
                    return child->HasEntity(entity);
                    });
            }

            std::unordered_set<std::type_index> GetGuaranteedComponents() override {
                if (children.empty()) return {};

                std::unordered_set<std::type_index> guaranteedComponents;

                for (IQueryNode* child : children)
                {
                    std::unordered_set<std::type_index> childComponents = child->GetGuaranteedComponents();
                    guaranteedComponents.insert(childComponents.begin(), childComponents.end());
                }

                return guaranteedComponents;
            }
        };

        class DifferenceNode : public IQueryNode 
        {
        private:
            IQueryNode* mainSet;
            IQueryNode* exclusionSet;

            void HandleNodeUpdated()
            {
                onNodeUpdated();
            }

        public:
            DifferenceNode(IQueryNode* _mainSet, IQueryNode* _exclusionSet) : mainSet(_mainSet), exclusionSet(_exclusionSet) 
            {
                mainSet->onNodeUpdated.Subscribe(*this, &DifferenceNode::HandleNodeUpdated);
                exclusionSet->onNodeUpdated.Subscribe(*this, &DifferenceNode::HandleNodeUpdated);
            }

            std::vector<std::size_t> GetValidEntities() override {
                std::vector<std::size_t> mainEntities = mainSet->GetValidEntities();
                std::vector<std::size_t> exclusionEntities = exclusionSet->GetValidEntities();

                std::unordered_set<std::size_t> exclusionSet(exclusionEntities.begin(), exclusionEntities.end());
                std::vector<std::size_t> result;

                std::copy_if(mainEntities.begin(), mainEntities.end(), std::back_inserter(result), [&exclusionSet](std::size_t id) {
                    return exclusionSet.find(id) == exclusionSet.end();
                    });

                return result;
            }

            std::vector<ISparseSet*> GetSparseSets() override {
                auto mainSets = mainSet->GetSparseSets();
                auto exclusionSets = exclusionSet->GetSparseSets();

                mainSets.insert(mainSets.end(), exclusionSets.begin(), exclusionSets.end());
                return mainSets;
            }

            bool HasEntity(std::size_t entity) override {
                return mainSet->HasEntity(entity) && !exclusionSet->HasEntity(entity);
            }

            std::unordered_set<std::type_index> GetGuaranteedComponents() override {
                return mainSet->GetGuaranteedComponents();
            }
        };

        template<typename Tuple, std::size_t... Indices>
        auto getComponentsForEntity(const Tuple & sets, std::size_t entity, std::index_sequence<Indices...>) {
            return std::tie(*std::get<Indices>(sets).Get(entity)...);
        }

        namespace detail {
            template<typename...> struct TypeList {};

            template<typename List, typename T>
            struct Contains;

            template<typename T>
            struct Contains<TypeList<>, T> : std::false_type {};

            template<typename Head, typename... Tail, typename T>
            struct Contains<TypeList<Head, Tail...>, T> : std::conditional_t<std::is_same_v<Head, T>, std::true_type, Contains<TypeList<Tail...>, T>> {};

            template<typename List, typename... Ts>
            struct EnsureUnique;

            template<typename List>
            struct EnsureUnique<List> : std::true_type {};

            template<typename List, typename Head, typename... Tail>
            struct EnsureUnique<List, Head, Tail...> : std::conditional_t<Contains<List, Head>::value, std::false_type, EnsureUnique<TypeList<List, Head>, Tail...>> {};
        }

        template<typename... Components>
        class QueryIterator {
        private:
            std::vector<std::size_t>::iterator current;
            std::vector<std::size_t>::iterator end;
            std::tuple<SparseSet<Components>&...> sets;

        public:
            QueryIterator(std::vector<std::size_t>::iterator _current, std::vector<std::size_t>::iterator _end, std::tuple<SparseSet<Components>&...> _sets)
                : current(_current), end(_end), sets(_sets) {}

            bool operator!=(const QueryIterator& other) const {
                return current != other.current;
            }

            void operator++() {
                ++current;
            }

            auto operator*() {
                return std::tuple_cat(std::make_tuple(*current), getComponentsForEntity(sets, *current, std::index_sequence_for<Components...>{}));
            }
        };

        template<typename... Components>
        class Query {
        private:
            IQueryNode* root;
            std::tuple<SparseSet<Components>&...> sets;
            std::vector<std::size_t> validEntities;

            bool isDirty;

            void HandleNodeUpdated()
            {
                isDirty = true;
            }

            void RebuildQuery() 
            {
                if (isDirty) 
                {
                    validEntities = root->GetValidEntities();
                    isDirty = false;
                }
            }

            static_assert(detail::EnsureUnique<detail::TypeList<>, Components...>::value, "Duplicate component types detected in Query. Each component type must be unique.");

        public:
            Query(IQueryNode* _root, SparseSet<Components>&... _sets) : root(_root), sets(_sets...) {
                validEntities = root->GetValidEntities();

                // Ensure requested components match guaranteed components (runtime check)
                auto guaranteedComponents = root->GetGuaranteedComponents();
                bool allComponentsGuaranteed = ((guaranteedComponents.count(std::type_index(typeid(Components))) > 0) && ...);
                if (!allComponentsGuaranteed) {
                    throw std::logic_error("Query requests components not guaranteed by the result.");
                }

                // Runtime check: Ensure all entities in validEntities exist in all SparseSets
                for (std::size_t entity : validEntities) {
                    if (!((std::get<SparseSet<Components>&>(sets).HasIndex(entity)) && ...)) {
                        throw std::runtime_error("Entity missing from one or more SparseSets");
                    }
                }

                root->onNodeUpdated.Subscribe(std::ref(*this), &Query::HandleNodeUpdated);
            }

            auto begin()
            {
                RebuildQuery();
                return QueryIterator<Components...>(validEntities.begin(), validEntities.end(), sets);
            }

            auto end()
            {
                RebuildQuery();
                return QueryIterator<Components...>(validEntities.end(), validEntities.end(), sets);
            }
        };
    }
}