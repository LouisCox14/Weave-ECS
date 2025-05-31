#pragma once
#include <cstdint>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <tuple>
#include <stdexcept>
#include "SparseSet.h"

namespace Weave
{
	namespace ECS
	{
		using EntityID = std::size_t;

		template<typename... Components>
		class WorldViewIterator 
		{
		public:
			using SparseSetsTuple = std::tuple<SparseSet<Components>&...>;

			WorldViewIterator(std::vector<EntityID>::iterator current, std::vector<EntityID>::iterator end, SparseSetsTuple sets)
				: current(current), end(end), sets(std::move(sets)) {}

			bool operator!=(const WorldViewIterator& other) const { return current != other.current; }
			void operator++() { ++current; }

			auto operator*() {
				EntityID entity = *current;
				return std::tuple_cat(std::make_tuple(entity), std::tie(*std::get<SparseSet<Components>&>(sets).Get(entity)...));
			}

		private:
			std::vector<EntityID>::iterator current, end;
			SparseSetsTuple sets;
		};

		template<typename... Components>
		class WorldView 
		{
		public:
			using SparseSetsTuple = std::tuple<SparseSet<Components>&...>;

			WorldView(std::vector<EntityID> entities, SparseSetsTuple sets)
				: validEntities(std::move(entities)), sets(std::move(sets)) {}

			WorldViewIterator<Components...> begin() { return WorldViewIterator<Components...>(validEntities.begin(), validEntities.end(), sets); }
			WorldViewIterator<Components...> end() { return WorldViewIterator<Components...>(validEntities.end(), validEntities.end(), sets); }
			WorldViewIterator<Components...> at(size_t index)
			{
				if (index >= validEntities.size()) throw std::out_of_range("Attempted to access entity outside of world view range.");
				return WorldViewIterator<Components...>(validEntities.begin() + index, validEntities.end(), sets);
			}

			std::size_t GetEntityCount()
			{
				return validEntities.size();
			}

		private:
			std::vector<EntityID> validEntities;
			SparseSetsTuple sets;
		};

		class World
		{
		private:
			std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> componentStorage;
			std::set<EntityID> availableEntityIDs;
			EntityID nextEntityID = 0;

			template<typename T>
			SparseSet<T>& GetComponentSet()
			{
				std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>>::const_iterator it = componentStorage.find(std::type_index(typeid(T)));;

				if (it == componentStorage.end())
				{
					std::unique_ptr<ISparseSet> newSet = std::make_unique<SparseSet<T>>();
					componentStorage[std::type_index(typeid(T))] = std::move(newSet);
					it = componentStorage.find(std::type_index(typeid(T)));
				}

				return static_cast<SparseSet<T>&>(*(it->second));
			}

			template<typename T>
			SparseSet<T>* TryGetComponentSet()
			{
				std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>>::const_iterator it = componentStorage.find(std::type_index(typeid(T)));;

				if (it == componentStorage.end())
					return nullptr;

				return static_cast<SparseSet<T>*>(it->second.get());
			}

		public:
			EntityID CreateEntity();
			void DeleteEntity(EntityID entity);

			bool IsEntityRegistered(EntityID entity) const;

			template<typename T>
			void AddComponent(EntityID entity, T component = T())
			{
				if (!IsEntityRegistered(entity))
					throw std::logic_error("Entity is not registered.");

				SparseSet<T>& set = GetComponentSet<T>();
				set.Set(entity, component);
			}

			template<typename... Components>
			void AddComponents(EntityID entity)
			{
				(AddComponent<Components>(entity), ...);
			}

			template <typename... Components>
			void AddComponents(EntityID entity, Components... components)
			{
				(AddComponent<Components>(entity, components), ...);
			}

			template<typename T>
			void RemoveComponent(EntityID entity) 
			{
				if (!IsEntityRegistered(entity))
					throw std::logic_error("Entity is not registered.");

				SparseSet<T>* componentSet = TryGetComponentSet<T>();

				if (!componentSet) return;

				componentSet->Delete(entity);
			}

			template<typename... Components>
			void RemoveComponents(EntityID entity)
			{
				(RemoveComponent<Components>(entity), ...);
			}

			template<typename T>
			T* TryGetComponent(EntityID entity)
			{
				if (!IsEntityRegistered(entity)) return nullptr;

				SparseSet<T>* componentSet = TryGetComponentSet<T>();

				if (!componentSet) return nullptr;

				return componentSet->Get(entity);
			}

			template<typename... ComponentTypes>
			WorldView<ComponentTypes...> GetView()
			{
				auto sets = std::forward_as_tuple(GetComponentSet<ComponentTypes>()...);

				std::vector<EntityID> baseEntities;
				std::size_t minSize = std::numeric_limits<std::size_t>::max();

				([&] {
					auto& set = GetComponentSet<ComponentTypes>();
					if (set.Size() < minSize) {
						baseEntities = set.GetIndexes();
						minSize = set.Size();
					}
					}(), ...);

				std::vector<EntityID> valid;
				for (EntityID entity : baseEntities) {
					if ((GetComponentSet<ComponentTypes>().HasIndex(entity) && ...)) {
						valid.push_back(entity);
					}
				}

				return WorldView<ComponentTypes...>(std::move(valid), std::forward_as_tuple(GetComponentSet<ComponentTypes>()...));
			}
		};
	}
}