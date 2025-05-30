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
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(EntityID, Components&...)> systemFn, float priority = 0.0f, int targetThreads = -1)
			{
				std::function<void(World&)> wrapper;

				if (targetThreads == 0)
				{
					wrapper = [systemFn](World& world)
						{
							WorldView<Components...> view = world.GetView<Components...>();

							size_t count = view.GetEntityCount();
							if (count == 0) return;

							for (auto entity : view)
							{
								std::apply(systemFn, entity);
							}
						};
				}
				else
				{
					wrapper = [systemFn, targetThreads](World& world)
						{
							WorldView<Components...> view = world.GetView<Components...>();
							size_t count = view.GetEntityCount();
							if (count == 0) return;

							size_t threadCount = targetThreads > 0 ? targetThreads :
								std::thread::hardware_concurrency();
							threadCount = threadCount > 0 ? threadCount : 4;

							threadCount = std::min(threadCount, count);

							size_t chunkSize = (count + threadCount - 1) / threadCount;

							std::vector<std::thread> threads;
							threads.reserve(threadCount);

							for (size_t t = 0; t < threadCount; t++) 
							{
								size_t start = t * chunkSize;
								size_t end = std::min(start + chunkSize, count);

								threads.emplace_back([&view, start, end, systemFn]() 
									{
									auto it = view.at(start);
									auto endIt = view.at(end);

									while (it != endIt) 
									{
										std::apply(systemFn, *it);
										++it;
									}
									});
							}

							for (auto& thread : threads) 
							{
								thread.join();
							}
						};
				}

				return RegisterSystem(groupID, wrapper, priority);
			}
		};
	}
}