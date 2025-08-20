RWBuffer<uint> screen_buf : register(u0);
RWBuffer<float> z_buf : register(u1);
StructuredBuffer<float> p : register(t0);
StructuredBuffer<float> c : register(t1);
StructuredBuffer<uint> misc : register(t2);
StructuredBuffer<float> tri_meta : register(t3);
StructuredBuffer<uint> indices : register(t4);

[numthreads(144, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint i = threadID.x;
    uint local = i % 144;
    uint t = i * rcp(144.0f);
    uint qx = local % 12;
    uint qy = local * rcp(12.0f);
    uint pb = t * 9;
    uint cb = t * 13;
    
    if (i < (misc[2] * 144)) {        
        float2 v1 = float2(p[pb], p[pb + 1]);
        float2 v2 = float2(p[pb + 3], p[pb + 4]);
        float2 v3 = float2(p[pb + 6], p[pb + 7]);
        float3 zs = float3(p[pb + 2], p[pb + 5], p[pb + 8]);
        float3 c1 = float3(c[cb], c[cb + 1], c[cb + 2]);
        float3 c2 = float3(c[cb + 4], c[cb + 5], c[cb + 6]);
        float3 c3 = float3(c[cb + 8], c[cb + 9], c[cb + 10]);
        float l1, l2, l3;
        uint hit = 0;
        
        float t5 = tri_meta[t * 5];
        float t51 = tri_meta[t * 5 + 1];
        float t54 = tri_meta[t * 5 + 4];
        uint m0 = misc[0];
        uint m1 = misc[1];
        
        uint width = uint(tri_meta[t * 5 + 2]) - uint(t5);
        uint height = uint(tri_meta[t * 5 + 3]) - uint(t51);
        
        uint minx = clamp(uint(t5) + floor((float(qx) * rcp(12.0f)) * float(width)), 0u, m0);
        uint miny = clamp(uint(t51) + floor((float(qy) * rcp(12.0f)) * float(height)), 0u, m1);
        uint maxx = clamp(minx + ceil(float(width) * rcp(12.0f)), 0u, m0);
        uint maxy = clamp(miny + ceil(float(height) * rcp(12.0f)), 0u, m1);
        
        float v3ymv2y = v3.y - v2.y;
        float v1ymv3y = v1.y - v3.y;
        float v3xmv2x = v3.x - v2.x;
        float v1xmv3x = v1.x - v3.x;
        
        l1 = ((float(minx) - v2.x) * v3ymv2y - (float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(minx) - v3.x) * v1ymv3y - (float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(maxx) - v2.x) * v3ymv2y - (float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(maxx) - v3.x) * v1ymv3y - (float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(minx) - v2.x) * v3ymv2y - (float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(minx) - v3.x) * v1ymv3y - (float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((float(maxx) - v2.x) * v3ymv2y - (float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(maxx) - v3.x) * v1ymv3y - (float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        if (hit == 0)
            return;
        
        hit = 4;
        
        l1 = ((float(minx) - v2.x) * v3ymv2y - (float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(minx) - v3.x) * v1ymv3y - (float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * miny + minx])
            hit--;
        
        l1 = ((float(maxx) - v2.x) * v3ymv2y - (float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(maxx) - v3.x) * v1ymv3y - (float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * miny + maxx])
            hit--;
        
        l1 = ((float(minx) - v2.x) * v3ymv2y - (float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(minx) - v3.x) * v1ymv3y - (float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * maxy + minx])
            hit--;
        
        l1 = ((float(maxx) - v2.x) * v3ymv2y - (float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(maxx) - v3.x) * v1ymv3y - (float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * maxy + maxx])
            hit--;
    
        if (hit == 0)
            return;
        
        if (c[t * 13 + 12] == 1.0f) {
            for (uint x = minx; x < maxx; x++) {
                for (uint y = miny; y < maxy; y++) {
                    l1 = ((float(x) - v2.x) * v3ymv2y - (float(y) - v2.y) * v3xmv2x) * t54;
                    l2 = ((float(x) - v3.x) * v1ymv3y - (float(y) - v3.y) * v1xmv3x) * t54;
                    l3 = 1.0f - l2 - l1;
                    
                    uint idx = m0 * y + x;
                    
                    if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f && z_buf[idx] > (l1 * zs.x + l2 * zs.y + l3 * zs.z)) {
                        uint color = uint(saturate(l1 * c1.r + l2 * c2.r + l3 * c3.r) * 255u) << 16 |
                                     uint(saturate(l1 * c1.g + l2 * c2.g + l3 * c3.g) * 255u) << 8 |
                                     uint(saturate(l1 * c1.b + l2 * c2.b + l3 * c3.b) * 255u);
                        float depth = l1 * zs.x + l2 * zs.y + l3 * zs.z;
                        
                        screen_buf[idx] = color;
                        z_buf[idx] = depth;
                    }
                }
            }
        }
    }
}
