#pragma once
#include "Archetype.h"
#include <unordered_map>
#include <set>
#include <typeindex>
#include <memory>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <iterator>
#include <utility>
#include <iostream>
#include <functional>

namespace Weave
{
	namespace ECS
	{
        template <typename... Components>
        class WorldView
        {
        private:
            std::vector<ArchetypeView<Components...>> archetypeViews;
            std::vector<size_t> cumulativeSizes;

        public:
            WorldView(std::vector<ArchetypeView<Components...>> views) : archetypeViews(std::move(views))
            {
                cumulativeSizes.reserve(archetypeViews.size());
                size_t total = 0;
                for (auto view : archetypeViews)
                {
                    total += view.GetEntityCount();
                    cumulativeSizes.push_back(total);
                }
            }

            class Iterator
            {
            private:
                using ArchetypeIterator = typename ArchetypeView<Components...>::Iterator;
                std::vector<ArchetypeView<Components...>>* archetypeViews;
                size_t currentArchetypeIndex;
                ArchetypeIterator currentIterator;

                void AdvanceToNextValidArchetype()
                {
                    while (currentArchetypeIndex < archetypeViews->size())
                    {
                        if (currentIterator != (*archetypeViews)[currentArchetypeIndex].end())
                        {
                            break;
                        }

                        ++currentArchetypeIndex;

                        if (currentArchetypeIndex < archetypeViews->size())
                        {
                            currentIterator = (*archetypeViews)[currentArchetypeIndex].begin();
                        }
                    }
                }

            public:
                Iterator(std::vector<ArchetypeView<Components...>>* views, size_t archetypeIndex, const ArchetypeIterator& iterator)
                    : archetypeViews(views), currentArchetypeIndex(archetypeIndex), currentIterator(iterator)
                {
                    AdvanceToNextValidArchetype();
                }

                Iterator& operator++()
                {
                    ++currentIterator;
                    AdvanceToNextValidArchetype();
                    return *this;
                }

                bool operator!=(const Iterator& other) const
                {
                    return currentArchetypeIndex != other.currentArchetypeIndex || currentIterator != other.currentIterator;
                }

                auto operator*()
                {
                    return *currentIterator;
                }
            };

            Iterator begin()
            {
                if (archetypeViews.empty())
                {
                    return end();
                }

                return Iterator(&archetypeViews, 0, archetypeViews[0].begin());
            }

            Iterator end()
            {
                if (archetypeViews.empty())
                    return Iterator(&archetypeViews, 0, typename ArchetypeView<Components...>::Iterator(0, nullptr, {}));

                return Iterator(&archetypeViews, archetypeViews.size(), archetypeViews.back().end());
            }

            Iterator at(size_t index) 
            {
                if (index >= cumulativeSizes.back()) {
                    throw std::out_of_range("WorldView index out of range");
                }

                auto it = std::upper_bound(cumulativeSizes.begin(), cumulativeSizes.end(), index);
                size_t archetypeIndex = std::distance(cumulativeSizes.begin(), it);

                size_t prevTotal = archetypeIndex > 0 ? cumulativeSizes[archetypeIndex - 1] : 0;
                size_t localIndex = index - prevTotal;

                return Iterator(&archetypeViews, archetypeIndex, archetypeViews[archetypeIndex].at(localIndex));
            }

            std::size_t GetEntityCount()
            {
                std::size_t count = 0;

                for (ArchetypeView<Components...> archetypeView : archetypeViews)
                {
                    count += archetypeView.GetEntityCount();
                }

                return count;
            }
        };

        class World;

		class World
		{
		private:
			EntityID nextEntityID = 0;
			std::set<EntityID> availableEntityIDs;

            std::map<EntityID, Archetype*> entityToArchetype;
            std::map<std::type_index, std::set<Archetype*>> componentToArchetypes;
            std::map<std::set<std::type_index>, std::unique_ptr<Archetype>> archetypes;

            std::map<std::type_index, std::size_t> componentSizes;

            template <typename... Components>
            Archetype& GetArchetype()
            {
                std::set<ComponentData> typeSet;

                (typeSet.insert({ typeid(Components), sizeof(Components) }), ...);

                return GetArchetype(typeSet);
            }

            Archetype& GetArchetype(const std::set<ComponentData> dataSet)
            {
                std::set<std::type_index> typeSet;

                for (ComponentData data : dataSet)
                {
                    typeSet.insert(data.index);
                }

                auto it = archetypes.find(typeSet);

                if (it == archetypes.end()) 
                {
                    auto archetype = std::make_unique<Archetype>(dataSet);
                    archetypes.emplace(typeSet, std::move(archetype));
                    Archetype* archetypePtr = archetypes[typeSet].get();

                    for (const auto& type : typeSet) {
                        componentToArchetypes[type].insert(archetypePtr);
                    }

                    return *archetypePtr;
                }
                return *it->second.get();
            }

