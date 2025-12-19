struct VertexIn {
    float3 position;
    float3 color;
    float2 texCoord;
};

struct VertexOut {
    float4 position : SV_Position;
    float3 color;
    float2 texCoord;
};

[shader("vertex")]
func vertMain(VertexIn input, uniform ConstantBuffer<float4x4> model) -> VertexOut {
    VertexOut output;
    output.position = mul(float4(input.position, 1.0), model);
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input, uniform Sampler2D texture) -> float4 {
    return texture.Sample(input.texCoord);
}
