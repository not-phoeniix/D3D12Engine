#define M_PI 3.14159f
#define RAYS_PER_PIXEL 50
#define MAX_RECUSION_DEPTH 20

// === Structs ===

// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition;
    float2 uv;
    float3 normal;
    float3 tangent;
};

// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible, and must match our C++ size definition
struct RayPayload
{
	float3 color;
	uint recusion_depth;
	uint iteration_index;
};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes struct
// for triangle attributes, so no need to define our own.  It contains a single float2.

struct SceneData {
	matrix InverseViewProjection;
	float3 CameraPos;
	float pad;
};

struct EntityData {
	float4 Color;
	uint VertexBufferDescriptorIndex;
	uint IndexBufferDescriptorIndex;
	bool Refractive;
	float RefractionIndex;
};

// === Constant buffers ===

cbuffer DrawData : register(b0)
{
	uint SceneDataConstantBufferIndex;
	uint EntityDataDescriptorIndex;
	uint SceneTLASDescriptorIndex;
	uint OutputUAVDescriptorIndex;
};

// === Helpers ===

// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float2 barycentrics)
{
	StructuredBuffer<EntityData> ed = ResourceDescriptorHeap[EntityDataDescriptorIndex];
	EntityData thisEntity = ed[InstanceIndex()];

	StructuredBuffer<uint> IndexBuffer = ResourceDescriptorHeap[thisEntity.IndexBufferDescriptorIndex];
	StructuredBuffer<Vertex> VertexBuffer = ResourceDescriptorHeap[thisEntity.VertexBufferDescriptorIndex];

	// Grab the 3 indices for this triangle
	uint firstIndex = triangleIndex * 3;
	uint indices[3];
	indices[0] = IndexBuffer[firstIndex + 0];
	indices[1] = IndexBuffer[firstIndex + 1];
	indices[2] = IndexBuffer[firstIndex + 2];

	// Grab the 3 corresponding vertices
	Vertex verts[3];
	verts[0] = VertexBuffer[indices[0]];
	verts[1] = VertexBuffer[indices[1]];
	verts[2] = VertexBuffer[indices[2]];
	
	// Calculate the barycentric data for vertex interpolation
	float3 barycentricData = float3(
		1.0f - barycentrics.x - barycentrics.y,
		barycentrics.x,
		barycentrics.y);
	
	// Loop through the barycentric data and interpolate
    Vertex finalVert = (Vertex)0;
	for (uint i = 0; i < 3; i++)
	{
		finalVert.localPosition += verts[i].localPosition * barycentricData[i];
		finalVert.uv += verts[i].uv * barycentricData[i];
		finalVert.normal += verts[i].normal * barycentricData[i];
		finalVert.tangent += verts[i].tangent * barycentricData[i];
	}
	return finalVert;
}


