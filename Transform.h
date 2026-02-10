#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>

class Transform {
   private:
    // separate transform data we can build matrix from later
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 pitchYawRoll;
    DirectX::XMFLOAT3 scale;

    // matrices themselves
    bool matricesDirty;
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 worldInverseTranspose;

    // references for parent/child system
    //! note: references here are just pointers, memory must be
    //!   managed and cleaned up manually outside class
    Transform* parent;
    std::vector<Transform*> children;

    // local directional vectors
    bool directionalsDirty;
    DirectX::XMFLOAT3 forward;
    DirectX::XMFLOAT3 right;
    DirectX::XMFLOAT3 up;

    void CalculateMatrices();
    void CalculateDirectionals();
    void MarkChildrenMatricesDirty();
    void MarkChildrenDirectionalsDirty();

   public:
    Transform();
    ~Transform();
    Transform(const Transform& other);
    Transform& operator=(const Transform& other);

    // setters
    void SetPosition(float x, float y, float z);
    void SetPosition(DirectX::XMFLOAT3 pos);
    void SetRotation(float pitch, float yaw, float roll);
    void SetRotation(DirectX::XMFLOAT3 rot);
    void SetScale(float x, float y, float z);
    void SetScale(float value);
    void SetScale(DirectX::XMFLOAT3 scale);

    // parent/child management
    void SetParent(Transform* parent);
    void AddChild(Transform* child);
    void RemoveChild(Transform* child);
    void ClearChildren();
    Transform* GetChild(int index);
    unsigned int GetNumChildren();

    // transformers (robots in disguise)
    void MoveAbsolute(float x, float y, float z);
    void MoveAbsolute(DirectX::XMFLOAT3 posOffset);
    void MoveRelative(float x, float y, float z);
    void MoveRelative(DirectX::XMFLOAT3 posOffset);
    void Rotate(float pitch, float yaw, float roll);
    void Rotate(DirectX::XMFLOAT3 rotOffset);
    void Scale(float x, float y, float z);
    void Scale(float value);
    void Scale(DirectX::XMFLOAT3 scaleOffset);

    // getters
    DirectX::XMFLOAT3 GetPosition();
    DirectX::XMFLOAT3 GetRotation();
    DirectX::XMFLOAT3 GetScale();
    DirectX::XMFLOAT4X4 GetWorldMatrix();
    DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
    DirectX::XMFLOAT3 GetForward();
    DirectX::XMFLOAT3 GetRight();
    DirectX::XMFLOAT3 GetUp();

    void BuildUI();
};
