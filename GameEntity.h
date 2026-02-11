#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <memory>

class GameEntity {
   private:
    Transform transform;
    std::shared_ptr<Mesh> mesh;

   public:
    GameEntity(std::shared_ptr<Mesh> mesh);
    GameEntity(std::shared_ptr<Mesh> mesh, const Transform& copy_transform);

    std::shared_ptr<Mesh> get_mesh() const { return mesh; }
    Transform& get_transform() { return transform; }
};
