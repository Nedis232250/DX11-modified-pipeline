RWBuffer<float> positions : register(u0);
RWBuffer<float> colors : register(u1);
RWBuffer<unsigned int> misc : register(u2);

[numthreads(1024, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID) {
    unsigned int index = threadID.x + groupID.x * 1024;
    
    if (index < misc[2]) {
        unsigned int x1 = positions[0 + index * 9];
        unsigned int y1 = positions[1 + index * 9];
        unsigned int z1 = positions[2 + index * 9];
        unsigned int x2 = positions[3 + index * 9];
        unsigned int y2 = positions[4 + index * 9];
        unsigned int z2 = positions[5 + index * 9];
        unsigned int x3 = positions[6 + index * 9];
        unsigned int y3 = positions[7 + index * 9];
        unsigned int z3 = positions[8 + index * 9];
        unsigned int width = misc[0];
        unsigned int height = misc[1];
        float4 color1 = { colors[0 + index * 13], colors[1 + index * 13], colors[2 + index * 13], colors[3 + index * 13] };
        float4 color2 = { colors[4 + index * 13], colors[5 + index * 13], colors[6 + index * 13], colors[7 + index * 13] };
        float4 color3 = { colors[8 + index * 13], colors[9 + index * 13], colors[10 + index * 13], colors[11 + index * 13] };
        
        // 0.0 for textures, 1.0 for colors, and when textures color.z/color.b doesnt matter
        float mode = colors[12 + index * 13];
    }
}
