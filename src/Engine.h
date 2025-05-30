#pragma once
#include <map>
#include <vector>
#include "ECS.h"
#include "ThreadPool.h"

namespace Weave
{
	namespace ECS
	{
		using SystemGroupID = size_t;
		using SystemID = size_t;

		class Engine
		{
		private:
			struct System
			{
				std::function<void(World&)> executor;

				SystemID id;
				float priority;
			};

			struct SystemGroup 
			{
				std::vector<System> systems;
				bool dirty = false;
			};

			World world;

			std::map<SystemGroupID, SystemGroup> systemGroups;
			std::unordered_map<SystemID, SystemGroupID> systemToGroup;

			SystemGroupID nextSystemGroupID = 0;
			SystemID nextSystemID = 0;

		public:
			World& GetWorld();

			SystemGroupID CreateSystemGroup();
			void RetireSystemGroup(SystemGroupID targetGroup);
			void CallSystemGroup(SystemGroupID targetGroup);

			void RetireSystem(SystemID targetSystem);
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(World&)> systemFn, float priority = 0.0f);

			template<typename... Components>
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(EntityID, Components&...)> systemFn, float priority = 0.0f, uint8_t targetThreads = 0)
			{
				auto wrapper = [systemFn, targetThreads](World& world)
					{
					WorldView<Components...> view = world.GetView<Components...>();

					size_t count = view.GetEntityCount();
					if (count == 0) return;

					size_t threadCount;
					if (targetThreads != 0) threadCount = targetThreads;
					else threadCount = std::thread::hardware_concurrency();

					Weave::Utilities::ThreadPool pool(threadCount > 0 ? threadCount : 4);

					for (auto entity : view)
					{
						pool.Enqueue([entity, systemFn]() mutable {
							std::apply(systemFn, entity);
							});
					}
					};

				return RegisterSystem(groupID, wrapper, priority);
			}
		};
	}
}