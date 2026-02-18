#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>

class GameEntity {
   private:
    Transform transform;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;

   public:
    GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const Transform& copy_transform);

    Transform& get_transform() { return transform; }
    std::shared_ptr<Mesh> get_mesh() const { return mesh; }
    std::shared_ptr<Material> get_material() const { return material; }
    void set_material(std::shared_ptr<Material> material) { this->material = material; }
};
