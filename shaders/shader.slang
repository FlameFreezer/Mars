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
    float4x4 view;
    float4x4 proj;
};

struct UniformBuffers {
    ConstantBuffer<MVP> mvp;
};

struct Samplers {
    Sampler2D texture;
};

uniform ParameterBlock<UniformBuffers> uniformBuffers;
uniform ParameterBlock<Samplers> samplers;

[shader("vertex")]
func vertMain(VertexIn input) -> VertexOut {
    VertexOut output;
    output.position = mul(uniformBuffers.mvp.proj, mul(uniformBuffers.mvp.view, mul(uniformBuffers.mvp.model, float4(input.position, 1.0f))));
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input) -> float4 {
    return samplers.texture.Sample(input.texCoord);
}
