#pragma once
#include <cstdint>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <tuple>
#include "SparseSet.h"
#include "Query.h"

namespace Weave
{
	namespace ECS
	{
		using EntityID = std::size_t;

		class SparseSetWorld
		{
		private:
			std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> componentStorage;
			std::set<EntityID> availableEntityIDs;
			EntityID nextEntityID = 0;

			std::unordered_map<std::type_index, std::unique_ptr<IQueryNode>> componentQueryNodes;
			std::vector<std::unique_ptr<IQueryNode>> queryNodes;


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

			template<typename T>
			void RemoveComponent(EntityID entity) 
			{
				if (!IsEntityRegistered(entity))
					throw std::logic_error("Entity is not registered.");

				SparseSet<T>* componentSet = TryGetComponentSet<T>();

				if (!componentSet) return;

				componentSet->Delete(entity);
			}

			template<typename T>
			T* TryGetComponent(EntityID entity)
			{
				if (!IsEntityRegistered(entity)) return nullptr;

				SparseSet<T>* componentSet = TryGetComponentSet<T>();

				if (!componentSet) return nullptr;

				return componentSet->Get(entity);
			}

			DifferenceNode& GetDifferenceNode(IQueryNode* mainSet, IQueryNode* exclusionSet)
			{
				std::unique_ptr<DifferenceNode> difference = std::make_unique<DifferenceNode>(mainSet, exclusionSet);
				queryNodes.push_back(std::move(difference));

				return *dynamic_cast<DifferenceNode*>(queryNodes.back().get());
			}

			IntersectionNode& GetIntersectionNode(std::vector<IQueryNode*> nodes)
			{
				std::unique_ptr<IntersectionNode> intersection = std::make_unique<IntersectionNode>(nodes);
				queryNodes.push_back(std::move(intersection));

				return *dynamic_cast<IntersectionNode*>(queryNodes.back().get());
			}

			template<typename T>
			SparseSetNode<T>& GetQueryNode()
			{
				std::unordered_map<std::type_index, std::unique_ptr<IQueryNode>>::const_iterator it = componentQueryNodes.find(std::type_index(typeid(T)));

				if (it == componentQueryNodes.end())
				{
					std::unique_ptr<IQueryNode> newNode = std::make_unique<SparseSetNode<T>>(&GetComponentSet<T>());
					componentQueryNodes[std::type_index(typeid(T))] = std::move(newNode);
					it = componentQueryNodes.find(std::type_index(typeid(T)));
				}

				SparseSetNode<T>* node = dynamic_cast<SparseSetNode<T>*>(it->second.get());

				if (!node)
					throw std::logic_error("Failed to retrieve SparseSetNode of the correct type.");

				return *node;
			}

			template<typename... ComponentTypes>
			Query<ComponentTypes...> CreateQuery(IQueryNode* rootNode = nullptr)
			{
				std::tuple<SparseSet<ComponentTypes>&...> sparseSets = std::forward_as_tuple(GetComponentSet<ComponentTypes>()...);

				if (!rootNode)
				{
					std::unique_ptr<IQueryNode> intersection = std::make_unique<IntersectionNode>(std::vector<IQueryNode*>{ &GetQueryNode<ComponentTypes>()... });
					queryNodes.push_back(std::move(intersection));
					rootNode = queryNodes.back().get();
				}

				return Query<ComponentTypes...>(rootNode, std::get<SparseSet<ComponentTypes>&>(sparseSets)...);
			}
		};
	}
}