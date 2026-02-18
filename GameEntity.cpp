#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
  : mesh(mesh),
    material(material) { }

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const Transform& copy_transform)
  : transform(copy_transform),
    mesh(mesh),
    material(material) { }
