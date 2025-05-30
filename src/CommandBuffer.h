#pragma once
#include <functional>
#include <vector>
#include <mutex>
#include "World.h"

namespace Weave
{
    namespace ECS
    {
        class CommandBuffer 
        {
        public:
            template<typename Fn, typename... Args>
            void AddCommand(Fn&& fn, Args&&... args) 
            {
                std::lock_guard<std::mutex> lock(mutex_);
                commands_.emplace_back(
                    [fn = std::forward<Fn>(fn), ...args = std::forward<Args>(args)](World& world) mutable
                    {
                        fn(world, std::forward<Args>(args)...);
                    }
                );
            }

            void Flush(World& world) {
                std::vector<std::function<void(World&)>> commands;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    commands.swap(commands_);
                }
                for (auto& cmd : commands) {
                    cmd(world);
                }
            }

        private:
            std::mutex mutex_;
            std::vector<std::function<void(World&)>> commands_;
        };
    }
}