// Calculates an origin and direction from the camera for specific pixel indices
RayDesc CalcRayFromCamera(float2 rayIndices, float3 camPos, float4x4 invVP)
{
	// Offset to the middle of the pixel
	float2 pixel = rayIndices + 0.5f;
	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(invVP, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the ray
	RayDesc ray;
	ray.Origin = camPos.xyz;
	ray.Direction = normalize(worldPos.xyz - ray.Origin);
	ray.TMin = 0.1f;
	ray.TMax = 1000.0f;
	return ray;
}

float rand(float2 uv) {
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float2 rand2(float2 uv) {
	return float2(
		rand(uv),
		rand(uv.yx)
	);
}

float3 length_sq(float3 v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

float3 RandomCosWeightedHemisphere(float u0, float u1, float3 unitNormal) {
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * M_PI * u1;

	return float3(
		unitNormal.x + b * cos(phi),
		unitNormal.y + b * sin(phi),
		unitNormal.z + a
	);
}

float GetReflectance(float cos, float refract_index) {
	// schlick's approx
	float r0 = (1.0f - refract_index) / (1.0f + refract_index);
	r0 *= r0;
	return r0 + (1.0f - r0) - pow((1.0f - cos), 5.0f);
}

// === Shaders ===

// Ray generation shader - Launched once for each ray we want to generate
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
	ConstantBuffer<SceneData> cb = ResourceDescriptorHeap[SceneDataConstantBufferIndex];

	// Get the ray indices
	uint2 rayIndices = DispatchRaysIndex().xy;

	float3 totalColor = float3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < RAYS_PER_PIXEL; i++) {
		float2 adjustedIndices = (float2)rayIndices;
		float ray1 = float(i) / RAYS_PER_PIXEL;
		adjustedIndices += rand2(rayIndices.xy * ray1);

		// Calculate the ray from the camera through a particular
		// pixel of the output buffer using this shader's indices
		RayDesc ray = CalcRayFromCamera(
			adjustedIndices, 
			cb.CameraPos,
			cb.InverseViewProjection);

		RayPayload payload = (RayPayload)0;
		payload.color = float3(1.0f, 1.0f, 1.0f);
		payload.recusion_depth = 0;
		payload.iteration_index = i;

		// Perform the ray trace for this ray
    	RaytracingAccelerationStructure SceneTLAS = ResourceDescriptorHeap[SceneTLASDescriptorIndex];
		TraceRay(
			SceneTLAS,
			RAY_FLAG_NONE,
			0xFF,
			0,
			0,
			0,
			ray,
			payload
		);

		totalColor += payload.color;
	}

	totalColor /= float(RAYS_PER_PIXEL);

	// Set the final color of the buffer (gama corrected)
	RWTexture2D <float4> OutputColor = ResourceDescriptorHeap[OutputUAVDescriptorIndex];
	OutputColor[rayIndices] = float4(pow(totalColor, 1.0f / 2.2f), 1);
}


// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color *= float3(0.3f, 0.5f, 0.75f);
}


// Closest hit shader - Runs the first time a ray hits anything
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	if (payload.recusion_depth == MAX_RECUSION_DEPTH) {
		payload.color = float3(0, 0, 0);
		return;
	}

	StructuredBuffer<EntityData> entityDataBuffer = ResourceDescriptorHeap[EntityDataDescriptorIndex];
	EntityData thisEntity = entityDataBuffer[InstanceIndex()];	
	

	// Get the interpolated vertex data
	Vertex vert = InterpolateVertices(
		PrimitiveIndex(), 
		hitAttributes.barycentrics
	);

	float3 normal = normalize(mul(vert.normal, (float3x3)ObjectToWorld4x3()));
	float3 dir;

	// getting rng seed for this particular pixel
	float2 uv = float2(DispatchRaysIndex().xy / DispatchRaysDimensions().xy);
	float2 rand_seed = uv * ((payload.recusion_depth + 1) + payload.iteration_index + RayTCurrent());

	if (!thisEntity.Refractive) {
		payload.color *= thisEntity.Color.rgb;

		float2 rng = rand2(rand_seed);

		// scatter rays in diff directions based on roughness, 
		//   lerp between the two values using roughness (alpha channel)
		float3 refl = reflect(WorldRayDirection(), normal);
		float3 random_bounce = RandomCosWeightedHemisphere(rand(rng), rand(rng.yx), normal);
		dir = normalize(lerp(refl, random_bounce, thisEntity.Color.a));
	} else {
		// we don't alter the color at all, just change the dir

		float cos_theta = min(dot(-WorldRayDirection(), normal), 1.0f);
		float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
	
		bool cannot_refract = thisEntity.RefractionIndex * sin_theta > 1.0f;
		if (cannot_refract || GetReflectance(cos_theta, thisEntity.RefractionIndex) > rand(rand_seed)) {
			// reflect
			dir = reflect(WorldRayDirection(), normal);
		} else {
			// refract
			float cos_theta = min(dot(-WorldRayDirection(), normal), 1);
			float3 r_out_perp = thisEntity.RefractionIndex * (WorldRayDirection() + (normal * cos_theta));
			float3 r_out_parallel = -sqrt(abs(1 - length_sq(r_out_perp))) * normal;
			dir = r_out_perp + r_out_parallel;
		}		
	}

	RayDesc ray;
	ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	ray.Direction = dir;
	ray.TMin = 0.1f;
	ray.TMax = 1000.0f;

	payload.recusion_depth++;

    RaytracingAccelerationStructure SceneTLAS = ResourceDescriptorHeap[SceneTLASDescriptorIndex];
	TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		ray,
		payload
	);
}
