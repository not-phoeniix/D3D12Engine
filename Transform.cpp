#include "Transform.h"

using namespace DirectX;

Transform::Transform(const DirectX::XMFLOAT3 position)
  : position(position),
    pitchYawRoll(0, 0, 0),
    scale(1, 1, 1),
    forward(0, 0, 1),
    right(1, 0, 0),
    up(0, 1, 0),
    parent(nullptr),
    children(),
    matricesDirty(false),
    directionalsDirty(false) {
    XMStoreFloat4x4(&world, XMMatrixIdentity());
    XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());
}

Transform::Transform()
  : position(0, 0, 0),
    pitchYawRoll(0, 0, 0),
    scale(1, 1, 1),
    forward(0, 0, 1),
    right(1, 0, 0),
    up(0, 1, 0),
    parent(nullptr),
    children(),
    matricesDirty(false),
    directionalsDirty(false) {
    XMStoreFloat4x4(&world, XMMatrixIdentity());
    XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());
}

Transform::~Transform() {
}

Transform::Transform(const Transform& other)
  : position(other.position),
    pitchYawRoll(other.pitchYawRoll),
    scale(other.scale),
    forward(other.forward),
    right(other.right),
    up(other.up),
    parent(other.parent),
    children(),
    directionalsDirty(other.directionalsDirty),
    matricesDirty(other.matricesDirty),
    world(other.world),
    worldInverseTranspose(other.worldInverseTranspose) {
    for (Transform* child : other.children) {
        children.push_back(child);
    }
}

Transform& Transform::operator=(const Transform& other) {
    position = other.position;
    pitchYawRoll = other.pitchYawRoll;
    scale = other.scale;
    forward = other.forward;
    right = other.right;
    up = other.up;
    parent = other.parent;
    directionalsDirty = other.directionalsDirty;
    matricesDirty = other.matricesDirty;
    world = other.world;
    worldInverseTranspose = other.worldInverseTranspose;

    for (Transform* child : other.children) {
        children.push_back(child);
    }

    return *this;
}

void Transform::CalculateMatrices() {
    XMMATRIX transMat = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
    XMMATRIX rotMat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
    XMMATRIX scaleMat = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

    XMMATRIX worldMat = scaleMat * rotMat * transMat;

    // recursively go back up the tree, recalculating if dirty...
    //   base case being when a transform doesn't have a parent
    if (parent != nullptr) {
        // apply matrix math of parent transform (prompting it to be recalculated as well)
        XMFLOAT4X4 parentMat = parent->GetWorldMatrix();
        worldMat *= XMLoadFloat4x4(&parentMat);
    }

    // store calculations in field matrices themselves
    XMStoreFloat4x4(&world, worldMat);
    XMStoreFloat4x4(&worldInverseTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMat)));
}

void Transform::CalculateDirectionals() {
    // rotate forward right and up all by current quaternion rotation
    XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
    XMVECTOR upRotVec = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotQuat);
    XMVECTOR forwardRotVec = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotQuat);
    XMVECTOR rightRotVec = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotQuat);

    // set values of fields
    XMStoreFloat3(&up, upRotVec);
    XMStoreFloat3(&forward, forwardRotVec);
    XMStoreFloat3(&right, rightRotVec);
}

void Transform::MarkChildrenMatricesDirty() {
    // this is recursive!! it shouldn't* misbehave if I'm not doing
    //   anything crazy with tons of children but keep in mind for the future
    for (Transform* child : children) {
        // base case is when there are no children in bottom-most class's vector
        child->matricesDirty = true;
        child->MarkChildrenMatricesDirty();
    }
}

void Transform::MarkChildrenDirectionalsDirty() {
    // this is recursive!! it shouldn't* misbehave if I'm not doing
    //   anything crazy with tons of children but keep in mind for the future
    for (Transform* child : children) {
        // base case is when there are no children in bottom-most class's vector
        child->directionalsDirty = true;
        child->MarkChildrenDirectionalsDirty();
    }
}

void Transform::SetParent(Transform* parent) {
    this->parent = parent;
    if (parent != nullptr) {
        this->parent->AddChild(this);
    }
}

void Transform::AddChild(Transform* child) {
    if (child != nullptr) {
        children.push_back(child);
        child->parent = this;
    }
}

void Transform::RemoveChild(Transform* child) {
    // find index of inputted child and erase it from vector
    children.erase(std::find(children.begin(), children.end(), child));
    child->parent = nullptr;
}

void Transform::ClearChildren() {
    for (Transform* child : children) {
        child->parent = nullptr;
    }

    children.clear();
}

Transform* Transform::GetChild(int index) {
    return children[index];
}

