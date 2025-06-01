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

	Archetype* archetype = entityToArchetype[entity];

	for (std::type_index componentType : archetype->GetComponentTypes())
	{
		archetype->DestroyComponent(entity, componentType);
	}

	archetype->RemoveEntity(entity);
	entityToArchetype.erase(entity);
	availableEntityIDs.insert(entity);
}

bool Weave::ECS::World::IsEntityRegistered(Weave::ECS::EntityID entity) const
{
	if (entity > nextEntityID) return false;
	if (availableEntityIDs.find(entity) != availableEntityIDs.end()) return false;

	return true;
}