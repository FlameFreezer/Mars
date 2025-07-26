struct VertexInput {
	float3 color;
	float2 position;
};

struct VertexOutput {
	float4 position : SV_Position;
	float3 color;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
	VertexOutput output;
	output.position = float4(input.position, 0.0, 1.0);
	output.color = input.color;
	return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
	return float4(inVert.color, 1.0);
}
