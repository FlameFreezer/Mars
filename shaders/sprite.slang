// Hardcoded square mesh vertices
static const float2 vertices[] = {
    float2(0, 0),
    float2(1, 0),
    float2(1, 1),
    float2(0, 1)
};
// Hardcoded square mesh indices
static const int indices[] = {
    0, 1, 2, 0, 2, 3
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
    StructuredBuffer<float2> textureCoords;
};

ParameterBlock<Global> global;
ParameterBlock<Instance> instance;

[shader("vertex")]
func vertMain(int vertexID: SV_VulkanVertexID, uniform float4x4 transform)->VertexOut {
    VertexOut output;
    output.position = mul(global.camera, mul(transform, float4(vertices[indices[vertexID]], 0.0, 1.0)));
    output.texCoord = instance.textureCoords[indices[vertexID]];
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input)->float4 {
    return instance.texture.Sample(input.texCoord);
}