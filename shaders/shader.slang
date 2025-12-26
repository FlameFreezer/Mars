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

struct Camera {
    ConstantBuffer<float4x4> viewProj;
};

struct Samplers {
    Sampler2D texture;
};

uniform ParameterBlock<Samplers> samplers;
uniform ParameterBlock<Camera> camera;

[shader("vertex")]
func vertMain(input : VertexIn, uniform model: float4x4) -> VertexOut {
    VertexOut output;
    output.position = mul(camera.viewProj, mul(model, float4(input.position, 1.0f)));
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(input : VertexOut) -> float4 {
    return samplers.texture.Sample(input.texCoord);
}
