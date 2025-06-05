#pragma once
#include <map>
#include <vector>
#include <concepts>
#include "ECS.h"
#include "ThreadPool.h"
#include "CommandBuffer.h"

namespace Weave
{
	namespace ECS
	{
		template<typename F, typename... Components>
		concept SystemFunction = requires(F && f, EntityID id, Components&... components) {
			{ f(id, components...) } -> std::same_as<void>;
		};

		// Concept for systems with CommandBuffer
		template<typename F, typename... Components>
		concept SystemFunctionWithCommandBuffer = requires(F && f, EntityID id, Components&... components, CommandBuffer & cmd) {
			{ f(id, components..., cmd) } -> std::same_as<void>;
		};

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

			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(World&, CommandBuffer&)> systemFn, float priority = 0.0f);
			SystemID RegisterSystem(SystemGroupID groupID, std::function<void(World&)> systemFn, float priority = 0.0f);

			template<typename... Components, SystemFunctionWithCommandBuffer<Components...> F>
			SystemID RegisterSystem(SystemGroupID groupID, F&& systemFn, float priority = 0.0f) 
			{
				std::function<void(World&)> wrapper = [this, systemFn](World& world)
					{
						WorldView<Components...> view = world.GetView<Components...>();
						size_t count = view.GetEntityCount();
						if (count == 0) return;

						for (auto entity : view)
						{
							std::apply([&systemFn, this](auto&&... args) {
								systemFn(args..., this->commandBuffer);
								}, entity);
						}
					};

				return RegisterSystem(groupID, wrapper, priority);
			}

			template<typename... Components, SystemFunction<Components...> F>
			SystemID RegisterSystem(SystemGroupID groupID, F&& systemFn, float priority = 0.0f) 
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

			template<typename... Components, SystemFunctionWithCommandBuffer<Components...> F>
			SystemID RegisterSystemThreaded(SystemGroupID groupID, F&& systemFn, float priority = 0.0f)
			{
				std::function<void(World&)> wrapper = [this, systemFn](World& world)
					{
						WorldView<Components...> view = world.GetView<Components...>();
						size_t count = view.GetEntityCount();
						if (count == 0) return;

						size_t threads = this->threadPool->GetThreadCount();

						if (count < threads)
						{
							for (auto entity : view)
							{
								std::apply([&systemFn, this](auto&&... args) {
									systemFn(args..., this->commandBuffer);
									}, entity);
							}
						}

						size_t chunkSize = (count + threads - 1) / threads;
						CommandBuffer& cmdBuffer = this->commandBuffer;

						for (size_t t = 0; t < threads; t++) {
							size_t start = t * chunkSize;
							size_t end = std::min(start + chunkSize, count - 1);

							this->threadPool->Enqueue([&view, start, end, &systemFn, &cmdBuffer]() {
								auto it = view.at(start);
								auto endIt = view.at(end);

								while (it != endIt) {
									std::apply([&systemFn, &cmdBuffer](auto&&... args) { systemFn(args..., cmdBuffer); }, *it);
									++it;
								}
								});
						}

						this->threadPool->WaitAll();
					};

				return RegisterSystem(groupID, wrapper, priority);
			}

			template<typename... Components, SystemFunction<Components...> F>
            SystemID RegisterSystemThreaded(SystemGroupID groupID, F&& systemFn, float priority = 0.0f)
            {
				std::function<void(World&)> wrapper = [this, systemFn](World& world)
						{
							WorldView<Components...> view = world.GetView<Components...>();
							size_t count = view.GetEntityCount();
							if (count == 0) return;

							size_t threads = this->threadPool->GetThreadCount();

							if (count < threads)
							{
								for (auto entity : view)
								{
									std::apply(systemFn, entity);
								}
							}

							size_t chunkSize = (count + threads - 1) / threads;

							for (size_t t = 0; t < threads; t++) {
								size_t start = t * chunkSize;
								size_t end = std::min(start + chunkSize, count - 1);

								this->threadPool->Enqueue([&view, start, end, &systemFn]() {
									auto it = view.at(start);
									auto endIt = view.at(end);

									while (it != endIt) {
										std::apply(systemFn, *it);
										++it;
									}
									});
							}

							this->threadPool->WaitAll();
						};

                return RegisterSystem(groupID, wrapper, priority);
            }
		};
	}
}