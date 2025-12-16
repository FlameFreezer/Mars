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
func vertMain(VertexIn input) -> VertexOut {
    VertexOut output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
func fragMain(VertexOut input)->float4 {
    return float4(input.color, 1.0);
}
