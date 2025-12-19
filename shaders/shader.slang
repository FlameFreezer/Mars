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

struct MVP {
    float4x4 model;
};

struct UniformBuffers {
    ConstantBuffer<MVP> mvp;
};

struct Samplers {
    Sampler2D texture;
};

[shader("vertex")]
func vertMain(VertexIn input, uniform ParameterBlock<UniformBuffers> uniformBuffers) -> VertexOut {
    VertexOut output;
    output.position = mul(float4(input.position, 1.0), uniformBuffers.mvp.model);
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input, uniform ParameterBlock<Samplers> samplers) -> float4 {
    return samplers.texture.Sample(input.texCoord);
}
