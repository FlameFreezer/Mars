struct VertexInput {
	float3 color;
	float3 position;
};

struct UniformBuffer {
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput {
	float4 position : SV_Position;
	float3 color;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
	VertexOutput output;
	//output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0))));
	output.position = mul(mul(mul(float4(input.position, 1.0), ubo.model), ubo.view), ubo.proj);
	output.color = input.color;
	return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
	return float4(inVert.color, 1.0);
}