unsigned int Transform::GetNumChildren() {
    return (unsigned int)children.size();
}

void Transform::SetPosition(float x, float y, float z) {
    position.x = x;
    position.y = y;
    position.z = z;
    matricesDirty = true;
    MarkChildrenMatricesDirty();
}

void Transform::SetPosition(DirectX::XMFLOAT3 pos) {
    SetPosition(pos.x, pos.y, pos.z);
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
    pitchYawRoll.x = pitch;
    pitchYawRoll.y = yaw;
    pitchYawRoll.z = roll;

    matricesDirty = true;
    MarkChildrenMatricesDirty();

    directionalsDirty = true;
    MarkChildrenDirectionalsDirty();
}

void Transform::SetRotation(DirectX::XMFLOAT3 rot) {
    SetRotation(rot.x, rot.y, rot.z);
}

void Transform::SetScale(float x, float y, float z) {
    scale.x = x;
    scale.y = y;
    scale.z = z;
    matricesDirty = true;
    MarkChildrenMatricesDirty();
}

void Transform::SetScale(float value) {
    SetScale(value, value, value);
}

void Transform::SetScale(DirectX::XMFLOAT3 scale) {
    SetScale(scale.x, scale.y, scale.z);
}

void Transform::MoveAbsolute(float x, float y, float z) {
    // fast CPU math functions for slight efficiency increase
    XMStoreFloat3(
        &position,
        XMLoadFloat3(&position) + XMVectorSet(x, y, z, 0)
    );
    matricesDirty = true;
    MarkChildrenMatricesDirty();
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 posOffset) {
    MoveAbsolute(posOffset.x, posOffset.y, posOffset.z);
}

void Transform::MoveRelative(float x, float y, float z) {
    // rotate inputted vector by current rotation
    XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
    XMVECTOR rotatedInput = XMVector3Rotate(XMVectorSet(x, y, z, 0), rotQuat);

    // add new offset to the current position
    XMStoreFloat3(&position, XMLoadFloat3(&position) + rotatedInput);

    // mark this and all children dirty
    matricesDirty = true;
    MarkChildrenMatricesDirty();
}

void Transform::MoveRelative(DirectX::XMFLOAT3 posOffset) {
    MoveRelative(posOffset.x, posOffset.y, posOffset.z);
}

void Transform::Rotate(float pitch, float yaw, float roll) {
    XMStoreFloat3(
        &pitchYawRoll,
        XMLoadFloat3(&pitchYawRoll) + XMVectorSet(pitch, yaw, roll, 0)
    );

    matricesDirty = true;
    MarkChildrenMatricesDirty();

    directionalsDirty = true;
    MarkChildrenDirectionalsDirty();
}

void Transform::Rotate(DirectX::XMFLOAT3 rotOffset) {
    Rotate(rotOffset.x, rotOffset.y, rotOffset.z);
}

void Transform::Scale(float x, float y, float z) {
    XMStoreFloat3(
        &scale,
        XMLoadFloat3(&scale) * XMVectorSet(x, y, z, 1)
    );
    matricesDirty = true;
    MarkChildrenMatricesDirty();
}

void Transform::Scale(float value) {
    Scale(value, value, value);
}

void Transform::Scale(DirectX::XMFLOAT3 scaleOffset) {
    Scale(scaleOffset.x, scaleOffset.y, scaleOffset.z);
}

DirectX::XMFLOAT3 Transform::GetPosition() {
    return position;
}

DirectX::XMFLOAT3 Transform::GetRotation() {
    return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale() {
    return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix() {
    if (matricesDirty) {
        CalculateMatrices();
        matricesDirty = false;
    }

    return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix() {
    if (matricesDirty) {
        CalculateMatrices();
        matricesDirty = false;
    }

    return worldInverseTranspose;
}

DirectX::XMFLOAT3 Transform::GetForward() {
    if (directionalsDirty) {
        CalculateDirectionals();
        directionalsDirty = false;
    }

    return forward;
}

DirectX::XMFLOAT3 Transform::GetRight() {
    if (directionalsDirty) {
        CalculateDirectionals();
        directionalsDirty = false;
    }

    return right;
}

DirectX::XMFLOAT3 Transform::GetUp() {
    if (directionalsDirty) {
        CalculateDirectionals();
        directionalsDirty = false;
    }

    return up;
}

// TODO: ImGui implementation
// void Transform::BuildUI() {
//     ImGui::DragFloat3("Position", &position.x, 0.05f, -100, 100);
//     ImGui::DragFloat3("PitchYawRoll (radians)", &pitchYawRoll.x, 0.01f, -20, 20);
//     ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0, 5);
//     matricesDirty = true;
//     directionalsDirty = true;
// }
