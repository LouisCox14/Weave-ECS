#pragma once
#include "World.h"

Weave::ECS::EntityID Weave::ECS::SparseSetWorld::CreateEntity()
{
	if (!availableEntityIDs.empty())
		return availableEntityIDs.extract(availableEntityIDs.begin()).value();

	return nextEntityID++;
}

void Weave::ECS::SparseSetWorld::DeleteEntity(EntityID entity)
{
	if (!IsEntityRegistered(entity))
		throw std::logic_error("Entity is not registered.");

	for (std::pair<const std::type_index, std::unique_ptr<ISparseSet>>& pair : componentStorage)
	{
		pair.second->Delete(entity);
	}

	availableEntityIDs.insert(entity);
}

bool Weave::ECS::SparseSetWorld::IsEntityRegistered(Weave::ECS::EntityID entity) const
{
	if (entity > nextEntityID) return false;
	if (availableEntityIDs.find(entity) != availableEntityIDs.end()) return false;

	return true;
}