struct VertexInput {
	float3 color;
	float3 position;
};

struct Camera {
	float4x4 view;
	float4x4 proj;
};

struct VertexOutput {
	float4 position : SV_Position;
	float3 color;
};

[shader("vertex")]
func vertMain(VertexInput input, uniform Camera camera) -> VertexOutput {
	VertexOutput output;
	output.position = mul(mul(float4(input.position, 1.0), camera.view), camera.proj);
	output.color = input.color;
	return output;
}

[shader("fragment")]
func fragMain(VertexOutput inVert) -> float4 {
	return float4(inVert.color, 1.0);
}