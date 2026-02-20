#pragma once

#include <DirectXMath.h>

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light {
    uint32_t type;
    DirectX::XMFLOAT3 direction;
    float range;
    DirectX::XMFLOAT3 position;
    float intensity;
    DirectX::XMFLOAT3 color;
    float spot_inner_angle;
    float spot_outer_angle;
    DirectX::XMFLOAT2 padding;
};
