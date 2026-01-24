RWTexture2D<uint> screen_buf : register(u0);
RWTexture2D<uint> z_buf : register(u1);
StructuredBuffer<uint> misc : register(t0);
groupshared half dimensions[2];

[numthreads(1024, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint i = threadID.x;
    
    if (i % 1024 == 0) {
        dimensions[0] = misc[0];
        dimensions[1] = misc[1];
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (i < (dimensions[0] * dimensions[1])) {
        screen_buf[uint2(i % dimensions[0], uint(i * rcp(float(dimensions[0]))))] = 0;
        z_buf[uint2(i % dimensions[0], uint(i * rcp(float(dimensions[0]))))] = 4000000000;
    }
}
