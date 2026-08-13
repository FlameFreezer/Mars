struct VertexIn {
    float3 position;
    float2 texCoord;
};

struct VertexOut {
    float4 position : SV_Position;
    float2 texCoord;
};

struct Global {
    ConstantBuffer<float4x4> camera;
};

struct Instance {
    Sampler2D texture;
};


ParameterBlock<Global> global;
ParameterBlock<Instance> instance;

[shader("vertex")]
func vertMain(VertexIn input, uniform float4x4 model) -> VertexOut {
    VertexOut output;
    output.position = mul(global.camera, mul(model, float4(input.position, 1.0f)));
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input) -> float4 {
    return instance.texture.Sample(input.texCoord);
}
