struct VertexInput {
	float3 color;
	float3 position;
	float2 texCoord;
};

struct Camera {
	float4x4 view;
	float4x4 proj;
};

struct VertexOutput {
	float4 position : SV_Position;
	float3 color;
	float2 texCoord;
};

[shader("vertex")]
func vertMain(VertexInput input, uniform Camera camera, ConstantBuffer<float4x4> model) -> VertexOutput {
	VertexOutput output;
	output.position = mul(mul(mul(float4(input.position, 1.0), model), camera.view), camera.proj);
	output.color = input.color;
	output.texCoord = input.texCoord;
	return output;
}

[shader("fragment")]
func fragMain(VertexOutput inVert) -> float4 {
	return float4(inVert.color, 1.0);
}