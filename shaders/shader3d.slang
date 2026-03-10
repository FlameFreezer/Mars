struct VertexIn {
    float3 position;
    float2 texCoord;
};

struct VertexOut {
    float4 position : SV_Position;
    float2 texCoord;
};

struct PushDescriptors {
    ConstantBuffer<float4x4> camera;
    Sampler2D texture;
};

ParameterBlock<PushDescriptors> push;

[shader("vertex")]
func vertMain(input : VertexIn, uniform model: float4x4) -> VertexOut {
    VertexOut output;
    output.position = mul(push.camera, mul(model, float4(input.position, 1.0f)));
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(input : VertexOut) -> float4 {
    return push.texture.Sample(input.texCoord);
}
