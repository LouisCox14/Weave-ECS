#pragma once
#include "World.h"

Weave::ECS::EntityID Weave::ECS::World::CreateEntity()
{
	if (!availableEntityIDs.empty())
		return availableEntityIDs.extract(availableEntityIDs.begin()).value();

	return nextEntityID++;
}

void Weave::ECS::World::DeleteEntity(EntityID entity)
{
	if (!IsEntityRegistered(entity))
		throw std::logic_error("Entity is not registered.");

	for (auto& pair : archetypes)
	{
		pair.second.get()->RemoveEntity(entity);
	}

	entityToArchetype.erase(entity);
	availableEntityIDs.insert(entity);
}

bool Weave::ECS::World::IsEntityRegistered(Weave::ECS::EntityID entity) const
{
	if (entity > nextEntityID) return false;
	if (availableEntityIDs.find(entity) != availableEntityIDs.end()) return false;

	return true;
}