            template <typename... Components>
            void TransferEntity(EntityID entity, Archetype* newArchetype, Archetype* oldArchetype, Components&&... newComponents)
            {
                if (!oldArchetype)
                {
                    newArchetype->AddEntity(entity, std::forward<Components>(newComponents)...);
                    entityToArchetype[entity] = newArchetype;
                    return;
                }

                newArchetype->AddEntity(entity, std::forward<Components>(newComponents)...);

                std::size_t newEntityIndex = newArchetype->GetEntityVector().size() - 1;

                std::set<std::type_index> newTypes = newArchetype->GetComponentTypes();

                for (std::type_index typeIndex : oldArchetype->GetComponentTypes())
                {
                    if (!newTypes.contains(typeIndex)) continue;

                    std::size_t typeSize = componentSizes[typeIndex];

                    void* oldComponentPtr = oldArchetype->GetComponent(entity, typeIndex);
                    void* newComponentPtr = static_cast<std::vector<std::byte>*>(newArchetype->GetComponentStore(typeIndex, typeSize).data)->data() + newEntityIndex * typeSize;

                    std::memcpy(newComponentPtr, oldComponentPtr, typeSize);
                }

                oldArchetype->RemoveEntity(entity);
                entityToArchetype[entity] = newArchetype;
            }

		public:
			EntityID CreateEntity();
			void DeleteEntity(EntityID entity);

			bool IsEntityRegistered(EntityID entity) const;

            template <typename Component>
            Component* TryGetComponent(EntityID entity)
            {
                if (!entityToArchetype.contains(entity)) return nullptr;

                Archetype* entityArchetype = entityToArchetype[entity];

                if (!entityArchetype->GetComponentTypes().contains(typeid(Component))) return nullptr;

                return entityArchetype->GetComponent<Component>(entity);
            }

            template <typename Component>
            void AddComponent(EntityID entity, Component component = Component())
            {
                AddComponents(entity, component);
            }

            template <typename... Components>
            void AddComponents(EntityID entity)
            {
                AddComponents(entity, Components{}...);
            }

            template <typename... Components>
            void AddComponents(EntityID entity, Components... components)
            {
                ((componentSizes[typeid(Components)] = sizeof(Components)), ...);

                std::set<ComponentData> newTypeSet;
                Archetype* oldArchetype = nullptr;

                if (entityToArchetype.contains(entity)) 
                {
                    oldArchetype = entityToArchetype[entity];
                    newTypeSet = oldArchetype->GetComponentData();
                }

                (newTypeSet.insert({ typeid(Components), sizeof(Components) }), ...);

                Archetype* newArchetype = &GetArchetype(newTypeSet);

                if (newArchetype == oldArchetype) return;

                TransferEntity(entity, newArchetype, oldArchetype, std::forward<Components>(components)...);
            }

            template <typename Component>
            void RemoveComponent(EntityID entity)
            {
                RemoveComponents<Component>(entity);
            }

            template <typename... Components>
            void RemoveComponents(EntityID entity)
            {
                if (!entityToArchetype.contains(entity)) return;

                std::set<ComponentData> newTypeSet;

                Archetype* oldArchetype = entityToArchetype[entity];
                newTypeSet = oldArchetype->GetComponentData();

                (newTypeSet.erase({ typeid(Components), sizeof(Components) }), ...);

                Archetype* newArchetype = &GetArchetype(newTypeSet);

                TransferEntity(entity, newArchetype, oldArchetype);
            }

            template <typename... QueryComponents>
            WorldView<QueryComponents...> GetView()
            {
                std::vector<ArchetypeView<QueryComponents...>> views;

                std::set<Archetype*> matchingArchetypes;

                if constexpr (sizeof...(QueryComponents) > 0)
                {
                    matchingArchetypes = componentToArchetypes[typeid(std::tuple_element_t<0, std::tuple<QueryComponents...>>)];
                }

                ([&]<typename T>(T*) {
                    const auto& archetypesForComponent = componentToArchetypes[typeid(T)];
                    std::set<Archetype*> intersection;
                    std::set_intersection(
                        matchingArchetypes.begin(), matchingArchetypes.end(),
                        archetypesForComponent.begin(), archetypesForComponent.end(),
                        std::inserter(intersection, intersection.begin())
                    );
                    matchingArchetypes = std::move(intersection);
                }(static_cast<QueryComponents*>(nullptr)), ...);

                for (Archetype* archetype : matchingArchetypes)
                {
                    std::vector<EntityID>& entityVector = archetype->GetEntityVector();
                    if (entityVector.size() == 0) continue;

                    auto componentVectors = std::make_tuple(
                        static_cast<std::vector<QueryComponents>*>(&archetype->GetComponentVector<QueryComponents>())...
                    );

                    views.emplace_back(ArchetypeView<QueryComponents...>(entityVector, *std::get<std::vector<QueryComponents>*>(componentVectors)...));
                }

                return WorldView<QueryComponents...>(std::move(views));
            }
		};
	}
}