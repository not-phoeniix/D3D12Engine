#include "Camera.h"

using namespace DirectX;

Camera::Camera(
    DirectX::XMFLOAT3 pos,
    float moveSpeed,
    float lookSpeed,
    float fov,
    float aspectRatio,
    float nearPlaneDist,
    float farPlaneDist
) : fov(fov),
    moveSpeed(moveSpeed),
    lookSpeed(lookSpeed),
    nearPlaneDist(nearPlaneDist),
    farPlaneDist(farPlaneDist),
    transform() {
    transform.SetPosition(pos);
    UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera() {
}

Camera::Camera(const Camera& other)
  : transform(other.transform),
    fov(other.fov),
    moveSpeed(other.moveSpeed),
    lookSpeed(other.lookSpeed),
    nearPlaneDist(other.nearPlaneDist),
    farPlaneDist(other.farPlaneDist),
    viewMatrix(other.viewMatrix),
    projMatrix(other.projMatrix) {
}

Camera& Camera::operator=(const Camera& other) {
    transform = other.transform;
    fov = other.fov;
    moveSpeed = other.moveSpeed;
    lookSpeed = other.lookSpeed;
    nearPlaneDist = other.nearPlaneDist;
    farPlaneDist = other.farPlaneDist;
    viewMatrix = other.viewMatrix;
    projMatrix = other.projMatrix;

    return *this;
}

void Camera::Update(float dt) {
    XMFLOAT3 movement = XMFLOAT3(0, 0, 0);

    float moveScale = Input::KeyDown(VK_LCONTROL) ? 0.3f : 1.0f;

    // relative WASD movement
    if (Input::KeyDown('W')) transform.MoveRelative(0, 0, moveSpeed * moveScale * dt);
    if (Input::KeyDown('S')) transform.MoveRelative(0, 0, -moveSpeed * moveScale * dt);
    if (Input::KeyDown('D')) transform.MoveRelative(moveSpeed * moveScale * dt, 0, 0);
    if (Input::KeyDown('A')) transform.MoveRelative(-moveSpeed * moveScale * dt, 0, 0);

    // absolute up/down movement
    if (Input::KeyDown('E')) transform.MoveAbsolute(0, moveSpeed * moveScale * dt, 0);
    if (Input::KeyDown('Q')) transform.MoveAbsolute(0, -moveSpeed * moveScale * dt, 0);

    // mouse looking movement, only applies when lmb or rmb
    //   is clicked and imgui is not being interacted with
    bool mouseDown = Input::MouseLeftDown() || Input::MouseRightDown();
    // TODO: restore ImGui functionality
    // if (mouseDown && !ImGui::GetIO().WantCaptureMouse) {
    if (mouseDown){
        // apply rotation in delta x/y for mouse movement
        int dX = Input::GetMouseXDelta();
        int dY = Input::GetMouseYDelta();
        transform.Rotate(dY * lookSpeed, dX * lookSpeed, 0);

        // grab current rotation from transform
        XMFLOAT3 pitchYawRoll = transform.GetRotation();

        // clamp pitch to prevent backflips <3
        float pitchOffset = 0.01f;
        if (pitchYawRoll.x >= XM_PIDIV2 - pitchOffset)
            pitchYawRoll.x = XM_PIDIV2 - pitchOffset;
        if (pitchYawRoll.x <= -XM_PIDIV2 + pitchOffset)
            pitchYawRoll.x = -XM_PIDIV2 + pitchOffset;

        // re-set rotation in transform
        transform.SetRotation(pitchYawRoll);
    }

    // update matrix after all transform updating has occured
    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix() {
    XMFLOAT3 pos = transform.GetPosition();
    XMFLOAT3 forward = transform.GetForward();

    XMMATRIX view = XMMatrixLookToLH(
        XMLoadFloat3(&pos),
        XMLoadFloat3(&forward),
        XMVectorSet(0, 1, 0, 0)
    );
    XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio) {
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        fov,           // FOV in radians
        aspectRatio,   // aspect ratio
        nearPlaneDist, // near plane
        farPlaneDist   // far plane
    );
    XMStoreFloat4x4(&projMatrix, proj);
}

DirectX::XMFLOAT4X4 Camera::GetView() {
    return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjection() {
    return projMatrix;
}

Transform& Camera::GetTransform() {
    return transform;
}

float Camera::GetFov() {
    return fov;
}

float Camera::GetNearPlaneDist() {
    return nearPlaneDist;
}

float Camera::GetFarPlaneDist() {
    return farPlaneDist;
}