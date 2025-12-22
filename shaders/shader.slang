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

struct CameraMatrices {
    float4x4 view;
    float4x4 proj;
};

struct Camera {
    ConstantBuffer<CameraMatrices> mats;

    [ForceInline]
    func view()->float4x4 {
        return mats.view;
    }
    [ForceInline]
    func proj()->float4x4 {
        return mats.proj;
    }
};

struct Samplers {
    Sampler2D texture;
};

uniform ParameterBlock<Samplers> samplers;
uniform ParameterBlock<Camera> camera;

[shader("vertex")]
func vertMain(input : VertexIn, uniform model: float4x4) -> VertexOut {
    VertexOut output;
    output.position = mul(camera.proj(), mul(camera.view(), mul(model, float4(input.position, 1.0f))));
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(input : VertexOut) -> float4 {
    return samplers.texture.Sample(input.texCoord);
}
