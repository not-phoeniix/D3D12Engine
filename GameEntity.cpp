#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh)
  : mesh(mesh) { }

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, const Transform& copy_transform)
  : transform(copy_transform),
    mesh(mesh) { }
