// Hardcoded square mesh vertices
static const float2 vertices[] = {
    float2(0, 0),
    float2(1, 0),
    float2(0, 1),
    float2(1, 0)
};
// Hardcoded square mesh indices
static const int indices[] = {
    0, 1, 2, 0, 2, 3
};

struct VertexOut {
    float4 position : SV_Position;
    float2 texCoord;
};

struct PushDescriptors {
    ConstantBuffer<float4x4> camera;
    Sampler2D texture;
    StructuredBuffer<float2> textureCoords;
};

ParameterBlock<PushDescriptors> push;

[shader("vertex")]
func vertMain(int vertexID: SV_VulkanVertexID, uniform float4x4 transform, uniform int zLayer)->VertexOut {
    VertexOut output;
    output.position = mul(push.camera, mul(transform, float4(vertices[indices[vertexID]], zLayer, 1.0)));
    output.texCoord = push.textureCoords[indices[vertexID]];
    return output;
}