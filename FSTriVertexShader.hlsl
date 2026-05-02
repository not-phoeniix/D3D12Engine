#include "IOStructs.hlsli"

PostProcessIn main(uint id : SV_VertexID) {
    PostProcessIn output;

    output.uv = float2(
        (id << 1) & 2,  // essentially: id % 2 * 2,
        id & 2
    );

    // beeg triangle
    output.position = float4(output.uv, 0, 1);
    output.position.x = output.position.x * 2 - 1;
    output.position.y = output.position.y * -2 + 1;

    return output;
}
