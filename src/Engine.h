#pragma once
#include <map>
#include <vector>
#include "ECS.h"
#include "ThreadPool.h"
#include "CommandBuffer.h"

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

            std::unique_ptr<Utilities::ThreadPool> threadPool;
			CommandBuffer commandBuffer;

		public:
            Engine(uint8_t threadCount = std::thread::hardware_concurrency());

			World& GetWorld();

			SystemGroupID CreateSystemGroup();
			void RetireSystemGroup(SystemGroupID targetGroup);
			void CallSystemGroup(SystemGroupID targetGroup);

			void RetireSystem(SystemID targetSystem);
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(World&)> systemFn, float priority = 0.0f);

			template<typename... Components>
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(EntityID, Components&...)> systemFn, float priority = 0.0f)
			{
				std::function<void(World&)> wrapper = [systemFn](World& world)
						{
							WorldView<Components...> view = world.GetView<Components...>();

							size_t count = view.GetEntityCount();
							if (count == 0) return;

							for (auto entity : view)
							{
								std::apply(systemFn, entity);
							}
						};

				return RegisterSystem(groupID, wrapper, priority);
			}

			template<typename... Components>
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(EntityID, Components&..., CommandBuffer&)> systemFn, float priority = 0.0f)
			{
				std::function<void(World&)> wrapper = [systemFn](World& world)
					{
						WorldView<Components...> view = world.GetView<Components...>();

						size_t count = view.GetEntityCount();
						if (count == 0) return;

						for (auto entity : view)
						{
							std::apply([&](auto&&... args) { systemFn(args..., commandBuffer); }, entity);
						}
					};

				return RegisterSystem(groupID, wrapper, priority);
			}

			template<typename... Components>
			SystemID RegisterSystemThreaded(SystemGroupID groupID, std::function<void(EntityID, Components&..., CommandBuffer&)> systemFn, float priority = 0.0f)
			{
				std::function<void(World&)> wrapper = [systemFn](World& world)
					{
						WorldView<Components...> view = world.GetView<Components...>();
						size_t count = view.GetEntityCount();
						if (count == 0) return;

						size_t chunkSize = (count + threadPool->GetThreadCount() - 1) / threadPool->GetThreadCount();

						for (size_t t = 0; t < threadPool->GetThreadCount(); t++) {
							size_t start = t * chunkSize;
							size_t end = std::min(start + chunkSize, count);

							threadPool->Enqueue([&view, start, end, systemFn]() {
								auto it = view.at(start);
								auto endIt = view.at(end);

								while (it != endIt) {
									std::apply([&](auto&&... args) { systemFn(args..., commandBuffer); }, *it);
									++it;
								}
								});
						}

						threadPool->WaitAll();
					};

				return RegisterSystem(groupID, wrapper, priority);
			}

            template<typename... Components>
            SystemID RegisterSystemThreaded(SystemGroupID groupID, std::function<void(EntityID, Components&...)> systemFn, float priority = 0.0f)
            {
				std::function<void(World&)> wrapper = [systemFn](World& world)
						{
							WorldView<Components...> view = world.GetView<Components...>();
							size_t count = view.GetEntityCount();
							if (count == 0) return;

							size_t chunkSize = (count + threadPool->GetThreadCount() - 1) / threadPool->GetThreadCount();

							for (size_t t = 0; t < threadPool->GetThreadCount(); t++) {
								size_t start = t * chunkSize;
								size_t end = std::min(start + chunkSize, count);

								threadPool->Enqueue([&view, start, end, systemFn]() {
									auto it = view.at(start);
									auto endIt = view.at(end);

									while (it != endIt) {
										std::apply(systemFn, *it);
										++it;
									}
									});
							}

							threadPool->WaitAll();
						};

                return RegisterSystem(groupID, wrapper, priority);
            }
		};
	}
}