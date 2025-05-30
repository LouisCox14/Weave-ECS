#pragma once
#include "Engine.h"

Weave::ECS::Engine::Engine(uint8_t threadCount) : threadPool(std::make_unique<Utilities::ThreadPool>(threadCount)) { }

Weave::ECS::World& Weave::ECS::Engine::GetWorld()
{
    return world;
}

Weave::ECS::SystemGroupID Weave::ECS::Engine::CreateSystemGroup()
{
	return nextSystemGroupID++;
}

void Weave::ECS::Engine::RetireSystemGroup(SystemGroupID targetGroup)
{
    std::map<SystemGroupID, SystemGroup>::iterator it = systemGroups.find(targetGroup);
    if (it == systemGroups.end()) return;

    for (const System& system : it->second.systems)
    {
        systemToGroup.erase(system.id);
    }

    systemGroups.erase(it);
}

void Weave::ECS::Engine::CallSystemGroup(SystemGroupID targetGroup)
{
    std::map<SystemGroupID, SystemGroup>::iterator it = systemGroups.find(targetGroup);
    if (it == systemGroups.end()) return;

    SystemGroup& group = it->second;
    if (group.dirty)
    {
        std::sort(group.systems.begin(), group.systems.end(), [](const auto& a, const auto& b)
            {
                return a.priority > b.priority;
            });
    }

    for (const System& system : group.systems)
    {
        system.executor(world);
    }
}

void Weave::ECS::Engine::RetireSystem(SystemID targetSystem)
{
    std::unordered_map<SystemID, SystemGroupID>::iterator mapIt = systemToGroup.find(targetSystem);
    if (mapIt == systemToGroup.end()) return;

    SystemGroupID groupID = mapIt->second;
    SystemGroup& group = systemGroups[groupID];

    auto it = std::remove_if(group.systems.begin(), group.systems.end(), [groupID](const System& entry) { return entry.id == groupID; });

    if (it != group.systems.end()) 
    {
        group.systems.erase(it, group.systems.end());
        group.dirty = true;
        systemToGroup.erase(mapIt);
    }
}


Weave::ECS::SystemID Weave::ECS::Engine::RegisterSystem(SystemGroupID groupID, std::function<void(World&)> systemFn, float priority)
{
    SystemID id = nextSystemID++;

    systemGroups[groupID].systems.push_back({ std::move(systemFn), id, priority });
    systemGroups[groupID].dirty = true;
    systemToGroup[id] = groupID;

    return id;
}
