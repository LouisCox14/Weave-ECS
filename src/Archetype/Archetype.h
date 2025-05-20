#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>
#include <cassert>
#include <stdexcept>
#include <set>
#include <iostream>
#include <mutex>

namespace Weave
{
	namespace ECS
	{
		using EntityID = std::size_t;

		template <typename T, typename... Ts>
		concept IsContainedIn = (std::same_as<T, Ts> || ...);

		template <typename... QueryComponents, typename... Components>
		concept ValidQuery = (IsContainedIn<QueryComponents, Components...> && ...);

		template <typename... Components>
		class ArchetypeView
		{
		private:
			std::vector<EntityID>* entities; 
			std::tuple<std::vector<Components>*...> componentVectors;

		public:
			ArchetypeView(std::vector<EntityID>& entities, std::vector<Components>&... components)
				: entities(&entities), componentVectors(&components...) {}

			class Iterator
			{
			private:
				size_t index;
				std::vector<EntityID>* entities;
				std::tuple<std::vector<Components>*...> componentVectors;

			public:
				Iterator(size_t idx, std::vector<EntityID>* entities, std::tuple<std::vector<Components>*...> components)
					: index(idx), entities(entities), componentVectors(components) {}

				Iterator(const Iterator&) = default;
				Iterator& operator=(const Iterator&) = default;

				Iterator(Iterator&&) = default;
				Iterator& operator=(Iterator&&) = default;

				bool operator==(const Iterator& other) const
				{
					return index == other.index && entities == other.entities;
				}

				Iterator& operator++()
				{
					++index;
					return *this;
				}

				bool operator!=(const Iterator& other) const
				{
					return index != other.index;
				}

				auto operator*()
				{
					return std::tuple<EntityID, Components&...>(
						(*entities)[index],
						(*std::get<std::vector<Components>*>(componentVectors))[index]...);
				}
			};

			Iterator begin() { return Iterator(0, entities, componentVectors); }
			Iterator end() { return Iterator(entities->size(), entities, componentVectors); }

            std::size_t GetEntityCount()
            {
                return entities->size();
            }
		};

        struct ComponentStore
        {
            void* data;
            std::size_t componentSize;
        };

        struct ComponentData
        {
            std::type_index index;
            std::size_t size;

            bool operator<(const ComponentData& other) const 
            {
                if (index != other.index) 
                {
                    return index < other.index;
                }

                return size < other.size;
            }
        };

        class Archetype {
        private:
            std::vector<EntityID> entities;
            std::unordered_map<std::type_index, ComponentStore> components;
            std::set<std::type_index> validTypes;

            std::mutex mutex;

            size_t GetEntityIndex(EntityID entity) 
            {
                auto it = std::find(entities.begin(), entities.end(), entity);

                if (it == entities.end())
                {
                    throw std::runtime_error("Entity not found in archetype");
                }

                return std::distance(entities.begin(), it);
            }

            void RemoveEntityAt(size_t index) 
            {
                size_t last = entities.size() - 1;
                if (index != last)
                {
                    entities[index] = entities[last];

                    for (auto& [type, store] : components)
                    {
                        std::vector<std::byte>* componentVec = static_cast<std::vector<std::byte>*>(store.data);
                        std::memcpy(componentVec->data() + index * store.componentSize, componentVec->data() + last * store.componentSize, store.componentSize);
                    }
                }

                entities.pop_back();
                for (auto& [type, store] : components)
                {
                    static_cast<std::vector<std::byte>*>(store.data)->resize(last * store.componentSize);
                }
            }

        public:
            explicit Archetype(const std::set<ComponentData> types)
            {
                for (const auto& type : types)
                {
                    components[type.index] = { new std::vector<std::byte>(), type.size };
                    validTypes.insert(type.index);
                }
            }

            ~Archetype() 
            {
                for (auto& [type, store] : components) 
                {
                    delete static_cast<std::vector<std::byte>*>(store.data);
                }
            }

            std::set<std::type_index> GetComponentTypes() 
            {
                return validTypes;
            }

            std::set<ComponentData> GetComponentData()
            {
                std::set<ComponentData> data;

                for (std::type_index type : validTypes)
                {
                    data.insert({ type, components[type].componentSize });
                }

                return data;
            }

            template <typename... Components>
            void AddEntity(EntityID entity, Components... componentData) 
            {
                mutex.lock();

                entities.push_back(entity);

                for (std::pair<const std::type_index, ComponentStore> componentPair : components)
                {
                    if (((typeid(Components) == componentPair.first) || ...)) continue;

                    std::vector<std::byte>* componentVector = static_cast<std::vector<std::byte>*>(componentPair.second.data);
                    componentVector->resize(entities.size() * componentPair.second.componentSize);
                }

                (GetComponentVector<Components>().emplace_back(std::move(componentData)), ...);

                mutex.unlock();
            }

            void RemoveEntity(EntityID entity)
            {
                mutex.lock();

                auto it = std::find(entities.begin(), entities.end(), entity);
                if (it != entities.end())
                {
                    size_t index = std::distance(entities.begin(), it);
                    RemoveEntityAt(index);
                }

                mutex.unlock();
            }

            template <typename... Components>
            std::tuple<Components&...> GetComponents(EntityID entity) 
            {
                size_t index = GetEntityIndex(entity);
                return std::tie(GetComponentVector<Components>()[index]...);
            }

            template <typename Component>
            Component* GetComponent(EntityID entity)
            {
                size_t index = GetEntityIndex(entity);

                if (!validTypes.contains(typeid(Component))) return nullptr;

                return &GetComponentVector<Component>()[index];
            }

            void* GetComponent(EntityID entity, std::type_index typeIndex)
            {
                size_t index = GetEntityIndex(entity);

                if (!components.contains(typeIndex)) return nullptr;

                return static_cast<std::vector<std::byte>*>(components[typeIndex].data)->data() + components[typeIndex].componentSize * index;
            }

            std::vector<EntityID>& GetEntityVector()
            {
                return entities;
            }

            template <typename Component>
            std::vector<Component>& GetComponentVector() 
            {
                std::type_index type = typeid(Component);
                if (validTypes.find(type) == validTypes.end())
                {
                    throw std::runtime_error("Invalid component type for this archetype");
                }

                if (components[type].data == nullptr)
                {
                    components[type].data = new std::vector<Component>();
                    components[type].componentSize = sizeof(Component);
                }

                std::vector<Component>* vec = static_cast<std::vector<Component>*>(components[type].data);

                return *vec;
            }

            ComponentStore& GetComponentStore(std::type_index type, size_t size) 
            {
                if (validTypes.find(type) == validTypes.end()) 
                {
                    throw std::runtime_error("Invalid component type for this archetype");
                }

                if (components[type].data == nullptr) 
                {
                    components[type].data = new std::vector<std::byte>();
                    components[type].componentSize = size;
                    static_cast<std::vector<std::byte>*>(components[type].data)->resize(size * entities.capacity());
                }

                return components[type];
            }
        };
	}
}