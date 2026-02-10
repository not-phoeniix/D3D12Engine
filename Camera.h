#pragma once

#include <DirectXMath.h>
#include <memory>
#include "Transform.h"
#include "Input.h"

class Camera {
   private:
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projMatrix;

    Transform transform;
    float fov;
    float moveSpeed;
    float lookSpeed;
    float nearPlaneDist;
    float farPlaneDist;

   public:
    Camera(DirectX::XMFLOAT3 pos, float moveSpeed, float lookSpeed, float fov, float aspectRatio, float nearPlaneDist, float farPlaneDist);
    ~Camera();
    Camera(const Camera& other);

    Camera& operator=(const Camera& other);

    void Update(float dt);
    void UpdateViewMatrix();
    void UpdateProjectionMatrix(float aspectRatio);

    // getters
    DirectX::XMFLOAT4X4 GetView();
    DirectX::XMFLOAT4X4 GetProjection();
    Transform& GetTransform();
    float GetFov();
    float GetNearPlaneDist();
    float GetFarPlaneDist();
};